cmake_minimum_required(VERSION 3.16)
project(SW3PRJ3_5 LANGUAGES CXX)

# Builds the whole system from the repository root:
#   cmake -S . -B build
#   cmake --build build -j
#   ctest --test-dir build --output-on-failure
#
# On Windows (MSVC) only the portable parts build: common, ringbuffer and
# event detection. Node (ALSA, PTP, TCP) and orchestrator need Linux.

# C++20 for std::stop_token / std::jthread (ring buffer and capture thread)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# compile_commands.json for VS Code / clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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

# Register tests so they can be run with ctest
enable_testing()

option(ATLAS_BUILD_SIMULATOR
  "Build the Simulator (off: it still uses NODE_EVENT/node_event.h, which no longer exist)"
  OFF)

set(ATLAS_CODE_DIR     ${CMAKE_CURRENT_SOURCE_DIR}/code)
set(ATLAS_EXTERNAL_DIR ${ATLAS_CODE_DIR}/tools/external)

# Shared types used by several modules (header-only): Frame, DetectedEvent.
# detected_event.h uses json_dto, so the bundled JSON headers come along.
add_library(common INTERFACE)
target_include_directories(common INTERFACE ${ATLAS_CODE_DIR}/common)
target_include_directories(common SYSTEM INTERFACE
  ${ATLAS_EXTERNAL_DIR}/json_dto
  ${ATLAS_EXTERNAL_DIR}/rapidjson)

# The orchestrator and microphone node build their own PTP components.
if(UNIX)
  add_subdirectory(code/orchestrator/system)
endif()

add_subdirectory(code/node/system)

if(ATLAS_BUILD_SIMULATOR)
  add_subdirectory(code/Simulator)
endif()
