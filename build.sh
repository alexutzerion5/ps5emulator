#!/bin/bash

# PS5 Emulator Build Script

set -e

echo "Building PS5 Emulator..."

# Create build directory
mkdir -p build
cd build

# Configure with CMake
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
make -j$(nproc)

echo "Build complete!"
echo "Run with: ./ps5_emulator"