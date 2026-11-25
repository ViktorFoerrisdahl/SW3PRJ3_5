# Writes one TDOA_RESULT to the real event log (logs/events.jsonl).
# Not registered with ctest, because it appends to that file every time it runs.
add_executable(logger_test logger_test.cpp)
target_link_libraries(logger_test PRIVATE orchestrator_logger)
