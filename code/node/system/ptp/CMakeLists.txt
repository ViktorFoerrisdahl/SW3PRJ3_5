add_library(ptp_service src/ptp_service.cpp)
target_include_directories(ptp_service PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(ptp_service PUBLIC cxx_std_20)

add_executable(ptp_node examples/ptp_node.cpp)
target_link_libraries(ptp_node PRIVATE ptp_service)

find_package(Python3 COMPONENTS Interpreter QUIET)
if(Python3_Interpreter_FOUND)
  add_test(NAME ptp_service_test
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/ptp_service_test.py
            $<TARGET_FILE:ptp_node>)
  set_tests_properties(ptp_service_test PROPERTIES TIMEOUT 30)
endif()

add_library(ptp_localization src/ptp_localization.cpp ../tdoa/tdoa.cpp)
target_include_directories(ptp_localization PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/../tdoa)
target_compile_features(ptp_localization PUBLIC cxx_std_20)

add_executable(ptp_localization_test tests/ptp_localization_test.cpp)
target_link_libraries(ptp_localization_test PRIVATE ptp_localization)
add_test(NAME ptp_localization_test COMMAND ptp_localization_test)
