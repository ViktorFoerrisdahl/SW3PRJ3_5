#include "event_detection.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

// Anonymt namespace betyder at der undgås navnekonflikter
namespace {

// Tjekker at konfigurationen giver mening, før detektoren bygges.
const DetectorConfig& validated(const DetectorConfig& c) {
    if (c.threshold_raw[0] <= 0.0 || c.threshold_raw[1] <= 0.0) {
        throw std::invalid_argument("EventDetector: threshold_raw er ikke sat for begge mikrofoner");
    }
    if (c.sample_rate == 0) {
        throw std::invalid_argument("EventDetector: sample_rate skal være > 0");
    }
    if (c.sample_shift < 0 || c.sample_shift > 31) {
        throw std::invalid_argument("EventDetector: sample_shift skal ligge i [0, 31]");
    }
    if (c.dc_shift_k < 1 || c.dc_shift_k > 30) {
        throw std::invalid_argument("EventDetector: dc_shift_k skal ligge i [1, 30]");
    }
    if (c.msToFrames(c.rms_window_ms) == 0) {
        throw std::invalid_argument("EventDetector: RMS-vinduet er for kort");
    }
    if (c.msToFrames(c.pre_ms) + c.msToFrames(c.post_ms) == 0) {
        throw std::invalid_argument("EventDetector: udsnittet er tomt");
    }
    if (c.msToFrames(c.holdoff_ms) < c.msToFrames(c.post_ms)) {
        throw std::invalid_argument("EventDetector: holdoff_ms skal være mindst post_ms");
    }
    if (c.chunk_frames == 0) {
        throw std::invalid_argument("EventDetector: chunk_frames skal være > 0");
    }
    return c;
}

}  // namespace

EventDetector::EventDetector(const RingBuffer& ring_buffer, EventQueue& output, const DetectorConfig& config)
    : ring_buffer_(ring_buffer),
      output_(output),
      config_(validated(config)),
      window_frames_(config_.msToFrames(config_.rms_window_ms)),
      pair_frames_(config_.msToFrames(config_.pair_window_ms)),
      pre_frames_(config_.msToFrames(config_.pre_ms)),
      post_frames_(config_.msToFrames(config_.post_ms)),
      holdoff_frames_(config_.msToFrames(config_.holdoff_ms)),
      // Der detekteres ikke, før RMS-vinduet er fyldt, og før der ligger
      // mindst pre_frames_ data bag read pointeren, så T - pre aldrig ligger
      // før det første frame vi har set.
      warmup_frames_(std::max(window_frames_, pre_frames_)),
      dc_{DcRemover(config_.dc_shift_k), DcRemover(config_.dc_shift_k)},
      rms_{SlidingRms(window_frames_), SlidingRms(window_frames_)},
      chunk_(config_.chunk_frames) {
    // sqrt(sum / N) > THRESHOLD_RAW  <=>  sum > N · THRESHOLD_RAW^2
    for (int c = 0; c < 2; ++c) {
        const double thr = config_.threshold_raw[c];
        limit_[c] = std::llround(static_cast<double>(window_frames_) * thr * thr);
    }
    // Start fra det der skrives fra nu af. Gammel data i bufferen ignoreres.
    resync(ring_buffer_.writeIndex());
    resyncs_ = 0;
}

EventDetector::~EventDetector() { stop(); }

void EventDetector::start() {
	// Start kun i tilfælde af at der ikke allerede kører en tråd
    if (!thread_.joinable()) {
		// Start tråden
		// Lambda funktionen:
		//  Vi sørger for at tråden har fuld adgang til denne instans af klassen ([this])
		//  Lambdaen skal blot kalde run() for at påbegynde detektoren
        thread_ = std::jthread([this](std::stop_token st) { run(st); });
    }
}

void EventDetector::stop() {
    if (thread_.joinable()) {
        thread_.request_stop();
        thread_.join();
    }
}

void EventDetector::run(std::stop_token st) {
    while (!st.stop_requested()) {
        // Sov indtil ringbufferen har skrevet mindst én ny frame. Ringbufferen
        // vækker os via sin condition variable (antagelse, se README.md), og
        // stop_token vækker os, hvis der anmodes om stop.
        ring_buffer_.waitForFrames(read_ + 1, st);
        if (st.stop_requested()) {
            break;
        }
        processAvailable();
    }
}

void EventDetector::processAvailable() {
    const uint64_t write = ring_buffer_.writeIndex();
    while (read_ < write) {
        const auto n = static_cast<std::size_t>(std::min<uint64_t>(write - read_, chunk_.size()));
        if (!ring_buffer_.copyFrames(read_, n, chunk_.data())) { // ring_buffer_.copyFrames() bliver stadig kaldt uafhængigt af udfald
            // Skriveren har overskrevet data vi ikke har nået at læse. Det
            // bør ikke ske i normal drift. Start forfra fra skriverens position.
            ++resyncs_;
            resync(ring_buffer_.writeIndex());
            return;
        }
        for (std::size_t i = 0; i < n; ++i) {
            processFrame(read_ + i, chunk_[i]);
        }
        read_ += n;
    }
}

void EventDetector::processFrame(uint64_t f, const Frame& frame) {
    // Opdater DC-filter og RMS-vindue for begge mikrofoner, og afgør om de
    // er over tærsklen. Det gøres for hver frame, også under holdoff, så
    // filtrene altid følger signalet.
    std::array<bool, 2> over{};
    for (int c = 0; c < 2; ++c) {
		// Samples bit shiftes til reel PCM værdi og dc offset fjernes
        const int32_t x = frame.ch[c] >> config_.sample_shift;  // aritmetisk skift, bevarer fortegn
        const int32_t y = dc_[c].process(x);
		// Samples tilføjes til rms vindue og der vurderes om sample er over threshold
        rms_[c].push(y);
        over[c] = rms_[c].sum() > limit_[c];
    }

	// Ikke lav nogen vurderinger før warmpup er færdigt
    if (warmup_left_ > 0) {
        --warmup_left_;
        return;
    }

    switch (state_) {
        case State::Listening:
            if (f < holdoff_until_ || !(over[0] || over[1])) {
				// Hvis der stadig er sandsynlighed for ekko eller intet sample er over threshold -> gør intet
                break;
            }
			// Hvis du er kommet så langt må der være sket en hændelse på en af mikrofonerne
            trigger_ = f;
            if (over[0] && over[1]) {
                // Begge mikrofoner er over i samme frame -> gør klar til post
                state_ = State::WaitingPost;
            } else {
				// Hvis kun en mikrofon er over, gem den i first_channel_,
				// indstil deadline for næste mikrofon og sæt state til WaitingSecond.
                first_channel_ = over[0] ? 0 : 1;
                pair_deadline_ = f + pair_frames_;
                state_ = State::WaitingSecond;
            }
            break;

        case State::WaitingSecond:
            if (over[1 - first_channel_]) {
				// Hvis den manglende mikrofon er over, gør klar til post
                state_ = State::WaitingPost;
            } else if (f >= pair_deadline_) {
                // Den anden mikrofon nåede ikke over tærsklen i tide, så det
                // tælles ikke som en hændelse på denne node.
                ++unpaired_triggers_;
                state_ = State::Listening;
            }
            break;

        case State::WaitingPost:
            break;
    }

    // Frame f er skrevet og læst, så når f = T + post - 1, ligger hele
    // udsnittet [T - pre, T + post) i ringbufferen.
    if (state_ == State::WaitingPost && f + 1 >= trigger_ + post_frames_) {
        extractAndSend();
    }
}

void EventDetector::extractAndSend() {
	// Få gobal startframe der skal kopieres fra i ringbuffer
    const uint64_t start = trigger_ - pre_frames_;
	// Få mængden af frames der skal kopieres
    const std::size_t count = static_cast<std::size_t>(pre_frames_) + post_frames_;

	// Initialiser en DetectedEvent og fyld med data
    DetectedEvent ev;
    ev.node_id = config_.node_id;
    ev.sample_rate = config_.sample_rate;
    ev.trigger_offset = pre_frames_;
    ev.frames.resize(count);

	// Anvend ringbufferens metoder til at kopiere frames og få PTP timestamps
    // copyFrames() returnerer false, hvis starten af udsnittet er blevet
    // overskrevet under kopien. Så kasseres hændelsen.
    const bool copied = ring_buffer_.copyFrames(start, count, ev.frames.data());
    const bool stamped = copied && ring_buffer_.timestampNs(start, ev.t_first_ns);

    if (copied && stamped) {
		// Hvis ringbuffer metoder afviklede uden fejl, skub hændelse til kø
        ev.event_seq = next_seq_++;
        output_.push(std::move(ev));
        ++events_sent_;
    } else {
		// Ellers registrer hændelse som slettet
        ++events_discarded_;
    }

	// Sæt cooldown så detektoren ikke opfanger ekko og reset state
    holdoff_until_ = trigger_ + holdoff_frames_;
    state_ = State::Listening;
}

void EventDetector::resync(uint64_t new_read) {
    read_ = new_read;
    for (int c = 0; c < 2; ++c) {
        dc_[c].reset();
        rms_[c].reset();
    }
    warmup_left_ = warmup_frames_;
    state_ = State::Listening;
    holdoff_until_ = 0;
}
