# Ring buffer: header-only, builds everywhere (also on Windows, for tests)
add_library(ringbuffer INTERFACE)
target_include_directories(ringbuffer INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(ringbuffer INTERFACE common Threads::Threads)
# Anything that uses the ring buffer needs C++20 (std::stop_token)
target_compile_features(ringbuffer INTERFACE cxx_std_20)

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
