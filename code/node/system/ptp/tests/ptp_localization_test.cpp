#include "ptp_localization.h"

#include <cstdlib>

int main()
{
    const auto now = std::chrono::steady_clock::now();
    const std::vector<MIC_POSITION> mics{
        {1, 0, 0, 0}, {1, 1, 3, 0}, {2, 0, 3, 3}, {2, 1, 0, 3}};
    const std::vector<DetectedEvent> events{
        {.node_id = 1, .sample_rate = 48000, .frames = {{{1, 1}}}},
        {.node_id = 2, .sample_rate = 48000, .frames = {{{1, 1}}}}};
    std::array<PTP_NODE_STATUS, 2> status{{
        {1, 50000, true, now, true},
        {2, 150000, true, now, true},
    }};

    if (!Ptp_ComputePosition(events, mics, status).valid)
        return EXIT_FAILURE;
    status[1].offset_budget_ns = 150001;
    if (Ptp_ComputePosition(events, mics, status).valid)
        return EXIT_FAILURE;
    status[1].offset_budget_ns = 150000;
    status[1].synchronized = false;
    if (Ptp_ComputePosition(events, mics, status).valid)
        return EXIT_FAILURE;
    status[1].synchronized = true;
    status[1].connected = false;
    if (Ptp_ComputePosition(events, mics, status).valid)
        return EXIT_FAILURE;
    status[1].connected = true;
    status[1].received_at -= std::chrono::seconds(3);
    if (Ptp_ComputePosition(events, mics, status).valid)
        return EXIT_FAILURE;
    status[1].received_at = std::chrono::steady_clock::now();
    return Ptp_ComputePosition(events, mics, status).valid ? EXIT_SUCCESS : EXIT_FAILURE;
}
