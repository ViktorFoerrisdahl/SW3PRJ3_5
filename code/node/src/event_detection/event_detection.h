#pragma once

#include <array>
#include <atomic>
#include <cstdint>
#include <stop_token>
#include <thread>
#include <vector>

#include "dc_remover.h"
#include "detector_config.h"
#include "event_queue.h"
#include "frame.h"        // Frame (fra common/frame.h)
#include "ring_buffer.h"
#include "sliding_rms.h"

// Hændelsesdetektering på én mikrofonnode (slave).
//
// Detektoren kører i sin egen tråd og læser frames fra nodens ringbuffer med
// sin egen read pointer. For hver frame og hver af de to mikrofoner:
//   1. skiftes samplet ned til 18 bit (x >> 14),
//   2. fjernes DC-offset med et løbende gennemsnit,
//   3. opdateres det glidende RMS-vindue, og summen sammenlignes med
//      N · THRESHOLD_RAW^2.
//
// Noden melder kun en hændelse, hvis begge mikrofoner overstiger tærsklen,
// den anden senest pair_window_ms efter den første. Det første tidspunkt
// hvor en mikrofon overstiger tærsklen er triggerpunktet. Når der er skrevet
// post_ms data efter triggerpunktet, kopieres et udsnit [T - pre, T + post)
// af de rå samples ud af ringbufferen og lægges i køen til TCP-modulet.
// Derefter ignoreres nye hændelser i holdoff_ms fra triggerpunktet.
class EventDetector {
public:
    // Kaster std::invalid_argument, hvis konfigurationen er ugyldig, fx hvis
    // tærsklen ikke er sat.
    EventDetector(const RingBuffer& ring_buffer, EventQueue& output, const DetectorConfig& config);
    ~EventDetector();

    EventDetector(const EventDetector&) = delete;
    EventDetector& operator=(const EventDetector&) = delete;

    // Starter detektortråden.
    void start();

    // Beder detektortråden om at stoppe og venter på at den er færdig.
    void stop();

    // Behandler alle frames der er skrevet til ringbufferen indtil nu, uden
    // at vente på nye. Bruges af run() og direkte af testene.
    void processAvailable();

    // Tællere til fejlfinding. Kan læses fra andre tråde.
    uint64_t eventsSent() const { return events_sent_; }
    uint64_t eventsDiscarded() const { return events_discarded_; }  // udsnit overskrevet eller uden timestamp
    uint64_t unpairedTriggers() const { return unpaired_triggers_; } // kun én mikrofon over tærsklen
    uint64_t resyncs() const { return resyncs_; }                    // detektoren blev overhalet af skriveren

private:
    enum class State {
        Listening,      // venter på at en mikrofon overstiger tærsklen
        WaitingSecond,  // én mikrofon er over, venter på den anden
        WaitingPost,    // begge er over, venter på data efter triggerpunktet
    };

    // Trådens hovedløkke: vent på nye frames, behandl dem, gentag til stop.
    void run(std::stop_token st);

    // Behandler én frame med globalt frame-nummer f.
    void processFrame(uint64_t f, const Frame& frame);

    // Kopierer udsnittet omkring triggerpunktet ud og lægger det i køen.
    void extractAndSend();

    // Starter forfra fra frame new_read, fx hvis skriveren har overhalet os.
    void resync(uint64_t new_read);

    const RingBuffer& ring_buffer_;
    EventQueue& output_;
    DetectorConfig config_;

    // Parametre omregnet til frames.
    uint32_t window_frames_;
    uint32_t pair_frames_;
    uint32_t pre_frames_;
    uint32_t post_frames_;
    uint32_t holdoff_frames_;
    uint32_t warmup_frames_;

    // N · THRESHOLD_RAW^2 for hver mikrofon, udregnet én gang ved opstart.
    std::array<int64_t, 2> limit_{};

    std::array<DcRemover, 2> dc_;
    std::array<SlidingRms, 2> rms_;

    uint64_t read_ = 0;               // detektorens read pointer (globalt frame-nummer)
    uint32_t warmup_left_ = 0;        // frames tilbage før der må detekteres
    State state_ = State::Listening;
    uint64_t trigger_ = 0;            // triggerpunktet T (globalt frame-nummer)
    int first_channel_ = 0;           // den mikrofon der overskred tærsklen først
    uint64_t pair_deadline_ = 0;      // sidste frame hvor den anden mikrofon må overskride
    uint64_t holdoff_until_ = 0;      // første frame hvor en ny hændelse må starte
    uint32_t next_seq_ = 0;           // løbenummer til næste hændelse

    std::vector<Frame> chunk_;        // arbejdsbuffer til frames læst fra ringbufferen

    std::atomic<uint64_t> events_sent_{0};
    std::atomic<uint64_t> events_discarded_{0};
    std::atomic<uint64_t> unpaired_triggers_{0};
    std::atomic<uint64_t> resyncs_{0};

    std::jthread thread_;
};
