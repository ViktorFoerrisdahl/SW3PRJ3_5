# SW3PRJ3_5
GitHub repository til 3. semesterprojekt

# Code conventions

---------------------------------------------------------------------------------------
Husk at bruge **`Shift + Alt + F`** inde på vs code for at få flot opsætning automatisk
---------------------------------------------------------------------------------------

All naming and comments must be done in English. 

For C & C++ language, the naming convention should follow:

| Type              | Format                              |
| ----------------- | ----------------------------------- |
| Defines           | CAPS_WITH_UNDERSCORE                |
| Public Functions  | Module_CamelCase                    |
| Private Functions | CamelCase                           |
| Enums             | CAPS_WITH_UNDERSCORE                |
| Enum Members      | CAPS_WITH_UNDERSCORE                |
| Structs           | CAPS_WITH_UNDERSCORE                |
| Struct Members    | no_cammelcase_with_underscore       |
| Pointers          | **p**_all_lower_case                |
| Global Variables  | **g**_no_cammelcase_with_underscore |
| Local Variables   | no_cammelcase_with_underscore       |
| Typedefs          | no_cammelcase_t                     |

**DO:**
* Use units in variable names like: "relay_timeout_ms" if it makes sense.

**DON'T:**
* Use _ or __ ( double underscore) at the beginning of a variable or function. These are used by some compilers. Examples: _set_variable() // Do not do this


# Doxygen
Når H-filer laves, bruges Doxygen-dokumentation. Disse kan bruges som eksempel:
@enum - sættes i toppen af en enum, hvor den også navngives
@class - sættes i toppen af en klasse, hvor den også navngives

@brief - Bruges til at lave en kort beskrivelse af klassen, funktionen osv.
@param - Bruges til at forklare parametrene, som en funktion bruger
@return - sættes i bunden af @brief og forklarer, hvad der returneres. Særligt smart når en funktion returenere en enum

Nedenfor ses et eksempel på at bruge "@enum" i en H-fil fra 2. semester:
```C++
/**
 * @enum SCD30_Status
 * @brief Returværdi for SCD30 metoder.
 *
 * Bruges til at indikere om en sensor-aflæsning lykkedes,
 * eller hvad der gik galt.
 */
enum SCD30_Status
{
  SCD30_OK,         ///< Måling modtaget og CRC valideret
  SCD30_TIMEOUT,    ///< Sensor returnerede ikke data inden for timeout (~5 sek)
  SCD30_CRC_ERROR,  ///< CRC validering fejlede på modtagne data
  SCD30_BUS_ERROR   ///< I2C kommunikationsfejl

};
```

Nedenfor ses eksempel på at bruge "@class" og "@param" i en H-fil fra 2. semester. Der laves både @brief i toppen af klassen samt inde i klasses metoder, som bla. constructoren:
```C++
/**
 * @class SCD30
 * @brief Håndterer kommunikation med SCD30 CO2 sensoren.
 *
 * Klassen opdaterer sine interne attributter (CO2 og temperatur) når
 * readData() kaldes. Værdierne hentes efterfølgende via getters.
 */
class SCD30 {
public:
  /**
   * @brief Opretter en SCD30 instans.
   * @param i2c Reference til I2C-bussen. Adressen er hardcoded (0x61),
   *            da der kun understøttes én SCD30 per bus.
   */
  SCD30(I2C &i2c);
```

Nedenfor ses eksempel på at bruge "@return" i en funktion, da den returnere en enum fra det første eksempel:
 ```C++
  /**
   * @brief Poller sensor for data-ready og læser CO2 + temperatur.
   *
   * Venter i op til ~5 sekunder (50 forsøg á 100 ms) på at sensoren
   * melder data klar. Validerer alle CRC-checksums før data gemmes.
   *
   * @return SCD30_OK ved succes, SCD30_TIMEOUT hvis sensoren ikke svarer,
   *         SCD30_CRC_ERROR ved checksum-fejl, SCD30_BUS_ERROR ved I2C-fejl.
   */
  SCD30_Status readData();
```
