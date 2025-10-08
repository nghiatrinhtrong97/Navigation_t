#include "guidance_service.h"
#include <cmath>
#include <algorithm>

namespace nav {

RouteMonitoring::RouteMonitoring()
    : current_segment_index_(0)
    , is_off_route_(false)
    , off_route_distance_(0.0)
    , remaining_distance_(0.0)
    , estimated_time_remaining_(0.0)
    , last_update_time_(0)
{
}

void RouteMonitoring::setRoute(const Route& route) {
    current_route_ = route;
    current_segment_index_ = 0;
    is_off_route_ = false;
    off_route_distance_ = 0.0;
    
    // Cache route point positions (in a real implementation, 
    // these would be looked up from the map service)
    route_points_.clear();
    for (int i = 0; i < route.node_count; ++i) {
        // For simulation, create dummy positions
        Point point(21.0 + i * 0.001, 105.8 + i * 0.001);
        route_points_.push_back(point);
    }
    
    updateRemainingDistance();
    updateEstimatedTime();
}

bool RouteMonitoring::updatePosition(const Point& current_position, double heading) {
    uint64_t current_time = NavUtils::getCurrentTimestampMs();
    
    // Check if enough time has passed since last update
    if (current_time - last_update_time_ < 500) { // 0.5 second minimum
        return false;
    }
    
    last_position_ = current_position;
    last_update_time_ = current_time;
    
    if (route_points_.empty()) {
        return false;
    }
    
    // Check if we've reached the next waypoint
    if (current_segment_index_ < static_cast<int>(route_points_.size()) - 1) {
        Point next_waypoint = route_points_[current_segment_index_ + 1];
        
        if (isAtWaypoint(current_position, next_waypoint)) {
            current_segment_index_++;
            is_off_route_ = false;
            std::cout << "Reached waypoint " << (current_segment_index_ + 1) 
                      << " of " << route_points_.size() << std::endl;
        }
    }
    
    // Check if off route
    double distance_to_route = distanceToRouteSegment(current_position, current_segment_index_);
    
    if (distance_to_route > OFF_ROUTE_THRESHOLD_METERS) {
        if (!is_off_route_) {
            is_off_route_ = true;
            off_route_distance_ = distance_to_route;
            std::cout << "Vehicle is off route by " << distance_to_route << " meters" << std::endl;
        }
    } else {
        if (is_off_route_) {
            is_off_route_ = false;
            std::cout << "Vehicle is back on route" << std::endl;
        }
    }
    
    updateRemainingDistance();
    updateEstimatedTime();
    
    return true;
}

double RouteMonitoring::getDistanceToNextWaypoint() const {
    if (current_segment_index_ >= static_cast<int>(route_points_.size()) - 1) {
        return 0.0;
    }
    
    Point next_waypoint = route_points_[current_segment_index_ + 1];
    return NavUtils::haversineDistance(last_position_, next_waypoint);
}

double RouteMonitoring::getRouteProgress() const {
    if (route_points_.empty()) {
        return 0.0;
    }
    
    return static_cast<double>(current_segment_index_) / 
           static_cast<double>(route_points_.size() - 1);
}

void RouteMonitoring::reset() {
    current_segment_index_ = 0;
    is_off_route_ = false;
    off_route_distance_ = 0.0;
    remaining_distance_ = 0.0;
    estimated_time_remaining_ = 0.0;
    route_points_.clear();
}

void RouteMonitoring::updateRemainingDistance() {
    remaining_distance_ = 0.0;
    
    if (route_points_.empty() || current_segment_index_ >= static_cast<int>(route_points_.size()) - 1) {
        return;
    }
    
    // Distance to next waypoint
    remaining_distance_ += getDistanceToNextWaypoint();
    
    // Distance for remaining segments
    for (int i = current_segment_index_ + 1; i < static_cast<int>(route_points_.size()) - 1; ++i) {
        remaining_distance_ += NavUtils::haversineDistance(route_points_[i], route_points_[i + 1]);
    }
}

void RouteMonitoring::updateEstimatedTime() {
    if (remaining_distance_ <= 0.0) {
        estimated_time_remaining_ = 0.0;
        return;
    }
    
    // Simple time estimation based on average speed
    double average_speed_ms = 50.0 / 3.6; // 50 km/h in m/s
    estimated_time_remaining_ = remaining_distance_ / average_speed_ms;
}

bool RouteMonitoring::isAtWaypoint(const Point& position, const Point& waypoint) const {
    double distance = NavUtils::haversineDistance(position, waypoint);
    return distance <= WAYPOINT_REACHED_THRESHOLD_METERS;
}

double RouteMonitoring::distanceToRouteSegment(const Point& position, int segment_index) const {
    if (segment_index >= static_cast<int>(route_points_.size()) - 1) {
        return 0.0;
    }
    
    Point segment_start = route_points_[segment_index];
    Point segment_end = route_points_[segment_index + 1];
    
    // Simplified distance calculation - distance to segment start
    // In a real implementation, this would calculate perpendicular distance to line segment
    double dist_to_start = NavUtils::haversineDistance(position, segment_start);
    double dist_to_end = NavUtils::haversineDistance(position, segment_end);
    
    return std::min(dist_to_start, dist_to_end);
}

} // namespace nav