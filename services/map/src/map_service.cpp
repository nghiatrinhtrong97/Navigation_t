#include "map_service.h"
#include <iostream>
#include <algorithm>
#include <limits>
#include <cstring>

#ifdef __QNX__
#include <sys/neutrino.h>
#include <sys/dispatch.h>
#endif

namespace nav {

MapService::MapService()
    : is_running_(false)
    , should_stop_(false)
    , ipc_channel_(-1)
{
    data_loader_ = std::make_shared<MapDataLoader>();
    tile_manager_ = std::make_shared<MapTileManager>(MAX_CACHE_SIZE);
    tile_manager_->setMapDataLoader(data_loader_);
}

MapService::~MapService() {
    stop();
    
#ifdef __QNX__
    if (ipc_channel_ >= 0) {
        ChannelDestroy(ipc_channel_);
    }
#endif
}

bool MapService::initialize(const std::string& map_data_path) {
    // Load map data
    if (!data_loader_->loadMapData(map_data_path)) {
        std::cerr << "Failed to load map data from: " << map_data_path << std::endl;
        return false;
    }
    
#ifdef __QNX__
    // Create IPC channel
    ipc_channel_ = ChannelCreate(0);
    if (ipc_channel_ < 0) {
        std::cerr << "Failed to create IPC channel for MapService" << std::endl;
        return false;
    }
#endif
    
    std::cout << "MapService initialized with map data: " << map_data_path << std::endl;
    return true;
}

bool MapService::start() {
    if (is_running_) {
        return true;
    }
    
    should_stop_ = false;
    
    try {
        // Start IPC server thread
        ipc_thread_ = std::thread(&MapService::ipcServerThread, this);
        
        is_running_ = true;
        
        std::cout << "MapService started successfully" << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to start MapService: " << e.what() << std::endl;
        return false;
    }
}

void MapService::stop() {
    if (!is_running_) {
        return;
    }
    
    should_stop_ = true;
    
    // Wait for threads to finish
    if (ipc_thread_.joinable()) {
        ipc_thread_.join();
    }
    
    is_running_ = false;
    
    std::cout << "MapService stopped" << std::endl;
}

std::vector<MapNode> MapService::getNodesInBoundingBox(const BoundingBox& bbox) {
    std::vector<MapNode> result;
    
    // Get tiles that intersect with the bounding box
    auto tile_ids = data_loader_->getTilesInBoundingBox(bbox);
    
    for (uint32_t tile_id : tile_ids) {
        auto tile = tile_manager_->getTile(tile_id);
        if (tile && tile->is_loaded) {
            for (const auto& node : tile->nodes) {
                if (bbox.contains(node.position)) {
                    result.push_back(node);
                }
            }
        }
    }
    
    return result;
}

std::vector<MapEdge> MapService::getEdgesInBoundingBox(const BoundingBox& bbox) {
    std::vector<MapEdge> result;
    
    // Get tiles that intersect with the bounding box
    auto tile_ids = data_loader_->getTilesInBoundingBox(bbox);
    
    for (uint32_t tile_id : tile_ids) {
        auto tile = tile_manager_->getTile(tile_id);
        if (tile && tile->is_loaded) {
            for (const auto& edge : tile->edges) {
                // Check if edge intersects with bounding box
                // For simplicity, we'll include edges if either endpoint is in the bbox
                MapNode from_node, to_node;
                if (getNodeById(edge.from_node, from_node) && 
                    getNodeById(edge.to_node, to_node)) {
                    if (bbox.contains(from_node.position) || 
                        bbox.contains(to_node.position)) {
                        result.push_back(edge);
                    }
                }
            }
        }
    }
    
    return result;
}

bool MapService::getMapGraphData(const BoundingBox& bbox, 
                                std::vector<MapNode>& nodes, 
                                std::vector<MapEdge>& edges) {
    nodes = getNodesInBoundingBox(bbox);
    edges = getEdgesInBoundingBox(bbox);
    
    return !nodes.empty() || !edges.empty();
}

MapNode MapService::findClosestNode(const Point& position) {
    MapNode closest_node;
    double min_distance = std::numeric_limits<double>::max();
    bool found = false;
    
    // Create a search bounding box around the position
    const double search_radius_deg = CLOSEST_NODE_SEARCH_RADIUS / 111000.0; // Rough conversion
    BoundingBox search_bbox(
        position.latitude - search_radius_deg,
        position.longitude - search_radius_deg,
        position.latitude + search_radius_deg,
        position.longitude + search_radius_deg
    );
    
    auto nodes = getNodesInBoundingBox(search_bbox);
    
    for (const auto& node : nodes) {
        double distance = NavUtils::haversineDistance(position, node.position);
        if (distance < min_distance) {
            min_distance = distance;
            closest_node = node;
            found = true;
        }
    }
    
    if (!found) {
        closest_node.id = 0; // Invalid node ID
    }
    
    return closest_node;
}

bool MapService::getNodeById(uint32_t node_id, MapNode& node) {
    std::lock_guard<std::mutex> lock(data_mutex_);
    
    auto it = node_lookup_.find(node_id);
    if (it != node_lookup_.end()) {
        node = it->second;
        return true;
    }
    
    // If not in lookup, search through loaded tiles
    // This is a fallback for nodes not yet indexed
    return false;
}

std::vector<MapEdge> MapService::getConnectedEdges(uint32_t node_id) {
    std::lock_guard<std::mutex> lock(data_mutex_);
    
    auto it = edge_lookup_.find(node_id);
    if (it != edge_lookup_.end()) {
        return it->second;
    }
    
    return std::vector<MapEdge>();
}

void MapService::ipcServerThread() {
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
                std::cerr << "MapService MsgReceive error: " << strerror(errno) << std::endl;
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

void MapService::handleIpcMessage(const NavMessage& message, int reply_channel) {
    switch (message.header.type) {
        case MessageType::REQUEST_MAP_DATA:
            handleMapDataRequest(message.map_data_request, reply_channel);
            break;
            
        case MessageType::REQUEST_NODES_IN_BBOX:
            handleNodesInBboxRequest(message.map_data_request, reply_channel);
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

void MapService::handleMapDataRequest(const MapDataRequestMsg& request, int reply_channel) {
    MapDataResponseMsg response;
    
    std::vector<MapNode> nodes;
    std::vector<MapEdge> edges;
    
    if (getMapGraphData(request.bbox, nodes, edges)) {
        response.node_count = static_cast<uint32_t>(nodes.size());
        response.edge_count = static_cast<uint32_t>(edges.size());
        response.header.error_code = static_cast<int32_t>(NavError::SUCCESS);
        
        // In a real implementation, the actual node and edge data would be sent
        // via shared memory or separate messages for large datasets
    } else {
        response.node_count = 0;
        response.edge_count = 0;
        response.header.error_code = static_cast<int32_t>(NavError::MAP_DATA_NOT_FOUND);
    }
    
#ifdef __QNX__
    MsgReply(reply_channel, EOK, &response, sizeof(response));
#endif
}

void MapService::handleNodesInBboxRequest(const MapDataRequestMsg& request, int reply_channel) {
    auto nodes = getNodesInBoundingBox(request.bbox);
    
    MapDataResponseMsg response;
    response.node_count = static_cast<uint32_t>(nodes.size());
    response.edge_count = 0;
    response.header.error_code = static_cast<int32_t>(NavError::SUCCESS);
    
#ifdef __QNX__
    MsgReply(reply_channel, EOK, &response, sizeof(response));
#endif
}

} // namespace nav