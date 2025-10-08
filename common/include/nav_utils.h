#pragma once

#include "nav_types.h"
#include <string>
#include <vector>
#include <unordered_map>

namespace nav {

// Utility functions
class NavUtils {
public:
    // Convert degrees to radians
    static double degToRad(double degrees);
    
    // Convert radians to degrees
    static double radToDeg(double radians);
    
    // Normalize angle to [0, 360) degrees
    static double normalizeAngle(double angle);
    
    // Calculate great circle distance between two points
    static double haversineDistance(const Point& p1, const Point& p2);
    
    // Calculate bearing from point 1 to point 2
    static double calculateBearing(const Point& from, const Point& to);
    
    // Project point along bearing for given distance
    static Point projectPoint(const Point& start, double bearing_degrees, double distance_meters);
    
    // Check if point is within bounding box
    static bool isPointInBoundingBox(const Point& point, const BoundingBox& bbox);
    
    // Get current timestamp in milliseconds
    static uint64_t getCurrentTimestampMs();
    
    // Format distance for display (e.g., "1.2 km", "500 m")
    static std::string formatDistance(double meters);
    
    // Format time for display (e.g., "5 min", "1 h 30 min")
    static std::string formatTime(double seconds);
    
    // Convert turn type to string
    static std::string turnTypeToString(TurnType turn);
    
    // Map matching - snap GPS point to nearest road
    static Point snapToRoad(const Point& gps_point, const std::vector<MapEdge>& nearby_edges);
};

// NMEA parser for GPS data
class NmeaParser {
public:
    // Parse NMEA sentence and extract GPS data
    static bool parseNmeaSentence(const std::string& sentence, GpsData& gps_data);
    
private:
    static bool parseGPRMC(const std::string& sentence, GpsData& gps_data);
    static bool parseGPGGA(const std::string& sentence, GpsData& gps_data);
    static std::vector<std::string> splitString(const std::string& str, char delimiter);
    static double parseCoordinate(const std::string& coord_str, char direction);
    static uint8_t calculateChecksum(const std::string& sentence);
    static bool validateChecksum(const std::string& sentence);
};

// CAN bus interface
class CanInterface {
public:
    CanInterface();
    ~CanInterface();
    
    // Initialize CAN interface
    bool initialize(const std::string& can_device = "can0");
    
    // Close CAN interface
    void close();
    
    // Read vehicle data from CAN bus
    bool readVehicleData(VehicleData& vehicle_data);
    
    // Send guidance data to instrument cluster
    bool sendGuidanceData(const GuidanceInstruction& instruction);
    
    // Check if CAN interface is connected
    bool isConnected() const { return can_socket_ >= 0; }
    
private:
    int can_socket_;
    bool is_initialized_;
    
    // CAN message IDs (these would be vehicle-specific)
    static constexpr uint32_t VEHICLE_SPEED_MSG_ID = 0x200;
    static constexpr uint32_t YAW_RATE_MSG_ID = 0x201;
    static constexpr uint32_t GUIDANCE_MSG_ID = 0x300;
};

// LRU Cache template for map tiles
template<typename Key, typename Value>
class LRUCache {
public:
    explicit LRUCache(size_t capacity) : capacity_(capacity), head_(nullptr), tail_(nullptr) {
        // Create dummy head and tail nodes
        head_ = new CacheNode(Key{}, Value{});
        tail_ = new CacheNode(Key{}, Value{});
        head_->next = tail_;
        tail_->prev = head_;
    }
    
    ~LRUCache() {
        clear();
        delete head_;
        delete tail_;
    }
    
    bool get(const Key& key, Value& value) {
        auto it = cache_map_.find(key);
        if (it != cache_map_.end()) {
            CacheNode* node = it->second;
            value = node->value;
            moveToHead(node);
            return true;
        }
        return false;
    }
    
    void put(const Key& key, const Value& value) {
        auto it = cache_map_.find(key);
        if (it != cache_map_.end()) {
            // Update existing node
            CacheNode* node = it->second;
            node->value = value;
            moveToHead(node);
        } else {
            // Create new node
            CacheNode* node = new CacheNode(key, value);
            cache_map_[key] = node;
            
            // Add to head
            node->next = head_->next;
            node->prev = head_;
            head_->next->prev = node;
            head_->next = node;
            
            // Check capacity
            if (cache_map_.size() > capacity_) {
                CacheNode* tail_node = removeTail();
                cache_map_.erase(tail_node->key);
                delete tail_node;
            }
        }
    }
    
    void clear() {
        for (auto& pair : cache_map_) {
            delete pair.second;
        }
        cache_map_.clear();
        head_->next = tail_;
        tail_->prev = head_;
    }
    
    size_t size() const { return cache_map_.size(); }
    
private:
    struct CacheNode {
        Key key;
        Value value;
        CacheNode* prev;
        CacheNode* next;
        
        CacheNode(const Key& k, const Value& v) : key(k), value(v), prev(nullptr), next(nullptr) {}
    };
    
    void moveToHead(CacheNode* node) {
        removeNode(node);
        
        node->next = head_->next;
        node->prev = head_;
        head_->next->prev = node;
        head_->next = node;
    }
    
    void removeNode(CacheNode* node) {
        node->prev->next = node->next;
        node->next->prev = node->prev;
    }
    
    CacheNode* removeTail() {
        CacheNode* last_node = tail_->prev;
        removeNode(last_node);
        return last_node;
    }
    
    size_t capacity_;
    CacheNode* head_;
    CacheNode* tail_;
    std::unordered_map<Key, CacheNode*> cache_map_;
};

} // namespace nav