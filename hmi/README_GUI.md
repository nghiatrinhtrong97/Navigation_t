# Qt GUI Navigation Application Setup

## Overview
This directory contains the Qt-based GUI implementation of the automotive navigation system. The application provides an interactive map interface for route planning, simulation, and navigation guidance.

## Features

### Interactive Map Display
- **Real-time Map Rendering**: Custom map widget with coordinate grid and geographic projection
- **Zoom Controls**: Mouse wheel and keyboard zoom (+ / -)
- **Pan Navigation**: Arrow keys for map movement
- **Click-to-Set Waypoints**: Left click to set start point, Shift+click for end point
- **Current Position Display**: Blue circle with heading indicator
- **Route Visualization**: Blue line with direction arrows
- **Waypoint Markers**: Green (start) and red (end) markers with labels

### Route Planning Interface
- **Coordinate Input**: Manual latitude/longitude entry for precise positioning
- **Interactive Point Selection**: Click on map to set waypoints
- **Route Calculation**: A* algorithm integration for optimal path finding
- **Route Information**: Distance, time estimates, and progress tracking

### Simulation Controls
- **Speed Control**: 1x to 10x simulation speed multiplier
- **Start/Stop Controls**: Begin and halt route simulation
- **Progress Tracking**: Real-time progress bar and position updates
- **Automatic Navigation**: Follows calculated route with smooth interpolation

### Information Panels
- **Current Position**: Real-time latitude, longitude, speed, and heading
- **Navigation Guidance**: Turn-by-turn instructions with distances
- **Route Statistics**: Total and remaining distance/time
- **System Log**: Event logging for debugging and monitoring

## Application Structure

### Main Window (`NavigationMainWindow`)
- **Central Interface**: Main application window with tabbed interface
- **Map View**: Large interactive map display area
- **Control Panels**: Route planning and simulation controls
- **Information Display**: Real-time navigation data and statistics
- **Menu System**: File operations and help information

### Navigation Controller (`NavigationController`)
- **Route Management**: Calculate, store, and manage navigation routes
- **Simulation Engine**: Vehicle movement simulation along calculated routes
- **Position Tracking**: Current location and movement state management
- **Guidance Generation**: Turn-by-turn instruction creation

### Map Renderer (`MapRenderer`)
- **Custom Widget**: Qt-based map rendering with geographic projections
- **Interactive Input**: Mouse and keyboard event handling
- **Visual Elements**: Route lines, waypoints, current position indicators
- **Coordinate Conversion**: Screen-to-geographic coordinate transformations

## Building and Running

### Linux (with Qt5)
```bash
./start_gui.sh
```

### Windows (with Qt5)
```cmd
start_gui.bat
```

### Manual Build
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make
./hmi/nav_hmi_gui
```

## Dependencies

### Required
- **Qt5**: Core, Widgets components
- **CMake 3.16+**: Build system
- **C++17**: Modern C++ standard
- **Common Library**: Navigation data types and utilities

### Optional
- **QNX**: Target platform support
- **Qt Quick**: Future enhanced UI components

## User Interface Guide

### Map Controls
- **Mouse Wheel**: Zoom in/out
- **Arrow Keys**: Pan map view
- **Left Click**: Set start point
- **Shift + Left Click**: Set end point
- **+ / - Keys**: Zoom controls

### Route Planning
1. **Set Start Point**: Click on map or enter coordinates
2. **Set End Point**: Shift+click on map or enter coordinates
3. **Calculate Route**: Click "Calculate Route" button
4. **View Route**: Blue line appears on map with statistics

### Simulation
1. **Start Simulation**: Click "Start Simulation" after route calculation
2. **Adjust Speed**: Use speed slider (1x - 10x)
3. **Monitor Progress**: Watch progress bar and position updates
4. **Stop Simulation**: Click "Stop Simulation" to halt

### Information Monitoring
- **Position Tab**: Real-time location and movement data
- **Guidance Tab**: Turn-by-turn navigation instructions
- **System Log**: Application events and debugging information

## Default Settings

### Map View
- **Center**: Hanoi, Vietnam (21.028511°N, 105.804817°E)
- **Zoom Level**: 12 (city-level view)
- **Grid**: Coordinate reference lines
- **Projection**: Simple Mercator-like projection

### Simulation
- **Default Speed**: 5x simulation multiplier
- **Update Rate**: 100ms intervals
- **Route Interpolation**: 20 intermediate points
- **Vehicle Speed**: 50 km/h simulated speed

## Customization

### Styling
- **Dark Theme**: Modern dark color palette
- **Fusion Style**: Cross-platform Qt widget style
- **Custom Colors**: Navigation-specific color scheme

### Map Settings
- **Zoom Range**: 5 (regional) to 20 (street level)
- **Grid Density**: Automatic based on zoom level
- **Coordinate Display**: High precision (6 decimal places)

## Technical Notes

### Coordinate System
- **Format**: WGS84 decimal degrees
- **Precision**: 6 decimal places (~0.1 meter accuracy)
- **Projection**: Simplified Mercator for display
- **Distance Calculations**: Haversine formula for accuracy

### Performance
- **Rendering**: Optimized paint events with antialiasing
- **Memory**: Efficient route point storage and management
- **Updates**: Minimal redraws for smooth animation
- **Threading**: Qt event loop for responsive UI

### Integration
- **Backend Services**: Connects to positioning, routing, and guidance services
- **Message Passing**: IPC communication with navigation microservices
- **Data Formats**: Compatible with automotive navigation protocols

This Qt GUI application provides a complete simulation environment for testing and demonstrating the automotive navigation system capabilities with an intuitive and professional interface.