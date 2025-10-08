#!/bin/bash

# System startup script for Automotive Navigation System
# This script starts all navigation services in the correct order

set -e

echo "=== Starting Automotive Navigation System ==="

# Configuration
INSTALL_DIR="install/bin"
LOG_DIR="logs"
GPS_DEVICE="/dev/ser1"
CAN_DEVICE="can0"
MAP_DATA_PATH="/opt/nav/data/map.data"

# Create log directory
mkdir -p $LOG_DIR

# Check if binaries exist
if [ ! -d "$INSTALL_DIR" ]; then
    echo "Error: Install directory not found. Please run build.sh first."
    exit 1
fi

echo "Starting services..."

# Start positioning service
echo "Starting positioning service..."
$INSTALL_DIR/positioning_service --gps $GPS_DEVICE --can $CAN_DEVICE > $LOG_DIR/positioning.log 2>&1 &
POSITIONING_PID=$!
echo "Positioning service started (PID: $POSITIONING_PID)"
sleep 2

# Start map service
echo "Starting map service..."
$INSTALL_DIR/map_service --data $MAP_DATA_PATH > $LOG_DIR/map.log 2>&1 &
MAP_PID=$!
echo "Map service started (PID: $MAP_PID)"
sleep 2

# Start routing service
echo "Starting routing service..."
$INSTALL_DIR/routing_service > $LOG_DIR/routing.log 2>&1 &
ROUTING_PID=$!
echo "Routing service started (PID: $ROUTING_PID)"
sleep 2

# Start guidance service
echo "Starting guidance service..."
$INSTALL_DIR/guidance_service > $LOG_DIR/guidance.log 2>&1 &
GUIDANCE_PID=$!
echo "Guidance service started (PID: $GUIDANCE_PID)"
sleep 2

echo "All services started successfully!"
echo ""
echo "Service PIDs:"
echo "  Positioning: $POSITIONING_PID"
echo "  Map:         $MAP_PID"
echo "  Routing:     $ROUTING_PID"
echo "  Guidance:    $GUIDANCE_PID"
echo ""
echo "Logs are written to: $LOG_DIR/"
echo ""
echo "Starting HMI application..."

# Start HMI application
$INSTALL_DIR/nav_hmi

echo ""
echo "HMI application terminated. Stopping services..."

# Stop services
echo "Stopping services..."
kill $GUIDANCE_PID 2>/dev/null || true
kill $ROUTING_PID 2>/dev/null || true
kill $MAP_PID 2>/dev/null || true
kill $POSITIONING_PID 2>/dev/null || true

echo "System stopped."