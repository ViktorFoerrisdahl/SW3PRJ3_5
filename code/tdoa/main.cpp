#include <cmath>
#include <cstdio>
#include "tdoa.h"

static constexpr uint32_t SAMPLE_RATE_HZ = 48000;
static constexpr size_t FRAME_COUNT = 2048;     // Frames per event
static constexpr long PRE_TRIGGER_FRAMES = 300; // Silence before the clap in each buffer

static constexpr double PULSE_WIDTH_SAMPLES = 6.0;   // Width of the Gaussian clap pulse
static constexpr double PULSE_CENTRE_SAMPLES = 32.0; // Pulse peak position after its start

// Builds one DetectedEvent per node with the same clap in every channel,
// delayed by distance / speed of sound. The clap is a smooth Gaussian pulse
// placed at the exact (fractional) delay, so the true peak lies between two
// samples - this tests the parabolic interpolation in CrossCorrelate.
// Node 2's buffer starts node2_offset_ns later than node 1's, to test that
// t_first_ns is taken into account.
static std::vector<DetectedEvent> MakeMockEvents(const std::vector<MIC_POSITION>& mics,
                                                 double src_x_m, double src_y_m,
                                                 int64_t node2_offset_ns)
{
    // 1. One event per node, filled with silent frames
    DetectedEvent node1;
    node1.node_id = 1;
    node1.sample_rate = SAMPLE_RATE_HZ;
    node1.t_first_ns = 0;
    node1.frames.assign(FRAME_COUNT, Frame{{0, 0}});

    DetectedEvent node2 = node1;
    node2.node_id = 2;
    node2.t_first_ns = node2_offset_ns;

    // 2. Write the clap into the channel belonging to each microphone
    for (const auto& mic : mics)
    {
        DetectedEvent& event = (mic.node_id == 1) ? node1 : node2;

        double arrival_s = std::hypot(src_x_m - mic.x_m, src_y_m - mic.y_m) / tdoa::SPEED_OF_SOUND_M_PER_S;
        double arrival_in_buffer_s = arrival_s - event.t_first_ns * 1e-9; // Relative to this buffer's first frame

        // Exact start position in samples, e.g. 412.37 - not rounded
        double start = PRE_TRIGGER_FRAMES + arrival_in_buffer_s * SAMPLE_RATE_HZ;

        for (size_t n = 0; n < FRAME_COUNT; n++)
        {
            double t = ((double)n - start - PULSE_CENTRE_SAMPLES) / PULSE_WIDTH_SAMPLES;
            event.frames[n].ch[mic.channel] = (int32_t)std::lround(1e6 * std::exp(-t * t)); // Interleaved: channel picks ch[0] or ch[1]
        }
    }

    // 3.
    return {node1, node2};
}

int main()
{
    tdoa locator; // TDOA object used for all tests

    // {node_id, channel, x_m, y_m}
    std::vector<MIC_POSITION> mics =
    {
        {1, 0, 0.0, 0.0},
        {1, 1, 3.0, 0.0},
        {2, 0, 3.0, 3.0},
        {2, 1, 0.0, 3.0}
    };

    double test_points[][2] = {{1.783, 2.988}, {1.575, 1.28999}, {0.59008, 0.29005}, {2.823, 1.512357}};

    // 0 ns: both buffers start together. 1.5 ms: node 2 starts later.
    int64_t node2_offsets_ns[] = {0, 1500000};

    for (int64_t offset_ns : node2_offsets_ns)
    {
        std::printf("--- Node 2 offset: %lld ns ---\n", (long long)offset_ns);

        for (auto& point : test_points)
        {
            TDOA_RESULT result = locator.ComputePosition(MakeMockEvents(mics, point[0], point[1], offset_ns), mics);
            double error_cm = 100.0 * std::hypot(result.x_m - point[0], result.y_m - point[1]);

            std::printf("TRUE=(%.2f, %.2f)  EST=(%.2f, %.2f)  ERROR=%.1f cm  VALID=%d\n",
                        point[0], point[1], result.x_m, result.y_m, error_cm, result.valid);
        }
    }

    return 0;
}
