#include "positioning_service.h"
#include <cmath>

namespace nav {

DeadReckoning::DeadReckoning()
    : last_heading_(0.0)
    , last_update_time_(0)
    , error_estimate_(0.0)
    , is_initialized_(false)
{
}

void DeadReckoning::initialize(const Point& last_gps_position, double heading) {
    last_position_ = last_gps_position;
    last_heading_ = heading;
    last_update_time_ = NavUtils::getCurrentTimestampMs();
    error_estimate_ = 0.0;
    is_initialized_ = true;
}

Point DeadReckoning::updatePosition(const VehicleData& vehicle_data) {
    if (!is_initialized_) {
        return last_position_;
    }
    
    uint64_t current_time = vehicle_data.timestamp_ms;
    if (last_update_time_ == 0) {
        last_update_time_ = current_time;
        return last_position_;
    }
    
    // Calculate time delta in seconds
    double dt = (current_time - last_update_time_) / 1000.0;
    if (dt <= 0 || dt > 1.0) { // Sanity check
        last_update_time_ = current_time;
        return last_position_;
    }
    
    // Update heading using yaw rate
    double new_heading = last_heading_ + (vehicle_data.yaw_rate * dt);
    new_heading = NavUtils::normalizeAngle(new_heading);
    
    // Calculate distance traveled
    double speed_ms = vehicle_data.speed_kmh / 3.6; // Convert km/h to m/s
    double distance = speed_ms * dt;
    
    // Project position forward
    Point new_position = NavUtils::projectPoint(last_position_, new_heading, distance);
    
    // Update error estimate (grows over time)
    error_estimate_ += distance * 0.01 + dt * 0.1; // 1% distance error + 0.1m/s drift
    
    // Update state
    last_position_ = new_position;
    last_heading_ = new_heading;
    last_update_time_ = current_time;
    
    return new_position;
}

void DeadReckoning::reset() {
    is_initialized_ = false;
    error_estimate_ = 0.0;
    last_update_time_ = 0;
}

} // namespace nav