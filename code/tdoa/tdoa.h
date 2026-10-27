#pragma once
#include <string>
#include <vector>
//#include "node_event.h"
#include "detected_event.h"

constexpr double SPEED_OF_SOUND_M_PER_S = 343.0;
//Hej

//Position of one microphone (in metres)
struct MIC_POSITION 
{
    uint16_t node_id;
    uint16_t channel;
    double x_m;
    double y_m;
};

//Estimated source position (in metres)
struct TDOA_RESULT 
{
    bool valid;
    double x_m;
    double y_m;
};

/**
 * @brief Estimate the source position from microphone node events.
 * @param events NODE_EVENTs from all microphone nodes for the same sound event.
 * @param mics Microphone positions, looked up by mic_id.
 * @return Position; valid is false if a mic_id is unknown or there are fewer than 3 microphones.
 */
TDOA_RESULT Tdoa_ComputePosition(const std::vector<NODE_EVENT>& events, const std::vector<MIC_POSITION>& mics);