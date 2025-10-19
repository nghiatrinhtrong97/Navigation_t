#pragma once

#include "navigation_models.h"
#include "nav_messages.h"
#include "positioning_service_core.h"
#include "routing_service_core.h"
#include "guidance_service_core.h"
#include "map_service_core.h"
#include "poi_service.h"
#include <QObject>
#include <QTimer>
#include <QMutex>
#include <memory>

namespace nav {

/**
 * @brief Integrated Navigation Controller with embedded service cores
 * This replaces the multi-process architecture with direct service integration
 */
class IntegratedNavigationController : public QObject
{
    Q_OBJECT

public:
    explicit IntegratedNavigationController(QObject *parent = nullptr);
    ~IntegratedNavigationController();
    
    // Service management
    bool initializeServices();
    bool areServicesReady();
    std::string getServiceStatus();
    void shutdownServices();
    
    // Route management
    bool calculateRoute(const Point& start, const Point& end, RoutingCriteria criteria = RoutingCriteria::SHORTEST_TIME);
    bool calculateRouteAsync(const Point& start, const Point& end, RoutingCriteria criteria = RoutingCriteria::SHORTEST_TIME);
    void clearRoute();
    
    // Navigation control
    void startNavigation();
    void stopNavigation();
    bool isNavigating() const;
    
    // Position management
    void setCurrentPosition(const Point& position);
    void setCurrentHeading(double heading);
    void setCurrentSpeed(double speed);
    Point getCurrentPosition() const;
    double getCurrentHeading() const;
    double getCurrentSpeed() const;
    
    // Route information
    bool hasActiveRoute() const;
    const Route& getActiveRoute() const;
    GuidanceInstruction getCurrentGuidance() const;
    double getDistanceToNextManeuver() const;
    double getRemainingDistance() const;
    int getRemainingTime() const;
    
    // Map and POI services
    std::vector<POI> findNearbyPOIs(const Point& location, double radiusMeters, const QString& category = QString());
    std::vector<POI> searchPOIs(const QString& searchTerm);
    POI getPOIById(uint64_t poiId);
    
    // Service access (for direct UI integration)
    PositioningServiceCore* getPositioningService() const { return m_positioningService.get(); }
    RoutingServiceCore* getRoutingService() const { return m_routingService.get(); }
    GuidanceServiceCore* getGuidanceService() const { return m_guidanceService.get(); }
    MapServiceCore* getMapService() const { return m_mapService.get(); }
    POIService* getPOIService() const { return m_poiService.get(); }

signals:
    void positionChanged(const Point& position, double heading, double speed);
    void routeCalculated(const Route& route);
    void routeCalculationFailed(const QString& errorMessage);
    void guidanceUpdated(const GuidanceInstruction& instruction);
    void destinationReached();
    void servicesReady(bool ready);
    void navigationStarted();
    void navigationStopped();

private slots:
    void onPositionChanged(const Point& position);
    void onHeadingChanged(double heading);
    void onSpeedChanged(double speed);
    void onRouteCalculated(const Route& route);
    void onRouteCalculationFailed(const QString& error);
    void onGuidanceUpdated(const GuidanceInstruction& instruction);
    void onDestinationReached();

private:
    void connectServiceSignals();
    void disconnectServiceSignals();
    
    // Integrated service cores
    std::unique_ptr<PositioningServiceCore> m_positioningService;
    std::unique_ptr<RoutingServiceCore> m_routingService;
    std::unique_ptr<GuidanceServiceCore> m_guidanceService;
    std::unique_ptr<MapServiceCore> m_mapService;
    std::unique_ptr<POIService> m_poiService;
    
    // Navigation state
    bool m_servicesInitialized;
    bool m_navigationActive;
    Route m_activeRoute;
    Point m_currentPosition;
    double m_currentHeading;
    double m_currentSpeed;
    
    // Thread safety
    mutable QMutex m_mutex;
    
    // Default coordinates (Hanoi, Vietnam)
    static constexpr double DEFAULT_LAT = 21.028511;
    static constexpr double DEFAULT_LON = 105.804817;
};

} // namespace nav