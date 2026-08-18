# Bootstrap Makefile for the n-body simulation.
#
# `make` will:
#   1. ensure a build toolchain (cmake) is available, installing it if missing,
#   2. configure the CMake project into ./build,
#   3. build the raylib_app executable (raylib itself is auto-downloaded and
#      built by CMake if it is not already installed on the system).
#
# Requires: a C compiler and git. On macOS, Homebrew is used to install cmake
# if it is missing; on Debian/Ubuntu, apt-get is used.

BUILD_DIR := build
BIN       := $(BUILD_DIR)/raylib_app

UNAME_S := $(shell uname -s)

.PHONY: all
all: $(BIN)

$(BIN): deps
	cmake -B $(BUILD_DIR)
	cmake --build $(BUILD_DIR)

# Ensure cmake exists; install it if it does not.
.PHONY: deps
deps:
	@command -v cmake >/dev/null 2>&1 || $(MAKE) install-cmake

.PHONY: install-cmake
install-cmake:
	@echo "cmake not found, installing..."
ifeq ($(UNAME_S),Darwin)
	@command -v brew >/dev/null 2>&1 || { \
		echo "Homebrew required. Install from https://brew.sh"; exit 1; }
	brew install cmake
else
	@if command -v apt-get >/dev/null 2>&1; then \
		sudo apt-get update && sudo apt-get install -y cmake git \
			build-essential libasound2-dev libx11-dev libxrandr-dev \
			libxi-dev libgl1-mesa-dev libglu1-mesa-dev libxcursor-dev \
			libxinerama-dev libwayland-dev libxkbcommon-dev; \
	else \
		echo "Unsupported platform. Install cmake and git manually."; exit 1; \
	fi
endif

.PHONY: run
run: $(BIN)
	./$(BIN)

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)
