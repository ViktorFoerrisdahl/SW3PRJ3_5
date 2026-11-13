// Unit test for RingBuffer (ring_buffer.h).
//
// Needs no ALSA, so it builds and runs on both Windows and the Raspberry Pi.
// Each test writes known frames / time anchors and checks the result.
// The program returns 0 if every check passes, 1 otherwise.
//
// Build and run with CMake:
//   cmake -S . -B build
//   cmake --build build --target ring_buffer_test
//   ctest --test-dir build --output-on-failure

#include "ring_buffer.h"

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <stop_token>
#include <thread>
#include <vector>

#define SAMPLE_RATE_HZ 48000
#define PERIOD_FRAMES 512                        ///< One time anchor per period, as in AlsaCapture
#define START_TIME_NS 1700000000000000000LL      ///< Realistic CLOCK_REALTIME value (year 2023)
#define TIMESTAMP_TOLERANCE_NS 2                 ///< Allowed rounding error on an exact line
#define WAIT_TIMEOUT_MS 2000                     ///< Watchdog so a broken wait cannot hang the test

static int g_check_count = 0;
static int g_fail_count = 0;

/** @brief Registers one check and prints a line if it fails. */
static void Check(bool condition, const char *description)
{
    ++g_check_count;
    if (!condition)
    {
        ++g_fail_count;
        std::cerr << "  FAIL: " << description << '\n';
    }
}

/** @brief Runs one test function and prints whether it passed. */
static void RunTest(void (*test)(), const char *name)
{
    const int fails_before = g_fail_count;
    test();
    std::cout << (g_fail_count == fails_before ? "[ OK ] " : "[FAIL] ") << name << '\n';
}

/** @brief Known frame for global frame number n. The channels differ so a swap is caught. */
static Frame MakeFrame(uint64_t n)
{
    const int32_t value = static_cast<int32_t>(n);
    return Frame{{value, -value - 1}};
}

/** @brief Writes frames first .. first + count - 1 in one call. */
static void WriteFrames(RingBuffer &rb, uint64_t first, std::size_t count)
{
    std::vector<Frame> frames(count);
    for (std::size_t i = 0; i < count; ++i)
    {
        frames[i] = MakeFrame(first + i);
    }
    rb.write(frames.data(), frames.size());
}

/** @brief Copies count frames from start and checks they are exactly start .. start + count - 1. */
static bool CopyMatches(const RingBuffer &rb, uint64_t start, std::size_t count)
{
    std::vector<Frame> dst(count);
    if (!rb.copyFrames(start, count, dst.data()))
    {
        return false;
    }
    for (std::size_t i = 0; i < count; ++i)
    {
        const Frame expected = MakeFrame(start + i);
        if (dst[i].ch[0] != expected.ch[0] || dst[i].ch[1] != expected.ch[1])
        {
            return false;
        }
    }
    return true;
}

/** @brief True if copyFrames accepts the range (the return value only, not the contents). */
static bool CopyAccepted(const RingBuffer &rb, uint64_t start, std::size_t count)
{
    std::vector<Frame> dst(count);
    return rb.copyFrames(start, count, dst.data());
}

/** @brief Synced time of a frame on a line with the given start time and sample period. */
static int64_t LineNs(uint64_t frame, double ns_per_frame, int64_t start_ns = START_TIME_NS)
{
    return start_ns + std::llround(static_cast<double>(frame) * ns_per_frame);
}

/** @brief Adds count anchors, one per period, starting at first_frame, on the given line. */
static void AddAnchors(RingBuffer &rb, uint64_t first_frame, int count, double ns_per_frame,
                       int64_t start_ns = START_TIME_NS)
{
    for (int i = 0; i < count; ++i)
    {
        const uint64_t frame = first_frame + static_cast<uint64_t>(i) * PERIOD_FRAMES;
        rb.addTimeAnchor(frame, LineNs(frame, ns_per_frame, start_ns));
    }
}

/** @brief True if timestampNs succeeds and is within tolerance_ns of expected_ns. */
static bool TimestampNear(const RingBuffer &rb, uint64_t frame, int64_t expected_ns,
                          int64_t tolerance_ns = TIMESTAMP_TOLERANCE_NS)
{
    int64_t t_ns = 0;
    return rb.timestampNs(frame, t_ns) && std::llabs(t_ns - expected_ns) <= tolerance_ns;
}

static const double NOMINAL_NS_PER_FRAME = 1e9 / SAMPLE_RATE_HZ;

// ---- Frame storage ----

static void TestEmptyBuffer()
{
    RingBuffer rb(8, SAMPLE_RATE_HZ);
    Frame dst = MakeFrame(1234);

    Check(rb.writeIndex() == 0, "new buffer has writeIndex 0");
    Check(!rb.copyFrames(0, 1, &dst), "copying from an empty buffer fails");
    Check(dst.ch[0] == 1234, "dst is untouched when copyFrames fails");
}

static void TestWriteAndCopy()
{
    RingBuffer rb(8, SAMPLE_RATE_HZ);
    WriteFrames(rb, 0, 5);

    Check(rb.writeIndex() == 5, "writeIndex counts the written frames");
    Check(CopyMatches(rb, 0, 5), "all 5 frames can be copied back in order");
    Check(CopyMatches(rb, 2, 3), "a range in the middle can be copied");
}

static void TestCopyNotYetWritten()
{
    RingBuffer rb(8, SAMPLE_RATE_HZ);
    WriteFrames(rb, 0, 5);

    Check(!CopyAccepted(rb, 3, 3), "a range that reaches past writeIndex is refused");
    Check(!CopyAccepted(rb, 5, 1), "the frame at writeIndex is not written yet");
}

static void TestOverwrittenFrames()
{
    RingBuffer rb(8, SAMPLE_RATE_HZ);
    WriteFrames(rb, 0, 20);

    Check(rb.writeIndex() == 20, "writeIndex keeps counting after the buffer wraps");
    Check(CopyMatches(rb, 12, 8), "the 8 newest frames (12..19) can be copied");
    Check(!CopyAccepted(rb, 11, 1), "frame 11 is overwritten and is refused");
    Check(!CopyAccepted(rb, 11, 9), "a range that starts in overwritten frames is refused");
}

static void TestWrapInsideOneWrite()
{
    RingBuffer rb(8, SAMPLE_RATE_HZ);
    WriteFrames(rb, 0, 6);
    WriteFrames(rb, 6, 5); // Wraps around the end of the buffer in a single write

    Check(CopyMatches(rb, 3, 8), "frames 3..10 are correct across the wrap");
}

static void TestWriteLargerThanCapacity()
{
    RingBuffer rb(8, SAMPLE_RATE_HZ);
    WriteFrames(rb, 0, 20); // One write of more frames than the buffer holds

    Check(CopyMatches(rb, 12, 8), "only the newest 8 frames remain after an oversized write");
}

static void TestSampleExtremes()
{
    // ALSA delivers S32_LE, so the full int32 range must survive unchanged.
    RingBuffer rb(4, SAMPLE_RATE_HZ);
    const int32_t min_sample = std::numeric_limits<int32_t>::min();
    const int32_t max_sample = std::numeric_limits<int32_t>::max();
    const Frame frames[2] = {{{min_sample, max_sample}}, {{max_sample, min_sample}}};
    rb.write(frames, 2);

    Frame dst[2] = {};
    Check(rb.copyFrames(0, 2, dst), "copying the extreme frames succeeds");
    Check(dst[0].ch[0] == min_sample && dst[0].ch[1] == max_sample,
          "frame 0 keeps min on ch[0] and max on ch[1]");
    Check(dst[1].ch[0] == max_sample && dst[1].ch[1] == min_sample,
          "frame 1 keeps max on ch[0] and min on ch[1]");
}

// ---- waitForFrames ----

/**
 * @brief Calls waitForFrames, but stops the wait after WAIT_TIMEOUT_MS.
 *
 * A broken buffer then makes the test fail instead of hanging forever.
 * @param timed_out Set to true if the watchdog had to stop the wait.
 */
static uint64_t WaitWithWatchdog(const RingBuffer &rb, uint64_t min_write_index, bool &timed_out)
{
    std::stop_source stop;
    std::jthread watchdog([&stop](std::stop_token st) {
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(WAIT_TIMEOUT_MS);
        while (!st.stop_requested())
        {
            if (std::chrono::steady_clock::now() >= deadline)
            {
                stop.request_stop();
                return;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    });

    const uint64_t index = rb.waitForFrames(min_write_index, stop.get_token());
    timed_out = stop.stop_requested(); // Read before the watchdog is stopped
    watchdog.request_stop();
    return index;
}

static void TestWaitAlreadyAvailable()
{
    RingBuffer rb(8, SAMPLE_RATE_HZ);
    WriteFrames(rb, 0, 5);

    bool timed_out = false;
    Check(WaitWithWatchdog(rb, 5, timed_out) == 5, "returns writeIndex (5) when the frames exist");
    Check(!timed_out, "returns at once without waiting for the timeout");
}

static void TestWaitWakesOnWrite()
{
    RingBuffer rb(1024, SAMPLE_RATE_HZ);

    // Plays the capture thread: writes one period after a short delay.
    std::jthread writer([&rb] {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        WriteFrames(rb, 0, PERIOD_FRAMES);
    });

    bool timed_out = false;
    const uint64_t index = WaitWithWatchdog(rb, PERIOD_FRAMES, timed_out);

    Check(index >= PERIOD_FRAMES, "waitForFrames returns the new writeIndex");
    Check(!timed_out, "the reader is woken by the write, not by the watchdog timeout");
}

static void TestWaitReturnsOnStop()
{
    RingBuffer rb(8, SAMPLE_RATE_HZ);
    std::stop_source stop;

    std::jthread stopper([&stop] {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        stop.request_stop();
    });

    Check(rb.waitForFrames(100, stop.get_token()) == 0,
          "the wait returns the current writeIndex (0) when stop is requested");
}

// ---- Time anchors and timestampNs ----

static void TestTimestampNeedsMinAnchors()
{
    RingBuffer rb(8, SAMPLE_RATE_HZ);
    int64_t t_ns = 0;

    Check(!rb.timestampNs(0, t_ns), "no timestamp without anchors");
    AddAnchors(rb, 0, MIN_TIME_ANCHORS - 1, NOMINAL_NS_PER_FRAME);
    Check(!rb.timestampNs(0, t_ns), "no timestamp with one anchor too few");
    rb.addTimeAnchor((MIN_TIME_ANCHORS - 1) * PERIOD_FRAMES,
                     LineNs((MIN_TIME_ANCHORS - 1) * PERIOD_FRAMES, NOMINAL_NS_PER_FRAME));
    Check(rb.timestampNs(0, t_ns), "timestamp works once MIN_TIME_ANCHORS anchors exist");
}

static void TestTimestampOnExactLine()
{
    RingBuffer rb(8, SAMPLE_RATE_HZ);
    const uint64_t first_anchor = 100000;
    AddAnchors(rb, first_anchor, 16, NOMINAL_NS_PER_FRAME);
    const uint64_t last_anchor = first_anchor + 15 * PERIOD_FRAMES;

    Check(TimestampNear(rb, first_anchor + 1234, LineNs(first_anchor + 1234, NOMINAL_NS_PER_FRAME)),
          "a frame between anchors gets the right time");
    Check(TimestampNear(rb, last_anchor + 1000, LineNs(last_anchor + 1000, NOMINAL_NS_PER_FRAME)),
          "a frame after the newest anchor is extrapolated correctly");
    // Frames still in the buffer can be older than the oldest anchor, which makes
    // frame - ref.frame negative. That must not wrap around as an unsigned number.
    Check(TimestampNear(rb, first_anchor - 5000, LineNs(first_anchor - 5000, NOMINAL_NS_PER_FRAME)),
          "a frame before the oldest anchor is extrapolated correctly");
}

static void TestTimestampFollowsCrystalDrift()
{
    // The I2S crystal runs 50 ppm fast: the fit must use the measured slope, not 1/48000.
    RingBuffer rb(8, SAMPLE_RATE_HZ);
    const double real_ns_per_frame = NOMINAL_NS_PER_FRAME / (1.0 + 50e-6);
    AddAnchors(rb, 0, 32, real_ns_per_frame);

    const uint64_t frame = 40 * PERIOD_FRAMES;
    const int64_t real_ns = LineNs(frame, real_ns_per_frame);
    const int64_t nominal_ns = LineNs(frame, NOMINAL_NS_PER_FRAME);

    Check(std::llabs(nominal_ns - real_ns) > 1000, "test setup: drift is larger than 1 us here");
    Check(TimestampNear(rb, frame, real_ns), "timestamp follows the real (drifting) sample rate");
}

static void TestTimestampAveragesJitter()
{
    // ALSA timestamps jitter. Alternating +-20 us must average out on the fitted line.
    RingBuffer rb(8, SAMPLE_RATE_HZ);
    const int64_t jitter_ns = 20000;
    for (int i = 0; i < 64; ++i)
    {
        const uint64_t frame = static_cast<uint64_t>(i) * PERIOD_FRAMES;
        const int64_t noise_ns = (i % 2 == 0) ? jitter_ns : -jitter_ns;
        rb.addTimeAnchor(frame, LineNs(frame, NOMINAL_NS_PER_FRAME) + noise_ns);
    }

    const uint64_t frame = 32 * PERIOD_FRAMES + 100;
    Check(TimestampNear(rb, frame, LineNs(frame, NOMINAL_NS_PER_FRAME), jitter_ns / 10),
          "the fit is much closer to the true line than the jitter");
}

static void TestSameFrameFallsBackToNominalRate()
{
    // All anchors on the same frame give no slope, so the nominal sample rate is used.
    RingBuffer rb(8, SAMPLE_RATE_HZ);
    for (int i = 0; i < MIN_TIME_ANCHORS; ++i)
    {
        rb.addTimeAnchor(1000, START_TIME_NS);
    }

    Check(TimestampNear(rb, 1000 + SAMPLE_RATE_HZ, START_TIME_NS + 1000000000LL),
          "one second of frames later is 1e9 ns later at the nominal rate");
}

static void TestClockJumpDiscardsAnchors()
{
    RingBuffer rb(8, SAMPLE_RATE_HZ);
    AddAnchors(rb, 0, 16, NOMINAL_NS_PER_FRAME);

    // phc2sys steps the clock 10 ms: the new anchor is far off the old line.
    const int64_t jump_ns = 10000000;
    const uint64_t jump_frame = 16 * PERIOD_FRAMES;
    AddAnchors(rb, jump_frame, 1, NOMINAL_NS_PER_FRAME, START_TIME_NS + jump_ns);

    int64_t t_ns = 0;
    Check(!rb.timestampNs(jump_frame - 1, t_ns), "frames from before the jump can no longer be timestamped");
    Check(!rb.timestampNs(jump_frame, t_ns), "only one anchor after the jump is too few");

    AddAnchors(rb, jump_frame + PERIOD_FRAMES, MIN_TIME_ANCHORS - 1, NOMINAL_NS_PER_FRAME,
               START_TIME_NS + jump_ns);
    Check(TimestampNear(rb, jump_frame + 100, LineNs(jump_frame + 100, NOMINAL_NS_PER_FRAME, START_TIME_NS + jump_ns)),
          "after enough new anchors, timestamps follow the new clock");
}

static void TestSmallDeviationKeepsAnchors()
{
    // 1 ms off the line is below CLOCK_STEP_LIMIT_NS (2 ms), so it is not a clock jump.
    RingBuffer rb(8, SAMPLE_RATE_HZ);
    AddAnchors(rb, 0, 16, NOMINAL_NS_PER_FRAME);
    const uint64_t frame = 16 * PERIOD_FRAMES;
    rb.addTimeAnchor(frame, LineNs(frame, NOMINAL_NS_PER_FRAME) + 1000000);

    int64_t t_ns = 0;
    Check(rb.timestampNs(100, t_ns), "old frames can still be timestamped after a small deviation");
}

static void TestResetAfterOverrun()
{
    RingBuffer rb(8, SAMPLE_RATE_HZ);
    AddAnchors(rb, 0, 16, NOMINAL_NS_PER_FRAME);

    // An overrun loses frames, so AlsaCapture discards the anchors from writeIndex().
    const uint64_t reset_frame = 20 * PERIOD_FRAMES;
    rb.resetTimeAnchors(reset_frame);

    int64_t t_ns = 0;
    Check(!rb.timestampNs(reset_frame, t_ns), "no timestamps right after a reset");

    // After the overrun the frames are on a new line (here 5 ms later than before).
    const int64_t new_start_ns = START_TIME_NS + 5000000;
    AddAnchors(rb, reset_frame, MIN_TIME_ANCHORS, NOMINAL_NS_PER_FRAME, new_start_ns);
    Check(!rb.timestampNs(reset_frame - 1, t_ns), "frames from before the overrun stay invalid");
    Check(TimestampNear(rb, reset_frame + 10, LineNs(reset_frame + 10, NOMINAL_NS_PER_FRAME, new_start_ns)),
          "frames after the overrun use the new anchors");
}

static void TestOnlyRecentAnchorsAreUsed()
{
    // First MAX_TIME_ANCHORS anchors at one rate, then MAX_TIME_ANCHORS at a rate 20 ppm
    // different (small enough not to count as a clock jump). The old anchors must have been
    // dropped, so the fit matches the new rate exactly.
    RingBuffer rb(8, SAMPLE_RATE_HZ);
    const double old_ns_per_frame = NOMINAL_NS_PER_FRAME;
    const double new_ns_per_frame = NOMINAL_NS_PER_FRAME * (1.0 + 20e-6);

    AddAnchors(rb, 0, MAX_TIME_ANCHORS, old_ns_per_frame);
    const uint64_t switch_frame = MAX_TIME_ANCHORS * PERIOD_FRAMES;
    // Start the new line where the old one ends, so there is no jump.
    const int64_t switch_ns = LineNs(switch_frame, old_ns_per_frame);
    const int64_t new_start_ns = switch_ns - std::llround(static_cast<double>(switch_frame) * new_ns_per_frame);
    AddAnchors(rb, switch_frame, MAX_TIME_ANCHORS, new_ns_per_frame, new_start_ns);

    const uint64_t frame = switch_frame + (MAX_TIME_ANCHORS - 1) * PERIOD_FRAMES;
    Check(TimestampNear(rb, frame, LineNs(frame, new_ns_per_frame, new_start_ns)),
          "only the newest MAX_TIME_ANCHORS anchors are used in the fit");
}

int main()
{
    std::cout << "Frame storage\n";
    RunTest(TestEmptyBuffer, "Empty buffer");
    RunTest(TestWriteAndCopy, "Write and copy back");
    RunTest(TestCopyNotYetWritten, "Frames not written yet are refused");
    RunTest(TestOverwrittenFrames, "Overwritten frames are refused");
    RunTest(TestWrapInsideOneWrite, "Wrap inside one write");
    RunTest(TestWriteLargerThanCapacity, "Write larger than capacity");
    RunTest(TestSampleExtremes, "int32 extremes on both channels");

    std::cout << "\nwaitForFrames\n";
    RunTest(TestWaitAlreadyAvailable, "Returns at once when frames exist");
    RunTest(TestWaitWakesOnWrite, "Wakes up when frames are written");
    RunTest(TestWaitReturnsOnStop, "Returns when stop is requested");

    std::cout << "\nTime anchors / timestampNs\n";
    RunTest(TestTimestampNeedsMinAnchors, "Needs MIN_TIME_ANCHORS anchors");
    RunTest(TestTimestampOnExactLine, "Exact line, inside and outside the anchors");
    RunTest(TestTimestampFollowsCrystalDrift, "Follows crystal drift");
    RunTest(TestTimestampAveragesJitter, "Averages out timestamp jitter");
    RunTest(TestSameFrameFallsBackToNominalRate, "Falls back to nominal rate");
    RunTest(TestClockJumpDiscardsAnchors, "Clock jump discards old anchors");
    RunTest(TestSmallDeviationKeepsAnchors, "Small deviation is not a clock jump");
    RunTest(TestResetAfterOverrun, "Reset after overrun");
    RunTest(TestOnlyRecentAnchorsAreUsed, "Only the newest anchors are used");

    std::cout << '\n' << (g_check_count - g_fail_count) << '/' << g_check_count << " checks passed\n";
    return g_fail_count == 0 ? 0 : 1;
}
