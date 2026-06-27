# Makefile for DateTimeForCpp project
# Targets:test (test/test.cpp)

CXX      := g++.exe
CXXFLAGS := -std=c++20 -O0 -g -Wall -Wextra -Wno-missing-field-initializers -Wno-subobject-linkage -MMD -MP
INCLUDES := -Iinclude
LDFLAGS  := 
LDLIBS   := -lcomdlg32 -lshell32 -lgdi32 -lole32 -luuid -ldwmapi

# Build dir
BUILD_DIR := build

# Target files
TARGETS := $(BUILD_DIR)/test.exe

# Auto-generated dependency files (one per source)
DEPS := $(TARGETS:.exe=.d)

.PHONY: all clean test

all: $(TARGETS)

# Make build dir
$(BUILD_DIR):
	mkdir $(BUILD_DIR)

# test - Test for DateTime
$(BUILD_DIR)/test.exe: test/test.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< -o $@ $(LDFLAGS) $(LDLIBS)

# Include auto-generated dependency files (if they exist)
-include $(DEPS)

test: $(BUILD_DIR)/test.exe

clean:
	-if exist $(BUILD_DIR) rmdir /s /q $(BUILD_DIR)
