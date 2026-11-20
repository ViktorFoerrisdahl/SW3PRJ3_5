#pragma once

#include <cstdint>

inline constexpr std::int64_t PTP_MAX_OFFSET_NS = 200000;

enum class PtpRole
{
    Grandmaster,
    Slave
};

/** @brief Controls local ptp4l and phc2sys services. */
class PtpService
{
public:
    PtpService(PtpRole role);

    bool start();
    bool stop();

    /** @brief Write the offset budget in ns on success and return whether it is valid. */
    bool readOffsetBudgetNs(std::int64_t &offset_ns) const;

private:
    PtpRole role_;
};
