#pragma once

// MOCK af ringbufferen, kun til test af hændelsesdetekteringen.
//
// Den rigtige ringbuffer laves af et andet gruppemedlem. Denne fil har samme
// navn og samme funktioner, som hændelsesdetekteringen antager, at den rigtige
// har (se src/event_detection/README.md). Testene bygges med
// -I tests/event_detection/mocks, så "ring_buffer.h" findes her.

#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <stop_token>
#include <vector>

#include "frame.h"  // Frame (fra common/frame.h)

class RingBuffer {
public:
    // capacity: antal frames bufferen kan holde.
    // t0_ns:    timestamp for frame 0. Mocken regner timestamps ud fra sample
    //           rate; den rigtige ringbuffer får dem fra ALSA/PTP.
    RingBuffer(std::size_t capacity, uint32_t sample_rate, int64_t t0_ns)
        : data_(capacity), sample_rate_(sample_rate), t0_ns_(t0_ns) {}

    // Kun i mocken: skriver frames, som ALSA-tråden ville gøre det, og vækker
    // ventende læsere.
    void write(const Frame* frames, std::size_t n) {
        {
            std::lock_guard lock(mutex_);
            for (std::size_t i = 0; i < n; ++i) {
                data_[(write_ + i) % data_.size()] = frames[i];
            }
            write_ += n;
        }
        cv_.notify_all();
    }

    // ---- Funktioner hændelsesdetekteringen antager ----

    // Globalt, monotont voksende antal frames skrevet i alt.
    uint64_t writeIndex() const {
        std::lock_guard lock(mutex_);
        return write_;
    }

    // Venter til writeIndex() >= min_write_index, eller til der anmodes om
    // stop. Returnerer writeIndex().
    uint64_t waitForFrames(uint64_t min_write_index, std::stop_token st) const {
        std::unique_lock lock(mutex_);
        cv_.wait(lock, st, [&] { return write_ >= min_write_index; });
        return write_;
    }

    // Kopierer count frames fra globalt frame-nummer start til dst. Returnerer
    // false, hvis nogle af dem endnu ikke er skrevet eller allerede er
    // overskrevet.
    bool copyFrames(uint64_t start, std::size_t count, Frame* dst) const {
        std::lock_guard lock(mutex_);
        if (start + count > write_ || write_ - start > data_.size()) {
            return false;
        }
        for (std::size_t i = 0; i < count; ++i) {
            dst[i] = data_[(start + i) % data_.size()];
        }
        return true;
    }

    // PTP-synkroniseret timestamp i ns for et globalt frame-nummer.
    bool timestampNs(uint64_t frame, int64_t& t_ns) const {
        t_ns = t0_ns_ + static_cast<int64_t>(frame) * 1'000'000'000LL / sample_rate_;
        return true;
    }

private:
    mutable std::mutex mutex_;
    mutable std::condition_variable_any cv_;
    std::vector<Frame> data_;
    uint64_t write_ = 0;
    uint32_t sample_rate_;
    int64_t t0_ns_;
};
