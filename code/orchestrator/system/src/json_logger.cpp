#include "json_logger.h"

#include <fstream>
#include <json_dto/pub.hpp>

struct EVENT_LOG_ENTRY
{
    TDOA_RESULT result;
    std::string rejection_reason;
};

template <typename JSON_IO>
void json_io(JSON_IO &io, EVENT_LOG_ENTRY &entry)
{
    auto &result = entry.result;
    const auto seconds = static_cast<std::time_t>(result.time_stamp_ns / 1'000'000'000);
    std::tm local_time{};
    localtime_r(&seconds, &local_time);
    char timestamp[32]{};
    std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &local_time);
    std::string time_stamp = timestamp; // json_dto kan ikke skrive et char-array, men godt en std::string
    io
        & json_dto::mandatory("valid", result.valid)
        & json_dto::mandatory("RMS_value", result.rms)
        & json_dto::mandatory("x_m", result.x_m)
        & json_dto::mandatory("y_m", result.y_m)
        & json_dto::mandatory("time_stamp", time_stamp)
        & json_dto::optional("rejection_reason", entry.rejection_reason, std::string{});
}

bool JsonLogger_AppendEvent(const TDOA_RESULT &result, const std::string &rejection_reason)
{
    const std::string file_path =
        std::string(ATLAS_PROJECT_ROOT)
        + "/code/orchestrator/system/logs/events.jsonl";
    const std::string json = json_dto::to_json(EVENT_LOG_ENTRY{result, rejection_reason});

    // app skriver videre i filen uden at overskrive de tidligere hændelser.
    std::ofstream file(file_path, std::ios::app);
    if (!file.is_open())
    {
        return false;
    }

    file << json << '\n';
    // Luk filen før fejlkontrollen, så den sidste skrivning også bliver kontrolleret.
    file.close();

    return !file.fail();
}
