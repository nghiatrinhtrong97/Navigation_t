#pragma once

#include "nav_types.h"
#include "nav_messages.h"
#include "nav_utils.h"
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>

namespace nav {

class DeadReckoning;
class MapMatching;

class PositioningService {
public:
    PositioningService();
    ~PositioningService();
    
    // Initialize the service
    bool initialize(const std::string& gps_device = "/dev/ser1", 
                   const std::string& can_device = "can0");
    
    // Start the service
    bool start();
    
    // Stop the service
    void stop();
    
    // Check if service is running
    bool isRunning() const { return is_running_; }
    
    // Get current position
    Point getCurrentPosition() const;
    
    // Get current GPS data
    GpsData getCurrentGpsData() const;
    
    // Get current vehicle data
    VehicleData getCurrentVehicleData() const;
    
    // Subscribe to position updates
    bool subscribeToPositionUpdates(uint32_t subscriber_pid, uint32_t update_interval_ms = 100);
    
    // Unsubscribe from position updates
    void unsubscribeFromPositionUpdates(uint32_t subscriber_pid);

private:
    // Threading functions
    void gpsReaderThread();
    void canReaderThread();
    void positionProcessorThread();
    void ipcServerThread();
    
    // Position calculation
    void updatePosition();
    void broadcastPositionUpdate();
    
    // IPC message handling
    void handleIpcMessage(const NavMessage& message, int reply_channel);
    
    // Member variables
    std::atomic<bool> is_running_;
    std::atomic<bool> should_stop_;
    
    // Threads
    std::thread gps_thread_;
    std::thread can_thread_;
    std::thread processor_thread_;
    std::thread ipc_thread_;
    
    // Data synchronization
    mutable std::mutex gps_mutex_;
    mutable std::mutex vehicle_mutex_;
    mutable std::mutex position_mutex_;
    mutable std::mutex subscribers_mutex_;
    
    // Current data
    GpsData current_gps_data_;
    VehicleData current_vehicle_data_;
    Point current_position_;
    bool using_dead_reckoning_;
    uint64_t last_valid_gps_time_;
    
    // Interfaces
    std::unique_ptr<CanInterface> can_interface_;
    std::string gps_device_path_;
    int gps_fd_;
    
    // Algorithms
    std::unique_ptr<DeadReckoning> dead_reckoning_;
    std::unique_ptr<MapMatching> map_matching_;
    
    // IPC
    int ipc_channel_;
    std::vector<uint32_t> subscribers_;
    
    // Configuration
    static constexpr uint64_t GPS_TIMEOUT_MS = 5000; // 5 seconds
    static constexpr uint32_t POSITION_UPDATE_INTERVAL_MS = 100; // 10 Hz
};

// Dead reckoning algorithm
class DeadReckoning {
public:
    DeadReckoning();
    
    // Initialize with last known GPS position
    void initialize(const Point& last_gps_position, double heading);
    
    // Update position using vehicle data
    Point updatePosition(const VehicleData& vehicle_data);
    
    // Reset the dead reckoning
    void reset();
    
    // Get accumulated error estimate
    double getErrorEstimate() const { return error_estimate_; }

private:
    Point last_position_;
    double last_heading_;
    uint64_t last_update_time_;
    double error_estimate_;
    bool is_initialized_;
};

// Map matching algorithm
class MapMatching {
public:
    MapMatching();
    
    // Apply map matching to GPS position
    Point matchToMap(const Point& gps_position, double heading, double speed_kmh);
    
    // Set map data for matching (simplified interface)
    void setMapData(const std::vector<MapEdge>& edges);

private:
    std::vector<MapEdge> map_edges_;
    Point findClosestRoadPoint(const Point& position) const;
};

} // namespace nav