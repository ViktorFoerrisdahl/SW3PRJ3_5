#include "ptp_localization.h"

#include <algorithm>

bool PtpStatusStore::update(const PTP_STATUS &status)
{
    if (status.node_id >= status_.size())
        return false;

    std::lock_guard lock(mutex_);
    status_[status.node_id] = {
        status.node_id,
        status.offset_budget_ns,
        status.synchronized,
        std::chrono::steady_clock::now(),
        true,
    };
    return true;
}

void PtpStatusStore::markDisconnected(std::uint16_t node_id)
{
    if (node_id >= status_.size())
        return;

    std::lock_guard lock(mutex_);
    status_[node_id].connected = false;
    status_[node_id].synchronized = false;
}

std::array<PTP_NODE_STATUS, 2> PtpStatusStore::snapshot() const
{
    std::lock_guard lock(mutex_);
    return status_;
}

std::string Ptp_StatusError(const std::array<PTP_NODE_STATUS, 2> &status)
{
    constexpr auto max_status_age = std::chrono::seconds(2);
    const auto now = std::chrono::steady_clock::now();
    if (status[0].node_id == status[1].node_id)
        return "Duplicate PTP node IDs";

    std::int64_t total_ns = 0;
    for (const auto &node : status)
    {
        const std::string node_label = "Node " + std::to_string(node.node_id);
        if (!node.connected)
            return node_label + " has no connected PTP status";
        if (node.received_at > now || now - node.received_at > max_status_age)
            return node_label + " has stale or invalid PTP status";
        if (!node.synchronized)
            return node_label + " is not synchronized";
        if (node.offset_budget_ns < 0 || node.offset_budget_ns > PTP_MAX_OFFSET_NS)
            return node_label + " has an invalid PTP offset budget";
        total_ns += node.offset_budget_ns;
    }
    if (total_ns > PTP_MAX_OFFSET_NS)
        return "Combined PTP offset budget exceeds 200 us";
    return {};
}

PTP_LOCALIZATION_RESULT Ptp_ComputePosition(const std::vector<DetectedEvent> &events,
                                           const std::vector<MIC_POSITION> &mics,
                                           const std::array<PTP_NODE_STATUS, 2> &status)
{
    TDOA_RESULT invalid{};
    if (!events.empty() && events.front().sample_rate > 0)
    {
        const auto &event = events.front();
        invalid.time_stamp_ns = event.t_first_ns +
            static_cast<std::int64_t>(event.trigger_offset) * 1'000'000'000 / event.sample_rate;
    }

    if (auto reason = Ptp_StatusError(status); !reason.empty())
        return {invalid, reason};
    if (events.size() != 2 || events[0].node_id == events[1].node_id)
        return {invalid, "Expected one sound event from each node"};

    for (const auto &node : status)
    {
        const auto event = std::find_if(events.begin(), events.end(),
                                        [&](const DetectedEvent &candidate)
                                        { return candidate.node_id == node.node_id; });
        if (event == events.end() || event->sample_rate == 0 ||
            event->sample_rate != events.front().sample_rate ||
            event->frames.empty())
            return {invalid, "Sound event pair does not match the configured nodes or sample rate"};
    }
    tdoa locator;
    auto position = locator.ComputePosition(events, mics);
    return {position, position.valid ? "" : "TDOA could not find a valid position"};
}
