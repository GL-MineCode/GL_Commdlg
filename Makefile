# Makefile for DateTimeForCpp project
# Targets:test (test/test.cpp)

CXX      := g++.exe
CXXFLAGS := -std=c++20 -O2 -g -Wall -Wextra -Wno-missing-field-initializers -Wno-subobject-linkage
INCLUDES := -Iinclude
LDFLAGS  := 
LDLIBS   := -lcomdlg32 -lshell32 -lgdi32 -lole32

# Build dir
BUILD_DIR := build

# Target files
TARGETS := $(BUILD_DIR)/test.exe

.PHONY: all clean test

all: $(TARGETS)

# Make build dir
$(BUILD_DIR):
	mkdir $(BUILD_DIR)

# test - Test for DateTime
$(BUILD_DIR)/test.exe: test/test.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< -o $@ $(LDFLAGS) $(LDLIBS)

test: $(BUILD_DIR)/test.exe

clean:
	-if exist $(BUILD_DIR) rmdir /s /q $(BUILD_DIR)
