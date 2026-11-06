# Integrationstest af hændelsesdetekteringen med den rigtige ringbuffer og ALSA.
#
# Bygges og køres på Pi'en fra denne mappe:
#   make              bygger event_detection_hw_test
#   make run          bygger og kører på hw:0,0 (andet kort: make run DEVICE=hw:1,0)
#   make clean        sletter programmet
#
# Kræver libasound2-dev:  sudo apt install libasound2-dev

CXX      ?= g++
CXXFLAGS ?= -std=c++23 -O2 -Wall -Wextra -pthread
LDLIBS   := -lasound

# Stier i forhold til denne mappe (code/node/tests/event_detection/integration_tests)
REPO_ROOT := ../../../../..
ED_DIR    := ../../../src/event_detection
RB_DIR    := $(REPO_ROOT)/ringbuffer
COMMON    := $(REPO_ROOT)/common

INCLUDES := -I$(ED_DIR) -I$(RB_DIR)/include -I$(COMMON)

SRCS := event_detection_hw_test.cpp \
        $(ED_DIR)/event_detection.cpp \
        $(ED_DIR)/sliding_rms.cpp \
        $(RB_DIR)/src/alsa_capture.cpp

HEADERS := $(wildcard $(ED_DIR)/*.h) $(wildcard $(RB_DIR)/include/*.h) $(COMMON)/frame.h

TARGET := event_detection_hw_test
DEVICE ?= hw:0,0

.PHONY: all run clean

all: $(TARGET)

# Genbygges hvis en af kilde- eller headerfilerne er ændret
$(TARGET): $(SRCS) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(SRCS) -o $@ $(LDLIBS)

run: $(TARGET)
	./$(TARGET) $(DEVICE)

clean:
	rm -f $(TARGET)
