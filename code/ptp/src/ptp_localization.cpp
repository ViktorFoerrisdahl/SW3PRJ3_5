#include "ptp_localization.h"

#include <algorithm>

TDOA_RESULT Ptp_ComputePosition(const std::vector<NODE_EVENT> &events,
                                const std::vector<MIC_POSITION> &mics,
                                const std::array<PTP_NODE_STATUS, 2> &status)
{
    constexpr auto max_status_age = std::chrono::seconds(2);
    const auto now = std::chrono::steady_clock::now();
    const TDOA_RESULT invalid{false, 0.0, 0.0};
    if (events.size() != 2 || events[0].node_id == events[1].node_id ||
        status[0].node_id == status[1].node_id)
        return invalid;

    std::int64_t total_ns = 0;
    for (const auto &node : status)
    {
        if (node.node_id.empty() || !node.connected || node.received_at > now ||
            now - node.received_at > max_status_age || !node.synchronized ||
            node.offset_budget_ns < 0 || node.offset_budget_ns > PTP_MAX_OFFSET_NS)
            return invalid;
        total_ns += node.offset_budget_ns;

        const auto event = std::find_if(events.begin(), events.end(),
                                        [&](const NODE_EVENT &candidate)
                                        { return candidate.node_id == node.node_id; });
        if (event == events.end() || event->sample_rate_hz == 0 ||
            event->sample_rate_hz != events.front().sample_rate_hz ||
            event->mic_ids.size() != 2 || event->samples.size() != 2 ||
            event->samples[0].empty() || event->samples[1].empty())
            return invalid;
    }
    if (total_ns > PTP_MAX_OFFSET_NS)
        return invalid;
    return Tdoa_ComputePosition(events, mics);
}
