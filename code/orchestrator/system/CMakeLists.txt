cmake_minimum_required(VERSION 3.16...4.10)

project(ATLAS_TUI VERSION 1.0
                DESCRIPTION "TUI for Atlas"
                LANGUAGES CXX)

# Can be built on its own (cmake -S code/orchestrator/system -B build)
# or as part of the whole system from the repository root.
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
  set(CMAKE_CXX_STANDARD 20)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_CXX_EXTENSIONS OFF)
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
  add_compile_options(-Wall -Wextra)
  enable_testing()
endif()

find_package(Threads REQUIRED)

set(ATLAS_PTP_DIR "${PROJECT_SOURCE_DIR}/../../node/system/ptp")
add_subdirectory(src)
add_subdirectory(test)
