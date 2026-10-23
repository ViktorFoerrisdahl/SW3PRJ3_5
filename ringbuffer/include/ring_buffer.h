#pragma once

// Real ring buffer for the microphone node.
//
// Same public interface as the mock in tests/event_detection/mocks/ring_buffer.h,
// so the event detection works against both. In addition it has the functions
// the ALSA capture thread uses to write frames and record time anchors.

#include <cmath>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <deque>
#include <mutex>
#include <stop_token>
#include <vector>

#include "frame.h"

/** @brief A known point on the line from frame number to synced time. */
struct TIME_ANCHOR
{
    uint64_t frame;  ///< Global frame number
    int64_t t_ns;    ///< Synced (PTP) time when that frame was sampled
};

#define MAX_TIME_ANCHORS 256      ///< About 2.7 s of anchors at one per 512-frame period
#define MIN_TIME_ANCHORS 8        ///< Fewer than this and timestampNs() refuses
#define CLOCK_STEP_LIMIT_NS 2000000  ///< An anchor this far off the line means the clock jumped

class RingBuffer
{
public:
    /**
     * @param capacity Number of frames the buffer holds.
     * @param sample_rate Nominal sample rate, used only until enough anchors exist.
     */
    RingBuffer(std::size_t capacity, uint32_t sample_rate)
        : data_(capacity), sample_rate_(sample_rate) {}

    // ---- Used by the ALSA capture thread ----

    /** @brief Copies frames in, overwriting the oldest, and wakes waiting readers. */
    void write(const Frame *frames, std::size_t n)
    {
        {
            std::lock_guard lock(mutex_);
            for (std::size_t i = 0; i < n; ++i)
            {
                data_[(write_ + i) % data_.size()] = frames[i];
            }
            write_ += n;
        }
        cv_.notify_all();
    }

    /**
     * @brief Records that @p frame was sampled at synced time @p t_ns.
     *
     * If the anchor is far off the line through the existing anchors, the clock
     * has jumped (e.g. phc2sys stepped it), and the old anchors are discarded.
     */
    void addTimeAnchor(uint64_t frame, int64_t t_ns)
    {
        std::lock_guard lock(mutex_);
        if (anchors_.size() >= MIN_TIME_ANCHORS)
        {
            int64_t predicted_ns = 0;
            if (FitLocked(frame, predicted_ns) && std::llabs(predicted_ns - t_ns) > CLOCK_STEP_LIMIT_NS)
            {
                anchors_.clear();
                valid_from_frame_ = frame;
            }
        }
        anchors_.push_back({frame, t_ns});
        if (anchors_.size() > MAX_TIME_ANCHORS)
        {
            anchors_.pop_front();
        }
    }

    /**
     * @brief Discards all anchors. Frames before @p from_frame can no longer be timestamped.
     *
     * Called after an ALSA overrun: frames were lost, so frame numbers before and
     * after the overrun are no longer on the same line.
     */
    void resetTimeAnchors(uint64_t from_frame)
    {
        std::lock_guard lock(mutex_);
        anchors_.clear();
        valid_from_frame_ = from_frame;
    }

    // ---- Interface the event detection uses (same as the mock) ----

    /** @brief Global, monotonically increasing number of frames written in total. */
    uint64_t writeIndex() const
    {
        std::lock_guard lock(mutex_);
        return write_;
    }

    /**
     * @brief Waits until writeIndex() >= min_write_index, or until stop is requested.
     * @return writeIndex(). Can be less than min_write_index if stop was requested.
     */
    uint64_t waitForFrames(uint64_t min_write_index, std::stop_token st) const
    {
        std::unique_lock lock(mutex_);
        cv_.wait(lock, st, [&] { return write_ >= min_write_index; });
        return write_;
    }

    /**
     * @brief Copies @p count frames starting at global frame number @p start to @p dst.
     * @return false if some of them are not written yet or already overwritten.
     */
    bool copyFrames(uint64_t start, std::size_t count, Frame *dst) const
    {
        std::lock_guard lock(mutex_);
        if (start + count > write_ || write_ - start > data_.size())
        {
            return false;
        }
        for (std::size_t i = 0; i < count; ++i)
        {
            dst[i] = data_[(start + i) % data_.size()];
        }
        return true;
    }

    /**
     * @brief Synced timestamp in ns for a global frame number.
     *
     * Fits a straight line through the recent anchors (least squares). The slope
     * is the real sample period, which differs slightly from 1/sample_rate
     * because the I2S clock crystal is not exact.
     *
     * @return false if there are too few anchors, or the frame is from before
     *         the last overrun or clock jump.
     */
    bool timestampNs(uint64_t frame, int64_t &t_ns) const
    {
        std::lock_guard lock(mutex_);
        if (frame < valid_from_frame_ || anchors_.size() < MIN_TIME_ANCHORS)
        {
            return false;
        }
        return FitLocked(frame, t_ns);
    }

private:
    /** @brief Least-squares line through the anchors, evaluated at @p frame. Mutex must be held. */
    bool FitLocked(uint64_t frame, int64_t &t_ns) const
    {
        // Work relative to the first anchor: absolute ns values are too large
        // to convert to double without losing precision.
        const TIME_ANCHOR &ref = anchors_.front();
        double n = static_cast<double>(anchors_.size());
        double sum_x = 0.0;
        double sum_y = 0.0;
        for (const TIME_ANCHOR &a : anchors_)
        {
            sum_x += static_cast<double>(a.frame - ref.frame);
            sum_y += static_cast<double>(a.t_ns - ref.t_ns);
        }
        double mean_x = sum_x / n;
        double mean_y = sum_y / n;

        double sxx = 0.0;
        double sxy = 0.0;
        for (const TIME_ANCHOR &a : anchors_)
        {
            double dx = static_cast<double>(a.frame - ref.frame) - mean_x;
            double dy = static_cast<double>(a.t_ns - ref.t_ns) - mean_y;
            sxx += dx * dx;
            sxy += dx * dy;
        }

        // All anchors on the same frame: fall back to the nominal sample rate
        double ns_per_frame = (sxx > 0.0) ? sxy / sxx : 1e9 / sample_rate_;

        double x = static_cast<double>(static_cast<int64_t>(frame - ref.frame));
        t_ns = ref.t_ns + std::llround(mean_y + ns_per_frame * (x - mean_x));
        return true;
    }

    mutable std::mutex mutex_;
    mutable std::condition_variable_any cv_;
    std::vector<Frame> data_;
    uint64_t write_ = 0;
    uint32_t sample_rate_;
    std::deque<TIME_ANCHOR> anchors_;
    uint64_t valid_from_frame_ = 0;
};