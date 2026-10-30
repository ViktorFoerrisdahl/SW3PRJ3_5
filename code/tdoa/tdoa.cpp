#include "tdoa.h"
#include <limits>
#include <cmath>
#include <iostream>

// Maximum lag between mics based on maximum distance between mics in (3x3m)
static const int MAX_LAG_SAMPLES = 600; // 3x3 m diagonal (4.24 m) - 4.24m divided by speed of sound then multiplied with sample frequency 48kHz: Roughly 594 samples (600 cap to be safe)

// Function to find specific microphone. Failsafe to get mics in right order.
static const MIC_POSITION* FindMic(const std::vector<MIC_POSITION> &mics, const uint16_t &node_id, const uint16_t &channel)
{
    for (const auto &mic : mics)
    {
        if (mic.node_id == node_id && mic.channel == channel)
            return &mic;
    }
    return nullptr;
}

// Lag in samples where sig best matches ref.
static int CrossCorrelate(const std::vector<int32_t> &ref, const std::vector<int32_t> &sig)
{
    double best_sum = -1e300; // Comparison metric: Initialised to an extremely small number that any real sum will surpass
    int best_lag = 0;         // Lag that matches the best sum (this value is returned)

    for (int lag = -MAX_LAG_SAMPLES; lag <= MAX_LAG_SAMPLES; lag++)
    {
        double sum = 0; // Initialise local sum set to zero
        for (int i = 0; i < ref.size(); i++) // Loop through indexes of ref
        {
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

// Computes a rough estimate using grid search. 20cm steps.
static void GridSearch(const std::vector<MIC_POSITION> &mics, const std::vector<double> &tdoa_s, double &x_m, double &y_m)
{
    if (mics.empty()) return; //Failsafe

    double min_error = std::numeric_limits<double>::max(); // Maximum limit

    //For x and y move 20 cm in the grid (0.20 m)
    for (double x = 0.0; x < 3.0; x += 0.20)
    {
        for (double y = 0.0; y < 3.0; y += 0.20)
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

// Computes the position using gauss newton least squares.
static void GaussNewton(const std::vector<MIC_POSITION> &mics, const std::vector<double> &tdoa_s, double &x_m, double &y_m)
{
    if (mics.size() < 3) return;

    //Current guessed position from Grid Search
    double x = x_m;
    double y = y_m;

    //Position of mic 0
    double x0 = mics[0].x_m;
    double y0 = mics[0].y_m;

    int MAX_ITERATIONS = 80; //Maximum wanted iterations to ensure quick solution
    double TOLERANCE = 1e-4; //0.1 mm

    for (int iter = 0; iter < MAX_ITERATIONS; iter++)
    {
        //Elements in 2x2 Matrix
        double Hxx = 0.0, Hxy = 0.0, Hyy = 0.0;

        //Elements in vektor
        double gx = 0.0, gy = 0.0;

        double d0 = std::sqrt((x - x0)*(x - x0) + (y - y0)*(y - y0)); //Distance til referencemikrofon

        for (size_t i = 1; i < mics.size(); i++)
        {
            double xi = mics[i].x_m;
            double yi = mics[i].y_m;

            double di = std::sqrt((x - xi)*(x - xi) + (y - yi)*(y - yi)); //Distance til den enkelte mikrofon

            //Calculating error r (residual)
            double theoretical_diff = di - d0;
            double measured_diff = tdoa_s[i] * SPEED_OF_SOUND_M_PER_S;
            double r = theoretical_diff - measured_diff;

            //The slope of the given mic
            double Jx = (x - xi) / di - (x - x0) / d0;
            double Jy = (y - yi) / di - (y - y0) / d0;

            //Building J^T * J
            Hxx += Jx * Jx;
            Hxy += Jx * Jy;
            Hyy += Jy * Jy;

            //Building J^T * r
            gx += Jx * r;
            gy += Jy * r;
        }

        //Calculating determinant
        double det = (Hxx * Hyy) - (Hxy * Hxy);

        //If determinant is close to 0 it is unsolvable
        if (std::abs(det) < 1e-9) break;

        //Solve the step to take using the inverse matrix
        double delta_x = (Hyy * gx - Hxy * gy) / det;
        double delta_y = (Hxx * gy - Hxy * gx) / det;

        x -= delta_x;
        y -= delta_y;

        //Check if the step is so small that we're there
        if (std::sqrt(delta_x * delta_x + delta_y * delta_y) < TOLERANCE) break;
    }
    //Updating final result
    x_m = x;
    y_m = y;
}

static void SolvePosition(const std::vector<MIC_POSITION> &mics, const std::vector<double> &tdoa_s, double &x_m, double &y_m)
{
    if (mics.size() < 3) return;  //Check for mics

    GridSearch(mics, tdoa_s, x_m, y_m);  //Call grid search

    std::cout << "Result after Grid Search: {" << x_m << ", " << y_m << "}\n";

    GaussNewton(mics, tdoa_s, x_m, y_m);  //Call gauss newton

    std::cout << "Result after Gauss Newton: {" << x_m << ", " << y_m << "}\n";
}

TDOA_RESULT Tdoa_ComputePosition(const std::vector<DetectedEvent> &events, const std::vector<MIC_POSITION> &mics)
{
    //Intialise lists and result
    TDOA_RESULT result = {false, 0.0, 0.0};     //Invalid until proven otherwise
    std::vector<MIC_POSITION> positions;        //Vector for microphones
    std::vector<std::vector<int32_t>> channels; //Samples of each channel
    std::vector<int64_t> t_first_ns;            //Vector for first sample time of each channel 
    uint32_t sample_rate = 0;                   //Shared variable for sample rate

    //Unpack events (One from each pi, two microphones per pi)
    for (const auto &event : events){   //For each event

        sample_rate = event.sample_rate;                    //Grabs sample rate from here (should be same for both events)
        for (int i = 0; i < 2; i++){                        //For each microphone
            const MIC_POSITION* p_mic = FindMic(mics, event.node_id, i);  //Find mic
            if (p_mic == nullptr) return result;            //Unknown id: Abandon

            //De-interleave samples
            std::vector<int32_t> samples;
            for (const Frame& f : event.frames){
                samples.push_back(f.ch[i]);                 //Push back sample for this mic
            }

            //Push back data
            positions.push_back(*p_mic);                    //Push back mic
            channels.push_back(samples);                    //Push back samples
            t_first_ns.push_back(event.t_first_ns);         //Push back first sample time
        }
    }
    if (channels.size() < 3) return result; //Needs three mics: If not abandon

    //Cross correlation and build a vector from this
    std::vector<double> tdoa_s{0.0};                        //Reference 0 at index 0
    for (int i = 1; i < channels.size(); i++){              //Starting at 1
        int lag = CrossCorrelate(channels[0], channels[i]);
        tdoa_s.push_back((double)lag / sample_rate);     // samples → seconds
    }

    //Call solveposition using vector
    
    SolvePosition(positions, tdoa_s, result.x_m, result.y_m);
    result.valid = true;

    //Return result
    return result;
}
