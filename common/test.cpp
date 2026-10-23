int trigger_detector(const std::vector<double>& signal, double triggerLevel)
{
    for(int i = 0; i < signal.size(); ++i)
    {
        if(signal[i] >= triggerLevel){
            return i;
            
        }
    }
    
    return -1; // No trigger found  
}


struct MIC_ARRIVAL
{
  std::string mic_id;        ///< Required
  int64_t arrival_time_ns;   ///< Required, synced clock
  float peak_amplitude;      ///< Optional, diagnostics
};

struct NODE_EVENT
{
  std::string node_id;                 ///< Optional, logging
  uint32_t event_id;                   ///< Optional, logging
  std::vector<MIC_ARRIVAL> arrivals;   ///< One per mic that detected the clap
};

