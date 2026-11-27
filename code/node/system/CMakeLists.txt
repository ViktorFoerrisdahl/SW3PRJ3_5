# Microphone node (Raspberry Pi 5). Added from the root CMakeLists.txt.

# Ring buffer (+ ALSA capture when ALSA is installed)
add_subdirectory(ringbuffer)

# Event detection: RingBuffer -> DetectedEvent -> EventQueue. Portable, builds everywhere.
add_library(event_detection
  event_detection/event_detection.cpp
  event_detection/sliding_rms.cpp)
target_include_directories(event_detection PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/event_detection)
target_link_libraries(event_detection PUBLIC ringbuffer common Threads::Threads)

# The rest uses POSIX sockets, linuxptp and ALSA: Linux / the Pi only
if(UNIX)
  add_subdirectory(ptp)

  # TCP: EventQueue -> orchestrator
  add_library(node_tcp tcp/node_tcp.cpp)
  target_include_directories(node_tcp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/tcp)
  target_link_libraries(node_tcp PUBLIC event_detection common ptp_service Threads::Threads)

  # The node program itself. Run: ./build/code/node/system/atlas_node <orchestrator-ip>
  if(TARGET alsa_capture)
    add_executable(atlas_node main.cpp)
    target_link_libraries(atlas_node PRIVATE
      alsa_capture event_detection node_tcp ptp_service Threads::Threads)
  else()
    message(STATUS "ALSA not found: skipping atlas_node (install libasound2-dev on the Pi)")
  endif()
endif()
