#include <cmath>
#include <cstdio>
#include <random>
#include "tdoa.h"

static constexpr uint32_t SAMPLE_RATE_HZ = 48000;
static constexpr size_t SAMPLE_COUNT = 2048;

static std::vector<NODE_EVENT>
MakeMockEvents(const std::vector<MIC_POSITION>& mics,
               double src_x,
               double src_y)
{
    // Samme signal til alle mikrofoner
    std::mt19937 rng(42);
    std::uniform_int_distribution<int32_t> noise(-1000, 1000);

    std::vector<int32_t> clap(64);

    for (auto& s : clap)
        s = noise(rng);

    NODE_EVENT node1;
    node1.node_id = "1";
    node1.sample_rate_hz = SAMPLE_RATE_HZ;

    NODE_EVENT node2;
    node2.node_id = "2";
    node2.sample_rate_hz = SAMPLE_RATE_HZ;

    for (const auto& mic : mics)
    {
        double distance =
            std::hypot(src_x - mic.x_m,
                       src_y - mic.y_m);

        int delay =
            (int)std::lround(
                distance /
                SPEED_OF_SOUND_M_PER_S *
                SAMPLE_RATE_HZ);

        std::vector<int32_t> samples(SAMPLE_COUNT, 0);

        for (size_t i = 0; i < clap.size(); i++)
        {
            size_t idx = 300 + delay + i;

            if (idx < SAMPLE_COUNT)
                samples[idx] = clap[i];
        }

        if (mic.node_id == 1)
        {
            node1.mic_ids.push_back(std::to_string(mic.channel));
            node1.samples.push_back(samples);
        }
        else
        {
            node2.mic_ids.push_back(std::to_string(mic.channel));
            node2.samples.push_back(samples);
        }
    }

    return {node1, node2};
}

int main()
{
    std::vector<MIC_POSITION> mics =
    {
        {1, 0, 0.0, 0.0},
        {1, 1, 3.0, 0.0},
        {2, 0, 3.0, 3.0},
        {2, 1, 0.0, 3.0}
    };

    double test_points[][2] =
    {
        {1.0, 2.0},
        {1.5, 1.5},
        {0.5, 0.5},
        {2.8, 1.5}
    };

    for (auto& p : test_points)
    {
        auto events = MakeMockEvents(mics, p[0], p[1]);

        TDOA_RESULT result =
            Tdoa_ComputePosition(events, mics);

        double error_cm =
            100.0 *
            std::hypot(
                result.x_m - p[0],
                result.y_m - p[1]);

        printf(
            "TRUE=(%.2f, %.2f)  "
            "EST=(%.2f, %.2f)  "
            "ERROR=%.1f cm  "
            "VALID=%d\n",
            p[0],
            p[1],
            result.x_m,
            result.y_m,
            error_cm,
            result.valid);
    }
    
    return 0;
}