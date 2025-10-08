#pragma once

#include "nav_types.h"
#include "nav_messages.h"
#include "nav_utils.h"
#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>

namespace nav {

// Route monitoring class
class RouteMonitoring {
public:
    RouteMonitoring();
    
    // Set the active route
    void setRoute(const Route& route);
    
    // Update current position and check route progress
    bool updatePosition(const Point& current_position, double heading);
    
    // Check if vehicle is off route
    bool isOffRoute() const { return is_off_route_; }
    
    // Get current route segment index
    int getCurrentSegmentIndex() const { return current_segment_index_; }
    
    // Get distance to next waypoint
    double getDistanceToNextWaypoint() const;
    
    // Get progress along route (0.0 to 1.0)
    double getRouteProgress() const;
    
    // Get remaining distance
    double getRemainingDistance() const { return remaining_distance_; }
    
    // Get estimated time to destination
    double getEstimatedTimeToDestination() const { return estimated_time_remaining_; }
    
    // Reset monitoring state
    void reset();

private:
    Route current_route_;
    std::vector<Point> route_points_; // Cached route point positions
    int current_segment_index_;
    Point last_position_;
    bool is_off_route_;
    double off_route_distance_;
    double remaining_distance_;
    double estimated_time_remaining_;
    uint64_t last_update_time_;
    
    // Configuration
    static constexpr double OFF_ROUTE_THRESHOLD_METERS = 100.0;
    static constexpr double WAYPOINT_REACHED_THRESHOLD_METERS = 50.0;
    
    // Helper functions
    void updateRemainingDistance();
    void updateEstimatedTime();
    bool isAtWaypoint(const Point& position, const Point& waypoint) const;
    double distanceToRouteSegment(const Point& position, int segment_index) const;
};

// Turn-by-turn instruction generator
class TurnByTurnGenerator {
public:
    TurnByTurnGenerator();
    
    // Set map data for instruction generation
    void setMapData(const std::vector<MapNode>& nodes, const std::vector<MapEdge>& edges);
    
    // Generate instruction for next maneuver
    GuidanceInstruction generateNextInstruction(const Route& route, 
                                               int current_segment_index,
                                               const Point& current_position);
    
    // Generate instruction text
    std::string generateInstructionText(const GuidanceInstruction& instruction) const;
    
private:
    std::unordered_map<uint32_t, MapNode> node_lookup_;
    std::unordered_map<uint32_t, std::vector<MapEdge>> edge_lookup_;
    
    // Determine turn type based on geometry
    TurnType determineTurnType(const Point& from, const Point& via, const Point& to) const;
    
    // Calculate bearing change
    double calculateBearingChange(double from_bearing, double to_bearing) const;
    
    // Get upcoming maneuver point
    bool getUpcomingManeuver(const Route& route, int current_index, 
                            int& maneuver_index, TurnType& turn_type) const;
    
    // Distance thresholds for different instruction types
    static constexpr double IMMEDIATE_INSTRUCTION_DISTANCE = 50.0;   // "Turn now"
    static constexpr double NEAR_INSTRUCTION_DISTANCE = 200.0;       // "Turn in 200m"
    static constexpr double FAR_INSTRUCTION_DISTANCE = 500.0;        // "Turn in 500m"
};

// Main guidance service
class GuidanceService {
public:
    GuidanceService();
    ~GuidanceService();
    
    // Initialize the service
    bool initialize();
    
    // Start the service
    bool start();
    
    // Stop the service
    void stop();
    
    // Check if service is running
    bool isRunning() const { return is_running_; }
    
    // Set active route
    void setActiveRoute(const Route& route);
    
    // Clear active route
    void clearActiveRoute();
    
    // Check if route is active
    bool hasActiveRoute() const { return has_active_route_; }
    
    // Get current guidance instruction
    GuidanceInstruction getCurrentInstruction() const;
    
    // Subscribe to positioning service
    bool subscribeToPositioning(int positioning_channel);
    
    // Connect to routing service for rerouting
    void setRoutingServiceChannel(int routing_channel);

private:
    // IPC server thread
    void ipcServerThread();
    
    // Guidance processing thread
    void guidanceProcessorThread();
    
    // Handle IPC messages
    void handleIpcMessage(const NavMessage& message, int reply_channel);
    
    // Handle position updates
    void handlePositionUpdate(const PositionUpdateMsg& position_msg);
    
    // Handle set route message
    void handleSetRoute(const SetRouteMsg& route_msg, int reply_channel);
    
    // Update guidance
    void updateGuidance(const Point& current_position, double heading);
    
    // Send guidance update to subscribers
    void broadcastGuidanceUpdate();
    
    // Request reroute if off route
    void requestReroute(const Point& current_position, const Point& destination);
    
    // Send guidance to instrument cluster via CAN
    void sendToInstrumentCluster(const GuidanceInstruction& instruction);
    
    std::atomic<bool> is_running_;
    std::atomic<bool> should_stop_;
    std::atomic<bool> has_active_route_;
    
    // Threading
    std::thread ipc_thread_;
    std::thread guidance_thread_;
    
    // Synchronization
    mutable std::mutex route_mutex_;
    mutable std::mutex position_mutex_;
    mutable std::mutex guidance_mutex_;
    std::condition_variable guidance_condition_;
    
    // Current state
    Route active_route_;
    Point current_position_;
    double current_heading_;
    bool has_current_position_;
    GuidanceInstruction current_instruction_;
    uint64_t last_guidance_update_;
    
    // Components
    std::unique_ptr<RouteMonitoring> route_monitor_;
    std::unique_ptr<TurnByTurnGenerator> instruction_generator_;
    std::unique_ptr<CanInterface> can_interface_;
    
    // IPC
    int ipc_channel_;
    int positioning_channel_;
    int routing_channel_;
    std::vector<uint32_t> subscribers_;
    mutable std::mutex subscribers_mutex_;
    
    // Configuration
    static constexpr uint32_t GUIDANCE_UPDATE_INTERVAL_MS = 500; // 2 Hz
    static constexpr double REROUTE_REQUEST_COOLDOWN_MS = 10000; // 10 seconds
    
    uint64_t last_reroute_request_time_;
};

} // namespace nav