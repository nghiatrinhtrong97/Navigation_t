#pragma once

#include "navigation_models.h"
#include <QObject>
#include <QTimer>
#include <vector>

namespace nav {

/**
 * @brief Core guidance service integrated into HMI
 * Provides turn-by-turn navigation instructions
 */
class GuidanceServiceCore : public QObject
{
    Q_OBJECT

public:
    explicit GuidanceServiceCore(QObject* parent = nullptr);
    ~GuidanceServiceCore();

    // Main service interface
    bool initialize();
    void shutdown();
    
    // Guidance control
    bool startGuidance(const Route& route);
    bool stopGuidance();
    bool updateCurrentPosition(const Point& position);
    
    // Guidance queries
    GuidanceInstruction getCurrentInstruction() const;
    bool isGuidanceActive() const;
    double getDistanceToNextManeuver() const;
    double getRemainingDistance() const;
    int getRemainingTime() const;
    
    // Service status
    bool isServiceReady() const;
    std::string getServiceStatus() const;

signals:
    void guidanceInstructionUpdated(const GuidanceInstruction& instruction);
    void distanceToNextManeuverChanged(double distance);
    void remainingDistanceChanged(double distance);
    void remainingTimeChanged(int timeSeconds);
    void destinationReached();
    void guidanceStarted();
    void guidanceStopped();
    void serviceStatusChanged(bool ready);

private slots:
    void updateGuidance();

private:
    // Guidance logic
    void updateGuidanceFromPosition();
    GuidanceInstruction generateInstruction(double progressMeters) const;
    double calculateBearing(const Point& from, const Point& to) const;
    double calculateDistance(const Point& p1, const Point& p2) const;
    
    // Service state
    bool m_initialized;
    bool m_serviceReady;
    bool m_guidanceActive;
    
    // Current guidance data
    Route m_activeRoute;
    std::vector<Point> m_routePoints;
    Point m_currentPosition;
    int m_currentRouteIndex;
    GuidanceInstruction m_currentInstruction;
    
    // Guidance metrics
    double m_distanceToNextManeuver;
    double m_remainingDistance;
    int m_remainingTime;
    
    // Update timer
    QTimer* m_guidanceTimer;
    
    // Thresholds
    static constexpr double MANEUVER_DISTANCE_THRESHOLD = 100.0; // meters
    static constexpr double DESTINATION_THRESHOLD = 50.0; // meters
};

} // namespace nav