# Writes one TDOA_RESULT to the real event log (logs/events.jsonl).
# Not registered with ctest, because it appends to that file every time it runs.
add_executable(logger_test logger_test.cpp)
target_link_libraries(logger_test PRIVATE orchestrator_logger)

add_executable(ptp_localization_test "${ATLAS_PTP_DIR}/tests/ptp_localization_test.cpp")
target_link_libraries(ptp_localization_test PRIVATE ptp_localization)
add_test(NAME ptp_localization_test COMMAND ptp_localization_test)
