#include "../include/service_interfaces.h"
#include "nav_utils.h"
#include <QLocalSocket>
#include <QLocalServer>
#include <QTimer>
#include <QThread>
#include <QDataStream>
#include <QDebug>
#include <QCoreApplication>

namespace nav {

/**
 * @brief Qt-based positioning service client using QLocalSocket
 */
class PositioningServiceClient : public QObject, public IPositioningService {
    Q_OBJECT

private:
    QLocalSocket* m_socket;
    bool m_connected;
    bool m_active;
    PositionCallback m_positionCallback;
    Point m_lastPosition;
    QTimer* m_reconnectTimer;

public:
    PositioningServiceClient(QObject* parent = nullptr) 
        : QObject(parent)
        , m_socket(nullptr)
        , m_connected(false)
        , m_active(false)
        , m_reconnectTimer(nullptr)
    {
        m_socket = new QLocalSocket(this);
        m_reconnectTimer = new QTimer(this);
        
        // Connect Qt signals
        connect(m_socket, &QLocalSocket::connected, this, &PositioningServiceClient::onConnected);
        connect(m_socket, &QLocalSocket::disconnected, this, &PositioningServiceClient::onDisconnected);
        connect(m_socket, &QLocalSocket::readyRead, this, &PositioningServiceClient::onDataReceived);
        connect(m_socket, QOverload<QLocalSocket::LocalSocketError>::of(&QLocalSocket::error),
                this, &PositioningServiceClient::onError);
        
        // Setup reconnect timer
        connect(m_reconnectTimer, &QTimer::timeout, this, &PositioningServiceClient::attemptReconnect);
        m_reconnectTimer->setSingleShot(true);
    }
    
    ~PositioningServiceClient() {
        disconnect();
    }
    
    bool connect() {
        if (m_connected) {
            return true;
        }
        
        QString serverName = "nav_positioning";
        qDebug() << "Connecting to positioning service:" << serverName;
        
        m_socket->connectToServer(serverName);
        
        // Wait for connection (with timeout)
        if (m_socket->waitForConnected(3000)) {
            return true;
        } else {
            qDebug() << "Failed to connect to positioning service:" << m_socket->errorString();
            return false;
        }
    }
    
    void disconnect() {
        if (m_socket && m_socket->state() != QLocalSocket::UnconnectedState) {
            m_socket->disconnectFromServer();
            m_socket->waitForDisconnected(1000);
        }
        m_connected = false;
        m_active = false;
        
        if (m_reconnectTimer) {
            m_reconnectTimer->stop();
        }
    }
    
    bool startPositioning() override {
        if (!m_connected && !connect()) {
            return false;
        }
        
        // Send start command
        QByteArray data;
        QDataStream stream(&data, QIODevice::WriteOnly);
        
        PositioningRequest request;
        request.request_type = PositioningRequest::START;
        
        stream << QString("START_POSITIONING");
        stream.writeRawData(reinterpret_cast<const char*>(&request), sizeof(request));
        
        if (m_socket->write(data) == -1) {
            qDebug() << "Failed to send start positioning request";
            return false;
        }
        
        m_active = true;
        qDebug() << "Positioning started";
        return true;
    }
    
    bool stopPositioning() override {
        if (!m_connected) return false;
        
        QByteArray data;
        QDataStream stream(&data, QIODevice::WriteOnly);
        
        PositioningRequest request;
        request.request_type = PositioningRequest::STOP;
        
        stream << QString("STOP_POSITIONING");
        stream.writeRawData(reinterpret_cast<const char*>(&request), sizeof(request));
        
        if (m_socket->write(data) == -1) {
            qDebug() << "Failed to send stop positioning request";
            return false;
        }
        
        m_active = false;
        qDebug() << "Positioning stopped";
        return true;
    }
    
    Point getCurrentPosition() override {
        return m_lastPosition;
    }
    
    bool setPosition(const Point& position) override {
        if (!m_connected) return false;
        
        QByteArray data;
        QDataStream stream(&data, QIODevice::WriteOnly);
        
        PositioningRequest request;
        request.request_type = PositioningRequest::SET_POSITION;
        request.position = position;
        
        stream << QString("SET_POSITION");
        stream.writeRawData(reinterpret_cast<const char*>(&request), sizeof(request));
        
        if (m_socket->write(data) == -1) {
            qDebug() << "Failed to send set position request";
            return false;
        }
        
        m_lastPosition = position;
        return true;
    }
    
    void setPositionCallback(PositionCallback callback) override {
        m_positionCallback = callback;
    }
    
    bool isConnected() override {
        return m_connected;
    }
    
    bool isActive() override {
        return m_active;
    }

private slots:
    void onConnected() {
        m_connected = true;
        qDebug() << "✓ Connected to positioning service";
        
        if (m_reconnectTimer->isActive()) {
            m_reconnectTimer->stop();
        }
    }
    
    void onDisconnected() {
        m_connected = false;
        m_active = false;
        qDebug() << "✗ Disconnected from positioning service";
        
        // Start reconnection timer
        if (!m_reconnectTimer->isActive()) {
            m_reconnectTimer->start(5000); // Retry in 5 seconds
        }
    }
    
    void onDataReceived() {
        while (m_socket->bytesAvailable()) {
            QByteArray data = m_socket->readAll();
            QDataStream stream(&data, QIODevice::ReadOnly);
            
            QString messageType;
            stream >> messageType;
            
            if (messageType == "POSITION_UPDATE") {
                PositioningResponse response;
                stream.readRawData(reinterpret_cast<char*>(&response), sizeof(response));
                
                m_lastPosition = response.position;
                
                if (m_positionCallback) {
                    m_positionCallback(response.position, response.heading, response.speed);
                }
            }
        }
    }
    
    void onError(QLocalSocket::LocalSocketError error) {
        qDebug() << "Positioning service error:" << m_socket->errorString();
        m_connected = false;
        m_active = false;
    }
    
    void attemptReconnect() {
        qDebug() << "Attempting to reconnect to positioning service...";
        connect();
    }
};

/**
 * @brief Qt-based routing service client
 */
class RoutingServiceClient : public QObject, public IRoutingService {
    Q_OBJECT

private:
    QLocalSocket* m_socket;
    bool m_connected;
    bool m_calculating;
    RouteCallback m_routeCallback;
    RouteErrorCallback m_errorCallback;
    Route m_lastRoute;
    QTimer* m_reconnectTimer;

public:
    RoutingServiceClient(QObject* parent = nullptr)
        : QObject(parent)
        , m_socket(nullptr)
        , m_connected(false)
        , m_calculating(false)
        , m_reconnectTimer(nullptr)
    {
        m_socket = new QLocalSocket(this);
        m_reconnectTimer = new QTimer(this);
        
        connect(m_socket, &QLocalSocket::connected, this, &RoutingServiceClient::onConnected);
        connect(m_socket, &QLocalSocket::disconnected, this, &RoutingServiceClient::onDisconnected);
        connect(m_socket, &QLocalSocket::readyRead, this, &RoutingServiceClient::onDataReceived);
        connect(m_socket, QOverload<QLocalSocket::LocalSocketError>::of(&QLocalSocket::error),
                this, &RoutingServiceClient::onError);
        
        connect(m_reconnectTimer, &QTimer::timeout, this, &RoutingServiceClient::attemptReconnect);
        m_reconnectTimer->setSingleShot(true);
    }
    
    ~RoutingServiceClient() {
        disconnect();
    }
    
    bool connect() {
        if (m_connected) return true;
        
        QString serverName = "nav_routing";
        qDebug() << "Connecting to routing service:" << serverName;
        
        m_socket->connectToServer(serverName);
        return m_socket->waitForConnected(3000);
    }
    
    void disconnect() {
        if (m_socket && m_socket->state() != QLocalSocket::UnconnectedState) {
            m_socket->disconnectFromServer();
            m_socket->waitForDisconnected(1000);
        }
        m_connected = false;
        m_calculating = false;
        
        if (m_reconnectTimer) {
            m_reconnectTimer->stop();
        }
    }
    
    bool calculateRoute(const RoutingRequest& request) override {
        if (!m_connected && !connect()) {
            return false;
        }
        
        QByteArray data;
        QDataStream stream(&data, QIODevice::WriteOnly);
        
        stream << QString("CALCULATE_ROUTE");
        stream.writeRawData(reinterpret_cast<const char*>(&request), sizeof(request));
        
        if (m_socket->write(data) == -1) {
            qDebug() << "Failed to send routing request";
            return false;
        }
        
        m_calculating = true;
        qDebug() << "Route calculation requested";
        return true;
    }
    
    bool cancelRouteCalculation() override {
        if (!m_connected) return false;
        
        QByteArray data;
        QDataStream stream(&data, QIODevice::WriteOnly);
        
        RoutingRequest request;
        request.request_type = RoutingRequest::CANCEL;
        
        stream << QString("CANCEL_ROUTE");
        stream.writeRawData(reinterpret_cast<const char*>(&request), sizeof(request));
        
        if (m_socket->write(data) == -1) {
            qDebug() << "Failed to send cancel request";
            return false;
        }
        
        m_calculating = false;
        return true;
    }
    
    Route getLastCalculatedRoute() override {
        return m_lastRoute;
    }
    
    void setRouteCallback(RouteCallback callback) override {
        m_routeCallback = callback;
    }
    
    void setRouteErrorCallback(RouteErrorCallback callback) override {
        m_errorCallback = callback;
    }
    
    bool isConnected() override {
        return m_connected;
    }
    
    bool isCalculating() override {
        return m_calculating;
    }

private slots:
    void onConnected() {
        m_connected = true;
        qDebug() << "✓ Connected to routing service";
        
        if (m_reconnectTimer->isActive()) {
            m_reconnectTimer->stop();
        }
    }
    
    void onDisconnected() {
        m_connected = false;
        m_calculating = false;
        qDebug() << "✗ Disconnected from routing service";
        
        if (!m_reconnectTimer->isActive()) {
            m_reconnectTimer->start(5000);
        }
    }
    
    void onDataReceived() {
        while (m_socket->bytesAvailable()) {
            QByteArray data = m_socket->readAll();
            QDataStream stream(&data, QIODevice::ReadOnly);
            
            QString messageType;
            stream >> messageType;
            
            if (messageType == "ROUTE_CALCULATED") {
                RoutingResponse response;
                stream.readRawData(reinterpret_cast<char*>(&response), sizeof(response));
                
                m_calculating = false;
                
                if (response.status == RoutingResponse::SUCCESS) {
                    m_lastRoute = response.route;
                    if (m_routeCallback) {
                        m_routeCallback(response.route);
                    }
                } else {
                    if (m_errorCallback) {
                        m_errorCallback("Route calculation failed");
                    }
                }
            }
        }
    }
    
    void onError(QLocalSocket::LocalSocketError error) {
        qDebug() << "Routing service error:" << m_socket->errorString();
        m_connected = false;
        m_calculating = false;
    }
    
    void attemptReconnect() {
        qDebug() << "Attempting to reconnect to routing service...";
        connect();
    }
};

/**
 * @brief Qt-based guidance service client
 */
class GuidanceServiceClient : public QObject, public IGuidanceService {
    Q_OBJECT

private:
    QLocalSocket* m_socket;
    bool m_connected;
    bool m_active;
    GuidanceCallback m_guidanceCallback;
    RouteDeviationCallback m_deviationCallback;
    GuidanceInstruction m_lastInstruction;
    QTimer* m_reconnectTimer;

public:
    GuidanceServiceClient(QObject* parent = nullptr)
        : QObject(parent)
        , m_socket(nullptr)
        , m_connected(false)
        , m_active(false)
        , m_reconnectTimer(nullptr)
    {
        m_socket = new QLocalSocket(this);
        m_reconnectTimer = new QTimer(this);
        
        connect(m_socket, &QLocalSocket::connected, this, &GuidanceServiceClient::onConnected);
        connect(m_socket, &QLocalSocket::disconnected, this, &GuidanceServiceClient::onDisconnected);
        connect(m_socket, &QLocalSocket::readyRead, this, &GuidanceServiceClient::onDataReceived);
        connect(m_socket, QOverload<QLocalSocket::LocalSocketError>::of(&QLocalSocket::error),
                this, &GuidanceServiceClient::onError);
        
        connect(m_reconnectTimer, &QTimer::timeout, this, &GuidanceServiceClient::attemptReconnect);
        m_reconnectTimer->setSingleShot(true);
    }
    
    ~GuidanceServiceClient() {
        disconnect();
    }
    
    bool connect() {
        if (m_connected) return true;
        
        QString serverName = "nav_guidance";
        qDebug() << "Connecting to guidance service:" << serverName;
        
        m_socket->connectToServer(serverName);
        return m_socket->waitForConnected(3000);
    }
    
    void disconnect() {
        if (m_socket && m_socket->state() != QLocalSocket::UnconnectedState) {
            m_socket->disconnectFromServer();
            m_socket->waitForDisconnected(1000);
        }
        m_connected = false;
        m_active = false;
        
        if (m_reconnectTimer) {
            m_reconnectTimer->stop();
        }
    }
    
    bool startGuidance(const Route& route) override {
        if (!m_connected && !connect()) {
            return false;
        }
        
        QByteArray data;
        QDataStream stream(&data, QIODevice::WriteOnly);
        
        GuidanceRequest request;
        request.request_type = GuidanceRequest::START;
        request.route = route;
        
        stream << QString("START_GUIDANCE");
        stream.writeRawData(reinterpret_cast<const char*>(&request), sizeof(request));
        
        if (m_socket->write(data) == -1) {
            qDebug() << "Failed to send start guidance request";
            return false;
        }
        
        m_active = true;
        qDebug() << "Guidance started";
        return true;
    }
    
    bool stopGuidance() override {
        if (!m_connected) return false;
        
        QByteArray data;
        QDataStream stream(&data, QIODevice::WriteOnly);
        
        GuidanceRequest request;
        request.request_type = GuidanceRequest::STOP;
        
        stream << QString("STOP_GUIDANCE");
        stream.writeRawData(reinterpret_cast<const char*>(&request), sizeof(request));
        
        if (m_socket->write(data) == -1) {
            qDebug() << "Failed to send stop guidance request";
            return false;
        }
        
        m_active = false;
        qDebug() << "Guidance stopped";
        return true;
    }
    
    bool updatePosition(const Point& position) override {
        if (!m_connected || !m_active) return false;
        
        QByteArray data;
        QDataStream stream(&data, QIODevice::WriteOnly);
        
        GuidanceRequest request;
        request.request_type = GuidanceRequest::UPDATE_POSITION;
        request.current_position = position;
        
        stream << QString("UPDATE_POSITION");
        stream.writeRawData(reinterpret_cast<const char*>(&request), sizeof(request));
        
        if (m_socket->write(data) == -1) {
            qDebug() << "Failed to send position update";
            return false;
        }
        
        return true;
    }
    
    GuidanceInstruction getCurrentInstruction() override {
        return m_lastInstruction;
    }
    
    void setGuidanceCallback(GuidanceCallback callback) override {
        m_guidanceCallback = callback;
    }
    
    void setRouteDeviationCallback(RouteDeviationCallback callback) override {
        m_deviationCallback = callback;
    }
    
    bool isConnected() override {
        return m_connected;
    }
    
    bool isActive() override {
        return m_active;
    }

private slots:
    void onConnected() {
        m_connected = true;
        qDebug() << "✓ Connected to guidance service";
        
        if (m_reconnectTimer->isActive()) {
            m_reconnectTimer->stop();
        }
    }
    
    void onDisconnected() {
        m_connected = false;
        m_active = false;
        qDebug() << "✗ Disconnected from guidance service";
        
        if (!m_reconnectTimer->isActive()) {
            m_reconnectTimer->start(5000);
        }
    }
    
    void onDataReceived() {
        while (m_socket->bytesAvailable()) {
            QByteArray data = m_socket->readAll();
            QDataStream stream(&data, QIODevice::ReadOnly);
            
            QString messageType;
            stream >> messageType;
            
            if (messageType == "GUIDANCE_UPDATE") {
                GuidanceResponse response;
                stream.readRawData(reinterpret_cast<char*>(&response), sizeof(response));
                
                if (response.response_type == GuidanceResponse::INSTRUCTION) {
                    m_lastInstruction = response.instruction;
                    
                    if (m_guidanceCallback) {
                        m_guidanceCallback(response.instruction);
                    }
                } else if (response.response_type == GuidanceResponse::ROUTE_DEVIATION) {
                    if (m_deviationCallback) {
                        m_deviationCallback(response.deviation_distance);
                    }
                }
            }
        }
    }
    
    void onError(QLocalSocket::LocalSocketError error) {
        qDebug() << "Guidance service error:" << m_socket->errorString();
        m_connected = false;
        m_active = false;
    }
    
    void attemptReconnect() {
        qDebug() << "Attempting to reconnect to guidance service...";
        connect();
    }
};

/**
 * @brief Qt-based map service client (simplified for now)
 */
class MapServiceClient : public QObject, public IMapService {
    Q_OBJECT

private:
    QLocalSocket* m_socket;
    bool m_connected;
    bool m_loading;
    MapDataCallback m_mapDataCallback;
    TileCallback m_tileCallback;
    QTimer* m_reconnectTimer;

public:
    MapServiceClient(QObject* parent = nullptr)
        : QObject(parent)
        , m_socket(nullptr)
        , m_connected(false)
        , m_loading(false)
        , m_reconnectTimer(nullptr)
    {
        m_socket = new QLocalSocket(this);
        m_reconnectTimer = new QTimer(this);
        
        connect(m_socket, &QLocalSocket::connected, this, &MapServiceClient::onConnected);
        connect(m_socket, &QLocalSocket::disconnected, this, &MapServiceClient::onDisconnected);
        connect(m_socket, &QLocalSocket::readyRead, this, &MapServiceClient::onDataReceived);
        connect(m_socket, QOverload<QLocalSocket::LocalSocketError>::of(&QLocalSocket::error),
                this, &MapServiceClient::onError);
        
        connect(m_reconnectTimer, &QTimer::timeout, this, &MapServiceClient::attemptReconnect);
        m_reconnectTimer->setSingleShot(true);
    }
    
    ~MapServiceClient() {
        disconnect();
    }
    
    bool connect() {
        if (m_connected) return true;
        
        QString serverName = "nav_map";
        qDebug() << "Connecting to map service:" << serverName;
        
        m_socket->connectToServer(serverName);
        return m_socket->waitForConnected(3000);
    }
    
    void disconnect() {
        if (m_socket && m_socket->state() != QLocalSocket::UnconnectedState) {
            m_socket->disconnectFromServer();
            m_socket->waitForDisconnected(1000);
        }
        m_connected = false;
        m_loading = false;
        
        if (m_reconnectTimer) {
            m_reconnectTimer->stop();
        }
    }
    
    bool requestMapData(const MapDataRequest& request) override {
        if (!m_connected && !connect()) {
            return false;
        }
        
        QByteArray data;
        QDataStream stream(&data, QIODevice::WriteOnly);
        
        stream << QString("MAP_DATA_REQUEST");
        stream.writeRawData(reinterpret_cast<const char*>(&request), sizeof(request));
        
        if (m_socket->write(data) == -1) {
            qDebug() << "Failed to send map data request";
            return false;
        }
        
        m_loading = true;
        return true;
    }
    
    bool requestTile(double lat, double lon, int zoom) override {
        if (!m_connected && !connect()) {
            return false;
        }
        
        QByteArray data;
        QDataStream stream(&data, QIODevice::WriteOnly);
        
        MapTileRequest request;
        request.center_lat = lat;
        request.center_lon = lon;
        request.zoom_level = zoom;
        
        stream << QString("TILE_REQUEST");
        stream.writeRawData(reinterpret_cast<const char*>(&request), sizeof(request));
        
        return m_socket->write(data) != -1;
    }
    
    MapTile getMapTile(double lat, double lon, int zoom) override {
        MapTile tile;
        tile.center_lat = lat;
        tile.center_lon = lon;
        tile.zoom_level = zoom;
        tile.has_data = false;
        return tile;
    }
    
    void setMapDataCallback(MapDataCallback callback) override {
        m_mapDataCallback = callback;
    }
    
    void setTileCallback(TileCallback callback) override {
        m_tileCallback = callback;
    }
    
    bool isConnected() override {
        return m_connected;
    }
    
    bool isLoading() override {
        return m_loading;
    }

private slots:
    void onConnected() {
        m_connected = true;
        qDebug() << "✓ Connected to map service";
        
        if (m_reconnectTimer->isActive()) {
            m_reconnectTimer->stop();
        }
    }
    
    void onDisconnected() {
        m_connected = false;
        m_loading = false;
        qDebug() << "✗ Disconnected from map service";
        
        if (!m_reconnectTimer->isActive()) {
            m_reconnectTimer->start(5000);
        }
    }
    
    void onDataReceived() {
        while (m_socket->bytesAvailable()) {
            QByteArray data = m_socket->readAll();
            QDataStream stream(&data, QIODevice::ReadOnly);
            
            QString messageType;
            stream >> messageType;
            
            if (messageType == "MAP_DATA_RESPONSE") {
                MapDataResponse response;
                stream.readRawData(reinterpret_cast<char*>(&response), sizeof(response));
                
                m_loading = false;
                
                if (m_mapDataCallback) {
                    m_mapDataCallback(response);
                }
            } else if (messageType == "TILE_RESPONSE") {
                MapTile tile;
                stream.readRawData(reinterpret_cast<char*>(&tile), sizeof(tile));
                
                if (m_tileCallback) {
                    m_tileCallback(tile);
                }
            }
        }
    }
    
    void onError(QLocalSocket::LocalSocketError error) {
        qDebug() << "Map service error:" << m_socket->errorString();
        m_connected = false;
        m_loading = false;
    }
    
    void attemptReconnect() {
        qDebug() << "Attempting to reconnect to map service...";
        connect();
    }
};

// Factory implementations using Qt objects
std::shared_ptr<IPositioningService> ServiceFactory::createPositioningService() {
    auto client = new PositioningServiceClient();
    return std::shared_ptr<IPositioningService>(client);
}

std::shared_ptr<IRoutingService> ServiceFactory::createRoutingService() {
    auto client = new RoutingServiceClient();
    return std::shared_ptr<IRoutingService>(client);
}

std::shared_ptr<IGuidanceService> ServiceFactory::createGuidanceService() {
    auto client = new GuidanceServiceClient();
    return std::shared_ptr<IGuidanceService>(client);
}

std::shared_ptr<IMapService> ServiceFactory::createMapService() {
    auto client = new MapServiceClient();
    return std::shared_ptr<IMapService>(client);
}

bool ServiceFactory::discoverServices() {
    // Try to connect to each service to check availability
    bool allAvailable = true;
    
    qDebug() << "Discovering navigation services...";
    
    // Test positioning service
    QLocalSocket testSocket;
    testSocket.connectToServer("nav_positioning");
    bool positioningAvailable = testSocket.waitForConnected(1000);
    testSocket.disconnectFromServer();
    
    // Test routing service
    testSocket.connectToServer("nav_routing");
    bool routingAvailable = testSocket.waitForConnected(1000);
    testSocket.disconnectFromServer();
    
    // Test guidance service
    testSocket.connectToServer("nav_guidance");
    bool guidanceAvailable = testSocket.waitForConnected(1000);
    testSocket.disconnectFromServer();
    
    // Test map service
    testSocket.connectToServer("nav_map");
    bool mapAvailable = testSocket.waitForConnected(1000);
    testSocket.disconnectFromServer();
    
    qDebug() << "Service discovery results:";
    qDebug() << "  Positioning:" << (positioningAvailable ? "Available" : "Not found");
    qDebug() << "  Routing:" << (routingAvailable ? "Available" : "Not found");
    qDebug() << "  Guidance:" << (guidanceAvailable ? "Available" : "Not found");
    qDebug() << "  Map:" << (mapAvailable ? "Available" : "Not found");
    
    return positioningAvailable && routingAvailable && guidanceAvailable && mapAvailable;
}

bool ServiceFactory::areServicesAvailable() {
    return discoverServices();
}

// ServiceManager implementation (updated for Qt)
ServiceManager::ServiceManager() : m_initialized(false), m_monitoring(false) {}

ServiceManager::~ServiceManager() {
    shutdown();
}

bool ServiceManager::initialize() {
    if (m_initialized) {
        return true;
    }
    
    qDebug() << "Initializing Qt-based service manager...";
    
    // Discover services
    if (!ServiceFactory::discoverServices()) {
        qDebug() << "Warning: Not all services are available";
    }
    
    // Create service instances
    m_positioningService = ServiceFactory::createPositioningService();
    m_routingService = ServiceFactory::createRoutingService();
    m_guidanceService = ServiceFactory::createGuidanceService();
    m_mapService = ServiceFactory::createMapService();
    
    m_initialized = true;
    qDebug() << "Qt service manager initialized";
    
    return true;
}

void ServiceManager::shutdown() {
    if (!m_initialized) {
        return;
    }
    
    stopServiceMonitoring();
    
    m_positioningService.reset();
    m_routingService.reset();
    m_guidanceService.reset();
    m_mapService.reset();
    
    m_initialized = false;
    qDebug() << "Qt service manager shutdown";
}

std::shared_ptr<IPositioningService> ServiceManager::getPositioningService() {
    return m_positioningService;
}

std::shared_ptr<IRoutingService> ServiceManager::getRoutingService() {
    return m_routingService;
}

std::shared_ptr<IGuidanceService> ServiceManager::getGuidanceService() {
    return m_guidanceService;
}

std::shared_ptr<IMapService> ServiceManager::getMapService() {
    return m_mapService;
}

bool ServiceManager::areAllServicesConnected() {
    if (!m_initialized) {
        return false;
    }
    
    return m_positioningService->isConnected() &&
           m_routingService->isConnected() &&
           m_guidanceService->isConnected() &&
           m_mapService->isConnected();
}

std::string ServiceManager::getServiceStatus() {
    if (!m_initialized) {
        return "Service manager not initialized";
    }
    
    QString status = "Qt Service Status:\n";
    status += QString("  Positioning: %1\n").arg(m_positioningService->isConnected() ? "Connected" : "Disconnected");
    status += QString("  Routing: %1\n").arg(m_routingService->isConnected() ? "Connected" : "Disconnected");
    status += QString("  Guidance: %1\n").arg(m_guidanceService->isConnected() ? "Connected" : "Disconnected");
    status += QString("  Map: %1").arg(m_mapService->isConnected() ? "Connected" : "Disconnected");
    
    return status.toStdString();
}

bool ServiceManager::reconnectServices() {
    qDebug() << "Attempting to reconnect Qt services...";
    return ServiceFactory::discoverServices();
}

void ServiceManager::startServiceMonitoring() {
    m_monitoring = true;
    qDebug() << "Started Qt service monitoring";
}

void ServiceManager::stopServiceMonitoring() {
    m_monitoring = false;
    qDebug() << "Stopped Qt service monitoring";
}

void ServiceManager::onServiceDisconnected(const std::string& serviceName) {
    qDebug() << "Qt service disconnected:" << serviceName.c_str();
}

bool ServiceManager::connectToService(const std::string& serviceName) {
    qDebug() << "Connecting to Qt service:" << serviceName.c_str();
    return true;
}

} // namespace nav

// Include the MOC file for Qt's meta-object system
#include "service_clients_qt.moc"