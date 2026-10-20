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


// Én frame: ét S32_LE-sample fra hver af nodens to mikrofoner.
struct Frame {
    int32_t ch[2];
};


struct DetectedEvent {
    uint16_t node_id = 0;         // hvilken mikrofonnode hændelsen kommer fra
    uint32_t event_seq = 0;       // løbenummer for hændelsen på denne node
    uint32_t sample_rate = 0;     // fx 48000
    uint16_t channels = 2;        // antal kanaler (mikrofoner) i udsnittet
    int64_t t_first_ns = 0;       // PTP-synkroniseret timestamp for udsnittets første frame
    uint32_t trigger_offset = 0;  // triggerpunktets placering i udsnittet (frame-nr.)
    std::vector<Frame> frames;    // de rå S32_LE-samples, kanalerne interleaved i hver frame
};
