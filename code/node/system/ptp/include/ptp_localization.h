#pragma once

#include "ptp_service.h"
#include "ptp_status.h"
#include "tdoa.h"

#include <array>
#include <chrono>
#include <mutex>
#include <string>

struct PTP_NODE_STATUS
{
    std::uint16_t node_id;
    std::int64_t offset_budget_ns;
    bool synchronized;
    std::chrono::steady_clock::time_point received_at;
    bool connected;
};

/** @brief Stores the latest PTP status received from each microphone node. */
class PtpStatusStore
{
public:
    /**
     * @brief Store a status message and timestamp its receipt locally.
     * @return false when the message names an unsupported node.
     */
    bool update(const PTP_STATUS &status);

    /** @brief Mark a node's status unusable when its TCP connection closes. */
    void markDisconnected(std::uint16_t node_id);

    /** @brief Return a consistent copy of both nodes' latest statuses. */
    std::array<PTP_NODE_STATUS, 2> snapshot() const;

private:
    mutable std::mutex mutex_;
    std::array<PTP_NODE_STATUS, 2> status_{{
        {0, 0, false, {}, false},
        {1, 0, false, {}, false},
    }};
};

/** @brief Return why PTP is unusable, or an empty string if both nodes are ready. */
std::string Ptp_StatusError(const std::array<PTP_NODE_STATUS, 2> &status);

struct PTP_LOCALIZATION_RESULT
{
    TDOA_RESULT position;
    std::string rejection_reason;
};

/**
 * @brief Localize only with a fresh, usable status for each contributing node.
 * @param events Two events from the same sound event, one per configured node.
 * @param mics Microphone positions passed to the TDOA solver.
 * @param status Latest status per configured node. Timestamp receipt locally.
 *               clear connected on transport loss.
 * @return Position result and a reason if the event pair cannot be localized.
 */
PTP_LOCALIZATION_RESULT Ptp_ComputePosition(const std::vector<DetectedEvent> &events,
                                           const std::vector<MIC_POSITION> &mics,
                                           const std::array<PTP_NODE_STATUS, 2> &status);
