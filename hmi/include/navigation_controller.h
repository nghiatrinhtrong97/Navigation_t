#pragma once

#include <QObject>
#include <QTimer>
#include <QThread>
#include <QMutex>
#include <QMutexLocker>
#include <memory>

#include "nav_types.h"
#include "nav_messages.h"

namespace nav {

// Forward declarations
class AStarAlgorithm;
class RouteMonitoring;
class TurnByTurnGenerator;

class NavigationController : public QObject
{
    Q_OBJECT

public:
    explicit NavigationController(QObject *parent = nullptr);
    ~NavigationController();

    // Route calculation
    bool calculateRoute(const Point& start, const Point& end);
    void clearRoute();
    
    // Simulation control
    void startSimulation();
    void stopSimulation();
    void setSimulationSpeed(double speedMultiplier);
    
    // Position control
    void setCurrentPosition(const Point& position);
    Point getCurrentPosition() const;
    
    // Route access
    bool hasActiveRoute() const { return m_hasActiveRoute; }
    const Route& getActiveRoute() const { return m_activeRoute; }
    
    // Guidance access
    GuidanceInstruction getCurrentGuidance() const;

signals:
    void positionChanged(const Point& position, double heading, double speed);
    void routeCalculated(const Route& route);
    void routeCalculationFailed(const QString& error);
    void guidanceUpdated(const GuidanceInstruction& instruction);
    void simulationStarted();
    void simulationStopped();

private slots:
    void onSimulationTimer();
    void onGuidanceTimer();

private:
    void generateDummyMapData();
    void updateGuidance();
    void updateSimulationPosition();
    
    // Simulation state
    QTimer *m_simulationTimer;
    QTimer *m_guidanceTimer;
    bool m_simulationRunning;
    double m_simulationSpeed;       // Speed multiplier (1.0 = real time)
    
    // Current state
    Point m_currentPosition;
    double m_currentHeading;        // Degrees
    double m_currentSpeed;          // km/h
    
    // Route data
    Route m_activeRoute;
    bool m_hasActiveRoute;
    std::vector<Point> m_routePoints;  // Cached route point positions
    int m_currentSegmentIndex;
    
    // Map data for routing
    std::vector<MapNode> m_mapNodes;
    std::vector<MapEdge> m_mapEdges;
    
    // Algorithms
    std::unique_ptr<AStarAlgorithm> m_astar;
    std::unique_ptr<RouteMonitoring> m_routeMonitor;
    std::unique_ptr<TurnByTurnGenerator> m_guidanceGenerator;
    
    // Thread safety
    mutable QMutex m_mutex;
    
    // Configuration
    static constexpr int SIMULATION_INTERVAL_MS = 100;  // 10 Hz
    static constexpr int GUIDANCE_INTERVAL_MS = 500;    // 2 Hz
    static constexpr double DEFAULT_SIMULATION_SPEED_KMH = 50.0;
};

} // namespace nav