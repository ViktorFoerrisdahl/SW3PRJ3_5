# SW3PRJ3_15
Github repository til 3.semesterprojekt

# Code conventions

Husk at bruge Shift + Alt + F inde på vs code for at få flot opsætning automatisk

All naming and comments must be done in English. 

For C language, the naming conversion should following:

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

#Doxygen
