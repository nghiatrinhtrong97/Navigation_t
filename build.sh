#!/bin/bash

# Cross-platform Linux build script for Automotive Navigation System
# This script builds the entire navigation system with Qt GUI support

set -e  # Exit on any error

echo "=== Building Automotive Navigation System for Linux ==="

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

# Configure with CMake - detect Qt and platform
echo "Configuring with CMake..."
if command -v qnx-gcc &> /dev/null; then
    echo "QNX toolchain detected"
    cmake .. -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
             -DCMAKE_INSTALL_PREFIX=../$INSTALL_DIR \
             -DQNX=ON \
             -DBUILD_TESTS=OFF
else
    echo "Using Linux system toolchain"
    cmake .. -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
             -DCMAKE_INSTALL_PREFIX=../$INSTALL_DIR \
             -DBUILD_TESTS=OFF
fi

# Build with parallel jobs
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

# Check if GUI was built
if [ -f "$INSTALL_DIR/bin/nav_hmi_gui" ]; then
    echo "  - nav_hmi_gui (Qt GUI)"
    echo ""
    echo "Qt GUI application built successfully!"
    echo "Required Qt5 libraries: qtbase5-dev, qttools5-dev"
else
    echo "  - GUI not built (Qt5 not found or incomplete)"
    echo ""
    echo "To build GUI, install Qt5 development packages:"
    echo "  Ubuntu/Debian: sudo apt install qtbase5-dev qttools5-dev"
    echo "  CentOS/RHEL: sudo yum install qt5-qtbase-devel qt5-qttools-devel"
fi

echo ""
echo "To run the system:"
echo "  1. cd $INSTALL_DIR/bin"
echo "  2. Start services: ./positioning_service & ./map_service & ..."
echo "  3. Run GUI: ./nav_hmi_gui"
echo ""
echo "Or install as system service:"
echo "  sudo cp $INSTALL_DIR/bin/* /usr/local/bin/"