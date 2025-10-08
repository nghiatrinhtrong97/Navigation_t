# Qt Framework Integration for Navigation System

## Overview
This document explains the Qt-based communication architecture between HMI and backend services, replacing Unix domain sockets with Qt's cross-platform networking components.

## Why Qt Framework Integration?

### Problems with Original Approach
1. **Unix Sockets**: Not cross-platform (Linux/QNX only)
2. **Raw Socket Programming**: Complex error handling and threading
3. **Manual Threading**: std::thread doesn't integrate well with Qt event loop
4. **Platform Dependencies**: sys/socket.h, unistd.h not available on Windows

### Qt Framework Benefits
1. **Cross-Platform**: Works on Linux, Windows, macOS, QNX
2. **QLocalSocket/QLocalServer**: Qt's cross-platform IPC mechanism
3. **Qt Event Loop**: Seamless integration with GUI event handling
4. **Qt Signals/Slots**: Type-safe callback system
5. **Automatic Threading**: Qt handles thread management internally
6. **Built-in Reconnection**: QTimer-based automatic reconnection

## Architecture Comparison

### Before (Unix Sockets)
```
HMI ←→ Unix Domain Socket ←→ Service
     (/tmp/nav_*.sock)
```

### After (Qt Framework)
```
HMI ←→ QLocalSocket ←→ QLocalServer ←→ Service
     (Named Pipe/Local Socket)
```

## Qt Components Used

### Client Side (HMI)
- **QLocalSocket**: Cross-platform local IPC client
- **QDataStream**: Structured data serialization
- **QTimer**: Automatic reconnection timing
- **Qt Signals/Slots**: Event-driven communication

### Server Side (Services)
- **QLocalServer**: Cross-platform local IPC server
- **QCoreApplication**: Event loop for services
- **Qt's MOC**: Meta-Object Compiler for signals/slots

## Message Protocol

### Qt-Based Message Format
```cpp
// Message structure
QByteArray data;
QDataStream stream(&data, QIODevice::WriteOnly);

// Header: Message type as QString
stream << QString("MESSAGE_TYPE");

// Payload: Binary struct data
stream.writeRawData(reinterpret_cast<const char*>(&request), sizeof(request));

// Send via QLocalSocket
socket->write(data);
```

### Message Types
- **START_POSITIONING** / **POSITION_UPDATE**
- **CALCULATE_ROUTE** / **ROUTE_CALCULATED**  
- **START_GUIDANCE** / **GUIDANCE_UPDATE**
- **MAP_DATA_REQUEST** / **MAP_DATA_RESPONSE**

## Service Connection Names

### Qt Local Server Names
Instead of file paths, Qt uses logical names:
- Positioning: `"nav_positioning"`
- Routing: `"nav_routing"`
- Guidance: `"nav_guidance"`
- Map: `"nav_map"`

### Cross-Platform Mapping
- **Linux/QNX**: Unix domain sockets in /tmp/
- **Windows**: Named pipes (\\.\pipe\nav_positioning)
- **macOS**: Unix domain sockets in temporary directory

## Implementation Details

### Service Client (Qt-based)
```cpp
class PositioningServiceClient : public QObject, public IPositioningService {
    Q_OBJECT
    
private:
    QLocalSocket* m_socket;
    QTimer* m_reconnectTimer;
    
public slots:
    void onConnected();
    void onDisconnected();
    void onDataReceived();
    void attemptReconnect();
};
```

### Service Server (Qt-based)
```cpp
class QtPositioningService : public QObject {
    Q_OBJECT
    
private:
    QLocalServer* m_server;
    QList<QLocalSocket*> m_clients;
    QTimer* m_updateTimer;
    
public slots:
    void onNewConnection();
    void onClientDataReceived();
    void sendPositionUpdates();
};
```

### Automatic Reconnection
```cpp
void onDisconnected() {
    m_connected = false;
    
    // Start reconnection timer (5 seconds)
    if (!m_reconnectTimer->isActive()) {
        m_reconnectTimer->start(5000);
    }
}

void attemptReconnect() {
    qDebug() << "Attempting to reconnect...";
    m_socket->connectToServer(serverName);
}
```

## Qt Event Integration

### Benefits of Qt Event Loop
1. **Non-blocking**: All I/O operations are asynchronous
2. **Thread-Safe**: Qt handles thread synchronization
3. **Integrated**: Works seamlessly with GUI event loop
4. **Efficient**: Uses platform-optimal I/O mechanisms

### Signal/Slot Connections
```cpp
// Connect Qt signals to slots
connect(m_socket, &QLocalSocket::connected, 
        this, &ServiceClient::onConnected);
connect(m_socket, &QLocalSocket::readyRead,
        this, &ServiceClient::onDataReceived);
connect(m_socket, &QLocalSocket::disconnected,
        this, &ServiceClient::onDisconnected);
```

## Cross-Platform Compatibility

### Build Configuration
```cmake
find_package(Qt5 COMPONENTS Core Widgets Network QUIET)

target_link_libraries(nav_hmi_gui
    nav_common
    Qt5::Core
    Qt5::Widgets
    Qt5::Network  # Qt networking components
)
```

### Platform-Specific Notes

#### Linux/QNX
- Uses Unix domain sockets internally
- High performance local IPC
- Full Qt integration

#### Windows  
- Uses named pipes internally
- Cross-process communication
- Same Qt API across platforms

#### macOS
- Unix domain sockets in temp directory
- Native macOS IPC integration

## Service Discovery

### Qt-Based Discovery
```cpp
bool ServiceFactory::discoverServices() {
    QLocalSocket testSocket;
    
    // Test each service
    testSocket.connectToServer("nav_positioning");
    bool positioningAvailable = testSocket.waitForConnected(1000);
    testSocket.disconnectFromServer();
    
    // ... test other services
    
    return allServicesAvailable;
}
```

### Advantages
- **Fast**: Connection test is quick
- **Reliable**: Qt handles platform differences
- **Non-blocking**: Can be done asynchronously

## Error Handling

### Qt Error Management
```cpp
void onError(QLocalSocket::LocalSocketError error) {
    switch (error) {
        case QLocalSocket::ServerNotFoundError:
            qDebug() << "Service not available";
            break;
        case QLocalSocket::ConnectionRefusedError:
            qDebug() << "Service refused connection";
            break;
        case QLocalSocket::PeerClosedError:
            qDebug() << "Service closed connection";
            break;
        default:
            qDebug() << "Service error:" << m_socket->errorString();
    }
    
    // Trigger reconnection
    attemptReconnect();
}
```

### Graceful Degradation
- **Service Unavailable**: Fall back to simulation mode
- **Connection Lost**: Automatic reconnection attempts
- **Partial Failure**: Individual service recovery

## Performance Characteristics

### Qt vs Raw Sockets
- **Throughput**: Comparable (Qt uses optimal OS mechanisms)
- **Latency**: Slightly higher (Qt event loop overhead)
- **Memory**: Lower (Qt handles buffer management)
- **CPU**: Lower (efficient event-driven I/O)

### Optimization Tips
1. **Batch Messages**: Combine multiple small messages
2. **Buffer Management**: Let Qt handle buffering
3. **Connection Pooling**: Reuse connections when possible
4. **Async Processing**: Use Qt's async capabilities

## Development Workflow

### Building Qt Version
```bash
# Build with Qt support
mkdir build && cd build
cmake .. -DQt5_DIR=/path/to/qt5
make

# Qt services will be built as nav_*_qt
./services/positioning/nav_positioning_qt
```

### Testing Qt Integration
```bash
# Start Qt positioning service
./services/positioning/nav_positioning_qt &

# Start Qt GUI (will auto-detect Qt services)
./hmi/nav_hmi_gui
```

### Debugging Qt Communication
```cpp
// Enable Qt logging
QLoggingCategory::setFilterRules("qt.network.ssl.debug=true");

// Debug service connections
qDebug() << "Service status:" << socket->state();
qDebug() << "Error string:" << socket->errorString();
```

## Migration Benefits

### For Developers
1. **Single Framework**: Consistent Qt APIs throughout
2. **Cross-Platform**: Same code works on all platforms
3. **Integrated Debugging**: Qt Creator debugging support
4. **Documentation**: Extensive Qt documentation available

### For Deployment
1. **Windows Support**: Now works on Windows development machines
2. **Docker**: Easier containerized deployment
3. **Testing**: Cross-platform testing capabilities
4. **Maintenance**: Single codebase for all platforms

## Future Enhancements

### Planned Qt Features
1. **QRemoteObjectRegistry**: For distributed services
2. **QWebSockets**: For web-based clients
3. **QSslSocket**: For secure communication
4. **QNetworkAccessManager**: For HTTP-based services

### Advanced Qt Integration
1. **QML Integration**: Expose services to QML
2. **Qt Quick**: Modern UI with Qt Quick
3. **Qt Charts**: Real-time data visualization
4. **Qt Positioning**: GPS integration with Qt Location

This Qt integration provides a robust, cross-platform foundation while maintaining the same functionality and performance characteristics of the original system.