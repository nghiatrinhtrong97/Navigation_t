#include "../include/service_clients_qt.h"
#include <QDataStream>
#include <QDebug>

namespace nav {

// PositioningServiceClient Implementation
PositioningServiceClient::PositioningServiceClient(QObject* parent) 
    : QObject(parent)
    , m_socket(nullptr)
    , m_connected(false)
    , m_active(false)
    , m_reconnectTimer(nullptr)
{
    m_socket = new QLocalSocket(this);
    m_reconnectTimer = new QTimer(this);
    
    connect(m_socket, &QLocalSocket::connected, this, &PositioningServiceClient::onConnected);
    connect(m_socket, &QLocalSocket::disconnected, this, &PositioningServiceClient::onDisconnected);
    connect(m_socket, &QLocalSocket::readyRead, this, &PositioningServiceClient::onReadyRead);
    connect(m_socket, QOverload<QLocalSocket::LocalSocketError>::of(&QLocalSocket::errorOccurred), 
            this, &PositioningServiceClient::onError);
    
    connect(m_reconnectTimer, &QTimer::timeout, this, &PositioningServiceClient::attemptReconnect);
    m_reconnectTimer->setSingleShot(true);
    
    connectToService();
}

PositioningServiceClient::~PositioningServiceClient() {
    disconnectFromService();
}

bool PositioningServiceClient::startPositioning() {
    if (!m_connected) return false;
    sendMessage("START_POSITIONING");
    m_active = true;
    return true;
}

bool PositioningServiceClient::stopPositioning() {
    if (!m_connected) return false;
    sendMessage("STOP_POSITIONING");
    m_active = false;
    return true;
}

Point PositioningServiceClient::getCurrentPosition() {
    return m_lastPosition;
}

bool PositioningServiceClient::setPosition(const Point& position) {
    m_lastPosition = position;
    return true;
}

void PositioningServiceClient::setPositionCallback(PositionCallback callback) {
    m_positionCallback = callback;
}

bool PositioningServiceClient::isConnected() {
    return m_connected;
}

bool PositioningServiceClient::isActive() {
    return m_active;
}

void PositioningServiceClient::onConnected() {
    m_connected = true;
    qDebug() << "PositioningServiceClient connected";
}

void PositioningServiceClient::onDisconnected() {
    m_connected = false;
    m_active = false;
    qDebug() << "PositioningServiceClient disconnected";
    
    // Auto reconnect after 5 seconds
    m_reconnectTimer->start(5000);
}

void PositioningServiceClient::onReadyRead() {
    QByteArray data = m_socket->readAll();
    // Parse position data and call callback if set
    qDebug() << "PositioningServiceClient received:" << data;
}

void PositioningServiceClient::onError(QLocalSocket::LocalSocketError error) {
    qDebug() << "PositioningServiceClient error:" << error;
    disconnectFromService();
}

void PositioningServiceClient::attemptReconnect() {
    qDebug() << "PositioningServiceClient attempting reconnect...";
    connectToService();
}

void PositioningServiceClient::connectToService() {
    if (m_socket->state() != QLocalSocket::UnconnectedState) {
        return;
    }
    
    m_socket->connectToServer("nav_positioning_service");
}

void PositioningServiceClient::disconnectFromService() {
    if (m_socket->state() != QLocalSocket::UnconnectedState) {
        m_socket->disconnectFromServer();
    }
}

void PositioningServiceClient::sendMessage(const QString& message) {
    if (m_connected) {
        m_socket->write(message.toUtf8());
    }
}

// RoutingServiceClient Implementation
RoutingServiceClient::RoutingServiceClient(QObject* parent)
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
    connect(m_socket, &QLocalSocket::readyRead, this, &RoutingServiceClient::onReadyRead);
    connect(m_socket, QOverload<QLocalSocket::LocalSocketError>::of(&QLocalSocket::errorOccurred), 
            this, &RoutingServiceClient::onError);
    
    connect(m_reconnectTimer, &QTimer::timeout, this, &RoutingServiceClient::attemptReconnect);
    m_reconnectTimer->setSingleShot(true);
    
    connectToService();
}

RoutingServiceClient::~RoutingServiceClient() {
    disconnectFromService();
}

bool RoutingServiceClient::calculateRoute(const RoutingRequest& request) {
    if (!m_connected) return false;
    
    m_calculating = true;
    // Convert request to message and send
    sendRoutingRequest(request);
    return true;
}

bool RoutingServiceClient::cancelRouteCalculation() {
    if (!m_connected) return false;
    
    m_calculating = false;
    sendMessage("CANCEL_ROUTE");
    return true;
}

Route RoutingServiceClient::getLastCalculatedRoute() {
    return m_lastRoute;
}

void RoutingServiceClient::setRouteCallback(RouteCallback callback) {
    m_routeCallback = callback;
}

void RoutingServiceClient::setRouteErrorCallback(RouteErrorCallback callback) {
    m_errorCallback = callback;
}

bool RoutingServiceClient::isConnected() {
    return m_connected;
}

bool RoutingServiceClient::isCalculating() {
    return m_calculating;
}

void RoutingServiceClient::onConnected() {
    m_connected = true;
    qDebug() << "RoutingServiceClient connected";
}

void RoutingServiceClient::onDisconnected() {
    m_connected = false;
    m_calculating = false;
    qDebug() << "RoutingServiceClient disconnected";
    
    m_reconnectTimer->start(5000);
}

void RoutingServiceClient::onReadyRead() {
    QByteArray data = m_socket->readAll();
    qDebug() << "RoutingServiceClient received:" << data;
    
    // For demo, create a simple route and call callback
    if (m_routeCallback && m_calculating) {
        Route route;
        route.route_id = 1;
        route.node_count = 2;
        route.total_distance_meters = 1000.0;
        route.estimated_time_seconds = 120.0;
        
        m_lastRoute = route;
        m_calculating = false;
        m_routeCallback(route);
    }
}

void RoutingServiceClient::onError(QLocalSocket::LocalSocketError error) {
    qDebug() << "RoutingServiceClient error:" << error;
    disconnectFromService();
}

void RoutingServiceClient::attemptReconnect() {
    qDebug() << "RoutingServiceClient attempting reconnect...";
    connectToService();
}

void RoutingServiceClient::connectToService() {
    if (m_socket->state() != QLocalSocket::UnconnectedState) {
        return;
    }
    
    m_socket->connectToServer("nav_routing_service");
}

void RoutingServiceClient::disconnectFromService() {
    if (m_socket->state() != QLocalSocket::UnconnectedState) {
        m_socket->disconnectFromServer();
    }
}

void RoutingServiceClient::sendMessage(const QString& message) {
    if (m_connected) {
        m_socket->write(message.toUtf8());
    }
}

void RoutingServiceClient::sendRoutingRequest(const RoutingRequest& request) {
    if (m_connected) {
        QString message = QString("CALCULATE_ROUTE:%1,%2:%3,%4")
            .arg(request.start_point.latitude)
            .arg(request.start_point.longitude)
            .arg(request.end_point.latitude)
            .arg(request.end_point.longitude);
        m_socket->write(message.toUtf8());
    }
}

// GuidanceServiceClient Implementation
GuidanceServiceClient::GuidanceServiceClient(QObject* parent)
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
    connect(m_socket, &QLocalSocket::readyRead, this, &GuidanceServiceClient::onReadyRead);
    connect(m_socket, QOverload<QLocalSocket::LocalSocketError>::of(&QLocalSocket::errorOccurred), 
            this, &GuidanceServiceClient::onError);
    
    connect(m_reconnectTimer, &QTimer::timeout, this, &GuidanceServiceClient::attemptReconnect);
    m_reconnectTimer->setSingleShot(true);
    
    connectToService();
}

GuidanceServiceClient::~GuidanceServiceClient() {
    disconnectFromService();
}

bool GuidanceServiceClient::startGuidance(const Route& route) {
    if (!m_connected) return false;
    
    m_currentRoute = route;
    m_active = true;
    sendGuidanceCommand("START_GUIDANCE");
    return true;
}

bool GuidanceServiceClient::stopGuidance() {
    if (!m_connected) return false;
    
    m_active = false;
    sendGuidanceCommand("STOP_GUIDANCE");
    return true;
}

bool GuidanceServiceClient::updatePosition(const Point& position) {
    if (!m_connected) return false;
    
    QString message = QString("UPDATE_POSITION:%1,%2")
        .arg(position.latitude)
        .arg(position.longitude);
    sendGuidanceCommand(message);
    return true;
}

GuidanceInstruction GuidanceServiceClient::getCurrentInstruction() {
    return m_lastInstruction;
}

void GuidanceServiceClient::setGuidanceCallback(GuidanceCallback callback) {
    m_guidanceCallback = callback;
}

void GuidanceServiceClient::setRouteDeviationCallback(RouteDeviationCallback callback) {
    m_deviationCallback = callback;
}

bool GuidanceServiceClient::isConnected() {
    return m_connected;
}

bool GuidanceServiceClient::isActive() {
    return m_active;
}

void GuidanceServiceClient::onConnected() {
    m_connected = true;
    qDebug() << "GuidanceServiceClient connected";
}

void GuidanceServiceClient::onDisconnected() {
    m_connected = false;
    m_active = false;
    qDebug() << "GuidanceServiceClient disconnected";
    
    m_reconnectTimer->start(5000);
}

void GuidanceServiceClient::onReadyRead() {
    QByteArray data = m_socket->readAll();
    qDebug() << "GuidanceServiceClient received:" << data;
}

void GuidanceServiceClient::onError(QLocalSocket::LocalSocketError error) {
    qDebug() << "GuidanceServiceClient error:" << error;
    disconnectFromService();
}

void GuidanceServiceClient::attemptReconnect() {
    qDebug() << "GuidanceServiceClient attempting reconnect...";
    connectToService();
}

void GuidanceServiceClient::connectToService() {
    if (m_socket->state() != QLocalSocket::UnconnectedState) {
        return;
    }
    
    m_socket->connectToServer("nav_guidance_service");
}

void GuidanceServiceClient::disconnectFromService() {
    if (m_socket->state() != QLocalSocket::UnconnectedState) {
        m_socket->disconnectFromServer();
    }
}

void GuidanceServiceClient::sendGuidanceCommand(const QString& command) {
    if (m_connected) {
        m_socket->write(command.toUtf8());
    }
}

// MapServiceClient Implementation
MapServiceClient::MapServiceClient(QObject* parent)
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
    connect(m_socket, &QLocalSocket::readyRead, this, &MapServiceClient::onReadyRead);
    connect(m_socket, QOverload<QLocalSocket::LocalSocketError>::of(&QLocalSocket::errorOccurred), 
            this, &MapServiceClient::onError);
    
    connect(m_reconnectTimer, &QTimer::timeout, this, &MapServiceClient::attemptReconnect);
    m_reconnectTimer->setSingleShot(true);
    
    connectToService();
}

MapServiceClient::~MapServiceClient() {
    disconnectFromService();
}

bool MapServiceClient::requestMapData(const MapDataRequest& request) {
    if (!m_connected) return false;
    
    m_loading = true;
    sendMapRequest("REQUEST_MAP_DATA");
    return true;
}

bool MapServiceClient::requestTile(double lat, double lon, int zoom) {
    if (!m_connected) return false;
    
    QString message = QString("REQUEST_TILE:%1,%2,%3")
        .arg(lat).arg(lon).arg(zoom);
    sendMapRequest(message);
    return true;
}

MapDisplayTile MapServiceClient::getMapTile(double lat, double lon, int zoom) {
    MapDisplayTile tile;
    // Return empty tile for now
    return tile;
}

void MapServiceClient::setMapDataCallback(MapDataCallback callback) {
    m_dataCallback = callback;
}

void MapServiceClient::setTileCallback(TileCallback callback) {
    m_tileCallback = callback;
}

bool MapServiceClient::isConnected() {
    return m_connected;
}

bool MapServiceClient::isLoading() {
    return m_loading;
}

void MapServiceClient::onConnected() {
    m_connected = true;
    qDebug() << "MapServiceClient connected";
}

void MapServiceClient::onDisconnected() {
    m_connected = false;
    m_loading = false;
    qDebug() << "MapServiceClient disconnected";
    
    m_reconnectTimer->start(5000);
}

void MapServiceClient::onReadyRead() {
    QByteArray data = m_socket->readAll();
    qDebug() << "MapServiceClient received:" << data;
    m_loading = false;
}

void MapServiceClient::onError(QLocalSocket::LocalSocketError error) {
    qDebug() << "MapServiceClient error:" << error;
    disconnectFromService();
}

void MapServiceClient::attemptReconnect() {
    qDebug() << "MapServiceClient attempting reconnect...";
    connectToService();
}

void MapServiceClient::connectToService() {
    if (m_socket->state() != QLocalSocket::UnconnectedState) {
        return;
    }
    
    m_socket->connectToServer("nav_map_service");
}

void MapServiceClient::disconnectFromService() {
    if (m_socket->state() != QLocalSocket::UnconnectedState) {
        m_socket->disconnectFromServer();
    }
}

void MapServiceClient::sendMapRequest(const QString& request) {
    if (m_connected) {
        m_socket->write(request.toUtf8());
    }
}

} // namespace nav
