#pragma once

#include "navigation_models.h"
#include "nav_messages.h"
#include <QObject>
#include <QTimer>
#include <vector>

namespace nav {

/**
 * @brief Core routing service integrated into HMI
 * Provides route calculation and path planning
 */
class RoutingServiceCore : public QObject
{
    Q_OBJECT

public:
    explicit RoutingServiceCore(QObject* parent = nullptr);
    ~RoutingServiceCore();

    // Main service interface
    bool initialize();
    void shutdown();
    
    // Route calculation
    bool calculateRoute(const Point& start, const Point& end, RoutingCriteria criteria = RoutingCriteria::SHORTEST_TIME);
    bool calculateRouteAsync(const Point& start, const Point& end, RoutingCriteria criteria = RoutingCriteria::SHORTEST_TIME);
    void cancelRouteCalculation();
    
    // Route queries
    Route getCurrentRoute() const;
    bool hasActiveRoute() const;
    double getRouteProgress() const;
    
    // Service status
    bool isServiceReady() const;
    QString getServiceStatus() const;

signals:
    void routeCalculated(const Route& route);
    void routeCalculationFailed(const QString& error);
    void routeProgressChanged(double progress);
    void serviceStatusChanged(bool ready);

private slots:
    void performAsyncCalculation();

private:
    // Core routing algorithms
    Route calculateShortestPath(const Point& start, const Point& end);
    Route calculateFastestPath(const Point& start, const Point& end);
    std::vector<Point> generateRoutePoints(const Point& start, const Point& end, int numPoints = 20);
    double calculateDistance(const Point& p1, const Point& p2) const;
    double calculateEstimatedTime(double distance, RoutingCriteria criteria) const;
    
    // A* algorithm implementation
    Route aStarAlgorithm(const Point& start, const Point& end);
    
    // Service state
    bool m_initialized;
    bool m_serviceReady;
    Route m_currentRoute;
    bool m_hasActiveRoute;
    double m_routeProgress;
    
    // Async calculation
    QTimer* m_calculationTimer;
    Point m_pendingStart;
    Point m_pendingEnd;
    RoutingCriteria m_pendingCriteria;
    bool m_calculationInProgress;
};

} // namespace nav