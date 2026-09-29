cmake_minimum_required(VERSION 3.16)
project(Simulator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Default to Debug when no build type is given (single-config generators only)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Debug)
endif()

# Warnings for both MSVC (Windows) and GCC (Raspberry Pi)
if(MSVC)
  add_compile_options(/W4)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# The simulator logic as a library, so tests and micnode replay can reuse it
add_library(simulator_lib src/simulator.cpp)
target_include_directories(simulator_lib PUBLIC include)

# Command-line program that writes the WAV files and ground truth
add_executable(simulator src/main.cpp)
target_link_libraries(simulator PRIVATE simulator_lib)