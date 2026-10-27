#include <cmath>
#include <cstdio>
#include <random>
#include "tdoa.h"

static const uint32_t SAMPLE_RATE_HZ = 44100;

// Same clap in every channel, delayed by distance / speed of sound.
static std::vector<NODE_EVENT> MakeMockEvents(const std::vector<MIC_POSITION>& mics, 
                                                     double src_x_m, double src_y_m) 
{
    // 1. The clap: 64 random values, fixed seed -> same every run
    std::mt19937 rng(42);
    std::uniform_int_distribution<int32_t> noise(-1000, 1000);
    std::vector<int32_t> clap(64);
    for (int32_t& sample : clap) sample = noise(rng);

    // 2. One event per node
    NODE_EVENT node_1{"node1", {}, SAMPLE_RATE_HZ, 0, {}};
    NODE_EVENT node_2{"node2", {}, SAMPLE_RATE_HZ, 0, {}};

    // 3. One channel per microphone
    for (size_t m = 0; m < mics.size(); m++)
    {
        double distance_m = std::hypot(src_x_m - mics[m].x_m, src_y_m - mics[m].y_m);
        int delay_samples = (int)std::lround(distance_m / SPEED_OF_SOUND_M_PER_S * SAMPLE_RATE_HZ);

        std::vector<int32_t> samples(2048, 0);
        for (size_t i = 0; i < clap.size(); i++)
            samples[256 + delay_samples + i] = clap[i];

        NODE_EVENT& node = (m < 2) ? node_1 : node_2;   // A, B -> node 1; C, D -> node 2
        node.mic_ids.push_back(std::to_string(mics[m].node_id));
        node.samples.push_back(samples);
    }

    // 4.
    return {node_1, node_2};


}

int main() 
{
    std::vector<MIC_POSITION> mics = {
        {"A", 0.0, 0.0}, {"B", 3.0, 0.0}, {"C", 3.0, 3.0}, {"D", 0.0, 3.0}};

    double test_points[][2] = {{1.0, 2.0}, {1.5, 1.5}, {0.5, 0.5}, {2.8, 1.5}};

    for (auto& point : test_points)
    {
        TDOA_RESULT result = Tdoa_ComputePosition(MakeMockEvents(mics, point[0], point[1]), mics);
        double error_cm = 100.0 * std::hypot(result.x_m - point[0], result.y_m - point[1]);
        std::printf("true (%.2f, %.2f)  estimate (%.2f, %.2f)  error %.1f cm  valid %d\n",
                    point[0], point[1], result.x_m, result.y_m, error_cm, result.valid);
    }
    
    return 0;
}