#pragma once

#include "navigation_models.h"
#include <QObject>
#include <QTimer>
#include <QDateTime>

namespace nav {

/**
 * @brief Core positioning service integrated into HMI
 * Provides GPS location and positioning data
 */
class PositioningServiceCore : public QObject
{
    Q_OBJECT

public:
    explicit PositioningServiceCore(QObject* parent = nullptr);
    ~PositioningServiceCore();

    // Main service interface
    bool initialize();
    void shutdown();
    
    // Position queries
    Point getCurrentPosition() const;
    double getCurrentHeading() const;
    double getCurrentSpeed() const;
    double getCurrentAltitude() const;
    QDateTime getLastUpdate() const;
    
    // Position control
    void setCurrentPosition(const Point& position);
    void setCurrentHeading(double heading);
    void setCurrentSpeed(double speed);
    
    // Service status
    bool isServiceReady() const;
    QString getServiceStatus() const;

signals:
    void positionChanged(const Point& position);
    void headingChanged(double heading);
    void speedChanged(double speed);
    void altitudeChanged(double altitude);
    void serviceStatusChanged(bool ready);

private slots:
    void updatePosition();
    void generateSimulatedData();

private:
    // Core positioning data
    Point m_currentPosition;
    double m_currentHeading;
    double m_currentSpeed;
    double m_currentAltitude;
    QDateTime m_lastUpdate;
    
    // Service state
    bool m_initialized;
    bool m_serviceReady;
    
    // Simulation and updates
    QTimer* m_updateTimer;
    QTimer* m_simulationTimer;
    bool m_simulationMode;
    
    // Default coordinates (Hanoi, Vietnam)
    static constexpr double DEFAULT_LAT = 21.028511;
    static constexpr double DEFAULT_LON = 105.804817;
};

} // namespace nav