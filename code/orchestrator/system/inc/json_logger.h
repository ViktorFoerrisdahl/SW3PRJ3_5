#pragma once

#include <string>
#include "tdoa.h"

/**
 * @brief Tilføj ét TDOA-resultat som en JSON-linje.
 * @param result Lokaliseringsresultat med koordinater i meter.
 * @param file_path Sti til logfilen; den overordnede mappe skal findes.
 * @return true, hvis skrivning og lukning lykkes; false ved filfejl.
 */
bool JsonLogger_AppendEvent(
    const TDOA_RESULT& result,
    const std::string& file_path);
