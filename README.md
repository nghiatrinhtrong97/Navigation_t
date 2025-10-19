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
│   ├── services/    # Core services (positioning, routing, map, guidance, POI, geocoding)
│   ├── ui/          # Qt UI components
│   └── resources/   # Icons and assets
├── config/          # Configuration files
├── docs/            # Documentation
└── scripts/         # Build and deployment scripts
```

## Key Components

### Services
- **Positioning Service** - GPS/GNSS integration with CAN bus fusion
- **Map Service** - Map data management and rendering
- **Routing Service** - A* pathfinding and route optimization
- **Guidance Service** - Turn-by-turn navigation instructions
- **POI Service** - Point of Interest search and management
- **Geocoding Service** - Address parsing, normalization, and spatial indexing (Phase 1 Enhanced API)

### HMI Features
- Interactive map widget with touch support
- Real-time vehicle position display
- Route visualization
- Navigation status panel
- Qt-based modern UI

## Build Instructions

### Prerequisites
- CMake 3.16+
- Qt 6.6.1 (msvc2019_64 for Windows)
- C++17 compatible compiler (GCC 7+, MSVC 2019+, Clang 7+)

### Quick Build (Windows)

```powershell
# From repository root
.\scripts\build.bat

# Quick rebuild and run
.\scripts\rebuild_and_run.bat

# Run without rebuilding
.\scripts\run_integrated_nav.bat
```

> **Note:** Build artifacts are created **outside** the repository in `../build_Automotive` and `../install_Automotive`

### Linux
```bash
./scripts/build.sh
```

### Manual Build (Windows)

```bash
mkdir build && cd build
cmake -G "Visual Studio 16 2019" -DCMAKE_PREFIX_PATH="C:/Qt/6.6.1/msvc2019_64" ..
cmake --build . --config Release
cmake --install . --config Release
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
# Windows
.\scripts\run_integrated_nav.bat

# Linux
./scripts/run_integrated_nav.sh

# Or directly from install directory
../install_Automotive/bin/nav_hmi_gui.exe
```

### Development Workflow
```powershell
# 1. First build
.\scripts\build.bat

# 2. Make code changes...

# 3. Quick rebuild and test
.\scripts\rebuild_and_run.bat
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

### Core Documentation
- [README](README.md) - This file
- [Service Integration](docs/SERVICE_INTEGRATION.md) - Service architecture details
- [Qt Framework Integration](docs/QT_FRAMEWORK_INTEGRATION.md) - Qt/GUI implementation
- [Enhanced HMI Features](docs/ENHANCED_HMI_FEATURES.md) - UI components

### Build & Deployment
- [Build Guide](docs/BUILD_GUIDE.md) - Detailed build instructions
- [Build Scripts Guide](docs/BUILD_SCRIPTS_GUIDE.md) - Script usage and customization
- [Build Structure](docs/BUILD_STRUCTURE.md) - Project structure explanation
- [Scripts README](scripts/README.md) - Quick reference for build scripts
- [Migration Guide](docs/MIGRATION_GUIDE.md) - Migration from old build structure

### Phase 1 - Enhanced Geocoding (COMPLETE ✅)
- [Geocoder Modernization Analysis](docs/GEOCODER_MODERNIZATION_ANALYSIS.md) - Initial analysis
- [Phase 1 Complete Guide](docs/PHASE1_COMPLETE_GUIDE.md) - Implementation details
- **Features**: Address parser, normalizer, spatial indexing, confidence scoring

### Phase 2 - Performance & Scalability (PLANNED 📋)
- [Phase 2 Implementation Plan](docs/PHASE2_IMPLEMENTATION_PLAN.md) - Detailed roadmap
- [Phase 2 Complete Guide](docs/PHASE2_COMPLETE_GUIDE.md) - Implementation guide
- **Features**: Fuzzy matching, caching, batch geocoding (code ready, temporarily reverted)

## Technology Stack

- **Language**: C++17
- **GUI Framework**: Qt 5/6 (Widgets, Network)
- **Build System**: CMake
- **Threading**: POSIX threads
- **IPC**: Unix domain sockets / Named pipes

## System Requirements