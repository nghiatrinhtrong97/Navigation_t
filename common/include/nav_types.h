#pragma once

#define _USE_MATH_DEFINES
#include <cstdint>
#include <cmath>

namespace nav {

// Geographic coordinate structure
struct Point {
    double latitude;   // Degrees
    double longitude;  // Degrees
    double altitude;   // Meters above sea level
    
    Point() : latitude(0.0), longitude(0.0), altitude(0.0) {}
    Point(double lat, double lon, double alt = 0.0) 
        : latitude(lat), longitude(lon), altitude(alt) {}
    
    // Calculate distance between two points (Haversine formula)
    double distanceTo(const Point& other) const {
        const double R = 6371000.0; // Earth radius in meters
        const double dLat = (other.latitude - latitude) * M_PI / 180.0;
        const double dLon = (other.longitude - longitude) * M_PI / 180.0;
        const double a = sin(dLat/2) * sin(dLat/2) +
                        cos(latitude * M_PI / 180.0) * cos(other.latitude * M_PI / 180.0) *
                        sin(dLon/2) * sin(dLon/2);
        const double c = 2 * atan2(sqrt(a), sqrt(1-a));
        return R * c;
    }
    
    // Calculate bearing to another point
    double bearingTo(const Point& other) const {
        const double dLon = (other.longitude - longitude) * M_PI / 180.0;
        const double lat1 = latitude * M_PI / 180.0;
        const double lat2 = other.latitude * M_PI / 180.0;
        
        const double y = sin(dLon) * cos(lat2);
        const double x = cos(lat1) * sin(lat2) - sin(lat1) * cos(lat2) * cos(dLon);
        
        double bearing = atan2(y, x) * 180.0 / M_PI;
        return fmod(bearing + 360.0, 360.0);
    }
};

// Bounding box for geographic regions
struct BoundingBox {
    double minLat, minLon, maxLat, maxLon;
    
    BoundingBox() : minLat(0), minLon(0), maxLat(0), maxLon(0) {}
    BoundingBox(double minLat, double minLon, double maxLat, double maxLon)
        : minLat(minLat), minLon(minLon), maxLat(maxLat), maxLon(maxLon) {}
    
    bool contains(const Point& point) const {
        return point.latitude >= minLat && point.latitude <= maxLat &&
               point.longitude >= minLon && point.longitude <= maxLon;
    }
    
    Point center() const {
        return Point((minLat + maxLat) / 2.0, (minLon + maxLon) / 2.0);
    }
};

// Vehicle data from CAN bus
struct VehicleData {
    double speed_kmh;      // Vehicle speed in km/h
    double yaw_rate;       // Yaw rate in degrees/second
    uint64_t timestamp_ms; // Timestamp in milliseconds
    
    VehicleData() : speed_kmh(0.0), yaw_rate(0.0), timestamp_ms(0) {}
};

// GPS data structure
struct GpsData {
    Point position;
    double speed_kmh;
    double course_degrees;     // Course over ground
    uint8_t satellites_used;   // Number of satellites used
    double hdop;              // Horizontal dilution of precision
    bool valid;               // GPS fix validity
    uint64_t timestamp_ms;    // Timestamp in milliseconds
    
    GpsData() : speed_kmh(0.0), course_degrees(0.0), satellites_used(0), 
                hdop(99.9), valid(false), timestamp_ms(0) {}
};

// Map node structure
struct MapNode {
    uint32_t id;
    Point position;
    uint8_t node_type;  // Intersection, waypoint, etc.
    
    MapNode() : id(0), node_type(0) {}
    MapNode(uint32_t id, const Point& pos, uint8_t type = 0)
        : id(id), position(pos), node_type(type) {}
};

// Map edge structure
struct MapEdge {
    uint32_t from_node;
    uint32_t to_node;
    double length_meters;
    uint8_t road_type;     // Highway, street, etc.
    uint8_t speed_limit;   // km/h
    uint16_t flags;        // One-way, toll, etc.
    
    MapEdge() : from_node(0), to_node(0), length_meters(0.0), 
                road_type(0), speed_limit(50), flags(0) {}
};

// Route structure
struct Route {
    static constexpr int MAX_NODES = 1024;
    
    uint32_t route_id;
    uint32_t nodes[MAX_NODES];
    int node_count;
    double total_distance_meters;
    double estimated_time_seconds;
    
    Route() : route_id(0), node_count(0), total_distance_meters(0.0), estimated_time_seconds(0.0) {
        for (int i = 0; i < MAX_NODES; ++i) {
            nodes[i] = 0;
        }
    }
};

// Turn instruction types
enum class TurnType : uint8_t {
    STRAIGHT = 0,
    SLIGHT_LEFT,
    LEFT,
    SHARP_LEFT,
    SLIGHT_RIGHT,
    RIGHT,
    SHARP_RIGHT,
    U_TURN,
    ROUNDABOUT_ENTER,
    ROUNDABOUT_EXIT,
    DESTINATION_REACHED
};

// Guidance instruction
struct GuidanceInstruction {
    TurnType turn_type;
    uint32_t target_node_id;
    double distance_to_turn_meters;
    char instruction_text[256];  // "Turn left in 200 meters"
    
    // Constants for backward compatibility
    static constexpr TurnType STRAIGHT = TurnType::STRAIGHT;
    static constexpr TurnType TURN_RIGHT = TurnType::RIGHT;
    static constexpr TurnType TURN_LEFT = TurnType::LEFT;
    static constexpr TurnType DESTINATION = TurnType::DESTINATION_REACHED;
    
    GuidanceInstruction() : turn_type(TurnType::STRAIGHT), target_node_id(0),
                           distance_to_turn_meters(0.0) {
        instruction_text[0] = '\0';
    }
};

// Error codes
enum class NavError : int32_t {
    SUCCESS = 0,
    GPS_NO_FIX = -1,
    MAP_DATA_NOT_FOUND = -2,
    ROUTE_NOT_FOUND = -3,
    IPC_ERROR = -4,
    INVALID_PARAMETER = -5,
    MEMORY_ERROR = -6,
    CAN_BUS_ERROR = -7,
    SERIAL_PORT_ERROR = -8
};

} // namespace nav
