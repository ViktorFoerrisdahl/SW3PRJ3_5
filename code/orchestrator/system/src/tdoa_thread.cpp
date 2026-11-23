#include "tdoa_thread.h"
#include <thread>
#include "tdoa.h"
#include "json_logger.h"

static constexpr int PLOT_WAIT_S = 60;                    // Plot thread sleep 60s

void TdoaThread(std::stop_token st,
                std::queue<EventPair>& queue, std::mutex& event_lock,
                std::condition_variable_any& cv_tdoa,
                microphones& mics, std::mutex& data_lock, plot& p)
{
    // Create instance of tdoa class and event id
    tdoa locator;
    int event_id = 0;

    // While loop for the thread
    while (true)
    {
        // Wait with pred: !queue.empty() & stop_token
        EventPair pair;
        {
            std::unique_lock lock(event_lock);
            if (!cv_tdoa.wait(lock, st, [&] { return !queue.empty(); }))
                return;                               // [stop requested]

            pair = std::move(queue.front());          // Load latest pair
            queue.pop();                              // Pop latest pair from queue
        }                                             // event lock released

        // Call TDOA and fill struct with result
        TDOA_RESULT result;
        {
            std::lock_guard lock(data_lock);          // Take data lock
            const std::vector<MIC_POSITION>& mic_pos = mics.get_MicrophonesVector();  // Get mic pos
            result = locator.ComputePosition(pair, mic_pos);                          // Begin TDOA
        }                                             // Release data lock

        // Append event to the log    
        JsonLogger_AppendEvent(result);

        // Start plot thread
        std::thread(&plot::plotNewEventAndDelete, &p, ++event_id, 
                    result.x_m, result.y_m, PLOT_WAIT_S).detach(); // Detach seperate thread from tdoa thread (avoids tying it to the waittime)
    }
}