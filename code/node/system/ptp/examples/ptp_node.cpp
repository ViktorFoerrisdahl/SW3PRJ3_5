#include "ptp_service.h"

#include <iostream>
#include <string>

int main(int argc, char **argv)
{
    const bool status_only = argc == 3 && std::string(argv[2]) == "status";
    if (argc != 2 && !status_only)
    {
        std::cerr << "Usage: ptp_node grandmaster|slave [status]\n";
        return 1;
    }

    PtpRole role;
    if (std::string(argv[1]) == "grandmaster")
    {
        role = PtpRole::Grandmaster;
    }
    else if (std::string(argv[1]) == "slave")
    {
        role = PtpRole::Slave;
    }
    else
    {
        std::cerr << "Choose grandmaster or slave.\n";
        return 1;
    }

    PtpService ptp(role);
    if (status_only)
    {
        std::int64_t offset_ns;
        if (!ptp.readOffsetBudgetNs(offset_ns))
        {
            std::cout << "PTP not synchronized.\n";
            return 2;
        }
        std::cout << offset_ns << "\n";
        return 0;
    }
    if (!ptp.start())
    {
        std::cerr << "Could not start PTP services.\n";
        if (!ptp.stop())
        {
            std::cerr << "Could not stop PTP services. Check systemctl.\n";
        }
        return 1;
    }

    std::cout << "PTP services started. Check synchronization with journalctl.\n";
    std::cout << "Press Enter to stop.\n";
    std::cin.get();

    if (!ptp.stop())
    {
        std::cerr << "Could not stop PTP services.\n";
        return 1;
    }

    return 0;
}
