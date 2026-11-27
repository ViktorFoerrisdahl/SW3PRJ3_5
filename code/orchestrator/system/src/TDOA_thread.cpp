#include "TDOA_thread.h"
#include <thread>
#include "tdoa.h"
#include "json_logger.h"

static constexpr int PLOT_WAIT_S = 60; // Plot thread sleep 60s

void TdoaThread(std::stop_token st, EventPairQueue &queue, microphones &mics, plot &p)
{
	// Create instance of tdoa class and event id (starting with 1 when plotting)
	tdoa locator;
	int event_id = 0;

	// Wait for TCP (pred: !queue.empty(), stop_token) - nullopt = [stop requested]
	while (std::optional<EventPair> pair = queue.pop(st))
	{
		// Get mic pos
		std::vector<MIC_POSITION> mic_pos = mics.snapshot();

		// Compute TDOA
		TDOA_RESULT result = locator.ComputePosition(*pair, mic_pos);

		// Append event to the log
		JsonLogger_AppendEvent(result);

		// Start plot thread, detached so TDOA isn't tied to the wait time
		std::thread(&plot::plotNewEventAndDelete, &p, ++event_id,
					result.x_m, result.y_m, result.valid, PLOT_WAIT_S)
			.detach();
	}
}
