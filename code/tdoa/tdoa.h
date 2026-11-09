#pragma once
#include <string>
#include <vector>
#include "detected_event.h"

// Position of one microphone (in metres)
struct MIC_POSITION
{
    uint16_t node_id;
    uint16_t channel;
    double x_m;
    double y_m;
};

// Estimated source position (in metres)
struct TDOA_RESULT
{
    bool valid;
    double x_m;
    double y_m;
};


class tdoa
{

private:
    // Maximum lag between mics based on maximum distance between mics in (3x3m)
    static const int MAX_LAG_SAMPLES = 600; // 3x3 m diagonal (4.24 m) - 4.24m divided by speed of sound then multiplied with sample frequency 48kHz: Roughly 594 samples (600 cap to be safe)

    // Initiliaze functions
    static const MIC_POSITION* FindMic(const std::vector<MIC_POSITION> &mics, const uint16_t &node_id, const uint16_t &channel);
    static int CrossCorrelate(const std::vector<int32_t> &ref, const std::vector<int32_t> &sig);
    static void GridSearch(const std::vector<MIC_POSITION> &mics, const std::vector<double> &tdoa_s, double &x_m, double &y_m);
    static void GaussNewton(const std::vector<MIC_POSITION> &mics, const std::vector<double> &tdoa_s, double &x_m, double &y_m);
    static void SolvePosition(const std::vector<MIC_POSITION> &mics, const std::vector<double> &tdoa_s, double &x_m, double &y_m);

public:
    // Main needs this for the mocktest (can be changed)
    static constexpr double SPEED_OF_SOUND_M_PER_S = 343.0;

    // Compute the position using solvePosition and return struct TDOA_RESULT
    TDOA_RESULT ComputePosition(const std::vector<DetectedEvent> &events, const std::vector<MIC_POSITION> &mics);
};
