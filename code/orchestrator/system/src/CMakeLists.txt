# Paths relative to code/orchestrator/system
get_filename_component(ATLAS_CODE_DIR "${PROJECT_SOURCE_DIR}/../.." ABSOLUTE)
get_filename_component(ATLAS_PROJECT_ROOT "${ATLAS_CODE_DIR}/.." ABSOLUTE)
set(ATLAS_EXTERNAL_DIR "${ATLAS_CODE_DIR}/tools/external")

# Include paths shared by everything in the orchestrator: own headers,
# code/common (DetectedEvent) and the bundled JSON headers.
add_library(orchestrator_headers INTERFACE)
target_include_directories(orchestrator_headers INTERFACE
    "${PROJECT_SOURCE_DIR}/inc"
    "${ATLAS_CODE_DIR}/common"
)
target_include_directories(orchestrator_headers SYSTEM INTERFACE
    "${ATLAS_EXTERNAL_DIR}/json_dto"
    "${ATLAS_EXTERNAL_DIR}/rapidjson"
)

# TDOA solver. A library of its own because the node's ptp_localization also uses it.
add_library(orchestrator_tdoa tdoa.cpp)
target_link_libraries(orchestrator_tdoa PUBLIC orchestrator_headers)

# JSON event log. Writes to <repo>/code/orchestrator/system/logs/events.jsonl
add_library(orchestrator_logger json_logger.cpp)
target_link_libraries(orchestrator_logger PUBLIC orchestrator_headers)
target_compile_definitions(orchestrator_logger PRIVATE
    ATLAS_PROJECT_ROOT="${ATLAS_PROJECT_ROOT}"
)

add_executable(main.out
    main.cpp
    menu.cpp
    microphones.cpp
    plot.cpp
    TCP_thread.cpp
    TDOA_thread.cpp
    TUI_thread.cpp
)
target_link_libraries(main.out PRIVATE
    orchestrator_tdoa
    orchestrator_logger
    Threads::Threads
)

# Executable goes in <build dir>/bin. Standalone build in code/orchestrator/system/build
# gives build/bin/main.out as before.
set_target_properties(main.out PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin"
)

# plot.cpp runs "python3 plot.py", so the script has to sit next to the executable
# (and main.out has to be started from that folder).
add_custom_target(copy_plot_script ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:main.out>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/plot.py"
        "$<TARGET_FILE_DIR:main.out>/plot.py"
)
add_dependencies(main.out copy_plot_script)
