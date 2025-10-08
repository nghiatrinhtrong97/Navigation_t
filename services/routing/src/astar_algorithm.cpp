#include "routing_service.h"
#include <cmath>
#include <algorithm>
#include <limits>

namespace nav {

AStarAlgorithm::AStarAlgorithm() : goal_node_id_(0) {
}

void AStarAlgorithm::setGraphData(const std::vector<MapNode>& nodes, 
                                 const std::vector<MapEdge>& edges) {
    clear();
    
    // Store nodes
    for (const auto& node : nodes) {
        nodes_[node.id] = AStarNode(node.id, node.position);
    }
    
    // Build adjacency list
    for (const auto& edge : edges) {
        double weight = calculateEdgeWeight(edge);
        adjacency_list_[edge.from_node].emplace_back(
            edge.to_node, weight, edge.road_type, edge.speed_limit);
    }
}

bool AStarAlgorithm::findPath(uint32_t start_node_id, uint32_t goal_node_id, Route& route) {
    if (nodes_.find(start_node_id) == nodes_.end() || 
        nodes_.find(goal_node_id) == nodes_.end()) {
        return false;
    }
    
    goal_node_id_ = goal_node_id;
    
    // Initialize start node
    AStarNode* start_node = &nodes_[start_node_id];
    start_node->g_cost = 0.0;
    start_node->h_cost = heuristic(start_node_id, goal_node_id);
    start_node->f_cost = start_node->g_cost + start_node->h_cost;
    start_node->is_open = true;
    
    open_set_.push(start_node);
    
    while (!open_set_.empty()) {
        // Get node with lowest f_cost
        AStarNode* current = open_set_.top();
        open_set_.pop();
        
        current->is_open = false;
        current->is_closed = true;
        
        // Check if we reached the goal
        if (current->node_id == goal_node_id) {
            return reconstructPath(goal_node_id, route);
        }
        
        // Explore neighbors
        auto neighbors = getNeighbors(current->node_id);
        for (const auto& neighbor_info : neighbors) {
            AStarNode* neighbor = &nodes_[neighbor_info.to_node];
            
            if (neighbor->is_closed) {
                continue;
            }
            
            double tentative_g = current->g_cost + neighbor_info.weight;
            
            if (!neighbor->is_open) {
                neighbor->is_open = true;
                neighbor->parent_id = current->node_id;
                neighbor->g_cost = tentative_g;
                neighbor->h_cost = heuristic(neighbor->node_id, goal_node_id);
                neighbor->f_cost = neighbor->g_cost + neighbor->h_cost;
                open_set_.push(neighbor);
            } else if (tentative_g < neighbor->g_cost) {
                neighbor->parent_id = current->node_id;
                neighbor->g_cost = tentative_g;
                neighbor->f_cost = neighbor->g_cost + neighbor->h_cost;
                // Note: We should update the priority queue, but for simplicity
                // we'll let the outdated entry be processed later
            }
        }
    }
    
    return false; // No path found
}

double AStarAlgorithm::estimatePathCost(uint32_t start_node_id, uint32_t goal_node_id) {
    return heuristic(start_node_id, goal_node_id);
}

void AStarAlgorithm::clear() {
    // Clear all node states
    for (auto& pair : nodes_) {
        AStarNode& node = pair.second;
        node.g_cost = 0.0;
        node.h_cost = 0.0;
        node.f_cost = 0.0;
        node.parent_id = 0;
        node.is_open = false;
        node.is_closed = false;
    }
    
    // Clear open set
    while (!open_set_.empty()) {
        open_set_.pop();
    }
    
    goal_node_id_ = 0;
}

double AStarAlgorithm::heuristic(uint32_t node_id, uint32_t goal_id) {
    auto node_it = nodes_.find(node_id);
    auto goal_it = nodes_.find(goal_id);
    
    if (node_it == nodes_.end() || goal_it == nodes_.end()) {
        return std::numeric_limits<double>::max();
    }
    
    // Use Euclidean distance as heuristic
    return NavUtils::haversineDistance(node_it->second.position, goal_it->second.position);
}

std::vector<AStarAlgorithm::EdgeInfo> AStarAlgorithm::getNeighbors(uint32_t node_id) {
    auto it = adjacency_list_.find(node_id);
    if (it != adjacency_list_.end()) {
        return it->second;
    }
    return std::vector<EdgeInfo>();
}

bool AStarAlgorithm::reconstructPath(uint32_t goal_id, Route& route) {
    std::vector<uint32_t> path;
    uint32_t current_id = goal_id;
    
    // Trace back from goal to start
    while (current_id != 0) {
        path.push_back(current_id);
        
        auto it = nodes_.find(current_id);
        if (it == nodes_.end()) {
            return false;
        }
        
        current_id = it->second.parent_id;
    }
    
    // Reverse path to get start->goal order
    std::reverse(path.begin(), path.end());
    
    // Fill route structure
    route.node_count = std::min(static_cast<int>(path.size()), Route::MAX_NODES);
    
    for (int i = 0; i < route.node_count; ++i) {
        route.nodes[i] = path[i];
    }
    
    // Calculate total distance and estimated time
    route.total_distance_meters = 0.0;
    route.estimated_time_seconds = 0.0;
    
    for (int i = 0; i < route.node_count - 1; ++i) {
        auto from_it = nodes_.find(route.nodes[i]);
        auto to_it = nodes_.find(route.nodes[i + 1]);
        
        if (from_it != nodes_.end() && to_it != nodes_.end()) {
            double segment_distance = NavUtils::haversineDistance(
                from_it->second.position, to_it->second.position);
            route.total_distance_meters += segment_distance;
            
            // Estimate time based on average speed (simplified)
            double avg_speed_ms = 50.0 / 3.6; // 50 km/h in m/s
            route.estimated_time_seconds += segment_distance / avg_speed_ms;
        }
    }
    
    return true;
}

double AStarAlgorithm::calculateEdgeWeight(const MapEdge& edge) {
    double base_weight = edge.length_meters;
    
    // Adjust weight based on road type and speed limit
    double speed_factor = 50.0 / std::max(static_cast<double>(edge.speed_limit), 10.0);
    
    // Road type penalties
    double road_type_factor = 1.0;
    switch (edge.road_type) {
        case 0: road_type_factor = 0.8; break; // Highway
        case 1: road_type_factor = 1.0; break; // Street
        case 2: road_type_factor = 1.2; break; // Residential
        case 3: road_type_factor = 1.5; break; // Unpaved
        default: road_type_factor = 1.0; break;
    }
    
    return base_weight * speed_factor * road_type_factor;
}

} // namespace nav