#pragma once

#include "nav_types.h"
#include "nav_messages.h"
#include <functional>
#include <memory>

namespace nav {

// Forward declarations
class IPositioningService;
class IRoutingService;
class IGuidanceService;
class IMapService;

/**
 * @brief Abstract interface for positioning service communication
 */
class IPositioningService {
public:
    virtual ~IPositioningService() = default;
    
    // Position updates
    virtual bool startPositioning() = 0;
    virtual bool stopPositioning() = 0;
    virtual Point getCurrentPosition() = 0;
    virtual bool setPosition(const Point& position) = 0;
    
    // Callbacks for position updates
    using PositionCallback = std::function<void(const Point&, double heading, double speed)>;
    virtual void setPositionCallback(PositionCallback callback) = 0;
    
    // Service status
    virtual bool isConnected() = 0;
    virtual bool isActive() = 0;
};

/**
 * @brief Abstract interface for routing service communication
 */
class IRoutingService {
public:
    virtual ~IRoutingService() = default;
    
    // Route calculation
    virtual bool calculateRoute(const RoutingRequest& request) = 0;
    virtual bool cancelRouteCalculation() = 0;
    virtual Route getLastCalculatedRoute() = 0;
    
    // Callbacks for route updates
    using RouteCallback = std::function<void(const Route&)>;
    using RouteErrorCallback = std::function<void(const std::string&)>;
    virtual void setRouteCallback(RouteCallback callback) = 0;
    virtual void setRouteErrorCallback(RouteErrorCallback callback) = 0;
    
    // Service status
    virtual bool isConnected() = 0;
    virtual bool isCalculating() = 0;
};

/**
 * @brief Abstract interface for guidance service communication
 */
class IGuidanceService {
public:
    virtual ~IGuidanceService() = default;
    
    // Guidance control
    virtual bool startGuidance(const Route& route) = 0;
    virtual bool stopGuidance() = 0;
    virtual bool updatePosition(const Point& position) = 0;
    virtual GuidanceInstruction getCurrentInstruction() = 0;
    
    // Callbacks for guidance updates
    using GuidanceCallback = std::function<void(const GuidanceInstruction&)>;
    using RouteDeviationCallback = std::function<void(double distance)>;
    virtual void setGuidanceCallback(GuidanceCallback callback) = 0;
    virtual void setRouteDeviationCallback(RouteDeviationCallback callback) = 0;
    
    // Service status
    virtual bool isConnected() = 0;
    virtual bool isActive() = 0;
};

/**
 * @brief Abstract interface for map service communication
 */
class IMapService {
public:
    virtual ~IMapService() = default;
    
    // Map data requests
    virtual bool requestMapData(const MapDataRequest& request) = 0;
    virtual bool requestTile(double lat, double lon, int zoom) = 0;
    virtual MapTile getMapTile(double lat, double lon, int zoom) = 0;
    
    // Callbacks for map updates
    using MapDataCallback = std::function<void(const MapDataResponse&)>;
    using TileCallback = std::function<void(const MapTile&)>;
    virtual void setMapDataCallback(MapDataCallback callback) = 0;
    virtual void setTileCallback(TileCallback callback) = 0;
    
    // Service status
    virtual bool isConnected() = 0;
    virtual bool isLoading() = 0;
};

/**
 * @brief Service factory for creating service instances
 */
class ServiceFactory {
public:
    static std::shared_ptr<IPositioningService> createPositioningService();
    static std::shared_ptr<IRoutingService> createRoutingService();
    static std::shared_ptr<IGuidanceService> createGuidanceService();
    static std::shared_ptr<IMapService> createMapService();
    
    // Service discovery
    static bool discoverServices();
    static bool areServicesAvailable();
};

/**
 * @brief Service manager for coordinating all navigation services
 */
class ServiceManager {
public:
    ServiceManager();
    ~ServiceManager();
    
    // Initialization
    bool initialize();
    void shutdown();
    
    // Service access
    std::shared_ptr<IPositioningService> getPositioningService();
    std::shared_ptr<IRoutingService> getRoutingService();
    std::shared_ptr<IGuidanceService> getGuidanceService();
    std::shared_ptr<IMapService> getMapService();
    
    // Service status
    bool areAllServicesConnected();
    std::string getServiceStatus();
    
    // Service discovery and reconnection
    bool reconnectServices();
    void startServiceMonitoring();
    void stopServiceMonitoring();
    
private:
    std::shared_ptr<IPositioningService> m_positioningService;
    std::shared_ptr<IRoutingService> m_routingService;
    std::shared_ptr<IGuidanceService> m_guidanceService;
    std::shared_ptr<IMapService> m_mapService;
    
    bool m_initialized;
    bool m_monitoring;
    
    void onServiceDisconnected(const std::string& serviceName);
    bool connectToService(const std::string& serviceName);
};

} // namespace nav