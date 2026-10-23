cmake_minimum_required(VERSION 3.16)
project(SW3PRJ3_5 LANGUAGES CXX)

# C++20 for std::stop_token / std::jthread (ring buffer and capture thread)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Default to Debug when no build type is given (single-config generators only)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Debug)
endif()

# Warnings for both MSVC (Windows) and GCC (Raspberry Pi).
# /utf-8 so MSVC reads Danish characters in comments correctly.
if(MSVC)
  add_compile_options(/W4 /utf-8)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(Threads REQUIRED)

# Shared types used by several modules (header-only): Frame, NODE_EVENT
add_library(common INTERFACE)
target_include_directories(common INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/common)

add_subdirectory(ringbuffer)
add_subdirectory(Simulator)
