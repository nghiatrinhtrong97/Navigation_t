#pragma once

#include "nav_types.h"
#include "nav_messages.h"
#include "nav_utils.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>

namespace nav {

// A* algorithm implementation
class AStarAlgorithm {
public:
    struct AStarNode {
        uint32_t node_id;
        Point position;
        double g_cost;      // Cost from start
        double h_cost;      // Heuristic cost to goal
        double f_cost;      // Total cost (g + h)
        uint32_t parent_id; // Parent node ID
        bool is_open;       // In open set
        bool is_closed;     // In closed set
        
        AStarNode() : node_id(0), g_cost(0), h_cost(0), f_cost(0), 
                     parent_id(0), is_open(false), is_closed(false) {}
        
        AStarNode(uint32_t id, const Point& pos) 
            : node_id(id), position(pos), g_cost(0), h_cost(0), f_cost(0),
              parent_id(0), is_open(false), is_closed(false) {}
    };
    
    struct EdgeInfo {
        uint32_t to_node;
        double weight;
        uint8_t road_type;
        uint8_t speed_limit;
        
        EdgeInfo(uint32_t to, double w, uint8_t type = 0, uint8_t speed = 50)
            : to_node(to), weight(w), road_type(type), speed_limit(speed) {}
    };
    
    AStarAlgorithm();
    
    // Set graph data
    void setGraphData(const std::vector<MapNode>& nodes, 
                     const std::vector<MapEdge>& edges);
    
    // Find path between two nodes
    bool findPath(uint32_t start_node_id, uint32_t goal_node_id, Route& route);
    
    // Get path cost estimation
    double estimatePathCost(uint32_t start_node_id, uint32_t goal_node_id);
    
    // Clear algorithm state
    void clear();

private:
    // Heuristic function (Euclidean distance)
    double heuristic(uint32_t node_id, uint32_t goal_id);
    
    // Get neighbors of a node
    std::vector<EdgeInfo> getNeighbors(uint32_t node_id);
    
    // Reconstruct path from goal to start
    bool reconstructPath(uint32_t goal_id, Route& route);
    
    // Calculate edge weight considering road type and speed
    double calculateEdgeWeight(const MapEdge& edge);
    
    std::unordered_map<uint32_t, AStarNode> nodes_;
    std::unordered_map<uint32_t, std::vector<EdgeInfo>> adjacency_list_;
    
    // Priority queue for open set (min-heap)
    struct NodeComparator {
        bool operator()(const AStarNode* a, const AStarNode* b) const {
            return a->f_cost > b->f_cost;
        }
    };
    
    std::priority_queue<AStarNode*, std::vector<AStarNode*>, NodeComparator> open_set_;
    uint32_t goal_node_id_;
};

// Route calculation preferences
enum class RoutePreference : uint8_t {
    FASTEST = 0,
    SHORTEST = 1,
    MOST_ECONOMICAL = 2,
    AVOID_HIGHWAYS = 3
};

// Routing service
class RoutingService {
public:
    RoutingService();
    ~RoutingService();
    
    // Initialize the service
    bool initialize();
    
    // Start the service
    bool start();
    
    // Stop the service
    void stop();
    
    // Check if service is running
    bool isRunning() const { return is_running_; }
    
    // Calculate route between two points
    bool calculateRoute(const Point& start, const Point& end, 
                       RoutePreference preference, Route& route);
    
    // Cancel ongoing route calculation
    void cancelRouteCalculation();
    
    // Set map service connection (for getting map data)
    void setMapServiceChannel(int map_channel);

private:
    // IPC server thread
    void ipcServerThread();
    
    // Route calculation thread
    void routeCalculatorThread();
    
    // Handle IPC messages
    void handleIpcMessage(const NavMessage& message, int reply_channel);
    
    // Handle route request
    void handleRouteRequest(const RouteRequestMsg& request, int reply_channel);
    
    // Get map data from map service
    bool requestMapData(const BoundingBox& bbox, 
                       std::vector<MapNode>& nodes, 
                       std::vector<MapEdge>& edges);
    
    // Find closest node to a point
    uint32_t findClosestNode(const Point& position, 
                            const std::vector<MapNode>& nodes);
    
    // Expand search area if no route found
    BoundingBox expandSearchArea(const BoundingBox& original_bbox, double factor = 2.0);
    
    std::atomic<bool> is_running_;
    std::atomic<bool> should_stop_;
    std::atomic<bool> cancel_calculation_;
    
    // Threading
    std::thread ipc_thread_;
    std::thread calculator_thread_;
    
    // Route calculation queue
    struct RouteRequest {
        Point start;
        Point end;
        RoutePreference preference;
        int reply_channel;
        uint32_t request_id;
        
        RouteRequest(const Point& s, const Point& e, RoutePreference pref, 
                    int channel, uint32_t id)
            : start(s), end(e), preference(pref), reply_channel(channel), request_id(id) {}
    };
    
    std::queue<RouteRequest> route_queue_;
    std::mutex queue_mutex_;
    std::condition_variable queue_condition_;
    
    // Algorithm
    std::unique_ptr<AStarAlgorithm> astar_;
    
    // IPC
    int ipc_channel_;
    int map_service_channel_;
    
    // Request tracking
    uint32_t next_request_id_;
    std::mutex request_mutex_;
    
    // Configuration
    static constexpr double INITIAL_SEARCH_RADIUS_KM = 5.0;
    static constexpr double MAX_SEARCH_RADIUS_KM = 50.0;
    static constexpr int MAX_ROUTE_CALCULATION_TIME_MS = 30000; // 30 seconds
};

} // namespace nav