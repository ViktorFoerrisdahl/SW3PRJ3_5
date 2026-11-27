#include "TDOA_thread.h"
#include <iostream>
#include <thread>
#include "json_logger.h"

static constexpr int PLOT_WAIT_S = 60; // Plot thread sleep 60s

void TdoaThread(std::stop_token st, EventPairQueue &queue, microphones &mics,
                plot &p, PtpStatusStore &ptp_status)
{
	// Create event id (starting with 1 when plotting)
	int event_id = 0;

	// Wait for TCP (pred: !queue.empty(), stop_token) - nullopt = [stop requested]
	while (std::optional<EventPair> pair = queue.pop(st))
	{
		// Get mic pos
		std::vector<MIC_POSITION> mic_pos = mics.snapshot();

		// Compute TDOA only if the latest PTP status from both nodes is usable.
		const auto localization = Ptp_ComputePosition(*pair, mic_pos, ptp_status.snapshot());
		const auto &result = localization.position;

		// Append event to the log
		if (!JsonLogger_AppendEvent(result, localization.rejection_reason))
			std::cerr << "Could not write localization result to the event log\n";

		// Skip plotting if result is invalid
		if(result.valid == false) continue;

		// Start plot thread, detached so TDOA isn't tied to the wait time
		std::thread(&plot::plotNewEventAndDelete, &p, ++event_id,
					result.x_m, result.y_m, PLOT_WAIT_S, result.rms)
			.detach();
	}
}
