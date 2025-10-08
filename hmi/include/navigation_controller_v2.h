#pragma once

#include <QObject>
#include <QTimer>
#include <QMutex>
#include <memory>

#include "nav_types.h"
#include "nav_messages.h"
#include "service_interfaces.h"

#define DEFAULT_LAT 21.028511
#define DEFAULT_LON 105.804817

namespace nav {

class NavigationController : public QObject
{
    Q_OBJECT

public:
    explicit NavigationController(QObject *parent = nullptr);
    ~NavigationController();
    
    // Service management
    bool initializeServices();
    bool areServicesConnected();
    std::string getServiceStatus();
    
    // Operation modes
    enum OperationMode {
        SERVICE_MODE,    // Use real backend services
        SIMULATION_MODE  // Use internal simulation
    };
    
    void setOperationMode(OperationMode mode);
    OperationMode getOperationMode() const;
    
    // Route management
    bool calculateRoute(const Point& start, const Point& end);
    void clearRoute();
    
    // Simulation control (fallback mode)
    void startSimulation();
    void stopSimulation();
    void setSimulationSpeed(int speed);
    
    // Position management
    void setCurrentPosition(const Point& position);
    Point getCurrentPosition() const;
    
    // Route information
    bool hasActiveRoute() const;
    const Route& getActiveRoute() const;
    GuidanceInstruction getCurrentGuidance() const;
    double getSimulationProgress() const;

signals:
    void positionChanged(const Point& position, double heading, double speed);
    void routeCalculated(const Route& route);
    void guidanceUpdated(const GuidanceInstruction& instruction);

private slots:
    void updateSimulation();
    
private:
    // Service integration
    void onServicePositionUpdate(const Point& position, double heading, double speed);
    void onServiceRouteCalculated(const Route& route);
    void onServiceGuidanceUpdate(const GuidanceInstruction& instruction);
    void onServiceRouteError(const std::string& error);
    
    // Simulation fallback methods
    void updatePositionAlongRoute();
    void generateRoutePoints(const Point& start, const Point& end, Route& route);
    double calculateDistance(const Point& p1, const Point& p2) const;
    double calculateBearing(const Point& from, const Point& to) const;
    
    // Service management
    std::shared_ptr<ServiceManager> m_serviceManager;
    OperationMode m_operationMode;
    
    // Services
    std::shared_ptr<IPositioningService> m_positioningService;
    std::shared_ptr<IRoutingService> m_routingService;
    std::shared_ptr<IGuidanceService> m_guidanceService;
    std::shared_ptr<IMapService> m_mapService;
    
    // Simulation state (fallback)
    QTimer* m_simulationTimer;
    int m_simulationSpeed;
    double m_simulationProgress;
    bool m_simulationRunning;
    std::vector<Point> m_routePoints;
    size_t m_currentRouteIndex;
    
    // Current state
    Point m_currentPosition;
    double m_currentHeading;
    double m_currentSpeed;
    Route m_activeRoute;
    bool m_hasActiveRoute;
    GuidanceInstruction m_currentGuidance;
    
    // Thread safety
    mutable QMutex m_mutex;
};

} // namespace nav