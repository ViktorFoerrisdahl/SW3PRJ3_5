#pragma once

#include <condition_variable>
#include <cstdint>
#include <deque>
#include <mutex>
#include <optional>
#include <stop_token>
#include <vector>

#include <json_dto/pub.hpp>

#include "frame.h"  // Frame (fra common/frame.h)

// Fortæller json_dto, hvordan std::vector<Frame> skrives/læses som JSON.
// Frames bliver til ét fladt array med kanalerne interleaved:
// [ch0, ch1, ch0, ch1, ...], samme rækkefølge som i ALSA-bufferen.
struct FramesJsonRW {
    void write(const std::vector<Frame>& frames, rapidjson::Value& to,
               rapidjson::MemoryPoolAllocator<>& allocator) const {
        to.SetArray();
        to.Reserve(static_cast<rapidjson::SizeType>(frames.size() * 2), allocator);
        for (const Frame& f : frames) {
            to.PushBack(f.ch[0], allocator);
            to.PushBack(f.ch[1], allocator);
        }
    }

    void read(std::vector<Frame>& frames, const rapidjson::Value& from) const {
        if (!from.IsArray() || from.Size() % 2 != 0) {
            throw json_dto::ex_t{"frames: forventede et array med et lige antal samples"};
        }
        frames.clear();
        frames.reserve(from.Size() / 2);
        for (rapidjson::SizeType i = 0; i < from.Size(); i += 2) {
            frames.push_back(Frame{{from[i].GetInt(), from[i + 1].GetInt()}});
        }
    }
};

// Én detekteret hændelse, klar til at blive sendt til orchestrator.
// Dette er hændelsesdetekteringens output. TCP-modulet står selv for at
// pakke felterne til bytes (header + interleaved samples).
struct DetectedEvent {
    uint16_t node_id = 0;         // hvilken mikrofonnode hændelsen kommer fra
    uint32_t event_seq = 0;       // løbenummer for hændelsen på denne node
    uint32_t sample_rate = 0;     // fx 48000
    int64_t t_first_ns = 0;       // PTP-synkroniseret timestamp for udsnittets første frame
    uint32_t trigger_offset = 0;  // triggerpunktets placering i udsnittet (frame-nr.)
    std::vector<Frame> frames;    // de rå S32_LE-samples, kanalerne interleaved i hver frame

    // Bestemmer JSON-formatet for json_dto (bruges både til at skrive og læse).
    template <typename Json_Io>
    void json_io(Json_Io& io) {
        io & json_dto::mandatory("node_id", node_id)
           & json_dto::mandatory("event_seq", event_seq)
           & json_dto::mandatory("sample_rate", sample_rate)
           & json_dto::mandatory("t_first_ns", t_first_ns)
           & json_dto::mandatory("trigger_offset", trigger_offset)
           & json_dto::mandatory(FramesJsonRW{}, "frames", frames);
    }
};

// Trådsikker kø mellem detektortråden (producent) og TCP-modulet (forbruger).
// Detektoren venter aldrig på netværket: er køen fuld, smides den ældste
// hændelse væk, så den nyeste altid kommer med.
class EventQueue {
public:
    explicit EventQueue(std::size_t max_size) : max_size_(max_size) {}

    // Lægger en hændelse i køen og vækker en ventende forbruger.
    void push(DetectedEvent&& ev) {
        {
            std::lock_guard lock(mutex_);
            if (max_size_ > 0 && queue_.size() >= max_size_) {
                queue_.pop_front();
                ++dropped_;
            }
            queue_.push_back(std::move(ev));
        }
        cv_.notify_one();
    }

    // Venter på næste hændelse. Returnerer std::nullopt, hvis der anmodes om
    // stop (via stop_token), før der kommer en hændelse.
    std::optional<DetectedEvent> pop(std::stop_token st) {
        std::unique_lock lock(mutex_);
        if (!cv_.wait(lock, st, [this] { return !queue_.empty(); })) {
            return std::nullopt;
        }
        DetectedEvent ev = std::move(queue_.front());
        queue_.pop_front();
        return ev;
    }

    // Henter en hændelse uden at vente, hvis der er en.
    std::optional<DetectedEvent> tryPop() {
        std::lock_guard lock(mutex_);
        if (queue_.empty()) {
            return std::nullopt;
        }
        DetectedEvent ev = std::move(queue_.front());
        queue_.pop_front();
        return ev;
    }

    std::size_t size() const {
        std::lock_guard lock(mutex_);
        return queue_.size();
    }

    // Antal hændelser der er smidt væk, fordi køen var fuld.
    uint64_t dropped() const {
        std::lock_guard lock(mutex_);
        return dropped_;
    }

private:
    mutable std::mutex mutex_;
    std::condition_variable_any cv_;  // _any, så pop() kan afbrydes med en stop_token
    std::deque<DetectedEvent> queue_;
    std::size_t max_size_;
    uint64_t dropped_ = 0;
};
