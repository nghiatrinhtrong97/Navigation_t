#!/bin/bash

# Complete navigation system startup script

echo "=== Automotive Navigation System Startup ==="

# Create build directory if it doesn't exist
mkdir -p build
cd build

# Build the entire system
echo "Building navigation system..."
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

if [ $? -ne 0 ]; then
    echo "Build failed!"
    exit 1
fi

echo "Build completed successfully"

# Function to start a service in background
start_service() {
    local service_name=$1
    local executable=$2
    
    echo "Starting $service_name..."
    $executable &
    local pid=$!
    echo "$service_name started with PID $pid"
    sleep 1
    
    # Check if service is still running
    if kill -0 $pid 2>/dev/null; then
        echo "✓ $service_name is running"
    else
        echo "✗ $service_name failed to start"
    fi
}

# Start backend services
echo ""
echo "=== Starting Qt Backend Services ==="

start_service "Positioning Service" "./services/positioning/nav_positioning_qt"
start_service "Map Service" "./services/map/map_service"
start_service "Routing Service" "./services/routing/routing_service"
start_service "Guidance Service" "./services/guidance/guidance_service"

# Wait a moment for services to initialize
echo ""
echo "Waiting for services to initialize..."
sleep 3

# Start GUI application
echo ""
echo "=== Starting GUI Application ==="

if [ -f "hmi/nav_hmi_gui" ]; then
    echo "Starting GUI Navigation Application..."
    ./hmi/nav_hmi_gui
elif [ -f "hmi/nav_hmi" ]; then
    echo "GUI not available, starting console application..."
    ./hmi/nav_hmi
else
    echo "No HMI application found!"
    exit 1
fi

echo ""
echo "Navigation system shutdown"