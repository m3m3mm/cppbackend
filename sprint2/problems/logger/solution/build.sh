#!/bin/bash
set -e

# Create build directory
mkdir -p build
cd build

# Install dependencies with Conan
conan install .. --build=missing

# Configure with CMake using the conan-generated toolchain
cmake .. -G "Unix Makefiles" \
    -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake \
    -DCMAKE_POLICY_DEFAULT_CMP0091=NEW \
    -DCMAKE_BUILD_TYPE=Release

# Build the project
cmake --build .

# Run the executable
./bin/hello_log 