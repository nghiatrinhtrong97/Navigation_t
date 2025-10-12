#include "../include/integrated_navigation_controller.h"
#include <QDebug>

namespace nav {

IntegratedNavigationController::IntegratedNavigationController(QObject *parent)
    : QObject(parent)
    , m_servicesInitialized(false)
    , m_navigationActive(false)
    , m_currentPosition(DEFAULT_LAT, DEFAULT_LON)
    , m_currentHeading(0.0)
    , m_currentSpeed(0.0)
{
    qDebug() << "🚀 [INTEGRATED CONTROLLER] Creating integrated navigation controller...";
    
    // Create service cores
    m_positioningService = std::make_unique<PositioningServiceCore>(this);
    m_routingService = std::make_unique<RoutingServiceCore>(this);
    m_guidanceService = std::make_unique<GuidanceServiceCore>(this);
    m_mapService = std::make_unique<MapServiceCore>(this);
    
    qDebug() << "✅ [INTEGRATED CONTROLLER] Service cores created";
}

IntegratedNavigationController::~IntegratedNavigationController()
{
    shutdownServices();
}

bool IntegratedNavigationController::initializeServices()
{
    QMutexLocker locker(&m_mutex);
    
    if (m_servicesInitialized) {
        qDebug() << "⚠️ [INTEGRATED CONTROLLER] Services already initialized";
        return true;
    }
    
    qDebug() << "🚀 [INTEGRATED CONTROLLER] Initializing all service cores...";
    
    // Initialize all services
    bool allInitialized = true;
    
    if (!m_positioningService->initialize()) {
        qWarning() << "❌ [INTEGRATED CONTROLLER] Failed to initialize positioning service";
        allInitialized = false;
    }
    
    if (!m_routingService->initialize()) {
        qWarning() << "❌ [INTEGRATED CONTROLLER] Failed to initialize routing service";
        allInitialized = false;
    }
    
    if (!m_guidanceService->initialize()) {
        qWarning() << "❌ [INTEGRATED CONTROLLER] Failed to initialize guidance service";
        allInitialized = false;
    }
    
    if (!m_mapService->initialize()) {
        qWarning() << "❌ [INTEGRATED CONTROLLER] Failed to initialize map service";
        allInitialized = false;
    }
    
    if (allInitialized) {
        connectServiceSignals();
        m_servicesInitialized = true;
        
        qDebug() << "✅ [INTEGRATED CONTROLLER] All services initialized successfully";
        emit servicesReady(true);
    } else {
        qWarning() << "❌ [INTEGRATED CONTROLLER] Failed to initialize some services";
        emit servicesReady(false);
    }
    
    return allInitialized;
}

bool IntegratedNavigationController::areServicesReady()
{
    QMutexLocker locker(&m_mutex);
    
    return m_servicesInitialized &&
           m_positioningService->isServiceReady() &&
           m_routingService->isServiceReady() &&
           m_guidanceService->isServiceReady() &&
           m_mapService->isServiceReady();
}

std::string IntegratedNavigationController::getServiceStatus()
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_servicesInitialized) {
        return "Services not initialized";
    }
    
    QString status = QString(
        "Positioning: %1\n"
        "Routing: %2\n"
        "Guidance: %3\n"
        "Map: %4"
    ).arg(m_positioningService->getServiceStatus())
     .arg(m_routingService->getServiceStatus())
     .arg(QString::fromStdString(m_guidanceService->getServiceStatus()))
     .arg(m_mapService->getServiceStatus());
    
    return status.toStdString();
}

void IntegratedNavigationController::shutdownServices()
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_servicesInitialized) {
        return;
    }
    
    qDebug() << "🛑 [INTEGRATED CONTROLLER] Shutting down all services...";
    
    if (m_navigationActive) {
        stopNavigation();
    }
    
    disconnectServiceSignals();
    
    m_guidanceService->shutdown();
    m_routingService->shutdown();
    m_positioningService->shutdown();
    m_mapService->shutdown();
    
    m_servicesInitialized = false;
    
    emit servicesReady(false);
    qDebug() << "✅ [INTEGRATED CONTROLLER] All services shut down";
}

bool IntegratedNavigationController::calculateRoute(const Point& start, const Point& end, RoutingCriteria criteria)
{
    if (!areServicesReady()) {
        qWarning() << "❌ [INTEGRATED CONTROLLER] Cannot calculate route - services not ready";
        emit routeCalculationFailed("Services not ready");
        return false;
    }
    
    qDebug() << "🧭 [INTEGRATED CONTROLLER] Calculating route from"
             << start.latitude << "," << start.longitude
             << "to" << end.latitude << "," << end.longitude;
    
    // Use routing service directly
    return m_routingService->calculateRoute(start, end, criteria);
}

bool IntegratedNavigationController::calculateRouteAsync(const Point& start, const Point& end, RoutingCriteria criteria)
{
    if (!areServicesReady()) {
        emit routeCalculationFailed("Services not ready");
        return false;
    }
    
    qDebug() << "🧭 [INTEGRATED CONTROLLER] Starting async route calculation...";
    
    return m_routingService->calculateRouteAsync(start, end, criteria);
}

void IntegratedNavigationController::clearRoute()
{
    QMutexLocker locker(&m_mutex);
    
    if (m_navigationActive) {
        stopNavigation();
    }
    
    m_activeRoute = Route{};
    
    qDebug() << "🗑️ [INTEGRATED CONTROLLER] Route cleared";
}

void IntegratedNavigationController::startNavigation()
{
    QMutexLocker locker(&m_mutex);
    
    if (!areServicesReady()) {
        qWarning() << "❌ [INTEGRATED CONTROLLER] Cannot start navigation - services not ready";
        return;
    }
    
    if (!hasActiveRoute()) {
        qWarning() << "❌ [INTEGRATED CONTROLLER] Cannot start navigation - no active route";
        return;
    }
    
    qDebug() << "🚀 [INTEGRATED CONTROLLER] Starting navigation...";
    
    // Start guidance with current route
    m_guidanceService->startGuidance(m_activeRoute);
    m_navigationActive = true;
    
    emit navigationStarted();
    qDebug() << "✅ [INTEGRATED CONTROLLER] Navigation started";
}

void IntegratedNavigationController::stopNavigation()
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_navigationActive) {
        return;
    }
    
    qDebug() << "🛑 [INTEGRATED CONTROLLER] Stopping navigation...";
    
    m_guidanceService->stopGuidance();
    m_navigationActive = false;
    
    emit navigationStopped();
    qDebug() << "✅ [INTEGRATED CONTROLLER] Navigation stopped";
}

bool IntegratedNavigationController::isNavigating() const
{
    QMutexLocker locker(&m_mutex);
    return m_navigationActive;
}

void IntegratedNavigationController::setCurrentPosition(const Point& position)
{
    m_positioningService->setCurrentPosition(position);
    
    if (m_navigationActive) {
        m_guidanceService->updateCurrentPosition(position);
    }
}

void IntegratedNavigationController::setCurrentHeading(double heading)
{
    m_positioningService->setCurrentHeading(heading);
}

void IntegratedNavigationController::setCurrentSpeed(double speed)
{
    m_positioningService->setCurrentSpeed(speed);
}

Point IntegratedNavigationController::getCurrentPosition() const
{
    return m_positioningService->getCurrentPosition();
}

double IntegratedNavigationController::getCurrentHeading() const
{
    return m_positioningService->getCurrentHeading();
}

double IntegratedNavigationController::getCurrentSpeed() const
{
    return m_positioningService->getCurrentSpeed();
}

bool IntegratedNavigationController::hasActiveRoute() const
{
    QMutexLocker locker(&m_mutex);
    return m_routingService->hasActiveRoute();
}

const Route& IntegratedNavigationController::getActiveRoute() const
{
    return m_routingService->getCurrentRoute();
}

GuidanceInstruction IntegratedNavigationController::getCurrentGuidance() const
{
    return m_guidanceService->getCurrentInstruction();
}

double IntegratedNavigationController::getDistanceToNextManeuver() const
{
    return m_guidanceService->getDistanceToNextManeuver();
}

double IntegratedNavigationController::getRemainingDistance() const
{
    return m_guidanceService->getRemainingDistance();
}

int IntegratedNavigationController::getRemainingTime() const
{
    return m_guidanceService->getRemainingTime();
}

std::vector<POI> IntegratedNavigationController::findNearbyPOIs(const Point& location, double radiusMeters, const QString& category)
{
    return m_mapService->findPOINearLocation(location, radiusMeters, category);
}

std::vector<POI> IntegratedNavigationController::searchPOIs(const QString& searchTerm)
{
    return m_mapService->searchPOI(searchTerm);
}

POI IntegratedNavigationController::getPOIById(uint32_t poiId)
{
    return m_mapService->getPOIById(poiId);
}

void IntegratedNavigationController::connectServiceSignals()
{
    // Positioning service signals
    connect(m_positioningService.get(), &PositioningServiceCore::positionChanged,
            this, &IntegratedNavigationController::onPositionChanged);
    connect(m_positioningService.get(), &PositioningServiceCore::headingChanged,
            this, &IntegratedNavigationController::onHeadingChanged);
    connect(m_positioningService.get(), &PositioningServiceCore::speedChanged,
            this, &IntegratedNavigationController::onSpeedChanged);
    
    // Routing service signals
    connect(m_routingService.get(), &RoutingServiceCore::routeCalculated,
            this, &IntegratedNavigationController::onRouteCalculated);
    connect(m_routingService.get(), &RoutingServiceCore::routeCalculationFailed,
            this, &IntegratedNavigationController::onRouteCalculationFailed);
    
    // Guidance service signals
    connect(m_guidanceService.get(), &GuidanceServiceCore::guidanceInstructionUpdated,
            this, &IntegratedNavigationController::onGuidanceUpdated);
    connect(m_guidanceService.get(), &GuidanceServiceCore::destinationReached,
            this, &IntegratedNavigationController::onDestinationReached);
    
    qDebug() << "🔗 [INTEGRATED CONTROLLER] Service signals connected";
}

void IntegratedNavigationController::disconnectServiceSignals()
{
    // Disconnect all signals
    disconnect(m_positioningService.get(), nullptr, this, nullptr);
    disconnect(m_routingService.get(), nullptr, this, nullptr);
    disconnect(m_guidanceService.get(), nullptr, this, nullptr);
    disconnect(m_mapService.get(), nullptr, this, nullptr);
    
    qDebug() << "🔌 [INTEGRATED CONTROLLER] Service signals disconnected";
}

void IntegratedNavigationController::onPositionChanged(const Point& position)
{
    QMutexLocker locker(&m_mutex);
    m_currentPosition = position;
    emit positionChanged(position, m_currentHeading, m_currentSpeed);
}

void IntegratedNavigationController::onHeadingChanged(double heading)
{
    QMutexLocker locker(&m_mutex);
    m_currentHeading = heading;
    emit positionChanged(m_currentPosition, heading, m_currentSpeed);
}

void IntegratedNavigationController::onSpeedChanged(double speed)
{
    QMutexLocker locker(&m_mutex);
    m_currentSpeed = speed;
    emit positionChanged(m_currentPosition, m_currentHeading, speed);
}

void IntegratedNavigationController::onRouteCalculated(const Route& route)
{
    QMutexLocker locker(&m_mutex);
    m_activeRoute = route;
    
    qDebug() << "✅ [INTEGRATED CONTROLLER] Route calculated successfully:"
             << route.total_distance_meters << "meters";
    
    emit routeCalculated(route);
}

void IntegratedNavigationController::onRouteCalculationFailed(const QString& error)
{
    qWarning() << "❌ [INTEGRATED CONTROLLER] Route calculation failed:" << error;
    emit routeCalculationFailed(error);
}

void IntegratedNavigationController::onGuidanceUpdated(const GuidanceInstruction& instruction)
{
    emit guidanceUpdated(instruction);
}

void IntegratedNavigationController::onDestinationReached()
{
    qDebug() << "🎯 [INTEGRATED CONTROLLER] Destination reached!";
    
    QMutexLocker locker(&m_mutex);
    m_navigationActive = false;
    
    emit destinationReached();
    emit navigationStopped();
}

} // namespace nav