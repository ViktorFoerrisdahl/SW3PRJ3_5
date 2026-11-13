#pragma once

#include "ptp_service.h"
#include "tdoa.h"

#include <array>
#include <chrono>

struct PTP_NODE_STATUS
{
    std::uint16_t node_id;
    std::int64_t offset_budget_ns;
    bool synchronized;
    std::chrono::steady_clock::time_point received_at;
    bool connected;
};

/**
 * @brief Localize only with a fresh, usable status for each contributing node.
 * @param events Two events from the same sound event, one per configured node.
 * @param mics Microphone positions passed to the TDOA solver.
 * @param status Latest status per configured node. Timestamp receipt locally.
 *               clear connected on transport loss.
 * @return Invalid result if clock status or the event pair is unusable.
 */
TDOA_RESULT Ptp_ComputePosition(const std::vector<DetectedEvent> &events,
                                const std::vector<MIC_POSITION> &mics,
                                const std::array<PTP_NODE_STATUS, 2> &status);
