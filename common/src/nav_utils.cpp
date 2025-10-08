#include "nav_utils.h"
#include <cmath>
#include <chrono>
#include <sstream>
#include <iomanip>

namespace nav {

double NavUtils::degToRad(double degrees) {
    return degrees * M_PI / 180.0;
}

double NavUtils::radToDeg(double radians) {
    return radians * 180.0 / M_PI;
}

double NavUtils::normalizeAngle(double angle) {
    while (angle < 0.0) angle += 360.0;
    while (angle >= 360.0) angle -= 360.0;
    return angle;
}

double NavUtils::haversineDistance(const Point& p1, const Point& p2) {
    const double R = 6371000.0; // Earth radius in meters
    const double dLat = degToRad(p2.latitude - p1.latitude);
    const double dLon = degToRad(p2.longitude - p1.longitude);
    
    const double a = sin(dLat/2) * sin(dLat/2) +
                    cos(degToRad(p1.latitude)) * cos(degToRad(p2.latitude)) *
                    sin(dLon/2) * sin(dLon/2);
    const double c = 2 * atan2(sqrt(a), sqrt(1-a));
    
    return R * c;
}

double NavUtils::calculateBearing(const Point& from, const Point& to) {
    const double dLon = degToRad(to.longitude - from.longitude);
    const double lat1 = degToRad(from.latitude);
    const double lat2 = degToRad(to.latitude);
    
    const double y = sin(dLon) * cos(lat2);
    const double x = cos(lat1) * sin(lat2) - sin(lat1) * cos(lat2) * cos(dLon);
    
    double bearing = radToDeg(atan2(y, x));
    return normalizeAngle(bearing);
}

Point NavUtils::projectPoint(const Point& start, double bearing_degrees, double distance_meters) {
    const double R = 6371000.0; // Earth radius in meters
    const double lat1 = degToRad(start.latitude);
    const double lon1 = degToRad(start.longitude);
    const double bearing = degToRad(bearing_degrees);
    const double d = distance_meters / R;
    
    const double lat2 = asin(sin(lat1) * cos(d) + cos(lat1) * sin(d) * cos(bearing));
    const double lon2 = lon1 + atan2(sin(bearing) * sin(d) * cos(lat1),
                                    cos(d) - sin(lat1) * sin(lat2));
    
    return Point(radToDeg(lat2), radToDeg(lon2), start.altitude);
}

bool NavUtils::isPointInBoundingBox(const Point& point, const BoundingBox& bbox) {
    return point.latitude >= bbox.minLat && point.latitude <= bbox.maxLat &&
           point.longitude >= bbox.minLon && point.longitude <= bbox.maxLon;
}

uint64_t NavUtils::getCurrentTimestampMs() {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

std::string NavUtils::formatDistance(double meters) {
    if (meters < 1000) {
        return std::to_string(static_cast<int>(meters)) + " m";
    } else {
        double km = meters / 1000.0;
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1) << km << " km";
        return oss.str();
    }
}

std::string NavUtils::formatTime(double seconds) {
    int total_seconds = static_cast<int>(seconds);
    int hours = total_seconds / 3600;
    int minutes = (total_seconds % 3600) / 60;
    
    if (hours > 0) {
        if (minutes > 0) {
            return std::to_string(hours) + " h " + std::to_string(minutes) + " min";
        } else {
            return std::to_string(hours) + " h";
        }
    } else {
        return std::to_string(minutes) + " min";
    }
}

std::string NavUtils::turnTypeToString(TurnType turn) {
    switch (turn) {
        case TurnType::STRAIGHT: return "Continue straight";
        case TurnType::SLIGHT_LEFT: return "Turn slightly left";
        case TurnType::LEFT: return "Turn left";
        case TurnType::SHARP_LEFT: return "Turn sharply left";
        case TurnType::SLIGHT_RIGHT: return "Turn slightly right";
        case TurnType::RIGHT: return "Turn right";
        case TurnType::SHARP_RIGHT: return "Turn sharply right";
        case TurnType::U_TURN: return "Make a U-turn";
        case TurnType::ROUNDABOUT_ENTER: return "Enter roundabout";
        case TurnType::ROUNDABOUT_EXIT: return "Exit roundabout";
        case TurnType::DESTINATION_REACHED: return "You have reached your destination";
        default: return "Continue";
    }
}

Point NavUtils::snapToRoad(const Point& gps_point, const std::vector<MapEdge>& nearby_edges) {
    // Simplified map matching - find closest edge and project point onto it
    // In a real implementation, this would be much more sophisticated
    if (nearby_edges.empty()) {
        return gps_point;
    }
    
    // For now, just return the original point
    // Real implementation would project the point onto the nearest road segment
    return gps_point;
}

} // namespace nav