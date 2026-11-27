add_library(ptp_service src/ptp_service.cpp)
target_include_directories(ptp_service PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(ptp_service PUBLIC cxx_std_20)

add_executable(ptp_node examples/ptp_node.cpp)
target_link_libraries(ptp_node PRIVATE ptp_service)

add_executable(ptp_service_test tests/ptp_service_test.cpp)
add_dependencies(ptp_service_test ptp_node)
add_test(NAME ptp_service_test COMMAND ptp_service_test $<TARGET_FILE:ptp_node>)
set_tests_properties(ptp_service_test PROPERTIES TIMEOUT 30)
