# Service Integration Architecture

## Overview
This document describes the complete integration between the Qt GUI application and the backend navigation services (positioning, routing, guidance, map).

## Architecture Components

### 1. Service Interface Layer (`service_interfaces.h`)
Abstract interfaces defining communication contracts:
- **IPositioningService**: GPS position tracking and updates
- **IRoutingService**: Route calculation and optimization  
- **IGuidanceService**: Turn-by-turn navigation guidance
- **IMapService**: Map data and tile management

### 2. Service Clients (`service_clients.cpp`)
IPC-based implementations using Unix domain sockets:
- **PositioningServiceClient**: Connects to `/tmp/nav_positioning.sock`
- **RoutingServiceClient**: Connects to `/tmp/nav_routing.sock`
- **GuidanceServiceClient**: Connects to `/tmp/nav_guidance.sock`
- **MapServiceClient**: Connects to `/tmp/nav_map.sock`

### 3. Service Manager
Centralized service discovery and lifecycle management:
- Automatic service detection
- Connection monitoring and reconnection
- Health checks and status reporting

### 4. Navigation Controller V2
Enhanced controller with dual operation modes:
- **SERVICE_MODE**: Uses real backend services via IPC
- **SIMULATION_MODE**: Fallback to internal simulation

## Communication Flow

### Service Mode Operation
```
GUI Application ←→ NavigationController ←→ ServiceManager ←→ Service Clients ←→ Backend Services
```

### Simulation Mode Fallback
```
GUI Application ←→ NavigationController ←→ Internal Simulation
```

## Service Discovery Process

### 1. Initialization
```cpp
ServiceManager manager;
if (manager.initialize()) {
    // Services available
    controller.setOperationMode(SERVICE_MODE);
} else {
    // Fallback to simulation
    controller.setOperationMode(SIMULATION_MODE);
}
```

### 2. Socket Detection
Checks for service socket files:
- `/tmp/nav_positioning.sock`
- `/tmp/nav_routing.sock`
- `/tmp/nav_guidance.sock`
- `/tmp/nav_map.sock`

### 3. Connection Establishment
Each service client attempts TCP connection:
```cpp
bool PositioningServiceClient::connect() {
    m_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    struct sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, "/tmp/nav_positioning.sock");
    return ::connect(m_socket, (struct sockaddr*)&addr, sizeof(addr)) == 0;
}
```

## Message Protocol

### Positioning Service
```cpp
// Request types
enum RequestType { START, STOP, SET_POSITION };

// Response format
struct PositioningResponse {
    Point position;
    double heading;
    double speed;
};
```

### Routing Service
```cpp
// Request format
struct RoutingRequest {
    RequestType request_type;
    Point start_point;
    Point end_point;
    AlgorithmType algorithm_type;
};

// Response format
struct RoutingResponse {
    StatusType status;
    Route route;
};
```

### Guidance Service
```cpp
// Request format
struct GuidanceRequest {
    RequestType request_type;
    Route route;
    Point current_position;
};

// Response format
struct GuidanceResponse {
    ResponseType response_type;
    GuidanceInstruction instruction;
    double deviation_distance;
};
```

## GUI Integration Features

### 1. Service Control Panel
New tab in main window provides:
- **Mode Display**: Current operation mode (Service/Simulation)
- **Toggle Button**: Switch between modes
- **Reconnect Button**: Attempt service reconnection
- **Status Display**: Real-time service connection status

### 2. Automatic Fallback
When services are unavailable:
- GUI automatically switches to simulation mode
- User is notified of service status
- Full functionality maintained with simulated data

### 3. Real-time Monitoring
- Continuous service health checks
- Connection status updates in UI
- Automatic reconnection attempts

## Usage Scenarios

### Scenario 1: Full Service Mode
1. All backend services running
2. GUI detects services and switches to SERVICE_MODE
3. Real route calculation via routing service
4. Real-time position updates from positioning service
5. Turn-by-turn guidance from guidance service

### Scenario 2: Partial Service Availability
1. Some services unavailable
2. GUI shows warning and available services
3. Falls back to simulation for missing services
4. User can manually retry connection

### Scenario 3: Simulation Only
1. No backend services available
2. GUI operates in pure simulation mode
3. All functionality works with generated data
4. User can switch to service mode when services become available

## Startup Procedures

### Complete System Startup
```bash
./start_full_system.sh
```
1. Builds entire navigation system
2. Starts all backend services in sequence
3. Waits for service initialization
4. Launches GUI application

### GUI Only (Simulation Mode)
```bash
./start_gui.sh
```
1. Builds GUI application only
2. Starts in simulation mode
3. Services can be connected later

## Configuration

### Service Socket Paths
Default Unix domain socket locations:
- Positioning: `/tmp/nav_positioning.sock`
- Routing: `/tmp/nav_routing.sock`
- Guidance: `/tmp/nav_guidance.sock`
- Map: `/tmp/nav_map.sock`

### Connection Timeouts
- Initial connection: 5 seconds
- Message timeout: 10 seconds
- Reconnection interval: 30 seconds

### Update Frequencies
- Position updates: 10 Hz (100ms)
- Guidance updates: 2 Hz (500ms)
- Service health checks: 1 Hz (1000ms)

## Error Handling

### Connection Failures
- Automatic retry with exponential backoff
- User notification of service status
- Graceful degradation to simulation mode

### Message Errors
- Message validation and error recovery
- Corrupt message detection and handling
- Service restart capability

### Service Crashes
- Detection of service disconnection
- Automatic cleanup of resources
- Notification to user and retry mechanism

## Development and Testing

### Service Mocking
For development without full backend:
```cpp
// Create mock service for testing
class MockPositioningService : public IPositioningService {
    // Implement mock behavior
};
```

### Integration Testing
Test scenarios:
1. Service startup/shutdown
2. Network interruption simulation
3. Service crash recovery
4. Message protocol validation

### Performance Monitoring
Metrics tracked:
- Message round-trip times
- Connection establishment time
- Service response rates
- Memory usage per service

## Future Enhancements

### Planned Features
1. **Service Load Balancing**: Multiple service instances
2. **Network Services**: TCP/IP support for distributed deployment
3. **Security**: Authentication and encryption for service communication
4. **Configuration Management**: Dynamic service endpoint configuration
5. **Logging Integration**: Centralized logging across services

### Scalability Considerations
- Service mesh architecture
- Container deployment support
- Cloud service integration
- Real-time telemetry and monitoring

This integration provides a robust, scalable foundation for the automotive navigation system with seamless fallback capabilities and real-time service monitoring.