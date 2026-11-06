#pragma once

#include <alsa/asoundlib.h>

#include <cstdint>
#include <stop_token>
#include <string>

#include "ring_buffer.h"

/** @brief Settings for the ALSA capture device. */
struct ALSA_CONFIG
{
    std::string device;       ///< ALSA device, e.g. "hw:0,0". Find it with `arecord -l`
    uint32_t sample_rate_hz;  ///< Must be supported exactly, no resampling
    uint32_t period_frames;   ///< Frames per read, e.g. 512 (about 10.7 ms at 48 kHz)
    uint32_t periods;         ///< ALSA buffer size in periods, e.g. 4
};

/**
 * @class AlsaCapture
 * @brief Reads frames from ALSA and writes them into the ring buffer.
 *
 * After every period it also records a time anchor (frame number + synced time),
 * which the ring buffer uses to convert frame numbers to PTP time.
 *
 * Usage:
 * @code
 * RingBuffer rb(131072, 48000);
 * AlsaCapture capture(rb);
 * std::string error;
 * if (!capture.open({"hw:0,0", 48000, 512, 4}, error)) { ... }
 * std::jthread capture_thread([&](std::stop_token st) { capture.run(st); });
 * @endcode
 */
class AlsaCapture
{
public:
    explicit AlsaCapture(RingBuffer &rb) : rb_(rb) {}
    ~AlsaCapture();

    AlsaCapture(const AlsaCapture &) = delete;
    AlsaCapture &operator=(const AlsaCapture &) = delete;

    /**
     * @brief Opens and configures the device.
     * @param config Device and buffer settings.
     * @param error Set to a description if opening fails.
     * @return true on success.
     */
    bool open(const ALSA_CONFIG &config, std::string &error);

    /**
     * @brief Capture loop. Runs until stop is requested or an unrecoverable error.
     * @return false if it stopped because of an error.
     */
    bool run(std::stop_token st);

    /** @brief Number of overruns since start. Should stay 0 in normal operation. */
    uint64_t overrunCount() const { return overruns_; }

private:
    void RecordTimeAnchor();

    RingBuffer &rb_;
    snd_pcm_t *pcm_ = nullptr;
    snd_pcm_status_t *status_ = nullptr;
    snd_pcm_uframes_t period_frames_ = 0;
    uint64_t overruns_ = 0;
};