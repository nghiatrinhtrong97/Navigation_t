#pragma once

#include "nav_types.h"
#include <cstdint>

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

} // namespace nav