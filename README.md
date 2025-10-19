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
Automotive/
├── common/          # Shared libraries and utilities
│   ├── include/     # Common headers (nav_types.h, nav_utils.h)
│   └── src/         # Common implementations
├── hmi/             # Human-Machine Interface (GUI)
│   ├── controllers/ # Navigation controllers
│   ├── models/      # Data models
│   ├── services/    # Core services (positioning, routing, map, POI, geocoding)
│   ├── ui/          # Qt UI components
│   └── resources/   # Icons and assets
├── config/          # Configuration files
├── docs/            # Documentation (guides, API docs)
├── scripts/         # Build and deployment scripts
└── tests/           # Unit and integration tests
```

**Note:** Build artifacts are created outside the repository:
- `../Automotive-build/` - Build directory
- `../Automotive-install/` - Installation directory
- `../Automotive-packages/` - Distribution packages

## Key Components

### Services
- **Positioning Service** - GPS/GNSS integration with CAN bus fusion
- **Map Service** - Map data management and rendering
- **Routing Service** - A* pathfinding and route optimization
- **Guidance Service** - Turn-by-turn navigation instructions
- **POI Service** - Point of Interest search and geocoding
- **Geocoding Service** - Address parsing, normalization, and reverse geocoding

### HMI Features
- Interactive map widget with touch support
- Real-time vehicle position display
- Route visualization
- Navigation status panel
- Qt-based modern UI

## Build Instructions

### Quick Start

**Windows (Recommended):**
```powershell
# From repository root
.\scripts\build.bat
```

**Linux/macOS:**
```bash
# From repository root
./scripts/build.sh
```

**Output Locations:**
- Build artifacts: `../Automotive-build/`
- Executable: `../Automotive-install/bin/nav_hmi_gui.exe`

For detailed build options, see [`docs/BUILD_GUIDE.md`](docs/BUILD_GUIDE.md)

### Prerequisites
- CMake 3.16+
- Qt 6.6.1+ (msvc2019_64 for Windows)
- C++17 compatible compiler (GCC 7+, MSVC 2019+, Clang 7+)
- Google Test (for unit tests)

### Manual Build (Advanced)

#### Linux
```bash
mkdir ../Automotive-build && cd ../Automotive-build
cmake ../Automotive
make -j$(nproc)
make install
```

#### Windows
```powershell
mkdir ..\Automotive-build
cd ..\Automotive-build
cmake -G "Visual Studio 17 2022" ..\Automotive
cmake --build . --config Release
cmake --install . --config Release
```

#### Cross-compilation for QNX
```bash
mkdir ../Automotive-build-qnx
cd ../Automotive-build-qnx
cmake -DCMAKE_TOOLCHAIN_FILE=../Automotive/qnx.cmake ../Automotive
make -j$(nproc)
```

## Quick Start

### Run the Application

**Windows:**
```powershell
.\scripts\run_integrated_nav.bat
```

**Linux/macOS:**
```bash
./scripts/run_integrated_nav.sh
```

**Or run directly:**
```bash
..\Automotive-install\bin\nav_hmi_gui.exe
```

### Rebuild and Run

```powershell
.\scripts\rebuild_and_run.bat
```

### Console Mode
```bash
..\Automotive-install\bin\nav_hmi_gui.exe --console
```

## Configuration

Edit [`config/navigation.conf`](config/navigation.conf) to customize:
- Map data paths
- GPS/CAN bus settings
- Service ports and timeouts
- UI preferences

## Testing

### Build with Tests
```powershell
# Enable tests during CMake configuration
cd ..\Automotive-build
cmake -DBUILD_TESTS=ON ..\Automotive
cmake --build . --config Release
```

### Run Tests
```powershell
# Run all tests
cd ..\Automotive-build
ctest --output-on-failure

# Run specific test
.\tests\Release\test_address_parser.exe
```

### Available Tests
- `test_address_parser` - Address parsing and normalization
- `test_address_normalizer` - Address standardization
- `test_spatial_index` - Spatial indexing (R-tree)
- More tests coming in Phase 2...

## 📦 Packaging

Create distribution packages:

```powershell
# Create package
.\scripts\create_package.bat

# Output: ..\Automotive-packages\AutomotiveNav_v1.0_YYYYMMDD.zip
```

**Manual packaging:**

```bash
# Linux (DEB/RPM)
cd ../Automotive-build
cpack -G DEB
cpack -G RPM

# Windows (NSIS installer)
cpack -G NSIS

# Source package
cpack --config CPackSourceConfig.cmake
```

## 📖 Documentation

### Core Documentation
- [`README.md`](README.md) - This file (project overview)
- [`docs/BUILD_GUIDE.md`](docs/BUILD_GUIDE.md) - Detailed build instructions
- [`docs/BUILD_SCRIPTS_GUIDE.md`](docs/BUILD_SCRIPTS_GUIDE.md) - Script usage guide
- [`scripts/README.md`](scripts/README.md) - Build scripts reference

### Architecture & Design
- [`docs/SERVICE_INTEGRATION.md`](docs/SERVICE_INTEGRATION.md) - Service architecture details
- [`docs/QT_FRAMEWORK_INTEGRATION.md`](docs/QT_FRAMEWORK_INTEGRATION.md) - Qt/GUI implementation
- [`docs/ENHANCED_HMI_FEATURES.md`](docs/ENHANCED_HMI_FEATURES.md) - UI components guide

### Phase 1: Enhanced Geocoding
- [`docs/GEOCODER_MODERNIZATION_ANALYSIS.md`](docs/GEOCODER_MODERNIZATION_ANALYSIS.md) - Modernization analysis
- [`docs/PHASE1_COMPLETE_GUIDE.md`](docs/PHASE1_COMPLETE_GUIDE.md) - Phase 1 implementation guide
  - Address parsing and normalization
  - Spatial indexing (R-tree)
  - Enhanced geocoding API
  - 40+ unit tests

### Deployment
- [`scripts/deploy.config`](scripts/deploy.config) - Deployment configuration
- [`docs/ICON_REMOVAL_SUMMARY.md`](docs/ICON_REMOVAL_SUMMARY.md) - Icon optimization notes

## Technology Stack

- **Language**: C++17
- **GUI Framework**: Qt 5/6 (Widgets, Network)
- **Build System**: CMake
- **Threading**: POSIX threads
- **IPC**: Unix domain sockets / Named pipes

## System Requirements