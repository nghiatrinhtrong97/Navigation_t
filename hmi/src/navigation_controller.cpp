#include "../include/navigation_controller.h"
#include <QDebug>
#include <QTimer>
#include <QThread>
#include <cmath>

namespace nav {

NavigationController::NavigationController(QObject *parent)
    : QObject(parent)
    , m_simulationTimer(nullptr)
    , m_simulationSpeed(5)
    , m_simulationProgress(0.0)
    , m_hasActiveRoute(false)
    , m_simulationRunning(false)
{
    // Initialize with default position (Hanoi, Vietnam)
    m_currentPosition = Point(DEFAULT_LAT, DEFAULT_LON);
    m_currentHeading = 0.0;
    m_currentSpeed = 0.0;
    
    // Setup simulation timer
    m_simulationTimer = new QTimer(this);
    connect(m_simulationTimer, &QTimer::timeout,
            this, &NavigationController::updateSimulation);
    
    qDebug() << "Navigation controller initialized";
}

NavigationController::~NavigationController()
{
    if (m_simulationRunning) {
        stopSimulation();
    }
}

bool NavigationController::calculateRoute(const Point& start, const Point& end)
{
    // Simulate route calculation
    qDebug() << "Calculating route from" << start.latitude << "," << start.longitude
             << "to" << end.latitude << "," << end.longitude;
    
    // Create a simple route for demonstration
    Route route;
    route.route_id = 1;
    route.node_count = 0;
    route.total_distance_meters = calculateDistance(start, end);
    route.estimated_time_seconds = static_cast<uint32_t>(route.total_distance_meters / 13.89); // ~50 km/h average
    
    // Generate intermediate waypoints
    generateRoutePoints(start, end, route);
    
    m_activeRoute = route;
    m_hasActiveRoute = true;
    m_simulationProgress = 0.0;
    
    emit routeCalculated(route);
    
    qDebug() << "Route calculated:" << route.node_count << "points,"
             << route.total_distance_meters / 1000.0 << "km,"
             << route.estimated_time_seconds / 60.0 << "minutes";
    
    return true;
}

void NavigationController::clearRoute()
{
    if (m_simulationRunning) {
        stopSimulation();
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
    if (!m_hasActiveRoute || m_simulationRunning) {
        qDebug() << "Cannot start simulation: no route or already running";
        return;
    }
    
    m_simulationRunning = true;
    m_currentRouteIndex = 0;
    m_simulationProgress = 0.0;
    
    // Start simulation timer (update every 100ms)
    int interval = 100;
    m_simulationTimer->start(interval);
    
    qDebug() << "Simulation started with speed" << m_simulationSpeed << "x";
}

void NavigationController::stopSimulation()
{
    if (!m_simulationRunning) {
        return;
    }
    
    m_simulationRunning = false;
    m_simulationTimer->stop();
    m_currentSpeed = 0.0;
    
    qDebug() << "Simulation stopped";
}

void NavigationController::setSimulationSpeed(int speed)
{
    m_simulationSpeed = qBound(1, speed, 10);
    qDebug() << "Simulation speed set to" << m_simulationSpeed << "x";
}

void NavigationController::setCurrentPosition(const Point& position)
{
    m_currentPosition = position;
    emit positionChanged(position, m_currentHeading, m_currentSpeed);
}

Point NavigationController::getCurrentPosition() const
{
    return m_currentPosition;
}

bool NavigationController::hasActiveRoute() const
{
    return m_hasActiveRoute;
}

const Route& NavigationController::getActiveRoute() const
{
    return m_activeRoute;
}

GuidanceInstruction NavigationController::getCurrentGuidance() const
{
    GuidanceInstruction instruction;
    
    if (!m_hasActiveRoute || m_routePoints.empty()) {
        strcpy(instruction.instruction_text, "No active route");
        instruction.distance_to_turn_meters = 0;
        instruction.turn_type = GuidanceInstruction::STRAIGHT;
        return instruction;
    }
    
    // Calculate guidance based on current position and route
    if (m_currentRouteIndex < m_routePoints.size() - 1) {
        Point nextPoint = m_routePoints[m_currentRouteIndex + 1];
        double distance = calculateDistance(m_currentPosition, nextPoint);
        
        if (distance < 100) { // Within 100m of turn
            snprintf(instruction.instruction_text, sizeof(instruction.instruction_text),
                    "In %d meters, turn right", static_cast<int>(distance));
            instruction.turn_type = GuidanceInstruction::TURN_RIGHT;
        } else if (distance < 500) { // Within 500m
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
    
    return instruction;
}

double NavigationController::getSimulationProgress() const
{
    return m_simulationProgress;
}

void NavigationController::updateSimulation()
{
    if (!m_simulationRunning || !m_hasActiveRoute || m_routePoints.empty()) {
        return;
    }
    
    // Calculate simulation step based on speed
    double stepSize = 0.001 * m_simulationSpeed; // Adjust based on speed multiplier
    m_simulationProgress += stepSize;
    
    if (m_simulationProgress >= 1.0) {
        // Reached destination
        m_simulationProgress = 1.0;
        stopSimulation();
        return;
    }
    
    // Interpolate position along route
    updatePositionAlongRoute();
    
    // Update guidance
    GuidanceInstruction instruction = getCurrentGuidance();
    emit guidanceUpdated(instruction);
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
    int numPoints = 20; // Number of intermediate points
    
    m_routePoints.push_back(start);
    
    // Add some curve to make it more interesting
    for (int i = 1; i < numPoints; i++) {
        double t = static_cast<double>(i) / numPoints;
        
        // Linear interpolation with some offset for curves
        double lat = start.latitude + t * (end.latitude - start.latitude);
        double lon = start.longitude + t * (end.longitude - start.longitude);
        
        // Add some curve variation
        double curve = 0.002 * sin(t * M_PI * 3); // Small sinusoidal variation
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
    // Haversine formula for great circle distance
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