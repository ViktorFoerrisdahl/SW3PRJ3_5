// Simulator/include/simulator.h
#pragma once
#include "node_event.h"

struct POSITION_M { double x; double y; double z; };

struct SIM_MIC  { std::string id; POSITION_M position; };

struct SIM_NODE
{
  std::string id;
  std::vector<SIM_MIC> mics;
  double clock_offset_s;   ///< Simulated sync error, 0 = perfect sync
};

struct SIM_CONFIG
{
  std::vector<SIM_NODE> nodes;
  POSITION_M source_position;
  double clap_time_s;
  double pre_trigger_s;
  double window_s;
  uint32_t sample_rate_hz;
  double noise_rms;        ///< Relative to full scale, 0 = no noise
  double speed_of_sound_mps;
  uint32_t seed;           ///< Same seed gives the same noise every run
};

/** @brief 3x3 m field, 2 nodes with 2 mics each in the corners. */
SIM_CONFIG Simulator_DefaultConfig();

/** @brief Returns one NODE_EVENT per node, as the real nodes would. */
std::vector<NODE_EVENT> Simulator_GenerateEvents(const SIM_CONFIG &config);