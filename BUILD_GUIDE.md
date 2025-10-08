# Automotive Navigation System - Build & Deployment Guide

## System Overview

This is a complete embedded navigation system designed for automotive IVI (In-Vehicle Infotainment) platforms, specifically targeting QNX Real-Time Operating System. The system consists of multiple cooperating services:

- **PositioningService**: GPS/GNSS data processing, CAN bus integration, dead reckoning
- **MapService**: Map data management, tile loading, spatial indexing
- **RoutingService**: A* pathfinding algorithm, route calculation
- **GuidanceService**: Turn-by-turn navigation, route monitoring
- **HMI Application**: User interface (console or Qt-based)

## Prerequisites

### For QNX Target System:
- QNX Neutrino RTOS 7.x
- QNX Software Development Platform (SDP)
- GCC toolchain for QNX
- CMake 3.16 or later

### For Development/Simulation:
- Linux, macOS, or Windows
- GCC 7+ or MSVC 2019+
- CMake 3.16 or later
- Qt5 (optional, for GUI HMI)

## Building the System

### Quick Build (Linux/macOS):
```bash
# Make build script executable
chmod +x build.sh

# Build in Release mode
./build.sh Release

# Build in Debug mode
./build.sh Debug

# Clean build
./build.sh Release clean
```

### Quick Build (Windows):
```batch
# Build in Release mode
build.bat Release

# Build in Debug mode  
build.bat Debug

# Clean build
build.bat Release clean
```

### Manual Build:
```bash
# Create build directory
mkdir build && cd build

# Configure for native development
cmake .. -DCMAKE_BUILD_TYPE=Release

# Configure for QNX (if QNX toolchain is available)
cmake .. -DCMAKE_BUILD_TYPE=Release -DQNX=ON

# Build
make -j$(nproc)

# Install
make install
```

## Deployment

### Directory Structure After Build:
```
install/
├── bin/
│   ├── positioning_service
│   ├── map_service
│   ├── routing_service
│   ├── guidance_service
│   └── nav_hmi
├── lib/
│   └── libnav_common.a
└── include/
    ├── nav_types.h
    ├── nav_messages.h
    └── nav_utils.h
```

### QNX Target Deployment:
1. Copy the `install/bin/` directory to the target QNX system
2. Ensure proper device permissions for `/dev/ser1` (GPS) and CAN interface
3. Create map data directory: `/opt/nav/data/`
4. Copy map data files to the target system
5. Set up system startup scripts in `/etc/rc.d/`

### Configuration Files:
- `config/navigation.conf` - Main system configuration
- Modify paths and hardware settings as needed for your target system

## Running the System

### Automatic Startup:
```bash
# Linux/macOS
chmod +x start_system.sh
./start_system.sh

# Windows
start_system.bat
```

### Manual Service Startup:
```bash
# Start services in order
./install/bin/positioning_service --gps /dev/ser1 --can can0 &
./install/bin/map_service --data /opt/nav/data/map.data &
./install/bin/routing_service &
./install/bin/guidance_service &

# Start HMI
./install/bin/nav_hmi
```

### Service Command Line Options:

**positioning_service:**
- `--gps <device>`: GPS serial device (default: /dev/ser1)
- `--can <device>`: CAN interface device (default: can0)

**map_service:**
- `--data <path>`: Map data file path (default: /opt/nav/data/map.data)

**routing_service:**
- `--map-channel <id>`: Map service IPC channel ID

**guidance_service:**
- `--positioning-channel <id>`: Positioning service IPC channel ID
- `--routing-channel <id>`: Routing service IPC channel ID

## System Configuration

### Hardware Setup:
1. **GPS Receiver**: Connect to serial port (default: /dev/ser1, 9600 baud)
2. **CAN Bus**: Configure CAN interface (default: can0, 500kbps)
3. **Display**: Connect display for HMI application

### Map Data:
- The system expects map data in a custom binary format
- For development, dummy map data is generated automatically
- Production systems require real map data tiles

### CAN Bus Configuration:
The system expects specific CAN message IDs:
- `0x200`: Vehicle speed data
- `0x201`: Yaw rate data  
- `0x300`: Guidance output to instrument cluster

Modify `can_interface.cpp` for your vehicle's CAN protocol.

## Development

### Adding New Features:
1. Extend message types in `nav_messages.h`
2. Add new service logic in respective service classes
3. Update IPC handling for new message types
4. Rebuild system

### Debugging:
- Enable debug builds: `cmake .. -DCMAKE_BUILD_TYPE=Debug`
- Use GDB for debugging: `gdb ./install/bin/positioning_service`
- Check log files in `logs/` directory
- Monitor IPC communication between services

### Testing:
```bash
# Build with tests enabled
cmake .. -DBUILD_TESTS=ON
make
ctest
```

## Performance Optimization

### For Embedded Targets:
- Use Release builds: `-DCMAKE_BUILD_TYPE=Release`
- Optimize compiler flags: `-O2 -DNDEBUG`
- Adjust tile cache sizes based on available RAM
- Configure service priorities in system configuration

### Memory Usage:
- Map tile cache: ~10MB per 100 tiles
- Route calculations: ~1-5MB peak usage
- Total system: ~50-100MB depending on map data

## Troubleshooting

### Common Issues:

**GPS not working:**
- Check device permissions: `sudo chmod 666 /dev/ser1`
- Verify baud rate settings (9600 for most GPS modules)
- Check NMEA sentence format compatibility

**CAN bus errors:**
- Verify CAN interface is up: `ip link show can0`
- Check bitrate: `ip link set can0 type can bitrate 500000`
- Ensure proper termination resistors on CAN bus

**Service startup failures:**
- Check IPC channel creation (QNX specific)
- Verify file permissions and paths
- Monitor system resources (memory, file descriptors)

**Route calculation slow:**
- Reduce search radius in configuration
- Optimize map data structure
- Check available memory for map cache

### Log Analysis:
```bash
# Monitor service logs
tail -f logs/positioning.log
tail -f logs/routing.log

# Check system resources
top -p $(pgrep -d, "positioning_service|map_service|routing_service|guidance_service")
```

## License

This project is designed for automotive industry use. Ensure compliance with automotive safety standards (ISO 26262) for production deployment.

## Support

For technical support and customization for specific vehicle platforms, contact the development team.