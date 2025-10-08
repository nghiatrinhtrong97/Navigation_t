#include "routing_service.h"
#include <iostream>
#include <algorithm>
#include <cstring>

#ifdef __QNX__
#include <sys/neutrino.h>
#include <sys/dispatch.h>
#endif

namespace nav {

RoutingService::RoutingService()
    : is_running_(false)
    , should_stop_(false)
    , cancel_calculation_(false)
    , ipc_channel_(-1)
    , map_service_channel_(-1)
    , next_request_id_(1)
{
    astar_ = std::make_unique<AStarAlgorithm>();
}

RoutingService::~RoutingService() {
    stop();
    
#ifdef __QNX__
    if (ipc_channel_ >= 0) {
        ChannelDestroy(ipc_channel_);
    }
#endif
}

bool RoutingService::initialize() {
#ifdef __QNX__
    // Create IPC channel
    ipc_channel_ = ChannelCreate(0);
    if (ipc_channel_ < 0) {
        std::cerr << "Failed to create IPC channel for RoutingService" << std::endl;
        return false;
    }
#endif
    
    std::cout << "RoutingService initialized" << std::endl;
    return true;
}

bool RoutingService::start() {
    if (is_running_) {
        return true;
    }
    
    should_stop_ = false;
    
    try {
        // Start threads
        ipc_thread_ = std::thread(&RoutingService::ipcServerThread, this);
        calculator_thread_ = std::thread(&RoutingService::routeCalculatorThread, this);
        
        is_running_ = true;
        
        std::cout << "RoutingService started successfully" << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to start RoutingService: " << e.what() << std::endl;
        return false;
    }
}

void RoutingService::stop() {
    if (!is_running_) {
        return;
    }
    
    should_stop_ = true;
    queue_condition_.notify_all();
    
    // Wait for threads to finish
    if (ipc_thread_.joinable()) {
        ipc_thread_.join();
    }
    if (calculator_thread_.joinable()) {
        calculator_thread_.join();
    }
    
    is_running_ = false;
    
    std::cout << "RoutingService stopped" << std::endl;
}

bool RoutingService::calculateRoute(const Point& start, const Point& end, 
                                   RoutePreference preference, Route& route) {
    // Create expanded bounding box around start and end points
    double min_lat = std::min(start.latitude, end.latitude);
    double max_lat = std::max(start.latitude, end.latitude);
    double min_lon = std::min(start.longitude, end.longitude);
    double max_lon = std::max(start.longitude, end.longitude);
    
    // Add search radius
    double radius_deg = INITIAL_SEARCH_RADIUS_KM / 111.0; // Rough conversion
    BoundingBox search_bbox(
        min_lat - radius_deg, min_lon - radius_deg,
        max_lat + radius_deg, max_lon + radius_deg
    );
    
    // Try progressively larger search areas
    for (int attempt = 0; attempt < 3; ++attempt) {
        if (cancel_calculation_) {
            return false;
        }
        
        // Get map data
        std::vector<MapNode> nodes;
        std::vector<MapEdge> edges;
        
        if (!requestMapData(search_bbox, nodes, edges)) {
            std::cerr << "Failed to get map data for routing" << std::endl;
            return false;
        }
        
        if (nodes.empty()) {
            std::cerr << "No map nodes found in search area" << std::endl;
            search_bbox = expandSearchArea(search_bbox);
            continue;
        }
        
        // Find closest nodes to start and end points
        uint32_t start_node_id = findClosestNode(start, nodes);
        uint32_t end_node_id = findClosestNode(end, nodes);
        
        if (start_node_id == 0 || end_node_id == 0) {
            std::cerr << "Could not find nodes near start/end points" << std::endl;
            search_bbox = expandSearchArea(search_bbox);
            continue;
        }
        
        // Set graph data and calculate route
        astar_->setGraphData(nodes, edges);
        
        if (astar_->findPath(start_node_id, end_node_id, route)) {
            std::cout << "Route found with " << route.node_count << " nodes, "
                      << "distance: " << route.total_distance_meters << "m" << std::endl;
            return true;
        }
        
        // Expand search area and try again
        search_bbox = expandSearchArea(search_bbox);
        
        std::cout << "Route not found, expanding search area (attempt " 
                  << (attempt + 1) << ")" << std::endl;
    }
    
    std::cerr << "Failed to find route after multiple attempts" << std::endl;
    return false;
}

void RoutingService::cancelRouteCalculation() {
    cancel_calculation_ = true;
}

void RoutingService::setMapServiceChannel(int map_channel) {
    map_service_channel_ = map_channel;
}

void RoutingService::ipcServerThread() {
#ifdef __QNX__
    struct _msg_info info;
    NavMessage msg;
    
    while (!should_stop_) {
        int rcvid = MsgReceive(ipc_channel_, &msg, sizeof(msg), &info);
        if (rcvid > 0) {
            handleIpcMessage(msg, rcvid);
        } else if (rcvid == 0) {
            // Pulse received
        } else {
            // Error
            if (errno != EINTR) {
                std::cerr << "RoutingService MsgReceive error: " << strerror(errno) << std::endl;
            }
        }
    }
#else
    // Simulation mode
    while (!should_stop_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
#endif
}

void RoutingService::routeCalculatorThread() {
    while (!should_stop_) {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        
        // Wait for route requests
        queue_condition_.wait(lock, [this]() {
            return !route_queue_.empty() || should_stop_;
        });
        
        if (should_stop_) {
            break;
        }
        
        if (!route_queue_.empty()) {
            RouteRequest request = route_queue_.front();
            route_queue_.pop();
            lock.unlock();
            
            // Calculate route
            cancel_calculation_ = false;
            Route route;
            bool success = calculateRoute(request.start, request.end, 
                                        request.preference, route);
            
            // Send response
            RouteResponseMsg response;
            if (success) {
                response.route = route;
                response.header.error_code = static_cast<int32_t>(NavError::SUCCESS);
            } else {
                response.header.error_code = static_cast<int32_t>(NavError::ROUTE_NOT_FOUND);
            }
            
#ifdef __QNX__
            MsgReply(request.reply_channel, EOK, &response, sizeof(response));
#else
            std::cout << "Route calculation " << (success ? "completed" : "failed") 
                      << " for request " << request.request_id << std::endl;
#endif
        }
    }
}

void RoutingService::handleIpcMessage(const NavMessage& message, int reply_channel) {
    switch (message.header.type) {
        case MessageType::REQUEST_ROUTE:
            handleRouteRequest(message.route_request, reply_channel);
            break;
            
        case MessageType::CANCEL_ROUTE:
            cancelRouteCalculation();
            {
                NavMessage reply;
                reply.header.type = MessageType::SERVICE_READY;
                reply.header.error_code = static_cast<int32_t>(NavError::SUCCESS);
#ifdef __QNX__
                MsgReply(reply_channel, EOK, &reply, sizeof(reply.header));
#endif
            }
            break;
            
        default: {
            // Unknown message type
            ErrorResponseMsg error_reply;
            error_reply.error_code = NavError::INVALID_PARAMETER;
            strcpy(error_reply.error_description, "Unknown message type");
            
#ifdef __QNX__
            MsgReply(reply_channel, EINVAL, &error_reply, sizeof(error_reply));
#endif
            break;
        }
    }
}

void RoutingService::handleRouteRequest(const RouteRequestMsg& request, int reply_channel) {
    std::lock_guard<std::mutex> lock1(request_mutex_);
    uint32_t request_id = next_request_id_++;
    
    // Add to calculation queue
    {
        std::lock_guard<std::mutex> lock2(queue_mutex_);
        route_queue_.emplace(
            request.start_point, 
            request.end_point,
            static_cast<RoutePreference>(request.route_preferences),
            reply_channel,
            request_id
        );
    }
    
    queue_condition_.notify_one();
    
    std::cout << "Route request queued: " << request_id << std::endl;
}

bool RoutingService::requestMapData(const BoundingBox& bbox, 
                                   std::vector<MapNode>& nodes, 
                                   std::vector<MapEdge>& edges) {
    // For simulation, create dummy map data
    if (map_service_channel_ < 0) {
        // Create a simple grid for testing
        double step = 0.001;
        uint32_t node_id = 1;
        
        for (double lat = bbox.minLat; lat <= bbox.maxLat; lat += step) {
            for (double lon = bbox.minLon; lon <= bbox.maxLon; lon += step) {
                MapNode node;
                node.id = node_id++;
                node.position = Point(lat, lon);
                nodes.push_back(node);
            }
        }
        
        // Create edges between adjacent nodes
        for (size_t i = 0; i < nodes.size(); ++i) {
            for (size_t j = i + 1; j < nodes.size(); ++j) {
                double distance = NavUtils::haversineDistance(
                    nodes[i].position, nodes[j].position);
                
                if (distance < 150.0) { // Connect nearby nodes
                    MapEdge edge;
                    edge.from_node = nodes[i].id;
                    edge.to_node = nodes[j].id;
                    edge.length_meters = distance;
                    edge.road_type = 1;
                    edge.speed_limit = 50;
                    edges.push_back(edge);
                    
                    // Reverse edge
                    MapEdge reverse_edge = edge;
                    reverse_edge.from_node = edge.to_node;
                    reverse_edge.to_node = edge.from_node;
                    edges.push_back(reverse_edge);
                }
            }
        }
        
        return !nodes.empty();
    }
    
#ifdef __QNX__
    // Send request to map service
    MapDataRequestMsg request;
    request.bbox = bbox;
    request.detail_level = 1;
    
    MapDataResponseMsg response;
    int status = MsgSend(map_service_channel_, &request, sizeof(request),
                        &response, sizeof(response));
    
    if (status == -1) {
        std::cerr << "Failed to communicate with map service" << std::endl;
        return false;
    }
    
    return response.header.error_code == static_cast<int32_t>(NavError::SUCCESS);
#else
    return true; // Simulation mode
#endif
}

uint32_t RoutingService::findClosestNode(const Point& position, 
                                        const std::vector<MapNode>& nodes) {
    if (nodes.empty()) {
        return 0;
    }
    
    uint32_t closest_id = 0;
    double min_distance = std::numeric_limits<double>::max();
    
    for (const auto& node : nodes) {
        double distance = NavUtils::haversineDistance(position, node.position);
        if (distance < min_distance) {
            min_distance = distance;
            closest_id = node.id;
        }
    }
    
    return closest_id;
}

BoundingBox RoutingService::expandSearchArea(const BoundingBox& original_bbox, double factor) {
    double center_lat = (original_bbox.minLat + original_bbox.maxLat) / 2.0;
    double center_lon = (original_bbox.minLon + original_bbox.maxLon) / 2.0;
    
    double half_width = (original_bbox.maxLat - original_bbox.minLat) * factor / 2.0;
    double half_height = (original_bbox.maxLon - original_bbox.minLon) * factor / 2.0;
    
    return BoundingBox(
        center_lat - half_width, center_lon - half_height,
        center_lat + half_width, center_lon + half_height
    );
}

} // namespace nav