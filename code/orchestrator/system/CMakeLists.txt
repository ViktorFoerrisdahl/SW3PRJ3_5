cmake_minimum_required(VERSION 3.15...4.10)

project(ATLAS_TUI VERSION 1.0
                DESCRIPTION "TUI for Atlas"
                LANGUAGES CXX)

set(CMAKE_CXX_FLAGS "-Wall")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_COMPILER g++)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/build/bin")

add_subdirectory(src)

message(STATUS "DEBUG: ${PROJECT_SOURCE_DIR}")