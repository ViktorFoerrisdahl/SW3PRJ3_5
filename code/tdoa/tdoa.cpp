#include "tdoa.h"
#include <limits>
#include <cmath>

static const int MAX_LAG_SAMPLES = 600; // 3x3 m diagonal (4.24 m) - 4.24m divided by speed of sound then multiplied with sample frequency: Roughly 545 samples (600 cap to be safe)


static const MIC_POSITION* FindMic(const std::vector<MIC_POSITION> &mics, const std::string &wanted_mic_id)
{
    for (const auto &mic : mics)
    {
        if (mic.mic_id == wanted_mic_id)
            return &mic;
    }
    return nullptr;
}

// Lag in samples where sig best matches ref. Positive: sig arrived later.
static int CrossCorrelate(const std::vector<int32_t> &ref, const std::vector<int32_t> &sig)
{
    double best_sum = -1e300; // Comparison metric: Initialised to an extremely small number that any real sum will surpass
    int best_lag = 0;         // Lag that matches the best sum (this value is returned)

    for (int lag = -MAX_LAG_SAMPLES; lag <= MAX_LAG_SAMPLES; lag++)
    {

        double sum = 0; // Initialise local sum set to zero
        for (int i = 0; i < ref.size(); i++)
        { // Loop through indexes of ref
            int j = i + lag;
            if (j >= 0 && j < sig.size())
            {                                   // For each value thats >= zero, multipy by ref and add to sum
                sum += (double)ref[i] * sig[j]; //>= zero: doesn't calculate on indexes outside the vector
            }
        }
        if (sum > best_sum)
            best_sum = sum, best_lag = lag; // If the found sum for this lag is bigger than the previous best, overwrite best_lag
    }
    return best_lag; // Return best lag found
}

// Tries every point in the 3x3 m area (1 cm steps) and keeps the one that best fits the TDOAs.
// Microphone 0 is the reference, so tdoa_s[0] is 0.
static void SolvePosition(const std::vector<MIC_POSITION> &mics, const std::vector<double> &tdoa_s, double &x_m, double &y_m)
{
    if (mics.empty()) return; //Failsafe
    
    double min_error = std::numeric_limits<double>::max(); // Maximum limit

    //For x and y move 1 cm in the grid (0.01 m)
    for (double x = 0.0; x < 3.0; x += 0.01)
    {
        for (double y = 0.0; y < 3.0; y += 0.01)
        {
            double current_error = 0.0; //Current error to be measured for the given x and y values

            double dist_ref = std::sqrt(std::pow(mics[0].x_m - x, 2) + std::pow(mics[0].y_m - y, 2)); //The distance to the reference microphone

            for (int i = 1; i < mics.size(); i++) //Looping through each microphone except reference 0
            {
                double dist_i = std::sqrt(std::pow(mics[i].x_m - x, 2) + std::pow(mics[i].y_m - y, 2)); //Distance to any of the other microphones

                double theoretical_diff = dist_i - dist_ref; //The theoretical difference in distance
                double measured_diff = tdoa_s[i] * SPEED_OF_SOUND_M_PER_S; //The measured difference in distance

                current_error += std::pow(theoretical_diff - measured_diff, 2); //Comparing the two differences with Sum of Squared Errors (SSE)
            }
            if (current_error < min_error) //Using SSE to find the lowest sum, which will give the most likely (x,y) coordinate
            {
                min_error = current_error;
                x_m = x;
                y_m = y;
            }
        }
    }
}


TDOA_RESULT Tdoa_ComputePosition(const std::vector<NODE_EVENT> &events, const std::vector<MIC_POSITION> &mics)
{
    //Intialise lists and result
    TDOA_RESULT result = {false, 0.0, 0.0};     //Invalid until proven otherwise
    std::vector<MIC_POSITION> positions;        //Vector for microphones
    std::vector<std::vector<int32_t>> channels; //Samples of each channel
    uint32_t sample_rate_hz = 0;
    
    //Unpack events (One from each pi, two microphones per pi)
    for (const auto &event : events){   //For each event
        
        sample_rate_hz = event.sample_rate_hz;              //Grabs sample rate from here (should be same for both events)
        for (int i = 0; i < event.mic_ids.size(); i++){     //For each microphone
            const MIC_POSITION* p_mic = FindMic(mics, event.mic_ids[i]);  //Find mic
            if (p_mic == nullptr) return result;          //Unknown id: Abandon
            positions.push_back(*p_mic);                  //Push back mic           
            channels.push_back(event.samples[i]);       //Push back samples
        } 
    }
    if (channels.size() < 3) return result; //Needs three mics: If not abandon

    // Cross correlation and build a vector from this
    std::vector<double> tdoa_s{0.0};                        //Reference 0 at index 0
    for (int i = 1; i < channels.size(); i++){              //Starting at 1
        int lag = CrossCorrelate(channels[0], channels[i]);
        tdoa_s.push_back((double)lag / sample_rate_hz);     // samples → seconds
    }

    //Call solveposition using vector
    SolvePosition(positions, tdoa_s, result.x_m, result.y_m);
    result.valid = true;

    //Return result
    return result;
}
