// Hardware test for AlsaCapture + RingBuffer with the real microphones on the Raspberry Pi.
//
// ring_buffer_test checks the ring buffer logic with made-up data. This test checks the
// whole chain on the real hardware and prints the measured numbers, so you can judge
// them yourself instead of only trusting "Passed":
//
//   1. Opens the ALSA device (S32_LE, 2 channels, 48 kHz) and starts the capture thread.
//   2. Measures the real sample rate against the Pi's clock.
//   3. Looks at a few seconds of silence: is each mic delivering data, are the two
//      channels different mics, are the lowest 14 bits 0 (SPH0645: 18 valid bits),
//      noise floor, DC offset and clipping.
//   4. Checks the time anchors: timestampNs() works, the newest frame is stamped
//      close to CLOCK_REALTIME now, and the sample rate seen through the anchors.
//   5. Clap test: you clap close to mic 0 and then close to mic 1. The test finds
//      the clap through waitForFrames/copyFrames (like the event detection does) and
//      measures which mic heard it first with cross-correlation. The sign must flip
//      between the two claps, which proves the channel order and the timing.
//   6. Optionally saves everything in the ring buffer as a WAV file (--wav) that you
//      can open in Audacity and look at.
//
// Not part of ctest, because it needs the microphones and a person to clap.
//
// Build and run on the Pi:
//   cmake -S . -B build && cmake --build build --target alsa_hw_test
//   arecord -l                                   # find the card and device number
//   ./build/ringbuffer/alsa_hw_test hw:0,0 --wav capture.wav
//
// Options:
//   <device>       ALSA device, default hw:0,0
//   --no-clap      skip the clap test (step 5)
//   --wav <file>   save the ring buffer contents as a 32-bit stereo WAV file

#include "alsa_capture.h"
#include "ring_buffer.h"

#include <algorithm>
#include <atomic>
#include <bit>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <iostream>
#include <limits>
#include <stop_token>
#include <string>
#include <thread>
#include <vector>

#define SAMPLE_RATE_HZ 48000
#define PERIOD_FRAMES 512
#define ALSA_PERIODS 4
#define BUFFER_SECONDS 60          ///< Ring buffer length, also the length of the WAV file
#define SETTLE_SECONDS 1           ///< Capture runs this long before measuring starts
#define MEASURE_SECONDS 5          ///< Length of the silence measurement
#define SAMPLE_SHIFT 14            ///< SPH0645: 18 valid bits at the top of the 32-bit word
#define RATE_TOLERANCE 0.01        ///< Allowed error on the rate measured with steady_clock
#define MAX_CLOCK_PPM 1000.0       ///< Allowed error on the rate measured through the anchors
#define MIN_TIMESTAMP_AGE_MS -10.0 ///< Newest frame may be stamped at most this far in the future
#define MAX_TIMESTAMP_AGE_MS 100.0 ///< ... and at most this far in the past
#define CLAP_TIMEOUT_S 15          ///< How long to wait for each clap
#define CLAP_NOISE_FACTOR 20.0     ///< A clap must be 26 dB above the noise floor ...
#define CLAP_MIN_DBFS -40.0        ///< ... and at least this loud
#define CLAP_PRE_MS 20.0           ///< Window before the trigger used for cross-correlation
#define CLAP_POST_MS 60.0          ///< Window after the trigger
#define MAX_LAG_MS 10.0            ///< Same as pair_window_ms in the event detection (3 m + margin)
#define SPEED_OF_SOUND_M_S 343.0

/** @brief Statistics for one channel over a block of frames. */
struct CHANNEL_STATS
{
    double dc;              ///< Mean value (DC offset), raw 32-bit units
    double rms;             ///< RMS with the DC removed
    double peak;            ///< Largest |x - dc|
    uint32_t used_bits;     ///< All samples OR'ed together, shows which bits are ever set
    std::size_t clipped;    ///< Samples at full scale
    bool constant;          ///< Every sample had the same value
};

static int g_check_count = 0;
static int g_fail_count = 0;

static const double FULL_SCALE = 2147483648.0; // 2^31

/** @brief Registers one check and prints it. */
static void Check(bool condition, const std::string &description)
{
    ++g_check_count;
    if (!condition)
    {
        ++g_fail_count;
    }
    std::cout << (condition ? "  [ OK ] " : "  [FAIL] ") << description << '\n';
}

/** @brief Prints a measured value that is not a pass/fail check. */
static void Info(const std::string &text)
{
    std::cout << "         " << text << '\n';
}

static std::string Format(const char *format, double a, double b = 0.0, double c = 0.0)
{
    char text[256];
    std::snprintf(text, sizeof(text), format, a, b, c);
    return text;
}

static double ToDbfs(double value)
{
    return 20.0 * std::log10(std::max(value, 1.0) / FULL_SCALE);
}

static int64_t RealtimeNowNs()
{
    timespec now{};
    clock_gettime(CLOCK_REALTIME, &now);
    return static_cast<int64_t>(now.tv_sec) * 1000000000LL + now.tv_nsec;
}

/** @brief Formats a CLOCK_REALTIME/PTP time in ns as local time with microseconds. */
static std::string FormatTimeNs(int64_t t_ns)
{
    const time_t seconds = static_cast<time_t>(t_ns / 1000000000LL);
    const long micros = static_cast<long>((t_ns % 1000000000LL) / 1000);
    tm local{};
    localtime_r(&seconds, &local);
    char text[64];
    const std::size_t length = std::strftime(text, sizeof(text), "%H:%M:%S", &local);
    std::snprintf(text + length, sizeof(text) - length, ".%06ld", micros);
    return text;
}

static CHANNEL_STATS ComputeStats(const std::vector<Frame> &frames, int ch)
{
    CHANNEL_STATS stats{};
    if (frames.empty())
    {
        stats.constant = true;
        return stats;
    }

    // Full scale for 18 valid bits: the largest value is 0x7FFFC000, not INT32_MAX.
    const int32_t clip_high = std::numeric_limits<int32_t>::max() - ((1 << SAMPLE_SHIFT) - 1);
    const int32_t clip_low = std::numeric_limits<int32_t>::min();

    double sum = 0.0;
    int32_t min_value = frames[0].ch[ch];
    int32_t max_value = frames[0].ch[ch];
    for (const Frame &frame : frames)
    {
        const int32_t x = frame.ch[ch];
        sum += x;
        min_value = std::min(min_value, x);
        max_value = std::max(max_value, x);
        stats.used_bits |= static_cast<uint32_t>(x);
        if (x >= clip_high || x == clip_low)
        {
            ++stats.clipped;
        }
    }
    stats.dc = sum / static_cast<double>(frames.size());
    stats.constant = (min_value == max_value);

    double sum_sq = 0.0;
    for (const Frame &frame : frames)
    {
        const double x = frame.ch[ch] - stats.dc;
        sum_sq += x * x;
        stats.peak = std::max(stats.peak, std::fabs(x));
    }
    stats.rms = std::sqrt(sum_sq / static_cast<double>(frames.size()));
    return stats;
}

/**
 * @brief Lag in samples where ch[1] best matches ch[0].
 *
 * Positive lag: ch[1] is delayed, so the sound reached mic 0 first.
 * Negative lag: the sound reached mic 1 first.
 */
static int CrossCorrelationLag(const std::vector<Frame> &frames, const double dc[2], int max_lag)
{
    const int n = static_cast<int>(frames.size());
    std::vector<double> x0(n);
    std::vector<double> x1(n);
    for (int i = 0; i < n; ++i)
    {
        x0[i] = frames[i].ch[0] - dc[0];
        x1[i] = frames[i].ch[1] - dc[1];
    }

    int best_lag = 0;
    double best_sum = -std::numeric_limits<double>::infinity();
    for (int lag = -max_lag; lag <= max_lag; ++lag)
    {
        double sum = 0.0;
        for (int i = std::max(0, -lag); i < n && i + lag < n; ++i)
        {
            sum += x0[i] * x1[i + lag];
        }
        if (sum > best_sum)
        {
            best_sum = sum;
            best_lag = lag;
        }
    }
    return best_lag;
}

/** @brief First frame in @p frames where channel ch exceeds threshold, or -1. */
static long FirstCrossing(const std::vector<Frame> &frames, int ch, double dc, double threshold)
{
    for (std::size_t i = 0; i < frames.size(); ++i)
    {
        if (std::fabs(frames[i].ch[ch] - dc) > threshold)
        {
            return static_cast<long>(i);
        }
    }
    return -1;
}

/** @brief Writes interleaved S32_LE stereo frames as a 32-bit PCM WAV file. */
static bool WriteWav(const std::string &path, const std::vector<Frame> &frames)
{
    std::ofstream file(path, std::ios::binary);
    auto put32 = [&file](uint32_t v) {
        const char bytes[4] = {static_cast<char>(v), static_cast<char>(v >> 8),
                               static_cast<char>(v >> 16), static_cast<char>(v >> 24)};
        file.write(bytes, 4);
    };
    auto put16 = [&file](uint16_t v) {
        const char bytes[2] = {static_cast<char>(v), static_cast<char>(v >> 8)};
        file.write(bytes, 2);
    };

    const uint32_t data_bytes = static_cast<uint32_t>(frames.size() * sizeof(Frame));
    file.write("RIFF", 4);
    put32(36 + data_bytes);
    file.write("WAVE", 4);
    file.write("fmt ", 4);
    put32(16);                                // Size of the fmt chunk
    put16(1);                                 // PCM
    put16(2);                                 // Channels
    put32(SAMPLE_RATE_HZ);
    put32(SAMPLE_RATE_HZ * sizeof(Frame));    // Bytes per second
    put16(sizeof(Frame));                     // Bytes per frame
    put16(32);                                // Bits per sample
    file.write("data", 4);
    put32(data_bytes);
    // Frame has ALSA's interleaved S32_LE layout, and the Pi is little-endian.
    file.write(reinterpret_cast<const char *>(frames.data()), data_bytes);
    return file.good();
}

/**
 * @brief Waits for one clap and checks which mic heard it first.
 * @param mic The mic the user claps close to (0 or 1).
 */
static void ClapTest(const RingBuffer &rb, int mic, const CHANNEL_STATS noise[2],
                     std::stop_token capture_stopped)
{
    const double min_level = FULL_SCALE * std::pow(10.0, CLAP_MIN_DBFS / 20.0);
    const double threshold[2] = {std::max(CLAP_NOISE_FACTOR * noise[0].rms, min_level),
                                 std::max(CLAP_NOISE_FACTOR * noise[1].rms, min_level)};
    const double dc[2] = {noise[0].dc, noise[1].dc};

    std::cout << "\n  >>> Clap ONCE, close to mic " << mic << " (ch[" << mic << "]), within "
              << CLAP_TIMEOUT_S << " s ...\n" << std::flush;

    // Read new frames in chunks, the same way the event detection reads the buffer.
    uint64_t next = rb.writeIndex();
    long long trigger = -1;
    std::vector<Frame> chunk;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(CLAP_TIMEOUT_S);
    while (trigger < 0 && std::chrono::steady_clock::now() < deadline)
    {
        const uint64_t write_index = rb.waitForFrames(next + PERIOD_FRAMES, capture_stopped);
        if (capture_stopped.stop_requested())
        {
            Check(false, "capture thread stopped during the clap test");
            return;
        }
        chunk.resize(write_index - next);
        if (!rb.copyFrames(next, chunk.size(), chunk.data()))
        {
            Check(false, "copyFrames refused frames the reader had not read yet");
            return;
        }
        for (std::size_t i = 0; i < chunk.size() && trigger < 0; ++i)
        {
            for (int ch = 0; ch < 2; ++ch)
            {
                if (std::fabs(chunk[i].ch[ch] - dc[ch]) > threshold[ch])
                {
                    trigger = static_cast<long long>(next + i);
                    break;
                }
            }
        }
        next = write_index;
    }

    if (trigger < 0)
    {
        Check(false, "no clap detected (threshold " + Format("%.1f / %.1f dBFS)", ToDbfs(threshold[0]),
                                                             ToDbfs(threshold[1])));
        return;
    }

    // Copy a window around the trigger once all of it has been captured.
    const uint64_t pre = static_cast<uint64_t>(CLAP_PRE_MS * SAMPLE_RATE_HZ / 1000.0);
    const uint64_t post = static_cast<uint64_t>(CLAP_POST_MS * SAMPLE_RATE_HZ / 1000.0);
    const uint64_t start = static_cast<uint64_t>(trigger) - pre;
    std::vector<Frame> window(pre + post);
    rb.waitForFrames(start + window.size(), capture_stopped);
    if (!rb.copyFrames(start, window.size(), window.data()))
    {
        Check(false, "could not copy the frames around the clap");
        return;
    }

    const CHANNEL_STATS clap0 = ComputeStats(window, 0);
    const CHANNEL_STATS clap1 = ComputeStats(window, 1);
    Info(Format("Peak: ch[0] %.1f dBFS, ch[1] %.1f dBFS", ToDbfs(clap0.peak), ToDbfs(clap1.peak)));
    Check(clap0.peak > threshold[0] && clap1.peak > threshold[1], "both mics heard the clap");
    Check(clap0.clipped == 0 && clap1.clipped == 0, "the clap did not clip (else clap a bit further away)");

    int64_t t_ns = 0;
    if (rb.timestampNs(static_cast<uint64_t>(trigger), t_ns))
    {
        Info("Clap at frame " + std::to_string(trigger) + ", synced time " + FormatTimeNs(t_ns));
    }
    else
    {
        Check(false, "timestampNs() refused the clap frame");
    }

    const long onset0 = FirstCrossing(window, 0, dc[0], threshold[0]);
    const long onset1 = FirstCrossing(window, 1, dc[1], threshold[1]);
    if (onset0 >= 0 && onset1 >= 0)
    {
        Info("Threshold crossing: ch[1] - ch[0] = " + std::to_string(onset1 - onset0) + " samples");
    }

    const int max_lag = static_cast<int>(MAX_LAG_MS * SAMPLE_RATE_HZ / 1000.0);
    const int lag = CrossCorrelationLag(window, dc, max_lag);
    const double lag_us = lag * 1e6 / SAMPLE_RATE_HZ;
    Info("Cross-correlation: ch[1] lags ch[0] by " + std::to_string(lag) + " samples = " +
         Format("%.0f us = %.1f cm path difference", lag_us, lag_us * 1e-6 * SPEED_OF_SOUND_M_S * 100.0));
    if (mic == 0)
    {
        Check(lag > 0, "the sound reached mic 0 first (positive lag)");
    }
    else
    {
        Check(lag < 0, "the sound reached mic 1 first (negative lag)");
    }

    // Let the echo die out before the next clap.
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

int main(int argc, char **argv)
{
    std::string device = "hw:0,0";
    std::string wav_path;
    bool clap = true;
    for (int i = 1; i < argc; ++i)
    {
        const std::string arg = argv[i];
        if (arg == "--no-clap")
        {
            clap = false;
        }
        else if (arg == "--wav" && i + 1 < argc)
        {
            wav_path = argv[++i];
        }
        else if (arg == "-h" || arg == "--help")
        {
            std::cout << "Usage: " << argv[0] << " [device, default hw:0,0] [--no-clap] [--wav <file>]\n";
            return 0;
        }
        else
        {
            device = arg;
        }
    }

    // ---- 1. Open the device and start capturing ----
    std::cout << "1. Open " << device << " (S32_LE, 2 channels, " << SAMPLE_RATE_HZ << " Hz)\n";
    RingBuffer rb(static_cast<std::size_t>(BUFFER_SECONDS) * SAMPLE_RATE_HZ, SAMPLE_RATE_HZ);
    AlsaCapture capture(rb);
    std::string error;
    const bool opened = capture.open({device, SAMPLE_RATE_HZ, PERIOD_FRAMES, ALSA_PERIODS}, error);
    Check(opened, "device opened and configured");
    if (!opened)
    {
        Info(error);
        Info("Find the right device with: arecord -l   (card X, device Y -> hw:X,Y)");
        return 1;
    }

    std::stop_source capture_stopped;  // Requested when the capture thread ends, wakes waiting readers
    std::atomic<bool> capture_ok{true};
    std::jthread capture_thread([&](std::stop_token st) {
        capture_ok = capture.run(st);
        capture_stopped.request_stop();
    });

    // ---- 2. Sample rate against the Pi's own clock ----
    std::cout << "\n2. Sample rate (keep quiet for " << SETTLE_SECONDS + MEASURE_SECONDS << " s)\n";
    std::this_thread::sleep_for(std::chrono::seconds(SETTLE_SECONDS));
    const auto t1 = std::chrono::steady_clock::now();
    const uint64_t w1 = rb.writeIndex();
    std::this_thread::sleep_for(std::chrono::seconds(MEASURE_SECONDS));
    const auto t2 = std::chrono::steady_clock::now();
    const uint64_t w2 = rb.writeIndex();

    const double elapsed_s = std::chrono::duration<double>(t2 - t1).count();
    const double rate_hz = static_cast<double>(w2 - w1) / elapsed_s;
    Info(Format("%.0f frames in %.3f s = %.1f Hz", static_cast<double>(w2 - w1), elapsed_s, rate_hz));
    Check(capture_ok, "capture thread is running");
    Check(std::fabs(rate_hz - SAMPLE_RATE_HZ) < RATE_TOLERANCE * SAMPLE_RATE_HZ,
          Format("sample rate within %.0f %% of %.0f Hz", RATE_TOLERANCE * 100.0, SAMPLE_RATE_HZ));

    // ---- 3. The signal itself, measured on the quiet seconds ----
    std::cout << "\n3. Signal in silence (" << MEASURE_SECONDS << " s)\n";
    std::vector<Frame> quiet(std::min<uint64_t>(w2 - w1, static_cast<uint64_t>(MEASURE_SECONDS) * SAMPLE_RATE_HZ));
    const bool copied = rb.copyFrames(w2 - quiet.size(), quiet.size(), quiet.data());
    Check(copied && !quiet.empty(), "copyFrames returns the last " + std::to_string(quiet.size()) + " frames");

    CHANNEL_STATS noise[2] = {ComputeStats(quiet, 0), ComputeStats(quiet, 1)};
    for (int ch = 0; ch < 2; ++ch)
    {
        const CHANNEL_STATS &s = noise[ch];
        Info("ch[" + std::to_string(ch) + "]: " +
             Format("noise %.1f dBFS RMS, peak %.1f dBFS, ", ToDbfs(s.rms), ToDbfs(s.peak)) +
             Format("DC offset %.3f %% of full scale", 100.0 * s.dc / FULL_SCALE));
        Check(!s.constant, "ch[" + std::to_string(ch) + "] changes over time (mic connected, on its own L/R slot)");

        const int lowest_bit = (s.used_bits == 0) ? 32 : std::countr_zero(s.used_bits);
        Info("ch[" + std::to_string(ch) + "]: " + std::to_string(32 - lowest_bit) + " bits in use, lowest " +
             std::to_string(lowest_bit) + " bits always 0");
        Check(lowest_bit >= SAMPLE_SHIFT,
              "ch[" + std::to_string(ch) + "] lowest " + std::to_string(SAMPLE_SHIFT) +
                  " bits always 0 (sample_shift = 14 in the event detection is right)");
        Check(s.clipped == 0, "ch[" + std::to_string(ch) + "] does not clip in silence");
    }

    std::size_t identical = 0;
    for (const Frame &frame : quiet)
    {
        identical += (frame.ch[0] == frame.ch[1]) ? 1 : 0;
    }
    Info(Format("%.1f %% of the frames have ch[0] == ch[1]", quiet.empty() ? 0.0 : 100.0 * identical / quiet.size()));
    Check(identical < quiet.size() / 2, "ch[0] and ch[1] are two different mics (not a copy of each other)");

    // ---- 4. Time anchors ----
    std::cout << "\n4. Time anchors / timestampNs\n";
    const uint64_t newest = rb.writeIndex() - 1;
    int64_t t_newest = 0;
    const bool has_timestamp = rb.timestampNs(newest, t_newest);
    const int64_t now_ns = RealtimeNowNs();
    Check(has_timestamp, "timestampNs() works (enough anchors, no clock jump or overrun)");
    if (has_timestamp)
    {
        const double age_ms = static_cast<double>(now_ns - t_newest) / 1e6;
        Info("Newest frame stamped " + FormatTimeNs(t_newest) + ", CLOCK_REALTIME now " + FormatTimeNs(now_ns));
        Check(age_ms > MIN_TIMESTAMP_AGE_MS && age_ms < MAX_TIMESTAMP_AGE_MS,
              Format("newest frame is stamped %.1f ms before now (allowed %.0f to %.0f ms)", age_ms,
                     MIN_TIMESTAMP_AGE_MS, MAX_TIMESTAMP_AGE_MS));

        const uint64_t span = static_cast<uint64_t>(MEASURE_SECONDS - 1) * SAMPLE_RATE_HZ;
        int64_t t_old = 0;
        if (rb.timestampNs(newest - span, t_old))
        {
            const double anchor_rate_hz = static_cast<double>(span) / (static_cast<double>(t_newest - t_old) / 1e9);
            const double ppm = (anchor_rate_hz / SAMPLE_RATE_HZ - 1.0) * 1e6;
            Info(Format("Sample rate seen through the anchors: %.3f Hz (%+.0f ppm)", anchor_rate_hz, ppm));
            Check(std::fabs(ppm) < MAX_CLOCK_PPM, Format("within %.0f ppm of nominal", MAX_CLOCK_PPM));
        }
        else
        {
            Check(false, "timestampNs() refused a frame from " + std::to_string(MEASURE_SECONDS - 1) + " s ago");
        }
    }

    // ---- 5. Clap test ----
    if (clap)
    {
        std::cout << "\n5. Clap test (clap 10-30 cm from one mic, as far from the other as possible)\n";
        ClapTest(rb, 0, noise, capture_stopped.get_token());
        ClapTest(rb, 1, noise, capture_stopped.get_token());
    }

    // ---- Stop and summarise ----
    capture_thread.request_stop();
    capture_thread.join();

    std::cout << "\nCapture\n";
    Check(capture_ok, "capture thread ended without errors");
    Check(capture.overrunCount() == 0, "no ALSA overruns (" + std::to_string(capture.overrunCount()) + ")");

    if (!wav_path.empty())
    {
        const uint64_t total = std::min<uint64_t>(rb.writeIndex(), static_cast<uint64_t>(BUFFER_SECONDS) * SAMPLE_RATE_HZ);
        std::vector<Frame> all(total);
        const bool saved = rb.copyFrames(rb.writeIndex() - total, all.size(), all.data()) && WriteWav(wav_path, all);
        Check(saved, "saved " + Format("%.1f s to ", static_cast<double>(total) / SAMPLE_RATE_HZ) + wav_path);
    }

    std::cout << '\n' << (g_check_count - g_fail_count) << '/' << g_check_count << " checks passed\n";
    return g_fail_count == 0 ? 0 : 1;
}
