/**
 * @file simulator.cpp
 * @brief Generates mock NODE_EVENT data for testing the TDOA logic.
 *
 * The clap is modelled as a Ricker pulse. Its peak arrives at each microphone
 * at clap_time_s + distance / speed_of_sound. The waveform is evaluated at the
 * exact time of every sample, so the time differences between microphones are
 * exact, also below one sample.
 */
#include "simulator.h"

#include <algorithm>
#include <cmath>
#include <random>

#define SIM_PI 3.14159265358979323846
#define CLAP_PEAK_FREQUENCY_HZ 2000.0 ///< Dominant frequency of the Ricker pulse
#define CLAP_AMPLITUDE 0.5            ///< Relative to full scale, leaves headroom for noise
#define CLAP_CUTOFF_PERIODS 5.0       ///< Waveform is 0 beyond this many periods from the peak
#define INT32_FULL_SCALE 2147483647.0

/**
 * @brief Time for sound to travel from the source to a microphone.
 * @param source Position of the sound source.
 * @param mic Position of the microphone.
 * @param speed_of_sound_mps Speed of sound in meters per second.
 * @return Delay in seconds.
 */
static double PropagationDelay(const POSITION_M &source, const POSITION_M &mic,
                               double speed_of_sound_mps)
{
  double dx_m = mic.x - source.x;
  double dy_m = mic.y - source.y;
  double dz_m = mic.z - source.z;
  return std::sqrt(dx_m * dx_m + dy_m * dy_m + dz_m * dz_m) / speed_of_sound_mps;
}

/**
 * @brief Ricker pulse with its peak at t = 0.
 *
 * Broadband like a real clap, and gives a single clear peak in a cross-correlation
 * (a sine would give a peak every period).
 *
 * @param t_s Time relative to the arrival of the peak, in seconds.
 * @return Amplitude relative to full scale.
 */
static double ClapWaveform(double t_s)
{
  if (std::fabs(t_s) > CLAP_CUTOFF_PERIODS / CLAP_PEAK_FREQUENCY_HZ)
  {
    return 0.0;
  }
  double a = SIM_PI * CLAP_PEAK_FREQUENCY_HZ * t_s;
  double a_squared = a * a;
  return CLAP_AMPLITUDE * (1.0 - 2.0 * a_squared) * std::exp(-a_squared);
}

/**
 * @brief Converts a value in [-1.0, 1.0] to a full-scale 32-bit sample.
 *
 * Values outside the range are clipped, as a real ADC would do.
 *
 * @param value Amplitude relative to full scale.
 * @return 32-bit sample.
 */
static int32_t ToInt32(double value)
{
  double clipped = std::clamp(value, -1.0, 1.0);
  return static_cast<int32_t>(std::llround(clipped * INT32_FULL_SCALE));
}

SIM_CONFIG Simulator_DefaultConfig()
{
  SIM_CONFIG config;

  // 3x3 m field. Node 1 has the two bottom corners, node 2 the two top corners.
  SIM_NODE node1;
  node1.id = "node1";
  node1.mics = {{"micA", {0.0, 0.0, 0.0}}, {"micB", {3.0, 0.0, 0.0}}};
  node1.clock_offset_s = 0.0;

  SIM_NODE node2;
  node2.id = "node2";
  node2.mics = {{"micC", {0.0, 3.0, 0.0}}, {"micD", {3.0, 3.0, 0.0}}};
  node2.clock_offset_s = 0.0;

  config.nodes = {node1, node2};
  config.source_position = {1.2, 0.8, 0.0};
  config.clap_time_s = 1.0;   // Arbitrary, keeps all timestamps positive
  config.pre_trigger_s = 0.005;
  config.window_s = 0.040;
  config.sample_rate_hz = 48000;
  config.noise_rms = 0.0;     // Ideal case. Add noise in the individual tests.
  config.speed_of_sound_mps = 343.0;
  config.seed = 42;

  return config;
}

std::vector<NODE_EVENT> Simulator_GenerateEvents(const SIM_CONFIG &config)
{
  std::mt19937 rng(config.seed);

  // normal_distribution requires a positive standard deviation (MSVC asserts on 0)
  bool add_noise = config.noise_rms > 0.0;
  std::normal_distribution<double> noise(0.0, add_noise ? config.noise_rms : 1.0);

  double sample_rate_hz = static_cast<double>(config.sample_rate_hz);
  size_t num_samples = static_cast<size_t>(std::llround(config.window_s * sample_rate_hz));
  std::vector<NODE_EVENT> events;

  for (const SIM_NODE &node : config.nodes)
  {
    if (node.mics.empty())
    {
      continue;
    }

    // True arrival time of the pulse peak at each mic
    std::vector<double> arrival_s;
    for (const SIM_MIC &mic : node.mics)
    {
      arrival_s.push_back(config.clap_time_s +
                          PropagationDelay(config.source_position, mic.position,
                                           config.speed_of_sound_mps));
    }

    // A real node samples at fixed times n / fs, so the window starts on that grid
    double earliest_arrival_s = *std::min_element(arrival_s.begin(), arrival_s.end());
    double first_sample_index =
        std::floor((earliest_arrival_s - config.pre_trigger_s) * sample_rate_hz);
    double window_start_s = first_sample_index / sample_rate_hz;

    NODE_EVENT event;
    event.node_id = node.id;
    event.sample_rate_hz = config.sample_rate_hz;
    // The node timestamps with its own clock, which is off by clock_offset_s
    event.first_sample_time_ns = std::llround((window_start_s + node.clock_offset_s) * 1e9);

    for (size_t ch = 0; ch < node.mics.size(); ch++)
    {
      event.mic_ids.push_back(node.mics[ch].id);

      std::vector<int32_t> channel(num_samples);
      for (size_t n = 0; n < num_samples; n++)
      {
        double t_s = (first_sample_index + static_cast<double>(n)) / sample_rate_hz;
        double value = ClapWaveform(t_s - arrival_s[ch]);
        if (add_noise)
        {
          value += noise(rng);
        }
        channel[n] = ToInt32(value);
      }
      event.samples.push_back(channel);
    }

    events.push_back(event);
  }

  return events;
}