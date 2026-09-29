# Kodekonventioner

Konventionerne er flyttet fra den tidligere README. Kode, navngivning, kommentarer, docstrings, Doxygen og commitbeskeder skrives på **engelsk**. Denne vejledning er på dansk. Historiske eksempler er erstattet med engelske eksempler, som følger navngivningstabellen.

Brug editorens »Format Document« (typisk `Shift + Alt + F` i VS Code; genvejen afhænger af platform). Projektet har endnu ikke en fælles formatter-konfiguration; automatisk formatering skal derfor gennemgås i diffen.

## Navngivning i C og C++

| Type | Format | Eksempel |
| --- | --- | --- |
| Makroer/defines | CAPS_WITH_UNDERSCORE | `BUFFER_CAPACITY` |
| Offentlige funktioner | Module_CamelCase | `Audio_ReadFrame` |
| Private funktioner | CamelCase | `ReadFrame` |
| Enum-typer | CAPS_WITH_UNDERSCORE | `AUDIO_STATUS` |
| Enum-medlemmer | CAPS_WITH_UNDERSCORE | `AUDIO_OK` |
| Struct-typer | CAPS_WITH_UNDERSCORE | `AUDIO_FRAME` |
| Struct-medlemmer | lower_case_with_underscores | `timestamp_ns` |
| Pointere | p_lower_case_with_underscores | `p_samples` |
| Globale variabler | g_lower_case_with_underscores | `g_sample_count` |
| Lokale variabler | lower_case_with_underscores | `sample_count` |
| Typedef-aliaser | lower_case_t | `sample_index_t` |

Den tidligere README brugte staveformen »no_cammelcase_with_underscore«; tabellen præciserer den som små bogstaver med underscores uden at ændre konventionen. Hvis et navn både er globalt og en pointer, bruges `g_p_`, f.eks. `g_p_samples`.

- Angiv enheder i relevante navne, f.eks. `relay_timeout_ms` og `sample_rate_hz`.
- Brug ikke `_` eller `__` i begyndelsen af egne variabel- eller funktionsnavne. Undgå også `__` andre steder i C++-identifikatorer.
- Den tidligere README fastlagde ikke en særskilt regel for C++-klassenavne og metoder. Aftal og dokumentér den, når C++-strukturen indføres; gør ikke de gamle SCD30-eksempler til en skjult regel.

## Doxygen i headerfiler

Dokumentér grænseflader i headerfiler med Doxygen. Brug `@enum` og `@class` til typer, `@brief` til en kort forklaring, `@param` til parametre og `@return` til returværdien. Forklar relevante enheder, ejerskab og fejltilfælde.

```cpp
/**
 * @enum AUDIO_STATUS
 * @brief Result of an audio capture operation.
 */
enum AUDIO_STATUS
{
    AUDIO_OK,       ///< A complete frame is available.
    AUDIO_TIMEOUT,  ///< No frame arrived within the requested timeout.
    AUDIO_ERROR     ///< The audio interface reported an error.
};

/**
 * @brief Wait for a complete audio frame.
 * @param timeout_ms Maximum wait time in milliseconds.
 * @return AUDIO_OK on success, AUDIO_TIMEOUT on timeout, or AUDIO_ERROR on failure.
 */
AUDIO_STATUS Audio_ReadFrame(unsigned int timeout_ms);
```

Eksemplet illustrerer dokumentationsformen og definerer ikke projektets kommende API.

## Ved ændringer

Følg den eksisterende struktur, skriv relevante tests og kontrollér ændringerne før review. Skeln mellem kodekonventioner og faktisk understøttede buildkommandoer; repoet har endnu ikke et buildsystem for ATLAS. Python-værktøjer bruger almindelig Python-navngivning (`snake_case`); C/C++-tabellen gælder ikke Python.
