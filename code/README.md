# Kode

Her placeres ATLAS-kode, tests og hjælpeværktøjer.

[`node/system/ptp/`](node/system/ptp/README.md) indeholder lokal styring af linuxptp på mikrofonnoderne,
systemd-servicefiler og en softwaretest. Modulet bygges med projektets CMake.

Se [kodekonventionerne](../docs/Projektstyring/code-conventions.md) og [AI-strategien](../docs/Projektstyring/ai-strategi.md) for sprog og arbejdsform.

<!-- BEGIN GENERATED NAVIGATION -->

## Filoversigt

Dette afsnit opdateres automatisk af CI. Ret teksten uden for markørerne.

- [common/event_queue.h](common/event_queue.h)
- [common/frame.h](common/frame.h)
- [common/node_event.h](common/node_event.h)
- [common/test.cpp](common/test.cpp)
- [node/system/event_detection/dc_remover.h](node/system/event_detection/dc_remover.h)
- [node/system/event_detection/detector_config.h](node/system/event_detection/detector_config.h)
- [node/system/event_detection/event_detection.cpp](node/system/event_detection/event_detection.cpp)
- [node/system/event_detection/event_detection.h](node/system/event_detection/event_detection.h)
- [node/system/event_detection/README.md](node/system/event_detection/README.md)
- [node/system/event_detection/sliding_rms.cpp](node/system/event_detection/sliding_rms.cpp)
- [node/system/event_detection/sliding_rms.h](node/system/event_detection/sliding_rms.h)
- [node/system/ptp/CMakeLists.txt](node/system/ptp/CMakeLists.txt)
- [node/system/ptp/examples/ptp_node.cpp](node/system/ptp/examples/ptp_node.cpp)
- [node/system/ptp/include/ptp_localization.h](node/system/ptp/include/ptp_localization.h)
- [node/system/ptp/include/ptp_service.h](node/system/ptp/include/ptp_service.h)
- [node/system/ptp/README.md](node/system/ptp/README.md)
- [node/system/ptp/src/ptp_localization.cpp](node/system/ptp/src/ptp_localization.cpp)
- [node/system/ptp/src/ptp_service.cpp](node/system/ptp/src/ptp_service.cpp)
- [node/system/ptp/systemd/atlas-phc-grandmaster.service](node/system/ptp/systemd/atlas-phc-grandmaster.service)
- [node/system/ptp/systemd/atlas-phc-slave.service](node/system/ptp/systemd/atlas-phc-slave.service)
- [node/system/ptp/systemd/atlas-ptp-grandmaster.service](node/system/ptp/systemd/atlas-ptp-grandmaster.service)
- [node/system/ptp/systemd/atlas-ptp-slave.service](node/system/ptp/systemd/atlas-ptp-slave.service)
- [node/system/ptp/tests/ptp_localization_test.cpp](node/system/ptp/tests/ptp_localization_test.cpp)
- [node/system/ptp/tests/ptp_service_test.py](node/system/ptp/tests/ptp_service_test.py)
- [node/system/ringbuffer/CMakeLists.txt](node/system/ringbuffer/CMakeLists.txt)
- [node/system/ringbuffer/include/alsa_capture.h](node/system/ringbuffer/include/alsa_capture.h)
- [node/system/ringbuffer/include/ring_buffer.h](node/system/ringbuffer/include/ring_buffer.h)
- [node/system/ringbuffer/src/alsa_capture.cpp](node/system/ringbuffer/src/alsa_capture.cpp)
- [node/system/ringbuffer/tests/alsa_hw_test.cpp](node/system/ringbuffer/tests/alsa_hw_test.cpp)
- [node/system/ringbuffer/tests/ring_buffer_test.cpp](node/system/ringbuffer/tests/ring_buffer_test.cpp)
- [node/system/tcp/node_tcp.cpp](node/system/tcp/node_tcp.cpp)
- [node/system/tcp/node_tcp.h](node/system/tcp/node_tcp.h)
- [node/tests/event_detection/integration_tests/event_detection_hw_test](node/tests/event_detection/integration_tests/event_detection_hw_test)
- [node/tests/event_detection/integration_tests/event_detection_hw_test.cpp](node/tests/event_detection/integration_tests/event_detection_hw_test.cpp)
- [node/tests/event_detection/integration_tests/Makefile](node/tests/event_detection/integration_tests/Makefile)
- [node/tests/event_detection/mock_tests/mocks/ring_buffer_mock.h](node/tests/event_detection/mock_tests/mocks/ring_buffer_mock.h)
- [node/tests/event_detection/mock_tests/test_event_detection.cpp](node/tests/event_detection/mock_tests/test_event_detection.cpp)
- [node/tests/event_detection/mocks/ring_buffer.h](node/tests/event_detection/mocks/ring_buffer.h)
- [node/tests/event_detection/test_event_detection.cpp](node/tests/event_detection/test_event_detection.cpp)
- [orchestrator/system/ATLAS_TUI/CMakeLists.txt](orchestrator/system/ATLAS_TUI/CMakeLists.txt)
- [orchestrator/system/ATLAS_TUI/inc/menu.h](orchestrator/system/ATLAS_TUI/inc/menu.h)
- [orchestrator/system/ATLAS_TUI/inc/microphones.h](orchestrator/system/ATLAS_TUI/inc/microphones.h)
- [orchestrator/system/ATLAS_TUI/inc/plot.h](orchestrator/system/ATLAS_TUI/inc/plot.h)
- [orchestrator/system/ATLAS_TUI/requirements.txt](orchestrator/system/ATLAS_TUI/requirements.txt)
- [orchestrator/system/ATLAS_TUI/src/CMakeLists.txt](orchestrator/system/ATLAS_TUI/src/CMakeLists.txt)
- [orchestrator/system/ATLAS_TUI/src/main.cpp](orchestrator/system/ATLAS_TUI/src/main.cpp)
- [orchestrator/system/ATLAS_TUI/src/menu.cpp](orchestrator/system/ATLAS_TUI/src/menu.cpp)
- [orchestrator/system/ATLAS_TUI/src/microphones.cpp](orchestrator/system/ATLAS_TUI/src/microphones.cpp)
- [orchestrator/system/ATLAS_TUI/src/plot.cpp](orchestrator/system/ATLAS_TUI/src/plot.cpp)
- [orchestrator/system/ATLAS_TUI/src/plot.py](orchestrator/system/ATLAS_TUI/src/plot.py)
- [orchestrator/system/tcp/orchestrator_tcp.cpp](orchestrator/system/tcp/orchestrator_tcp.cpp)
- [orchestrator/system/tcp/orchestrator_tcp.h](orchestrator/system/tcp/orchestrator_tcp.h)
- [orchestrator/system/tdoa/detected_event.h](orchestrator/system/tdoa/detected_event.h)
- [orchestrator/system/tdoa/main.cpp](orchestrator/system/tdoa/main.cpp)
- [orchestrator/system/tdoa/tdoa.cpp](orchestrator/system/tdoa/tdoa.cpp)
- [orchestrator/system/tdoa/tdoa.h](orchestrator/system/tdoa/tdoa.h)
- [Simulator/CMakeLists.txt](Simulator/CMakeLists.txt)
- [Simulator/include/simulator.h](Simulator/include/simulator.h)
- [Simulator/src/main.cpp](Simulator/src/main.cpp)
- [Simulator/src/simulator.cpp](Simulator/src/simulator.cpp)
- [tools/context/generate.py](tools/context/generate.py)
- [tools/context/quality.py](tools/context/quality.py)
- [tools/context/requirements.txt](tools/context/requirements.txt)
- [tools/navigation/generate.py](tools/navigation/generate.py)

<!-- END GENERATED NAVIGATION -->
