// Common/include/node_event.h
#pragma once
#include <cstdint>
#include <string>
#include <vector>

struct NODE_EVENT
{
  std::string node_id;
  std::vector<std::string> mic_ids;          
  uint32_t sample_rate_hz;
  int64_t first_sample_time_ns;
  std::vector<std::vector<int32_t>> samples; 
};