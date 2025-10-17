# Navigation_t
# Automotive Navigation System

A cross-platform automotive navigation system built with C++ and Qt, featuring integrated positioning, routing, map rendering, and guidance services.

## Overview

This project implements a complete navigation solution for automotive applications with the following key features:

- **Real-time GPS Positioning** - Accurate vehicle location tracking
- **Map Rendering** - Interactive map display with zoom and pan controls
- **Route Planning** - Intelligent routing with turn-by-turn navigation
- **Guidance Service** - Real-time navigation instructions
- **Cross-Platform** - Supports Linux, Windows, and QNX

## Architecture

```
Navigation_t/
├── common/          # Shared libraries and utilities
│   ├── include/     # Common headers (nav_types.h, nav_utils.h)
│   └── src/         # Common implementations
├── hmi/             # Human-Machine Interface (GUI)
│   ├── controllers/ # Navigation controllers
│   ├── models/      # Data models
│   ├── services/    # Core services (positioning, routing, map, guidance)
│   ├── ui/          # Qt UI components
│   └── resources/   # Icons and assets
├── config/          # Configuration files
└── docs/            # Documentation
```

## Key Components

### Services
- **Positioning Service** - GPS/GNSS integration with CAN bus fusion
- **Map Service** - Map data management and rendering
- **Routing Service** - A* pathfinding and route optimization
- **Guidance Service** - Turn-by-turn navigation instructions

### HMI Features
- Interactive map widget with touch support
- Real-time vehicle position display
- Route visualization
- Navigation status panel
- Qt-based modern UI

## Build Instructions

### Prerequisites
- CMake 3.16+
- Qt 5.15+ or Qt 6.x
- C++17 compatible compiler (GCC 7+, MSVC 2019+, Clang 7+)

### Linux
```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### Windows
```bash
mkdir build && cd build
cmake -G "Visual Studio 16 2019" ..
cmake --build . --config Release
```

### Cross-compilation for QNX
```bash
mkdir build-qnx && cd build-qnx
cmake -DCMAKE_TOOLCHAIN_FILE=qnx.cmake ..
make -j$(nproc)
```

## Quick Start

### Run the GUI Application
```bash
./build/hmi/nav_hmi_gui
```

### Run Console Mode
```bash
./build/hmi/nav_hmi_gui --console
```

## Configuration

Edit [`config/navigation.conf`](config/navigation.conf) to customize:
- Map data paths
- GPS/CAN bus settings
- Service ports and timeouts
- UI preferences

## Testing

```bash
# Enable tests during build
cmake -DBUILD_TESTS=ON ..
make

# Run tests
ctest --verbose
```

## 📦 Packaging

Create distribution packages:

```bash
# Linux (DEB/RPM)
cpack -G DEB
cpack -G RPM

# Windows (NSIS installer)
cpack -G NSIS

# Source package
cpack --config CPackSourceConfig.cmake
```

## 📖 Documentation

- [Service Integration](docs/SERVICE_INTEGRATION.md) - Service architecture details
- [Qt Framework Integration](docs/QT_FRAMEWORK_INTEGRATION.md) - Qt/GUI implementation
- [Cross-Platform Build](CROSS_PLATFORM_BUILD_SUMMARY.md) - Build system guide
- [Enhanced HMI Features](docs/ENHANCED_HMI_FEATURES.md) - UI components

## Technology Stack

- **Language**: C++17
- **GUI Framework**: Qt 5/6 (Widgets, Network)
- **Build System**: CMake
- **Threading**: POSIX threads
- **IPC**: Unix domain sockets / Named pipes

## System Requirements