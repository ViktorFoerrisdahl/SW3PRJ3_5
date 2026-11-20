cmake_minimum_required(VERSION 3.16)
project(ATLAS_LoggerTest LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(logger_test
    logger_test.cpp
    ../src/json_logger.cpp
)

target_include_directories(logger_test PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../inc"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../common"
)

target_include_directories(logger_test SYSTEM PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../tools/external/json_dto"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../tools/external/rapidjson"
)

get_filename_component(ATLAS_PROJECT_ROOT
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../.."
    ABSOLUTE
)

target_compile_definitions(logger_test PRIVATE
    ATLAS_PROJECT_ROOT="${ATLAS_PROJECT_ROOT}"
)
