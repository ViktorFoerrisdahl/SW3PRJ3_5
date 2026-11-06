# Generates mock NODE_EVENT data for testing the TDOA logic
add_library(simulator_lib src/simulator.cpp)
target_include_directories(simulator_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(simulator_lib PUBLIC common)
