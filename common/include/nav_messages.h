#pragma once

#include "nav_types.h"
#include <cstdint>
#include <string>
#include <vector>

namespace nav {

#define MAX_ROUTE_NODES 1024
#define MAX_INSTRUCTION_TEXT 256

// Message types for IPC communication
enum class MessageType : uint32_t {
    // Positioning service messages
    POSITION_UPDATE = 1000,
    SUBSCRIBE_POSITION,
    UNSUBSCRIBE_POSITION,
    
    // Map service messages
    REQUEST_MAP_DATA = 2000,
    MAP_DATA_RESPONSE,
    REQUEST_NODES_IN_BBOX,
    NODES_IN_BBOX_RESPONSE,
    
    // Routing service messages
    REQUEST_ROUTE = 3000,
    ROUTE_RESPONSE,
    CANCEL_ROUTE,
    
    // Guidance service messages
    REQUEST_GUIDANCE = 4000,
    GUIDANCE_UPDATE,
    SET_ROUTE,
    ROUTE_DEVIATION,
    
    // Error responses
    ERROR_RESPONSE = 9000,
    
    // System messages
    SERVICE_READY = 9900,
    SHUTDOWN_REQUEST,
    HEARTBEAT
};

// Base message header
struct MessageHeader {
    MessageType type;
    uint32_t message_id;
    uint32_t sender_pid;
    uint32_t data_size;
    int32_t error_code;
    
    MessageHeader() : type(MessageType::HEARTBEAT), message_id(0), 
                     sender_pid(0), data_size(0), error_code(0) {}
    MessageHeader(MessageType t) : type(t), message_id(0), 
                                  sender_pid(0), data_size(0), error_code(0) {}
};

// Position update message
struct PositionUpdateMsg {
    MessageHeader header;
    Point current_position;
    double speed_kmh;
    double heading_degrees;
    bool gps_valid;
    uint8_t positioning_mode; // 0=GPS, 1=Dead Reckoning
    uint64_t timestamp_ms;
    
    PositionUpdateMsg() : header(MessageType::POSITION_UPDATE), 
                         speed_kmh(0.0), heading_degrees(0.0), 
                         gps_valid(false), positioning_mode(0), timestamp_ms(0) {}
};

// Route request message
struct RouteRequestMsg {
    MessageHeader header;
    Point start_point;
    Point end_point;
    uint8_t route_preferences; // Fastest, shortest, etc.
    
    RouteRequestMsg() : header(MessageType::REQUEST_ROUTE), route_preferences(0) {}
};

// Route response message
struct RouteResponseMsg {
    MessageHeader header;
    Route route;
    
    RouteResponseMsg() : header(MessageType::ROUTE_RESPONSE) {}
};

// Map data request message
struct MapDataRequestMsg {
    MessageHeader header;
    BoundingBox bbox;
    uint8_t detail_level; // 0=overview, 1=detailed
    
    MapDataRequestMsg() : header(MessageType::REQUEST_MAP_DATA), detail_level(1) {}
};

// Map data response message
struct MapDataResponseMsg {
    MessageHeader header;
    uint32_t node_count;
    uint32_t edge_count;
    // Note: Actual node and edge data would be sent in separate messages
    // or through shared memory for large datasets
    
    MapDataResponseMsg() : header(MessageType::MAP_DATA_RESPONSE), 
                          node_count(0), edge_count(0) {}
};

// Guidance update message
struct GuidanceUpdateMsg {
    MessageHeader header;
    GuidanceInstruction instruction;
    double distance_to_destination_meters;
    double time_to_destination_seconds;
    uint32_t current_route_node;
    bool route_active;
    
    GuidanceUpdateMsg() : header(MessageType::GUIDANCE_UPDATE),
                         distance_to_destination_meters(0.0),
                         time_to_destination_seconds(0.0),
                         current_route_node(0), route_active(false) {}
};

// Set route message for guidance service
struct SetRouteMsg {
    MessageHeader header;
    Route route;
    
    SetRouteMsg() : header(MessageType::SET_ROUTE) {}
};

// Error response message
struct ErrorResponseMsg {
    MessageHeader header;
    NavError error_code;
    char error_description[256];
    
    ErrorResponseMsg() : header(MessageType::ERROR_RESPONSE), 
                        error_code(NavError::SUCCESS) {
        error_description[0] = '\0';
    }
};

// Service subscription message
struct SubscriptionMsg {
    MessageHeader header;
    uint32_t subscriber_pid;
    uint32_t update_interval_ms; // Minimum interval between updates
    
    SubscriptionMsg() : header(MessageType::SUBSCRIBE_POSITION),
                       subscriber_pid(0), update_interval_ms(100) {}
};

// Generic navigation message union for QNX message passing
struct NavMessage {
    union {
        MessageHeader header;
        PositionUpdateMsg position_update;
        RouteRequestMsg route_request;
        RouteResponseMsg route_response;
        MapDataRequestMsg map_data_request;
        MapDataResponseMsg map_data_response;
        GuidanceUpdateMsg guidance_update;
        SetRouteMsg set_route;
        ErrorResponseMsg error_response;
        SubscriptionMsg subscription;
    };
    
    NavMessage() {
        // Initialize with header
        header = MessageHeader();
    }
    
    explicit NavMessage(MessageType type) {
        header = MessageHeader(type);
    }
};

// Service request/response types for HMI communication
enum class RoutingRequestType : uint32_t {
    CALCULATE = 1,
    CANCEL = 2
};

enum class RoutingAlgorithm : uint32_t {
    ASTAR = 1,
    DIJKSTRA = 2
};

enum class RoutingCriteria : uint32_t {
    SHORTEST_DISTANCE = 1,
    SHORTEST_TIME = 2,
    LEAST_FUEL = 3
};

struct RoutingRequest {
    RoutingRequestType request_type;  // Changed from 'type' to 'request_type'
    Point start_point;
    Point end_point;
    RoutingAlgorithm algorithm_type;  // Changed from 'algorithm' to 'algorithm_type'
    RoutingCriteria optimization_type; // Changed from 'criteria' to 'optimization_type'
    
    // Backward compatibility constants
    static constexpr RoutingRequestType CALCULATE = RoutingRequestType::CALCULATE;
    static constexpr RoutingRequestType CANCEL = RoutingRequestType::CANCEL;
    
    RoutingRequest() : request_type(RoutingRequestType::CALCULATE), 
                      algorithm_type(RoutingAlgorithm::ASTAR), 
                      optimization_type(RoutingCriteria::SHORTEST_TIME) {}
};

enum class RoutingResponseStatus : uint32_t {
    SUCCESS = 1,
    FAILED = 2,
    CANCELLED = 3
};

struct RoutingResponse {
    RoutingResponseStatus status;
    Route route;
    std::string error_message;
    
    // Backward compatibility constants
    static constexpr RoutingResponseStatus SUCCESS = RoutingResponseStatus::SUCCESS;
    static constexpr RoutingResponseStatus FAILED = RoutingResponseStatus::FAILED;
    
    RoutingResponse() : status(RoutingResponseStatus::FAILED) {}
};

enum class PositioningRequestType : uint32_t {
    START = 1,
    STOP = 2,
    SET_POSITION = 3
};

struct PositioningRequest {
    PositioningRequestType request_type;  // Changed from 'type' to 'request_type'
    Point position; // Used for SET_POSITION
    
    // Backward compatibility constants
    static constexpr PositioningRequestType START = PositioningRequestType::START;
    static constexpr PositioningRequestType STOP = PositioningRequestType::STOP;
    static constexpr PositioningRequestType SET_POSITION = PositioningRequestType::SET_POSITION;
    
    PositioningRequest() : request_type(PositioningRequestType::START) {}
};

struct PositioningResponse {
    bool success;
    Point current_position;
    Point position;  // Added for compatibility
    double heading;  // Added for compatibility
    double speed;    // Added for compatibility
    GpsData gps_data;
    VehicleData vehicle_data;
    std::string error_message;
    
    PositioningResponse() : success(false), heading(0.0), speed(0.0) {}
};

enum class GuidanceRequestType : uint32_t {
    START = 1,
    STOP = 2,
    UPDATE_POSITION = 3
};

struct GuidanceRequest {
    GuidanceRequestType request_type;  // Changed from 'type' to 'request_type'
    Route route; // Used for START
    Point position; // Used for UPDATE_POSITION
    Point current_position; // Added for compatibility
    
    // Backward compatibility constants
    static constexpr GuidanceRequestType START = GuidanceRequestType::START;
    static constexpr GuidanceRequestType STOP = GuidanceRequestType::STOP;
    static constexpr GuidanceRequestType UPDATE_POSITION = GuidanceRequestType::UPDATE_POSITION;
    
    GuidanceRequest() : request_type(GuidanceRequestType::START) {}
};

enum class GuidanceResponseType : uint32_t {
    INSTRUCTION = 1,
    ROUTE_DEVIATION = 2,
    ARRIVAL = 3
};

struct GuidanceResponse {
    bool success;
    GuidanceResponseType response_type;
    GuidanceInstruction current_instruction;
    GuidanceInstruction instruction;  // Added for compatibility
    double deviation_distance;  // For route deviation responses
    std::string error_message;
    
    // Backward compatibility constants
    static constexpr GuidanceResponseType INSTRUCTION = GuidanceResponseType::INSTRUCTION;
    static constexpr GuidanceResponseType ROUTE_DEVIATION = GuidanceResponseType::ROUTE_DEVIATION;
    static constexpr GuidanceResponseType ARRIVAL = GuidanceResponseType::ARRIVAL;
    
    GuidanceResponse() : success(false), response_type(GuidanceResponseType::INSTRUCTION), deviation_distance(0.0) {}
};

// Map service types
struct MapDataRequest {
    double min_latitude;
    double max_latitude;
    double min_longitude;
    double max_longitude;
    int zoom_level;
    
    MapDataRequest() : min_latitude(0.0), max_latitude(0.0), 
                      min_longitude(0.0), max_longitude(0.0), zoom_level(10) {}
};

struct MapDataResponse {
    bool success;
    std::vector<MapNode> nodes;
    std::vector<MapEdge> edges;
    std::string error_message;
    
    MapDataResponse() : success(false) {}
};

struct MapImageTile {
    double latitude;
    double longitude;
    int zoom_level;
    std::vector<uint8_t> image_data;
    std::string format; // "PNG", "JPEG", etc.
    
    MapImageTile() : latitude(0.0), longitude(0.0), zoom_level(10), format("PNG") {}
};

struct MapDisplayTile {
    double center_lat;
    double center_lon;
    int zoom_level;
    bool has_data;
    std::vector<uint8_t> tile_data;
    
    MapDisplayTile() : center_lat(0.0), center_lon(0.0), zoom_level(10), has_data(false) {}
};

struct MapTileRequest {
    double center_lat;   // Changed from latitude to center_lat
    double center_lon;   // Changed from longitude to center_lon
    int zoom_level;
    
    MapTileRequest() : center_lat(0.0), center_lon(0.0), zoom_level(10) {}
};

} // namespace nav