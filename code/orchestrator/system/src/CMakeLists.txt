set (INC ${PROJECT_SOURCE_DIR}/inc)

set(SRC main.cpp menu.cpp plot.cpp microphones.cpp json_logger.cpp TUI_thread.cpp)

add_executable(main.out ${SRC})

add_custom_target(copy_plot_script ALL
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/plot.py
        $<TARGET_FILE_DIR:main.out>/plot.py
)
add_dependencies(main.out copy_plot_script)

target_include_directories(main.out PUBLIC
    ${INC}
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../common"
)

# Bundled JSON headers shared by the orchestrator modules.
set(ATLAS_EXTERNAL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../tools/external")
target_include_directories(main.out SYSTEM PRIVATE
    "${ATLAS_EXTERNAL_DIR}/json_dto"
    "${ATLAS_EXTERNAL_DIR}/rapidjson"
)

get_filename_component(ATLAS_PROJECT_ROOT
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../.."
    ABSOLUTE
)

target_compile_definitions(main.out PRIVATE
    ATLAS_PROJECT_ROOT="${ATLAS_PROJECT_ROOT}"
)
