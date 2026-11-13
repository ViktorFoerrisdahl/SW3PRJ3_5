#include "ptp_localization.h"

#include <cstdlib>

int main()
{
    const auto now = std::chrono::steady_clock::now();
    const std::vector<MIC_POSITION> mics{
        {"A", 0, 0}, {"B", 3, 0}, {"C", 3, 3}, {"D", 0, 3}};
    const std::vector<NODE_EVENT> events{
        {"left", {"A", "B"}, 48000, 0, {{1}, {1}}},
        {"right", {"C", "D"}, 48000, 0, {{1}, {1}}}};
    std::array<PTP_NODE_STATUS, 2> status{{
        {"left", 50000, true, now, true},
        {"right", 150000, true, now, true},
    }};

    if (!Ptp_ComputePosition(events, mics, status).valid)
        return EXIT_FAILURE;
    status[1].offset_budget_ns = 150001;
    if (Ptp_ComputePosition(events, mics, status).valid)
        return EXIT_FAILURE;
    status[1].synchronized = false;
    if (Ptp_ComputePosition(events, mics, status).valid)
        return EXIT_FAILURE;
    status[1].offset_budget_ns = 150000;
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
