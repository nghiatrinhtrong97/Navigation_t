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

namespace nav {

// Map tile structure
struct MapTile {
    uint32_t tile_id;
    BoundingBox bounds;
    std::vector<MapNode> nodes;
    std::vector<MapEdge> edges;
    bool is_loaded;
    uint64_t last_accessed;
    
    MapTile() : tile_id(0), is_loaded(false), last_accessed(0) {}
};

// Map data loader
class MapDataLoader {
public:
    MapDataLoader();
    
    // Load map data from file
    bool loadMapData(const std::string& map_file_path);
    
    // Get tiles that intersect with bounding box
    std::vector<uint32_t> getTilesInBoundingBox(const BoundingBox& bbox) const;
    
    // Load specific tile
    bool loadTile(uint32_t tile_id, MapTile& tile);
    
    // Get tile info
    bool getTileInfo(uint32_t tile_id, BoundingBox& bounds) const;
    
private:
    struct TileInfo {
        uint32_t tile_id;
        BoundingBox bounds;
        uint64_t file_offset;
        uint32_t data_size;
    };
    
    std::string map_file_path_;
    std::vector<TileInfo> tile_index_;
    
    bool loadTileIndex();
    bool loadTileFromFile(uint32_t tile_id, MapTile& tile);
    void createDummyTileIndex();
    void createDummyTileData(MapTile& tile);
};

// Tile manager with LRU cache
class MapTileManager {
public:
    explicit MapTileManager(size_t max_cache_size = 100);
    
    // Get tile (loads if not in cache)
    std::shared_ptr<MapTile> getTile(uint32_t tile_id);
    
    // Preload tiles in bounding box
    void preloadTiles(const BoundingBox& bbox);
    
    // Clear cache
    void clearCache();
    
    // Set map data loader
    void setMapDataLoader(std::shared_ptr<MapDataLoader> loader);
    
    // Get cache statistics
    size_t getCacheSize() const { return cache_.size(); }
    size_t getMaxCacheSize() const { return max_cache_size_; }

private:
    std::shared_ptr<MapDataLoader> data_loader_;
    LRUCache<uint32_t, std::shared_ptr<MapTile>> cache_;
    size_t max_cache_size_;
    mutable std::mutex cache_mutex_;
    
    void evictOldTiles();
};

// Main map service
class MapService {
public:
    MapService();
    ~MapService();
    
    // Initialize the service
    bool initialize(const std::string& map_data_path);
    
    // Start the service
    bool start();
    
    // Stop the service
    void stop();
    
    // Check if service is running
    bool isRunning() const { return is_running_; }
    
    // Get nodes in bounding box
    std::vector<MapNode> getNodesInBoundingBox(const BoundingBox& bbox);
    
    // Get edges in bounding box
    std::vector<MapEdge> getEdgesInBoundingBox(const BoundingBox& bbox);
    
    // Get map data for routing (graph structure)
    bool getMapGraphData(const BoundingBox& bbox, 
                        std::vector<MapNode>& nodes, 
                        std::vector<MapEdge>& edges);
    
    // Find closest node to a point
    MapNode findClosestNode(const Point& position);
    
    // Get node by ID
    bool getNodeById(uint32_t node_id, MapNode& node);
    
    // Get edges connected to a node
    std::vector<MapEdge> getConnectedEdges(uint32_t node_id);

private:
    // IPC server thread
    void ipcServerThread();
    
    // Handle IPC messages
    void handleIpcMessage(const NavMessage& message, int reply_channel);
    
    // Handle map data request
    void handleMapDataRequest(const MapDataRequestMsg& request, int reply_channel);
    
    // Handle nodes in bbox request
    void handleNodesInBboxRequest(const MapDataRequestMsg& request, int reply_channel);
    
    std::atomic<bool> is_running_;
    std::atomic<bool> should_stop_;
    
    // Threading
    std::thread ipc_thread_;
    
    // Map data management
    std::shared_ptr<MapDataLoader> data_loader_;
    std::shared_ptr<MapTileManager> tile_manager_;
    
    // Spatial indexing for fast node lookup
    std::unordered_map<uint32_t, MapNode> node_lookup_;
    std::unordered_map<uint32_t, std::vector<MapEdge>> edge_lookup_;
    mutable std::mutex data_mutex_;
    
    // IPC
    int ipc_channel_;
    
    // Configuration
    static constexpr size_t MAX_CACHE_SIZE = 100;
    static constexpr double CLOSEST_NODE_SEARCH_RADIUS = 1000.0; // meters
};

} // namespace nav