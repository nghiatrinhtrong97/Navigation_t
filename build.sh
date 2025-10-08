#!/bin/bash

# Build script for Automotive Navigation System
# This script builds the entire navigation system

set -e  # Exit on any error

echo "=== Building Automotive Navigation System ==="

# Configuration
BUILD_TYPE=${1:-Release}
BUILD_DIR="build"
INSTALL_DIR="install"

echo "Build type: $BUILD_TYPE"

# Clean previous build if requested
if [ "$2" = "clean" ]; then
    echo "Cleaning previous build..."
    rm -rf $BUILD_DIR $INSTALL_DIR
fi

# Create build directory
mkdir -p $BUILD_DIR
cd $BUILD_DIR

# Configure with CMake
echo "Configuring with CMake..."
if command -v qnx-gcc &> /dev/null; then
    echo "QNX toolchain detected"
    cmake .. -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
             -DCMAKE_INSTALL_PREFIX=../$INSTALL_DIR \
             -DQNX=ON
else
    echo "Using system toolchain"
    cmake .. -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
             -DCMAKE_INSTALL_PREFIX=../$INSTALL_DIR
fi

# Build
echo "Building..."
make -j$(nproc)

# Install
echo "Installing..."
make install

cd ..

echo "=== Build Complete ==="
echo "Binaries installed in: $INSTALL_DIR/bin"
echo ""
echo "Services built:"
echo "  - positioning_service"
echo "  - map_service"
echo "  - routing_service"
echo "  - guidance_service"
echo "  - nav_hmi"
echo ""
echo "To run the system:"
echo "  1. Start services in order: positioning, map, routing, guidance"
echo "  2. Run HMI application"
echo "  3. Or use the start_system.sh script"