#!/bin/bash

# Build and run Qt GUI navigation application

echo "Building Qt GUI Navigation Application..."

# Create build directory
mkdir -p build
cd build

# Configure with CMake
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build the project
make -j$(nproc)

# Check if GUI application was built
if [ -f "hmi/nav_hmi_gui" ]; then
    echo "Starting GUI Navigation Application..."
    ./hmi/nav_hmi_gui
elif [ -f "hmi/nav_hmi" ]; then
    echo "GUI not available, starting console application..."
    ./hmi/nav_hmi
else
    echo "Build failed - no executable found"
    exit 1
fi