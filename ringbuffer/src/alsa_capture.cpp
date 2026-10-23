#include "alsa_capture.h"

#include <cstdio>
#include <vector>

AlsaCapture::~AlsaCapture()
{
    if (pcm_ != nullptr)
    {
        snd_pcm_drop(pcm_);
        snd_pcm_close(pcm_);
    }
    if (status_ != nullptr)
    {
        snd_pcm_status_free(status_);
    }
}

bool AlsaCapture::open(const ALSA_CONFIG &config, std::string &error)
{
    int err = snd_pcm_open(&pcm_, config.device.c_str(), SND_PCM_STREAM_CAPTURE, 0);
    if (err < 0)
    {
        error = "snd_pcm_open: " + std::string(snd_strerror(err));
        pcm_ = nullptr;
        return false;
    }

    // ---- Hardware parameters: format, channels, rate, buffer sizes ----
    snd_pcm_hw_params_t *hw;
    snd_pcm_hw_params_alloca(&hw);
    snd_pcm_hw_params_any(pcm_, hw);

    if ((err = snd_pcm_hw_params_set_access(pcm_, hw, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0 ||
        (err = snd_pcm_hw_params_set_format(pcm_, hw, SND_PCM_FORMAT_S32_LE)) < 0 ||
        (err = snd_pcm_hw_params_set_channels(pcm_, hw, 2)) < 0 ||
        // Resampling would change the timing of every sample, so it must be off
        (err = snd_pcm_hw_params_set_rate_resample(pcm_, hw, 0)) < 0 ||
        (err = snd_pcm_hw_params_set_rate(pcm_, hw, config.sample_rate_hz, 0)) < 0)
    {
        error = "hw params (format/channels/rate): " + std::string(snd_strerror(err));
        return false;
    }

    snd_pcm_uframes_t period = config.period_frames;
    snd_pcm_uframes_t buffer = static_cast<snd_pcm_uframes_t>(config.period_frames) * config.periods;
    int dir = 0;
    if ((err = snd_pcm_hw_params_set_period_size_near(pcm_, hw, &period, &dir)) < 0 ||
        (err = snd_pcm_hw_params_set_buffer_size_near(pcm_, hw, &buffer)) < 0 ||
        (err = snd_pcm_hw_params(pcm_, hw)) < 0)
    {
        error = "hw params (period/buffer): " + std::string(snd_strerror(err));
        return false;
    }
    period_frames_ = period;  // The driver may have adjusted it

    // ---- Software parameters: timestamps in CLOCK_REALTIME, which phc2sys syncs to PTP ----
    snd_pcm_sw_params_t *sw;
    snd_pcm_sw_params_alloca(&sw);
    snd_pcm_sw_params_current(pcm_, sw);
    if ((err = snd_pcm_sw_params_set_tstamp_mode(pcm_, sw, SND_PCM_TSTAMP_ENABLE)) < 0 ||
        (err = snd_pcm_sw_params_set_tstamp_type(pcm_, sw, SND_PCM_TSTAMP_TYPE_GETTIMEOFDAY)) < 0 ||
        (err = snd_pcm_sw_params_set_avail_min(pcm_, sw, period_frames_)) < 0 ||
        (err = snd_pcm_sw_params(pcm_, sw)) < 0)
    {
        error = "sw params: " + std::string(snd_strerror(err));
        return false;
    }

    if ((err = snd_pcm_status_malloc(&status_)) < 0)
    {
        error = "snd_pcm_status_malloc: " + std::string(snd_strerror(err));
        return false;
    }

    std::fprintf(stderr, "ALSA: %s, %u Hz, period %lu frames, buffer %lu frames\n",
                 config.device.c_str(), config.sample_rate_hz,
                 static_cast<unsigned long>(period_frames_), static_cast<unsigned long>(buffer));
    return true;
}

bool AlsaCapture::run(std::stop_token st)
{
    std::vector<Frame> period(period_frames_);

    while (!st.stop_requested())
    {
        // Blocks until one period is captured (about 10.7 ms at 512 frames / 48 kHz).
        // Frame has the same layout as ALSA's interleaved S32_LE, so ALSA writes straight into it.
        snd_pcm_sframes_t n = snd_pcm_readi(pcm_, period.data(), period_frames_);

        if (n < 0)
        {
            // -EPIPE = overrun: we were too slow and ALSA overwrote frames we never read.
            // The frames are lost, so frame numbers before and after no longer line up in time.
            ++overruns_;
            std::fprintf(stderr, "ALSA: %s, recovering\n", snd_strerror(static_cast<int>(n)));
            if (snd_pcm_recover(pcm_, static_cast<int>(n), 1) < 0)
            {
                return false;
            }
            rb_.resetTimeAnchors(rb_.writeIndex());
            continue;
        }

        rb_.write(period.data(), static_cast<std::size_t>(n));
        RecordTimeAnchor();
    }
    return true;
}

void AlsaCapture::RecordTimeAnchor()
{
    if (snd_pcm_status(pcm_, status_) < 0)
    {
        return;
    }

    snd_htimestamp_t ts;
    snd_pcm_status_get_htstamp(status_, &ts);
    snd_pcm_uframes_t avail = snd_pcm_status_get_avail(status_);

    // At time ts the hardware had captured everything we have read, plus `avail`
    // frames still waiting in ALSA's buffer. So the next frame to be captured,
    // number writeIndex() + avail, is sampled at about ts.
    uint64_t frame = rb_.writeIndex() + avail;
    int64_t t_ns = static_cast<int64_t>(ts.tv_sec) * 1000000000LL + ts.tv_nsec;
    rb_.addTimeAnchor(frame, t_ns);
}