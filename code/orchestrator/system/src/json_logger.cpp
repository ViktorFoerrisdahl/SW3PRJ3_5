#include "json_logger.h"

#include <fstream>
#include <json_dto/pub.hpp>

// Fortæller json_dto, hvilke felter fra TDOA-resultatet der skal gemmes.
// Parameteren er uden const, så biblioteket vælger denne funktion.
template <typename JSON_IO>
void json_io(JSON_IO &io, TDOA_RESULT &result)
{
    io
        & json_dto::mandatory("valid", result.valid)
        & json_dto::mandatory("x_m", result.x_m)
        & json_dto::mandatory("y_m", result.y_m)
        & json_dto::mandatory("time_stamp_ns", result.time_stamp_ns);
}

bool JsonLogger_AppendEvent(const TDOA_RESULT &result)
{
    const std::string file_path =
        std::string(ATLAS_PROJECT_ROOT)
        + "/code/orchestrator/system/logs/events.jsonl";
    const std::string json = json_dto::to_json(result);

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
