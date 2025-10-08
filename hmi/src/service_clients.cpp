#include "../include/service_interfaces.h"
#include "nav_utils.h"
#include <thread>
#include <chrono>
#include <iostream>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>

namespace nav {

/**
 * @brief IPC-based positioning service client
 */
class PositioningServiceClient : public IPositioningService {
private:
    int m_socket;
    bool m_connected;
    bool m_active;
    PositionCallback m_positionCallback;
    std::thread m_listenerThread;
    bool m_listening;
    Point m_lastPosition;

public:
    PositioningServiceClient() : m_socket(-1), m_connected(false), m_active(false), m_listening(false) {}
    
    ~PositioningServiceClient() {
        disconnect();
    }
    
    bool connect() {
        m_socket = socket(AF_UNIX, SOCK_STREAM, 0);
        if (m_socket == -1) {
            std::cerr << "Failed to create positioning socket" << std::endl;
            return false;
        }
        
        struct sockaddr_un addr;
        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        strcpy(addr.sun_path, "/tmp/nav_positioning.sock");
        
        if (::connect(m_socket, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
            std::cerr << "Failed to connect to positioning service" << std::endl;
            close(m_socket);
            m_socket = -1;
            return false;
        }
        
        m_connected = true;
        std::cout << "Connected to positioning service" << std::endl;
        return true;
    }
    
    void disconnect() {
        if (m_listening) {
            m_listening = false;
            if (m_listenerThread.joinable()) {
                m_listenerThread.join();
            }
        }
        
        if (m_socket != -1) {
            close(m_socket);
            m_socket = -1;
        }
        m_connected = false;
        m_active = false;
    }
    
    bool startPositioning() override {
        if (!m_connected && !connect()) {
            return false;
        }
        
        // Send start command
        PositioningRequest request;
        request.request_type = PositioningRequest::START;
        
        if (send(m_socket, &request, sizeof(request), 0) == -1) {
            std::cerr << "Failed to send start positioning request" << std::endl;
            return false;
        }
        
        m_active = true;
        
        // Start listener thread
        if (!m_listening) {
            m_listening = true;
            m_listenerThread = std::thread(&PositioningServiceClient::listenForUpdates, this);
        }
        
        return true;
    }
    
    bool stopPositioning() override {
        if (!m_connected) return false;
        
        PositioningRequest request;
        request.request_type = PositioningRequest::STOP;
        
        if (send(m_socket, &request, sizeof(request), 0) == -1) {
            std::cerr << "Failed to send stop positioning request" << std::endl;
            return false;
        }
        
        m_active = false;
        return true;
    }
    
    Point getCurrentPosition() override {
        return m_lastPosition;
    }
    
    bool setPosition(const Point& position) override {
        if (!m_connected) return false;
        
        PositioningRequest request;
        request.request_type = PositioningRequest::SET_POSITION;
        request.position = position;
        
        if (send(m_socket, &request, sizeof(request), 0) == -1) {
            std::cerr << "Failed to send set position request" << std::endl;
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
    
private:
    void listenForUpdates() {
        while (m_listening && m_connected) {
            PositioningResponse response;
            ssize_t bytes = recv(m_socket, &response, sizeof(response), 0);
            
            if (bytes == sizeof(response)) {
                m_lastPosition = response.position;
                
                if (m_positionCallback) {
                    m_positionCallback(response.position, response.heading, response.speed);
                }
            } else if (bytes == 0) {
                // Connection closed
                std::cerr << "Positioning service connection closed" << std::endl;
                m_connected = false;
                break;
            } else if (bytes == -1) {
                std::cerr << "Error receiving positioning data" << std::endl;
                break;
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
};

/**
 * @brief IPC-based routing service client
 */
class RoutingServiceClient : public IRoutingService {
private:
    int m_socket;
    bool m_connected;
    bool m_calculating;
    RouteCallback m_routeCallback;
    RouteErrorCallback m_errorCallback;
    Route m_lastRoute;

public:
    RoutingServiceClient() : m_socket(-1), m_connected(false), m_calculating(false) {}
    
    ~RoutingServiceClient() {
        disconnect();
    }
    
    bool connect() {
        m_socket = socket(AF_UNIX, SOCK_STREAM, 0);
        if (m_socket == -1) {
            std::cerr << "Failed to create routing socket" << std::endl;
            return false;
        }
        
        struct sockaddr_un addr;
        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        strcpy(addr.sun_path, "/tmp/nav_routing.sock");
        
        if (::connect(m_socket, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
            std::cerr << "Failed to connect to routing service" << std::endl;
            close(m_socket);
            m_socket = -1;
            return false;
        }
        
        m_connected = true;
        std::cout << "Connected to routing service" << std::endl;
        return true;
    }
    
    void disconnect() {
        if (m_socket != -1) {
            close(m_socket);
            m_socket = -1;
        }
        m_connected = false;
        m_calculating = false;
    }
    
    bool calculateRoute(const RoutingRequest& request) override {
        if (!m_connected && !connect()) {
            return false;
        }
        
        if (send(m_socket, &request, sizeof(request), 0) == -1) {
            std::cerr << "Failed to send routing request" << std::endl;
            return false;
        }
        
        m_calculating = true;
        
        // Start async response handler
        std::thread([this]() {
            RoutingResponse response;
            ssize_t bytes = recv(m_socket, &response, sizeof(response), 0);
            
            m_calculating = false;
            
            if (bytes == sizeof(response)) {
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
            } else {
                if (m_errorCallback) {
                    m_errorCallback("Communication error with routing service");
                }
            }
        }).detach();
        
        return true;
    }
    
    bool cancelRouteCalculation() override {
        if (!m_connected) return false;
        
        RoutingRequest request;
        request.request_type = RoutingRequest::CANCEL;
        
        if (send(m_socket, &request, sizeof(request), 0) == -1) {
            std::cerr << "Failed to send cancel request" << std::endl;
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
};

/**
 * @brief IPC-based guidance service client
 */
class GuidanceServiceClient : public IGuidanceService {
private:
    int m_socket;
    bool m_connected;
    bool m_active;
    GuidanceCallback m_guidanceCallback;
    RouteDeviationCallback m_deviationCallback;
    std::thread m_listenerThread;
    bool m_listening;
    GuidanceInstruction m_lastInstruction;

public:
    GuidanceServiceClient() : m_socket(-1), m_connected(false), m_active(false), m_listening(false) {}
    
    ~GuidanceServiceClient() {
        disconnect();
    }
    
    bool connect() {
        m_socket = socket(AF_UNIX, SOCK_STREAM, 0);
        if (m_socket == -1) {
            std::cerr << "Failed to create guidance socket" << std::endl;
            return false;
        }
        
        struct sockaddr_un addr;
        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        strcpy(addr.sun_path, "/tmp/nav_guidance.sock");
        
        if (::connect(m_socket, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
            std::cerr << "Failed to connect to guidance service" << std::endl;
            close(m_socket);
            m_socket = -1;
            return false;
        }
        
        m_connected = true;
        std::cout << "Connected to guidance service" << std::endl;
        return true;
    }
    
    void disconnect() {
        if (m_listening) {
            m_listening = false;
            if (m_listenerThread.joinable()) {
                m_listenerThread.join();
            }
        }
        
        if (m_socket != -1) {
            close(m_socket);
            m_socket = -1;
        }
        m_connected = false;
        m_active = false;
    }
    
    bool startGuidance(const Route& route) override {
        if (!m_connected && !connect()) {
            return false;
        }
        
        GuidanceRequest request;
        request.request_type = GuidanceRequest::START;
        request.route = route;
        
        if (send(m_socket, &request, sizeof(request), 0) == -1) {
            std::cerr << "Failed to send start guidance request" << std::endl;
            return false;
        }
        
        m_active = true;
        
        // Start listener thread
        if (!m_listening) {
            m_listening = true;
            m_listenerThread = std::thread(&GuidanceServiceClient::listenForUpdates, this);
        }
        
        return true;
    }
    
    bool stopGuidance() override {
        if (!m_connected) return false;
        
        GuidanceRequest request;
        request.request_type = GuidanceRequest::STOP;
        
        if (send(m_socket, &request, sizeof(request), 0) == -1) {
            std::cerr << "Failed to send stop guidance request" << std::endl;
            return false;
        }
        
        m_active = false;
        return true;
    }
    
    bool updatePosition(const Point& position) override {
        if (!m_connected || !m_active) return false;
        
        GuidanceRequest request;
        request.request_type = GuidanceRequest::UPDATE_POSITION;
        request.current_position = position;
        
        if (send(m_socket, &request, sizeof(request), 0) == -1) {
            std::cerr << "Failed to send position update" << std::endl;
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
    
private:
    void listenForUpdates() {
        while (m_listening && m_connected) {
            GuidanceResponse response;
            ssize_t bytes = recv(m_socket, &response, sizeof(response), 0);
            
            if (bytes == sizeof(response)) {
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
            } else if (bytes == 0) {
                std::cerr << "Guidance service connection closed" << std::endl;
                m_connected = false;
                break;
            } else if (bytes == -1) {
                std::cerr << "Error receiving guidance data" << std::endl;
                break;
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
};

/**
 * @brief IPC-based map service client
 */
class MapServiceClient : public IMapService {
private:
    int m_socket;
    bool m_connected;
    bool m_loading;
    MapDataCallback m_mapDataCallback;
    TileCallback m_tileCallback;

public:
    MapServiceClient() : m_socket(-1), m_connected(false), m_loading(false) {}
    
    ~MapServiceClient() {
        disconnect();
    }
    
    bool connect() {
        m_socket = socket(AF_UNIX, SOCK_STREAM, 0);
        if (m_socket == -1) {
            std::cerr << "Failed to create map socket" << std::endl;
            return false;
        }
        
        struct sockaddr_un addr;
        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        strcpy(addr.sun_path, "/tmp/nav_map.sock");
        
        if (::connect(m_socket, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
            std::cerr << "Failed to connect to map service" << std::endl;
            close(m_socket);
            m_socket = -1;
            return false;
        }
        
        m_connected = true;
        std::cout << "Connected to map service" << std::endl;
        return true;
    }
    
    void disconnect() {
        if (m_socket != -1) {
            close(m_socket);
            m_socket = -1;
        }
        m_connected = false;
        m_loading = false;
    }
    
    bool requestMapData(const MapDataRequest& request) override {
        if (!m_connected && !connect()) {
            return false;
        }
        
        if (send(m_socket, &request, sizeof(request), 0) == -1) {
            std::cerr << "Failed to send map data request" << std::endl;
            return false;
        }
        
        m_loading = true;
        
        // Start async response handler
        std::thread([this]() {
            MapDataResponse response;
            ssize_t bytes = recv(m_socket, &response, sizeof(response), 0);
            
            m_loading = false;
            
            if (bytes == sizeof(response) && m_mapDataCallback) {
                m_mapDataCallback(response);
            }
        }).detach();
        
        return true;
    }
    
    bool requestTile(double lat, double lon, int zoom) override {
        if (!m_connected && !connect()) {
            return false;
        }
        
        MapTileRequest request;
        request.center_lat = lat;
        request.center_lon = lon;
        request.zoom_level = zoom;
        
        if (send(m_socket, &request, sizeof(request), 0) == -1) {
            std::cerr << "Failed to send tile request" << std::endl;
            return false;
        }
        
        return true;
    }
    
    MapTile getMapTile(double lat, double lon, int zoom) override {
        // This would be synchronous request - simplified implementation
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
};

// Factory implementations
std::shared_ptr<IPositioningService> ServiceFactory::createPositioningService() {
    return std::make_shared<PositioningServiceClient>();
}

std::shared_ptr<IRoutingService> ServiceFactory::createRoutingService() {
    return std::make_shared<RoutingServiceClient>();
}

std::shared_ptr<IGuidanceService> ServiceFactory::createGuidanceService() {
    return std::make_shared<GuidanceServiceClient>();
}

std::shared_ptr<IMapService> ServiceFactory::createMapService() {
    return std::make_shared<MapServiceClient>();
}

bool ServiceFactory::discoverServices() {
    // Check if service sockets exist
    bool positioningAvailable = access("/tmp/nav_positioning.sock", F_OK) == 0;
    bool routingAvailable = access("/tmp/nav_routing.sock", F_OK) == 0;
    bool guidanceAvailable = access("/tmp/nav_guidance.sock", F_OK) == 0;
    bool mapAvailable = access("/tmp/nav_map.sock", F_OK) == 0;
    
    std::cout << "Service discovery results:" << std::endl;
    std::cout << "  Positioning: " << (positioningAvailable ? "Available" : "Not found") << std::endl;
    std::cout << "  Routing: " << (routingAvailable ? "Available" : "Not found") << std::endl;
    std::cout << "  Guidance: " << (guidanceAvailable ? "Available" : "Not found") << std::endl;
    std::cout << "  Map: " << (mapAvailable ? "Available" : "Not found") << std::endl;
    
    return positioningAvailable && routingAvailable && guidanceAvailable && mapAvailable;
}

bool ServiceFactory::areServicesAvailable() {
    return discoverServices();
}

// ServiceManager implementation
ServiceManager::ServiceManager() : m_initialized(false), m_monitoring(false) {}

ServiceManager::~ServiceManager() {
    shutdown();
}

bool ServiceManager::initialize() {
    if (m_initialized) {
        return true;
    }
    
    std::cout << "Initializing service manager..." << std::endl;
    
    // Discover services
    if (!ServiceFactory::discoverServices()) {
        std::cout << "Warning: Not all services are available" << std::endl;
    }
    
    // Create service instances
    m_positioningService = ServiceFactory::createPositioningService();
    m_routingService = ServiceFactory::createRoutingService();
    m_guidanceService = ServiceFactory::createGuidanceService();
    m_mapService = ServiceFactory::createMapService();
    
    m_initialized = true;
    std::cout << "Service manager initialized" << std::endl;
    
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
    std::cout << "Service manager shutdown" << std::endl;
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
    
    std::string status = "Service Status:\n";
    status += "  Positioning: " + std::string(m_positioningService->isConnected() ? "Connected" : "Disconnected") + "\n";
    status += "  Routing: " + std::string(m_routingService->isConnected() ? "Connected" : "Disconnected") + "\n";
    status += "  Guidance: " + std::string(m_guidanceService->isConnected() ? "Connected" : "Disconnected") + "\n";
    status += "  Map: " + std::string(m_mapService->isConnected() ? "Connected" : "Disconnected");
    
    return status;
}

bool ServiceManager::reconnectServices() {
    std::cout << "Attempting to reconnect services..." << std::endl;
    
    // This would attempt to reconnect each service
    // Implementation depends on specific service requirements
    
    return ServiceFactory::discoverServices();
}

void ServiceManager::startServiceMonitoring() {
    m_monitoring = true;
    // Implementation for periodic service health checks
}

void ServiceManager::stopServiceMonitoring() {
    m_monitoring = false;
}

void ServiceManager::onServiceDisconnected(const std::string& serviceName) {
    std::cout << "Service disconnected: " << serviceName << std::endl;
    // Implementation for handling service disconnections
}

bool ServiceManager::connectToService(const std::string& serviceName) {
    std::cout << "Connecting to service: " << serviceName << std::endl;
    // Implementation for connecting to specific service
    return true;
}

} // namespace nav