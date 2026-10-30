#include <cmath>
#include <cstdio>
#include <random>
#include "tdoa.h"

static constexpr uint32_t SAMPLE_RATE_HZ = 48000;
static constexpr size_t FRAME_COUNT = 2048;     // Frames per event
static constexpr long PRE_TRIGGER_FRAMES = 300; // Silence before the clap in each buffer

// Builds one DetectedEvent per node with the same clap in every channel,
// delayed by distance / speed of sound. Node 2's buffer starts node2_offset_ns
// later than node 1's, to test that t_first_ns is taken into account.
static std::vector<DetectedEvent> MakeMockEvents(const std::vector<MIC_POSITION>& mics,
                                                 double src_x_m, double src_y_m,
                                                 int64_t node2_offset_ns)
{
    // 1. The clap: 64 random values, fixed seed -> same every run
    std::mt19937 rng(42);
    std::uniform_int_distribution<int32_t> noise(-1000, 1000);
    std::vector<int32_t> clap(64);
    for (int32_t& sample : clap) sample = noise(rng);

    // 2. One event per node, filled with silent frames
    DetectedEvent node1;
    node1.node_id = 1;
    node1.sample_rate = SAMPLE_RATE_HZ;
    node1.t_first_ns = 0;
    node1.frames.assign(FRAME_COUNT, Frame{{0, 0}});

    DetectedEvent node2 = node1;
    node2.node_id = 2;
    node2.t_first_ns = node2_offset_ns;

    // 3. Write the clap into the channel belonging to each microphone
    for (const auto& mic : mics)
    {
        DetectedEvent& event = (mic.node_id == 1) ? node1 : node2;

        double arrival_s = std::hypot(src_x_m - mic.x_m, src_y_m - mic.y_m) / SPEED_OF_SOUND_M_PER_S;
        double arrival_in_buffer_s = arrival_s - event.t_first_ns * 1e-9; // Relative to this buffer's first frame
        long start = PRE_TRIGGER_FRAMES + std::lround(arrival_in_buffer_s * SAMPLE_RATE_HZ);

        for (size_t i = 0; i < clap.size(); i++)
        {
            long idx = start + (long)i;
            if (idx >= 0 && idx < (long)FRAME_COUNT)
                event.frames[idx].ch[mic.channel] = clap[i]; // Interleaved: channel picks ch[0] or ch[1]
        }
    }

    // 4.
    return {node1, node2};
}

int main()
{
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
            TDOA_RESULT result = Tdoa_ComputePosition(MakeMockEvents(mics, point[0], point[1], offset_ns), mics);
            double error_cm = 100.0 * std::hypot(result.x_m - point[0], result.y_m - point[1]);

            std::printf("TRUE=(%.2f, %.2f)  EST=(%.2f, %.2f)  ERROR=%.1f cm  VALID=%d\n",
                        point[0], point[1], result.x_m, result.y_m, error_cm, result.valid);
        }
    }

    return 0;
}
