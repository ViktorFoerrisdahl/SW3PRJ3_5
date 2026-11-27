#pragma once

#include <string>
#include "tdoa.h"

/**
 * @brief Append one localization result to the event log as a JSON line.
 * @param result Localization result with coordinates in metres.
 * @param rejection_reason Optional explanation for rejecting the event.
 * @return True if the log was written and closed successfully.
 */
bool JsonLogger_AppendEvent(
    const TDOA_RESULT &result, const std::string &rejection_reason = {});
