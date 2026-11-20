# Test af OrchestratorTCP: modtager DetectedEvent fra begge nodes, parrer dem og udskriver parrene.
#
# Bygges og køres på orchestratoren fra denne mappe:
#   make              bygger orchestrator_tcp_test
#   make run          kører på port 5000 (anden port: make run PORT=6000)
#   make clean        sletter programmet
#
# Start dette program før node_tcp_test (node/tests/tcp).

CXX      ?= g++
CXXFLAGS ?= -std=c++20 -O2 -Wall -Wextra -pthread

# Stier i forhold til denne mappe (code/orchestrator/tests/tcp)
CODE     := ../../..
SYS      := ../../system
COMMON   := $(CODE)/common
EXTERNAL := $(CODE)/tools/external

# common: detected_event.h, frame.h. inc: orchestrator_tcp.h, event_pair_queue.h
INCLUDES := -I$(COMMON) -I$(SYS)/inc \
            -isystem $(EXTERNAL)/json_dto -isystem $(EXTERNAL)/rapidjson

SRCS := orchestrator_tcp_test.cpp $(SYS)/src/orchestrator_tcp.cpp

HEADERS := $(SYS)/inc/orchestrator_tcp.h $(SYS)/inc/event_pair_queue.h $(wildcard $(COMMON)/*.h)

TARGET := orchestrator_tcp_test
PORT   ?= 5000

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(SRCS) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(SRCS) -o $@

run: $(TARGET)
	./$(TARGET) $(PORT)

clean:
	rm -f $(TARGET)
