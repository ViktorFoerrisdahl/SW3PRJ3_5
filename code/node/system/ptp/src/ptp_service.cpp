#include "ptp_service.h"

#include <cstdlib>
#include <array>
#include <cstdio>
#include <sstream>
#include <string>

static bool ReadCommand(const std::string &command, std::string &output)
{
    FILE *pipe = popen(command.c_str(), "r");
    if (!pipe)
        return false;

    output.clear();
    std::array<char, 512> buffer{};
    while (fgets(buffer.data(), buffer.size(), pipe))
        output += buffer.data();

    if (pclose(pipe) != 0)
        return false;
    return true;
}

static bool ReadOffset(const std::string &unit, const std::string &marker,
                       std::int64_t &absolute_offset_ns)
{
    const std::string read_run_id_command = "systemctl show --property=InvocationID --value " + unit;
    std::string run_id;
    if (!ReadCommand(read_run_id_command, run_id) || run_id.size() != 33 || run_id.back() != '\n' ||
        run_id.substr(0, 32).find_first_not_of("0123456789abcdef") != std::string::npos ||
        run_id.substr(0, 32) == std::string(32, '0'))
        return false;

    std::string log;
    if (!ReadCommand(
            "journalctl --quiet --no-pager --output=cat --boot --since='3 seconds ago' "
            "_SYSTEMD_INVOCATION_ID=" +
                run_id.substr(0, 32),
            log))
        return false;

    const auto position = log.rfind(marker);
    if (position == std::string::npos)
        return false;

    const auto start = position + marker.size();
    std::istringstream fields(log.substr(start, log.find('\n', start) - start));
    std::int64_t offset_ns;
    std::string servo;
    if (!(fields >> offset_ns >> servo) || (servo != "s2" && servo != "s3"))
        return false;
    if (offset_ns < -PTP_MAX_OFFSET_NS || offset_ns > PTP_MAX_OFFSET_NS)
        return false;

    // A recent log entry can outlive a stopped or restarted service.
    std::string current;
    if (!ReadCommand("systemctl is-active " + unit, current) || current != "active\n" ||
        !ReadCommand(read_run_id_command, current) || current != run_id)
        return false;
    absolute_offset_ns = std::abs(offset_ns);
    return true;
}

PtpService::PtpService(PtpRole role) : role_(role)
{
}

bool PtpService::start()
{
    const char *command = role_ == PtpRole::Grandmaster
                              ? "systemctl start atlas-ptp-grandmaster.service atlas-phc-grandmaster.service"
                              : "systemctl start atlas-ptp-slave.service atlas-phc-slave.service";
    return std::system(command) == 0;
}

bool PtpService::stop()
{
    const char *command = role_ == PtpRole::Grandmaster
                              ? "systemctl stop atlas-phc-grandmaster.service atlas-ptp-grandmaster.service"
                              : "systemctl stop atlas-phc-slave.service atlas-ptp-slave.service";
    return std::system(command) == 0;
}

bool PtpService::readOffsetBudgetNs(std::int64_t &offset_ns) const
{
    const bool grandmaster = role_ == PtpRole::Grandmaster;
    std::int64_t phc_ns;
    if (!ReadOffset(
            grandmaster ? "atlas-phc-grandmaster.service" : "atlas-phc-slave.service",
            grandmaster ? "eth0 sys offset " : "CLOCK_REALTIME phc offset ", phc_ns))
        return false;

    std::string port;
    if (!ReadCommand(
            "pmc -u -b 0 -s /run/atlas-ptp4lro 'GET PORT_DATA_SET'", port))
        return false;
    std::istringstream fields(port);
    std::string field;
    while (fields >> field && field != "portState")
    {
    }
    std::string state;
    if (!(fields >> state) || state != (grandmaster ? "MASTER" : "SLAVE"))
        return false;

    std::int64_t network_ns = 0;
    if (!grandmaster && !ReadOffset("atlas-ptp-slave.service", "master offset ", network_ns))
        return false;
    if (phc_ns + network_ns > PTP_MAX_OFFSET_NS)
        return false;

    offset_ns = phc_ns + network_ns;
    return true;
}
