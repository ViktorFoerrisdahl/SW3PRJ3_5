#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "frame.h"

/** @brief What a microphone node delivers when it detects an event. */
struct NODE_EVENT
{
  std::string node_id;
  std::vector<std::string> mic_ids;   ///< mic_ids[c] is the mic in Frame::ch[c]
  uint32_t sample_rate_hz;
  int64_t first_sample_time_ns;       ///< Synced time of frames[0]
  std::vector<Frame> frames;          ///< The event window, one Frame per sample time
};
