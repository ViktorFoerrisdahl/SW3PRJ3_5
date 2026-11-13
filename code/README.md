# Kode

Her placeres ATLAS-kode, tests og hjælpeværktøjer.

[`ptp/`](ptp/README.md) indeholder lokal styring af linuxptp på mikrofonnoderne,
systemd-servicefiler og en softwaretest. Modulet bygges med projektets CMake.

Se [kodekonventionerne](../docs/Projektstyring/code-conventions.md) og [AI-strategien](../docs/Projektstyring/ai-strategi.md) for sprog og arbejdsform.

<!-- BEGIN GENERATED NAVIGATION -->

## Filoversigt

Dette afsnit opdateres automatisk af CI. Ret teksten uden for markørerne.

- [node/src/event_detection/dc_remover.h](node/src/event_detection/dc_remover.h)
- [node/src/event_detection/detector_config.h](node/src/event_detection/detector_config.h)
- [node/src/event_detection/event_detection.cpp](node/src/event_detection/event_detection.cpp)
- [node/src/event_detection/event_detection.h](node/src/event_detection/event_detection.h)
- [node/src/event_detection/README.md](node/src/event_detection/README.md)
- [node/src/event_detection/sliding_rms.cpp](node/src/event_detection/sliding_rms.cpp)
- [node/src/event_detection/sliding_rms.h](node/src/event_detection/sliding_rms.h)
- [node/src/tcp/node_tcp.cpp](node/src/tcp/node_tcp.cpp)
- [node/src/tcp/node_tcp.h](node/src/tcp/node_tcp.h)
- [node/tests/event_detection/integration_tests/event_detection_hw_test](node/tests/event_detection/integration_tests/event_detection_hw_test)
- [node/tests/event_detection/integration_tests/event_detection_hw_test.cpp](node/tests/event_detection/integration_tests/event_detection_hw_test.cpp)
- [node/tests/event_detection/integration_tests/Makefile](node/tests/event_detection/integration_tests/Makefile)
- [node/tests/event_detection/mock_tests/mocks/ring_buffer_mock.h](node/tests/event_detection/mock_tests/mocks/ring_buffer_mock.h)
- [node/tests/event_detection/mock_tests/test_event_detection.cpp](node/tests/event_detection/mock_tests/test_event_detection.cpp)
- [node/tests/event_detection/mocks/ring_buffer.h](node/tests/event_detection/mocks/ring_buffer.h)
- [node/tests/event_detection/test_event_detection.cpp](node/tests/event_detection/test_event_detection.cpp)
- [orchestrator/src/tcp/orchestrator_tcp.cpp](orchestrator/src/tcp/orchestrator_tcp.cpp)
- [orchestrator/src/tcp/orchestrator_tcp.h](orchestrator/src/tcp/orchestrator_tcp.h)
- [ptp/CMakeLists.txt](ptp/CMakeLists.txt)
- [ptp/examples/ptp_node.cpp](ptp/examples/ptp_node.cpp)
- [ptp/include/ptp_localization.h](ptp/include/ptp_localization.h)
- [ptp/include/ptp_service.h](ptp/include/ptp_service.h)
- [ptp/README.md](ptp/README.md)
- [ptp/src/ptp_localization.cpp](ptp/src/ptp_localization.cpp)
- [ptp/src/ptp_service.cpp](ptp/src/ptp_service.cpp)
- [ptp/systemd/atlas-phc-grandmaster.service](ptp/systemd/atlas-phc-grandmaster.service)
- [ptp/systemd/atlas-phc-slave.service](ptp/systemd/atlas-phc-slave.service)
- [ptp/systemd/atlas-ptp-grandmaster.service](ptp/systemd/atlas-ptp-grandmaster.service)
- [ptp/systemd/atlas-ptp-slave.service](ptp/systemd/atlas-ptp-slave.service)
- [ptp/tests/ptp_localization_test.cpp](ptp/tests/ptp_localization_test.cpp)
- [ptp/tests/ptp_service_test.py](ptp/tests/ptp_service_test.py)
- [tdoa/detected_event.h](tdoa/detected_event.h)
- [tdoa/main.cpp](tdoa/main.cpp)
- [tdoa/tdoa.cpp](tdoa/tdoa.cpp)
- [tdoa/tdoa.h](tdoa/tdoa.h)
- [tools/context/generate.py](tools/context/generate.py)
- [tools/context/quality.py](tools/context/quality.py)
- [tools/context/requirements.txt](tools/context/requirements.txt)
- [tools/navigation/generate.py](tools/navigation/generate.py)

<!-- END GENERATED NAVIGATION -->
