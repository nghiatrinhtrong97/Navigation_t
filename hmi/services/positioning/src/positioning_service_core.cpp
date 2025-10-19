#include "../include/positioning_service_core.h"
#include <QDebug>
#include <QRandomGenerator>

namespace nav {

PositioningServiceCore::PositioningServiceCore(QObject* parent)
    : QObject(parent)
    , m_currentPosition(DEFAULT_LAT, DEFAULT_LON)
    , m_currentHeading(0.0)
    , m_currentSpeed(0.0)
    , m_currentAltitude(0.0)
    , m_lastUpdate(QDateTime::currentDateTime())
    , m_initialized(false)
    , m_serviceReady(false)
    , m_updateTimer(new QTimer(this))
    , m_simulationTimer(new QTimer(this))
    , m_simulationMode(true)
{
    // Setup update timer for position broadcasting
    connect(m_updateTimer, &QTimer::timeout, this, &PositioningServiceCore::updatePosition);
    m_updateTimer->setInterval(1000); // 1 second updates
    
    // Setup simulation timer for demo data
    connect(m_simulationTimer, &QTimer::timeout, this, &PositioningServiceCore::generateSimulatedData);
    m_simulationTimer->setInterval(2000); // 2 second simulation updates
}

PositioningServiceCore::~PositioningServiceCore()
{
    shutdown();
}

bool PositioningServiceCore::initialize()
{
    if (m_initialized) {
        return true;
    }
    
    qDebug() << "🌍 [POSITIONING CORE] Initializing positioning service...";
    
    // Initialize positioning hardware/simulation
    m_currentPosition = Point(DEFAULT_LAT, DEFAULT_LON);
    m_currentHeading = 0.0;
    m_currentSpeed = 0.0;
    m_currentAltitude = 10.0; // Default altitude in meters
    m_lastUpdate = QDateTime::currentDateTime();
    
    // Start timers
    m_updateTimer->start();
    if (m_simulationMode) {
        m_simulationTimer->start();
    }
    
    m_initialized = true;
    m_serviceReady = true;
    
    qDebug() << "[POSITIONING CORE] Positioning service initialized successfully";
    emit serviceStatusChanged(true);
    
    return true;
}

void PositioningServiceCore::shutdown()
{
    if (!m_initialized) {
        return;
    }
    
    qDebug() << "🌍 [POSITIONING CORE] Shutting down positioning service...";
    
    m_updateTimer->stop();
    m_simulationTimer->stop();
    
    m_initialized = false;
    m_serviceReady = false;
    
    emit serviceStatusChanged(false);
    qDebug() << "[POSITIONING CORE] Positioning service shut down";
}

Point PositioningServiceCore::getCurrentPosition() const
{
    return m_currentPosition;
}

double PositioningServiceCore::getCurrentHeading() const
{
    return m_currentHeading;
}

double PositioningServiceCore::getCurrentSpeed() const
{
    return m_currentSpeed;
}

double PositioningServiceCore::getCurrentAltitude() const
{
    return m_currentAltitude;
}

QDateTime PositioningServiceCore::getLastUpdate() const
{
    return m_lastUpdate;
}

void PositioningServiceCore::setCurrentPosition(const Point& position)
{
    if (m_currentPosition.latitude != position.latitude || 
        m_currentPosition.longitude != position.longitude) {
        m_currentPosition = position;
        m_lastUpdate = QDateTime::currentDateTime();
        emit positionChanged(m_currentPosition);
    }
}

void PositioningServiceCore::setCurrentHeading(double heading)
{
    if (qAbs(m_currentHeading - heading) > 0.1) {
        m_currentHeading = heading;
        emit headingChanged(m_currentHeading);
    }
}

void PositioningServiceCore::setCurrentSpeed(double speed)
{
    if (qAbs(m_currentSpeed - speed) > 0.1) {
        m_currentSpeed = speed;
        emit speedChanged(m_currentSpeed);
    }
}

bool PositioningServiceCore::isServiceReady() const
{
    return m_serviceReady;
}

QString PositioningServiceCore::getServiceStatus() const
{
    if (!m_initialized) {
        return "Not Initialized";
    }
    if (!m_serviceReady) {
        return "Not Ready";
    }
    return QString("Ready - Last Update: %1").arg(m_lastUpdate.toString());
}

void PositioningServiceCore::updatePosition()
{
    if (!m_serviceReady) {
        return;
    }
    
    m_lastUpdate = QDateTime::currentDateTime();
    
    // Emit position update signals
    emit positionChanged(m_currentPosition);
    emit headingChanged(m_currentHeading);
    emit speedChanged(m_currentSpeed);
    emit altitudeChanged(m_currentAltitude);
}

void PositioningServiceCore::generateSimulatedData()
{
    if (!m_simulationMode || !m_serviceReady) {
        return;
    }
    
    // Generate slight random movement for simulation
    QRandomGenerator* rng = QRandomGenerator::global();
    
    // Small random movement (±0.0001 degrees ≈ ±10 meters)
    double deltaLat = (rng->generateDouble() - 0.5) * 0.0002;
    double deltaLon = (rng->generateDouble() - 0.5) * 0.0002;
    
    Point newPosition(
        m_currentPosition.latitude + deltaLat,
        m_currentPosition.longitude + deltaLon
    );
    
    // Random heading change (±5 degrees)
    double deltaHeading = (rng->generateDouble() - 0.5) * 10.0;
    double newHeading = fmod(m_currentHeading + deltaHeading + 360.0, 360.0);
    
    // Random speed variation (0-60 km/h)
    double newSpeed = rng->bounded(0, 61); // Generate integer between 0-60, convert to double
    
    setCurrentPosition(newPosition);
    setCurrentHeading(newHeading);
    setCurrentSpeed(newSpeed);
    
    // Reduce log spam - only log every 10 seconds (5 cycles of 2 seconds each)
    static int logCounter = 0;
    if (++logCounter >= 5) {
        qDebug() << "🌍 [POSITIONING CORE] Simulated position:" 
                 << newPosition.latitude << "," << newPosition.longitude
                 << "heading:" << newHeading << "speed:" << newSpeed;
        logCounter = 0;
    }
}

} // namespace nav