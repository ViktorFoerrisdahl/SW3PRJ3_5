#include "ptp_localization.h"

#include <cstdlib>
#include <cmath>
#include <iostream>

int main()
{
    const std::vector<MIC_POSITION> mics{
        {0, 0, 0, 0}, {3, 0, 0, 1}, {3, 3, 1, 0}, {0, 3, 1, 1}};
    DetectedEvent first{
        .node_id = 0, .sample_rate = 48000,
        .t_first_ns = 1'700'000'000'000'000'000LL, .trigger_offset = 100,
        .frames = std::vector<Frame>(2048)};
    // A sound source at the centre is equally far from all four microphones.
    first.frames[first.trigger_offset] = Frame{{100000, 100000}};
    DetectedEvent second = first;
    second.node_id = 1;
    const std::vector<DetectedEvent> events{first, second};

    const PTP_STATUS transmitted{0, 50000, true};
    const PTP_STATUS parsed = json_dto::from_json<PTP_STATUS>(json_dto::to_json(transmitted));
    if (parsed.node_id != transmitted.node_id ||
        parsed.offset_budget_ns != transmitted.offset_budget_ns ||
        parsed.synchronized != transmitted.synchronized)
        return EXIT_FAILURE;

    PtpStatusStore store;
    const PtpStatusStore empty_store;
    if (!store.update(parsed) || !store.update({1, 150000, true}) ||
        store.update({2, 0, false}))
        return EXIT_FAILURE;
    const auto valid = Ptp_ComputePosition(events, mics, store.snapshot());
    if (!valid.position.valid || !valid.rejection_reason.empty() ||
        std::hypot(valid.position.x_m - 1.5, valid.position.y_m - 1.5) > 0.01)
    {
        std::cerr << "Expected the centre position at the 200 us budget limit\n";
        return EXIT_FAILURE;
    }

    auto rejected = [&](const std::array<PTP_NODE_STATUS, 2> &status, const char *expected_reason)
    {
        const auto result = Ptp_ComputePosition(events, mics, status);
        if (!result.position.valid && result.rejection_reason == expected_reason &&
            result.position.time_stamp_ns == valid.position.time_stamp_ns)
            return true;
        std::cerr << "Expected rejection with the original event timestamp: "
                  << expected_reason << '\n';
        return false;
    };

    if (!rejected(empty_store.snapshot(), "Node 0 has no connected PTP status"))
        return EXIT_FAILURE;
    auto status = store.snapshot();
    status[1].offset_budget_ns = 150001;
    if (!rejected(status, "Combined PTP offset budget exceeds 200 us"))
        return EXIT_FAILURE;
    status = store.snapshot();
    status[1].synchronized = false;
    if (!rejected(status, "Node 1 is not synchronized"))
        return EXIT_FAILURE;
    status = store.snapshot();
    status[1].received_at -= std::chrono::seconds(3);
    if (!rejected(status, "Node 1 has stale or invalid PTP status"))
        return EXIT_FAILURE;
    store.markDisconnected(1);
    if (!rejected(store.snapshot(), "Node 1 has no connected PTP status"))
        return EXIT_FAILURE;
    store.update({1, 150000, true});
    return Ptp_ComputePosition(events, mics, store.snapshot()).position.valid ? EXIT_SUCCESS : EXIT_FAILURE;
}
