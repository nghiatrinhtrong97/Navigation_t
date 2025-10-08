#include "../include/navigation_controller_v2.h"
#include <QDebug>
#include <QMutexLocker>
#include <cmath>

namespace nav {

NavigationController::NavigationController(QObject *parent)
    : QObject(parent)
    , m_serviceManager(nullptr)
    , m_operationMode(SIMULATION_MODE)
    , m_simulationTimer(nullptr)
    , m_simulationSpeed(5)
    , m_simulationProgress(0.0)
    , m_simulationRunning(false)
    , m_currentRouteIndex(0)
    , m_currentPosition(DEFAULT_LAT, DEFAULT_LON)
    , m_currentHeading(0.0)
    , m_currentSpeed(0.0)
    , m_hasActiveRoute(false)
{
    // Initialize service manager
    m_serviceManager = std::make_shared<ServiceManager>();
    
    // Setup simulation timer (fallback)
    m_simulationTimer = new QTimer(this);
    connect(m_simulationTimer, &QTimer::timeout,
            this, &NavigationController::updateSimulation);
    
    // Try to initialize services
    if (initializeServices()) {
        setOperationMode(SERVICE_MODE);
        qDebug() << "Navigation controller initialized in SERVICE mode";
    } else {
        setOperationMode(SIMULATION_MODE);
        qDebug() << "Navigation controller initialized in SIMULATION mode (services unavailable)";
    }
}

NavigationController::~NavigationController()
{
    if (m_simulationRunning) {
        stopSimulation();
    }
    
    if (m_serviceManager) {
        m_serviceManager->shutdown();
    }
}

bool NavigationController::initializeServices()
{
    if (!m_serviceManager->initialize()) {
        qDebug() << "Failed to initialize service manager";
        return false;
    }
    
    // Get service instances
    m_positioningService = m_serviceManager->getPositioningService();
    m_routingService = m_serviceManager->getRoutingService();
    m_guidanceService = m_serviceManager->getGuidanceService();
    m_mapService = m_serviceManager->getMapService();
    
    if (!m_positioningService || !m_routingService || !m_guidanceService || !m_mapService) {
        qDebug() << "Failed to create service instances";
        return false;
    }
    
    // Setup service callbacks
    m_positioningService->setPositionCallback(
        [this](const Point& pos, double heading, double speed) {
            onServicePositionUpdate(pos, heading, speed);
        });
    
    m_routingService->setRouteCallback(
        [this](const Route& route) {
            onServiceRouteCalculated(route);
        });
    
    m_routingService->setRouteErrorCallback(
        [this](const std::string& error) {
            onServiceRouteError(error);
        });
    
    m_guidanceService->setGuidanceCallback(
        [this](const GuidanceInstruction& instruction) {
            onServiceGuidanceUpdate(instruction);
        });
    
    qDebug() << "Services initialized successfully";
    return true;
}

bool NavigationController::areServicesConnected()
{
    if (!m_serviceManager) {
        return false;
    }
    
    return m_serviceManager->areAllServicesConnected();
}

std::string NavigationController::getServiceStatus()
{
    if (!m_serviceManager) {
        return "Service manager not available";
    }
    
    return m_serviceManager->getServiceStatus();
}

void NavigationController::setOperationMode(OperationMode mode)
{
    QMutexLocker locker(&m_mutex);
    
    if (m_operationMode == mode) {
        return;
    }
    
    // Stop current operations
    if (m_simulationRunning) {
        stopSimulation();
    }
    
    m_operationMode = mode;
    
    if (mode == SERVICE_MODE) {
        qDebug() << "Switched to SERVICE mode";
        // Start positioning service if available
        if (m_positioningService && m_positioningService->isConnected()) {
            m_positioningService->startPositioning();
        }
    } else {
        qDebug() << "Switched to SIMULATION mode";
        // Stop services
        if (m_positioningService) {
            m_positioningService->stopPositioning();
        }
        if (m_guidanceService) {
            m_guidanceService->stopGuidance();
        }
    }
}

NavigationController::OperationMode NavigationController::getOperationMode() const
{
    QMutexLocker locker(&m_mutex);
    return m_operationMode;
}

bool NavigationController::calculateRoute(const Point& start, const Point& end)
{
    QMutexLocker locker(&m_mutex);
    
    qDebug() << "Calculating route from" << start.latitude << "," << start.longitude
             << "to" << end.latitude << "," << end.longitude;
    
    if (m_operationMode == SERVICE_MODE && m_routingService && m_routingService->isConnected()) {
        // Use real routing service
        RoutingRequest request;
        request.request_type = RoutingRequest::CALCULATE;
        request.start_point = start;
        request.end_point = end;
        request.algorithm_type = RoutingRequest::ASTAR;
        request.optimization_type = RoutingRequest::SHORTEST_TIME;
        
        return m_routingService->calculateRoute(request);
    } else {
        // Fallback to simulation
        Route route;
        route.route_id = 1;
        route.node_count = 0;
        route.total_distance_meters = calculateDistance(start, end);
        route.estimated_time_seconds = static_cast<uint32_t>(route.total_distance_meters / 13.89); // ~50 km/h
        
        generateRoutePoints(start, end, route);
        
        m_activeRoute = route;
        m_hasActiveRoute = true;
        m_simulationProgress = 0.0;
        
        // Emit signal with delay to simulate calculation time
        QTimer::singleShot(500, [this, route]() {
            emit routeCalculated(route);
        });
        
        return true;
    }
}

void NavigationController::clearRoute()
{
    QMutexLocker locker(&m_mutex);
    
    if (m_simulationRunning) {
        stopSimulation();
    }
    
    if (m_operationMode == SERVICE_MODE && m_guidanceService) {
        m_guidanceService->stopGuidance();
    }
    
    m_hasActiveRoute = false;
    m_activeRoute = Route{};
    m_routePoints.clear();
    m_currentRouteIndex = 0;
    m_simulationProgress = 0.0;
    
    qDebug() << "Route cleared";
}

void NavigationController::startSimulation()
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_hasActiveRoute || m_simulationRunning) {
        qDebug() << "Cannot start simulation: no route or already running";
        return;
    }
    
    m_simulationRunning = true;
    m_currentRouteIndex = 0;
    m_simulationProgress = 0.0;
    
    if (m_operationMode == SERVICE_MODE) {
        // Start real guidance service
        if (m_guidanceService && m_guidanceService->isConnected()) {
            m_guidanceService->startGuidance(m_activeRoute);
        }
        
        // Set initial position in positioning service
        if (m_positioningService && m_positioningService->isConnected() && !m_routePoints.empty()) {
            m_positioningService->setPosition(m_routePoints[0]);
        }
    } else {
        // Start simulation timer
        m_simulationTimer->start(100); // 10 Hz update rate
    }
    
    qDebug() << "Simulation started with speed" << m_simulationSpeed << "x";
}

void NavigationController::stopSimulation()
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_simulationRunning) {
        return;
    }
    
    m_simulationRunning = false;
    m_simulationTimer->stop();
    m_currentSpeed = 0.0;
    
    if (m_operationMode == SERVICE_MODE && m_guidanceService) {
        m_guidanceService->stopGuidance();
    }
    
    qDebug() << "Simulation stopped";
}

void NavigationController::setSimulationSpeed(int speed)
{
    QMutexLocker locker(&m_mutex);
    m_simulationSpeed = qBound(1, speed, 10);
    qDebug() << "Simulation speed set to" << m_simulationSpeed << "x";
}

void NavigationController::setCurrentPosition(const Point& position)
{
    QMutexLocker locker(&m_mutex);
    
    m_currentPosition = position;
    
    if (m_operationMode == SERVICE_MODE) {
        // Update positioning service
        if (m_positioningService && m_positioningService->isConnected()) {
            m_positioningService->setPosition(position);
        }
        
        // Update guidance service
        if (m_guidanceService && m_guidanceService->isConnected() && m_hasActiveRoute) {
            m_guidanceService->updatePosition(position);
        }
    }
    
    emit positionChanged(position, m_currentHeading, m_currentSpeed);
}

Point NavigationController::getCurrentPosition() const
{
    QMutexLocker locker(&m_mutex);
    return m_currentPosition;
}

bool NavigationController::hasActiveRoute() const
{
    QMutexLocker locker(&m_mutex);
    return m_hasActiveRoute;
}

const Route& NavigationController::getActiveRoute() const
{
    QMutexLocker locker(&m_mutex);
    return m_activeRoute;
}

GuidanceInstruction NavigationController::getCurrentGuidance() const
{
    QMutexLocker locker(&m_mutex);
    
    if (m_operationMode == SERVICE_MODE && m_guidanceService) {
        return m_guidanceService->getCurrentInstruction();
    } else {
        return m_currentGuidance;
    }
}

double NavigationController::getSimulationProgress() const
{
    QMutexLocker locker(&m_mutex);
    return m_simulationProgress;
}

void NavigationController::updateSimulation()
{
    if (!m_simulationRunning || !m_hasActiveRoute || m_routePoints.empty()) {
        return;
    }
    
    // Only update in simulation mode
    if (m_operationMode != SIMULATION_MODE) {
        return;
    }
    
    // Calculate simulation step based on speed
    double stepSize = 0.001 * m_simulationSpeed;
    m_simulationProgress += stepSize;
    
    if (m_simulationProgress >= 1.0) {
        m_simulationProgress = 1.0;
        stopSimulation();
        return;
    }
    
    updatePositionAlongRoute();
    
    // Update guidance in simulation mode
    GuidanceInstruction instruction;
    if (m_currentRouteIndex < m_routePoints.size() - 1) {
        Point nextPoint = m_routePoints[m_currentRouteIndex + 1];
        double distance = calculateDistance(m_currentPosition, nextPoint);
        
        if (distance < 100) {
            snprintf(instruction.instruction_text, sizeof(instruction.instruction_text),
                    "In %d meters, turn right", static_cast<int>(distance));
            instruction.turn_type = GuidanceInstruction::TURN_RIGHT;
        } else {
            strcpy(instruction.instruction_text, "Continue straight");
            instruction.turn_type = GuidanceInstruction::STRAIGHT;
        }
        
        instruction.distance_to_turn_meters = static_cast<uint32_t>(distance);
    } else {
        strcpy(instruction.instruction_text, "You have arrived at your destination");
        instruction.distance_to_turn_meters = 0;
        instruction.turn_type = GuidanceInstruction::DESTINATION;
    }
    
    m_currentGuidance = instruction;
    emit guidanceUpdated(instruction);
}

void NavigationController::onServicePositionUpdate(const Point& position, double heading, double speed)
{
    QMutexLocker locker(&m_mutex);
    
    m_currentPosition = position;
    m_currentHeading = heading;
    m_currentSpeed = speed;
    
    emit positionChanged(position, heading, speed);
}

void NavigationController::onServiceRouteCalculated(const Route& route)
{
    QMutexLocker locker(&m_mutex);
    
    m_activeRoute = route;
    m_hasActiveRoute = true;
    m_simulationProgress = 0.0;
    
    qDebug() << "Route calculated via service:" << route.node_count << "points,"
             << route.total_distance_meters / 1000.0 << "km";
    
    emit routeCalculated(route);
}

void NavigationController::onServiceGuidanceUpdate(const GuidanceInstruction& instruction)
{
    QMutexLocker locker(&m_mutex);
    
    m_currentGuidance = instruction;
    emit guidanceUpdated(instruction);
}

void NavigationController::onServiceRouteError(const std::string& error)
{
    qDebug() << "Route calculation error:" << error.c_str();
    
    // Fallback to simulation mode for this calculation
    if (m_operationMode == SERVICE_MODE) {
        qDebug() << "Falling back to simulation for route calculation";
        // The route calculation will be retried in simulation mode
    }
}

void NavigationController::updatePositionAlongRoute()
{
    if (m_routePoints.empty()) {
        return;
    }
    
    // Find current segment based on simulation progress
    size_t totalSegments = m_routePoints.size() - 1;
    if (totalSegments == 0) {
        return;
    }
    
    double segmentProgress = m_simulationProgress * totalSegments;
    size_t currentSegment = static_cast<size_t>(segmentProgress);
    
    if (currentSegment >= totalSegments) {
        currentSegment = totalSegments - 1;
        m_currentPosition = m_routePoints.back();
        m_currentSpeed = 0.0;
    } else {
        // Interpolate between current and next point
        double t = segmentProgress - currentSegment;
        Point start = m_routePoints[currentSegment];
        Point end = m_routePoints[currentSegment + 1];
        
        m_currentPosition.latitude = start.latitude + t * (end.latitude - start.latitude);
        m_currentPosition.longitude = start.longitude + t * (end.longitude - start.longitude);
        
        // Calculate heading (bearing to next point)
        m_currentHeading = calculateBearing(start, end);
        m_currentSpeed = 50.0; // km/h simulation speed
    }
    
    m_currentRouteIndex = currentSegment;
    
    emit positionChanged(m_currentPosition, m_currentHeading, m_currentSpeed);
}

void NavigationController::generateRoutePoints(const Point& start, const Point& end, Route& route)
{
    m_routePoints.clear();
    
    // Generate intermediate points for a more realistic route
    int numPoints = 20;
    
    m_routePoints.push_back(start);
    
    // Add some curve to make it more interesting
    for (int i = 1; i < numPoints; i++) {
        double t = static_cast<double>(i) / numPoints;
        
        // Linear interpolation with some offset for curves
        double lat = start.latitude + t * (end.latitude - start.latitude);
        double lon = start.longitude + t * (end.longitude - start.longitude);
        
        // Add some curve variation
        double curve = 0.002 * sin(t * M_PI * 3);
        lat += curve;
        lon += curve * 0.5;
        
        m_routePoints.push_back(Point(lat, lon));
    }
    
    m_routePoints.push_back(end);
    
    route.node_count = m_routePoints.size();
    m_currentRouteIndex = 0;
    
    qDebug() << "Generated route with" << m_routePoints.size() << "points";
}

double NavigationController::calculateDistance(const Point& p1, const Point& p2) const
{
    // Haversine formula
    const double R = 6371000; // Earth radius in meters
    
    double lat1Rad = p1.latitude * M_PI / 180.0;
    double lat2Rad = p2.latitude * M_PI / 180.0;
    double deltaLatRad = (p2.latitude - p1.latitude) * M_PI / 180.0;
    double deltaLonRad = (p2.longitude - p1.longitude) * M_PI / 180.0;
    
    double a = sin(deltaLatRad / 2) * sin(deltaLatRad / 2) +
               cos(lat1Rad) * cos(lat2Rad) *
               sin(deltaLonRad / 2) * sin(deltaLonRad / 2);
    double c = 2 * atan2(sqrt(a), sqrt(1 - a));
    
    return R * c;
}

double NavigationController::calculateBearing(const Point& from, const Point& to) const
{
    double lat1 = from.latitude * M_PI / 180.0;
    double lat2 = to.latitude * M_PI / 180.0;
    double deltaLon = (to.longitude - from.longitude) * M_PI / 180.0;
    
    double y = sin(deltaLon) * cos(lat2);
    double x = cos(lat1) * sin(lat2) - sin(lat1) * cos(lat2) * cos(deltaLon);
    
    double bearing = atan2(y, x) * 180.0 / M_PI;
    return fmod(bearing + 360.0, 360.0); // Normalize to 0-360 degrees
}

} // namespace nav