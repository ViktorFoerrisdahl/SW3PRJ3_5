set (INC ${PROJECT_SOURCE_DIR}/inc)

set(SRC main.cpp menu.cpp plot.cpp microphones.cpp)

add_executable(main.out ${SRC})

add_custom_target(copy_plot_script ALL
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/plot.py
        $<TARGET_FILE_DIR:main.out>/plot.py
)
add_dependencies(main.out copy_plot_script)

target_include_directories(main.out PUBLIC ${INC})


