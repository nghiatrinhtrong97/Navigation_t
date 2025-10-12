#include "../include/guidance_service_core.h"
#include <QDebug>
#include <cmath>
#include <cstring>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace nav {

GuidanceServiceCore::GuidanceServiceCore(QObject* parent)
    : QObject(parent)
    , m_initialized(false)
    , m_serviceReady(false)
    , m_guidanceActive(false)
    , m_currentRouteIndex(0)
    , m_distanceToNextManeuver(0.0)
    , m_remainingDistance(0.0)
    , m_remainingTime(0)
    , m_guidanceTimer(new QTimer(this))
{
    // Setup guidance update timer
    connect(m_guidanceTimer, &QTimer::timeout, this, &GuidanceServiceCore::updateGuidance);
    m_guidanceTimer->setInterval(1000); // 1 second updates
    
    // Initialize current instruction
    m_currentInstruction.turn_type = TurnType::STRAIGHT;
    m_currentInstruction.target_node_id = 0;
    m_currentInstruction.distance_to_turn_meters = 0.0;
    strcpy_s(m_currentInstruction.instruction_text, sizeof(m_currentInstruction.instruction_text), "No guidance");
}

GuidanceServiceCore::~GuidanceServiceCore()
{
    shutdown();
}

bool GuidanceServiceCore::initialize()
{
    if (m_initialized) {
        return true;
    }
    
    qDebug() << "🚀 [GUIDANCE SERVICE CORE] Initializing guidance service...";
    
    try {
        // Initialize guidance state
        m_serviceReady = true;
        m_initialized = true;
        
        qDebug() << "✅ [GUIDANCE SERVICE CORE] Guidance service initialized successfully";
        return true;
    } catch (const std::exception& e) {
        qWarning() << "❌ [GUIDANCE SERVICE CORE] Failed to initialize:" << e.what();
        return false;
    }
}

void GuidanceServiceCore::shutdown()
{
    if (!m_initialized) {
        return;
    }
    
    qDebug() << "🛑 [GUIDANCE SERVICE CORE] Shutting down guidance service...";
    
    stopGuidance();
    m_serviceReady = false;
    m_initialized = false;
    
    qDebug() << "✅ [GUIDANCE SERVICE CORE] Guidance service shut down";
}

bool GuidanceServiceCore::isServiceReady() const
{
    return m_serviceReady;
}

std::string GuidanceServiceCore::getServiceStatus() const
{
    if (!m_initialized) {
        return "Not initialized";
    }
    
    if (m_guidanceActive) {
        return "Active - Providing guidance";
    }
    
    return "Ready - Waiting for navigation";
}

bool GuidanceServiceCore::startGuidance(const Route& route)
{
    if (!m_serviceReady) {
        qWarning() << "❌ [GUIDANCE SERVICE CORE] Cannot start guidance - service not ready";
        return false;
    }
    
    if (route.node_count == 0) {
        qWarning() << "❌ [GUIDANCE SERVICE CORE] Cannot start guidance - empty route";
        return false;
    }
    
    qDebug() << "🚀 [GUIDANCE SERVICE CORE] Starting guidance for route with" << route.node_count << "nodes";
    
    m_activeRoute = route;
    m_currentRouteIndex = 0;
    m_guidanceActive = true;
    m_remainingDistance = route.total_distance_meters;
    m_remainingTime = route.estimated_time_seconds;
    
    // Generate initial instruction
    GuidanceInstruction instruction;
    instruction.turn_type = TurnType::STRAIGHT;
    instruction.target_node_id = 0;
    instruction.distance_to_turn_meters = 0.0;
    strcpy_s(instruction.instruction_text, sizeof(instruction.instruction_text), "Starting navigation - proceed to route");
    
    m_currentInstruction = instruction;
    
    // Start guidance timer
    m_guidanceTimer->start();
    
    emit guidanceInstructionUpdated(instruction);
    
    qDebug() << "✅ [GUIDANCE SERVICE CORE] Guidance started successfully";
    return true;
}

bool GuidanceServiceCore::stopGuidance()
{
    if (!m_guidanceActive) {
        return true;
    }
    
    qDebug() << "🛑 [GUIDANCE SERVICE CORE] Stopping guidance...";
    
    m_guidanceActive = false;
    m_guidanceTimer->stop();
    
    // Clear guidance state
    m_currentRouteIndex = 0;
    m_distanceToNextManeuver = 0.0;
    m_remainingDistance = 0.0;
    m_remainingTime = 0;
    
    // Reset instruction
    m_currentInstruction.turn_type = TurnType::STRAIGHT;
    m_currentInstruction.target_node_id = 0;
    m_currentInstruction.distance_to_turn_meters = 0.0;
    strcpy_s(m_currentInstruction.instruction_text, sizeof(m_currentInstruction.instruction_text), "Navigation stopped");
    
    qDebug() << "✅ [GUIDANCE SERVICE CORE] Guidance stopped";
    return true;
}

bool GuidanceServiceCore::updateCurrentPosition(const Point& position)
{
    if (!m_guidanceActive) {
        return false;
    }
    
    m_currentPosition = position;
    
    // Calculate distance to next maneuver and update guidance
    updateGuidanceFromPosition();
    
    return true;
}

void GuidanceServiceCore::updateGuidance()
{
    if (!m_guidanceActive || m_activeRoute.node_count == 0) {
        return;
    }
    
    // Simulate progress along route
    updateGuidanceFromPosition();
}

GuidanceInstruction GuidanceServiceCore::getCurrentInstruction() const
{
    return m_currentInstruction;
}

double GuidanceServiceCore::getDistanceToNextManeuver() const
{
    return m_distanceToNextManeuver;
}

double GuidanceServiceCore::getRemainingDistance() const
{
    return m_remainingDistance;
}

int GuidanceServiceCore::getRemainingTime() const
{
    return m_remainingTime;
}

void GuidanceServiceCore::updateGuidanceFromPosition()
{
    if (!m_guidanceActive || m_activeRoute.node_count == 0) {
        return;
    }
    
    // Simulate guidance progress
    static double progressMeters = 0.0;
    progressMeters += 10.0; // Simulate 10 meters per second progress
    
    // Update remaining distance and time
    m_remainingDistance = std::max(0.0, m_activeRoute.total_distance_meters - progressMeters);
    m_remainingTime = static_cast<int>(m_remainingDistance / 10.0); // Simplified time calculation
    
    // Check if destination reached
    if (m_remainingDistance <= 0.0) {
        // Destination reached
        GuidanceInstruction instruction;
        instruction.turn_type = TurnType::DESTINATION_REACHED;
        instruction.target_node_id = 0;
        instruction.distance_to_turn_meters = 0.0;
        strcpy_s(instruction.instruction_text, sizeof(instruction.instruction_text), "Destination reached");
        
        m_currentInstruction = instruction;
        m_distanceToNextManeuver = 0.0;
        
        emit guidanceInstructionUpdated(instruction);
        emit destinationReached();
        
        stopGuidance();
        return;
    }
    
    // Generate next instruction based on progress
    GuidanceInstruction instruction = generateInstruction(progressMeters);
    
    if (strcmp(instruction.instruction_text, m_currentInstruction.instruction_text) != 0 ||
        instruction.turn_type != m_currentInstruction.turn_type) {
        
        m_currentInstruction = instruction;
        emit guidanceInstructionUpdated(instruction);
    }
}

GuidanceInstruction GuidanceServiceCore::generateInstruction(double progressMeters) const
{
    GuidanceInstruction instruction;
    
    double totalDistance = m_activeRoute.total_distance_meters;
    double progressRatio = progressMeters / totalDistance;
    
    // Simple instruction generation based on progress
    if (progressRatio < 0.1) {
        instruction.turn_type = TurnType::STRAIGHT;
        strcpy_s(instruction.instruction_text, sizeof(instruction.instruction_text), "Continue straight");
        instruction.distance_to_turn_meters = totalDistance * 0.1 - progressMeters;
    } else if (progressRatio < 0.3) {
        instruction.turn_type = TurnType::RIGHT;
        strcpy_s(instruction.instruction_text, sizeof(instruction.instruction_text), "Turn right in 500 meters");
        instruction.distance_to_turn_meters = totalDistance * 0.3 - progressMeters;
    } else if (progressRatio < 0.5) {
        instruction.turn_type = TurnType::STRAIGHT;
        strcpy_s(instruction.instruction_text, sizeof(instruction.instruction_text), "Continue straight");
        instruction.distance_to_turn_meters = totalDistance * 0.5 - progressMeters;
    } else if (progressRatio < 0.7) {
        instruction.turn_type = TurnType::LEFT;
        strcpy_s(instruction.instruction_text, sizeof(instruction.instruction_text), "Turn left in 300 meters");
        instruction.distance_to_turn_meters = totalDistance * 0.7 - progressMeters;
    } else if (progressRatio < 0.9) {
        instruction.turn_type = TurnType::STRAIGHT;
        strcpy_s(instruction.instruction_text, sizeof(instruction.instruction_text), "Continue straight");
        instruction.distance_to_turn_meters = totalDistance * 0.9 - progressMeters;
    } else {
        instruction.turn_type = TurnType::DESTINATION_REACHED;
        strcpy_s(instruction.instruction_text, sizeof(instruction.instruction_text), "Approaching destination");
        instruction.distance_to_turn_meters = totalDistance - progressMeters;
    }
    
    instruction.target_node_id = static_cast<uint32_t>(progressRatio * m_activeRoute.node_count);
    instruction.distance_to_turn_meters = std::max(0.0, instruction.distance_to_turn_meters);
    
    return instruction;
}

double GuidanceServiceCore::calculateDistance(const Point& p1, const Point& p2) const
{
    // Haversine formula for distance calculation
    const double R = 6371000.0; // Earth radius in meters
    
    double lat1_rad = p1.latitude * M_PI / 180.0;
    double lat2_rad = p2.latitude * M_PI / 180.0;
    double dlat = (p2.latitude - p1.latitude) * M_PI / 180.0;
    double dlon = (p2.longitude - p1.longitude) * M_PI / 180.0;
    
    double a = sin(dlat/2) * sin(dlat/2) +
               cos(lat1_rad) * cos(lat2_rad) * sin(dlon/2) * sin(dlon/2);
    double c = 2 * atan2(sqrt(a), sqrt(1-a));
    
    return R * c;
}

double GuidanceServiceCore::calculateBearing(const Point& from, const Point& to) const
{
    double lat1_rad = from.latitude * M_PI / 180.0;
    double lat2_rad = to.latitude * M_PI / 180.0;
    double dlon_rad = (to.longitude - from.longitude) * M_PI / 180.0;
    
    double y = sin(dlon_rad) * cos(lat2_rad);
    double x = cos(lat1_rad) * sin(lat2_rad) - sin(lat1_rad) * cos(lat2_rad) * cos(dlon_rad);
    
    double bearing_rad = atan2(y, x);
    double bearing_deg = bearing_rad * 180.0 / M_PI;
    
    // Normalize to 0-360 degrees
    return fmod(bearing_deg + 360.0, 360.0);
}

} // namespace nav