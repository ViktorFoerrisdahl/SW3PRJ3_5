# Test af kæden klap -> hændelsesdetektering -> NodeTCP -> orchestrator.
#
# Bygges og køres på noden fra denne mappe:
#   make                                       bygger node_tcp_test
#   make run IP=192.168.1.42                   kører med mikrofonerne på hw:0,0
#   make run IP=192.168.1.42 DEVICE=hw:1,0     andet lydkort (find det med: arecord -l)
#   make fake IP=192.168.1.42                  kun TCP, sender en kunstig hændelse hvert 2. sekund
#   make clean                                 sletter programmet
#
# Start orchestrator_tcp_test på orchestratoren først (orchestrator/tests/tcp).
# Kræver libasound2-dev:  sudo apt install libasound2-dev

CXX      ?= g++
CXXFLAGS ?= -std=c++20 -O2 -Wall -Wextra -pthread
LDLIBS   := -lasound

# Stier i forhold til denne mappe (code/node/tests/tcp)
CODE     := ../../..
SYS      := ../../system
ED_DIR   := $(SYS)/event_detection
RB_DIR   := $(SYS)/ringbuffer
TCP_DIR  := $(SYS)/tcp
COMMON   := $(CODE)/common
EXTERNAL := $(CODE)/tools/external

INCLUDES := -I$(ED_DIR) -I$(RB_DIR)/include -I$(TCP_DIR) -I$(COMMON) \
            -isystem $(EXTERNAL)/json_dto -isystem $(EXTERNAL)/rapidjson

SRCS := node_tcp_test.cpp \
        $(ED_DIR)/event_detection.cpp \
        $(ED_DIR)/sliding_rms.cpp \
        $(RB_DIR)/src/alsa_capture.cpp \
        $(TCP_DIR)/node_tcp.cpp

HEADERS := $(wildcard $(ED_DIR)/*.h) $(wildcard $(RB_DIR)/include/*.h) \
           $(wildcard $(TCP_DIR)/*.h) $(wildcard $(COMMON)/*.h)

TARGET := node_tcp_test
PORT   ?= 5000
DEVICE ?= hw:0,0
NODE   ?= 1

.PHONY: all run fake clean check-ip

all: $(TARGET)

$(TARGET): $(SRCS) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(SRCS) -o $@ $(LDLIBS)

check-ip:
	@test -n "$(IP)" || (echo "Angiv orchestratorens IP, fx: make run IP=192.168.1.42" && false)

run: $(TARGET) check-ip
	./$(TARGET) $(IP) $(PORT) $(DEVICE) $(NODE)

fake: $(TARGET) check-ip
	./$(TARGET) $(IP) $(PORT) --fake $(NODE)

clean:
	rm -f $(TARGET)
