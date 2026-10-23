# Ring buffer: header-only, builds everywhere (also on Windows, for tests)
add_library(ringbuffer INTERFACE)
target_include_directories(ringbuffer INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(ringbuffer INTERFACE common Threads::Threads)
# Anything that uses the ring buffer needs C++20 (std::stop_token)
target_compile_features(ringbuffer INTERFACE cxx_std_20)

# Unit test for the ring buffer: needs no ALSA, so it builds on Windows and the Pi.
# Run with: ctest --test-dir build --output-on-failure
add_executable(ring_buffer_test tests/ring_buffer_test.cpp)
target_link_libraries(ring_buffer_test PRIVATE ringbuffer)
add_test(NAME ring_buffer_test COMMAND ring_buffer_test)
set_tests_properties(ring_buffer_test PROPERTIES TIMEOUT 60)

# ALSA capture: only where ALSA exists, i.e. Linux / the Raspberry Pi.
# Install it on the Pi with: sudo apt install libasound2-dev
find_package(ALSA QUIET)
if(ALSA_FOUND)
  add_library(alsa_capture src/alsa_capture.cpp)
  target_include_directories(alsa_capture PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
  target_link_libraries(alsa_capture PUBLIC ringbuffer ALSA::ALSA)
else()
  message(STATUS "ALSA not found: skipping alsa_capture (it only builds on Linux)")
endif()
