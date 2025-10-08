#include "map_service.h"
#include <fstream>
#include <iostream>

namespace nav {

MapDataLoader::MapDataLoader() {
}

bool MapDataLoader::loadMapData(const std::string& map_file_path) {
    map_file_path_ = map_file_path;
    
    // Load tile index first
    if (!loadTileIndex()) {
        std::cerr << "Failed to load tile index" << std::endl;
        return false;
    }
    
    std::cout << "Loaded " << tile_index_.size() << " tiles from map data" << std::endl;
    return true;
}

std::vector<uint32_t> MapDataLoader::getTilesInBoundingBox(const BoundingBox& bbox) const {
    std::vector<uint32_t> result;
    
    for (const auto& tile_info : tile_index_) {
        // Check if tile bounding box intersects with request bounding box
        if (!(tile_info.bounds.maxLat < bbox.minLat || 
              tile_info.bounds.minLat > bbox.maxLat ||
              tile_info.bounds.maxLon < bbox.minLon || 
              tile_info.bounds.minLon > bbox.maxLon)) {
            result.push_back(tile_info.tile_id);
        }
    }
    
    return result;
}

bool MapDataLoader::loadTile(uint32_t tile_id, MapTile& tile) {
    return loadTileFromFile(tile_id, tile);
}

bool MapDataLoader::getTileInfo(uint32_t tile_id, BoundingBox& bounds) const {
    for (const auto& tile_info : tile_index_) {
        if (tile_info.tile_id == tile_id) {
            bounds = tile_info.bounds;
            return true;
        }
    }
    return false;
}

bool MapDataLoader::loadTileIndex() {
    // This is a simplified implementation
    // In a real system, this would read from a proper map file format
    
    std::ifstream file(map_file_path_ + ".index", std::ios::binary);
    if (!file.is_open()) {
        // Create a dummy tile index for demonstration
        createDummyTileIndex();
        return true;
    }
    
    // Read tile index from file
    uint32_t tile_count;
    file.read(reinterpret_cast<char*>(&tile_count), sizeof(tile_count));
    
    tile_index_.reserve(tile_count);
    
    for (uint32_t i = 0; i < tile_count; ++i) {
        TileInfo tile_info;
        file.read(reinterpret_cast<char*>(&tile_info), sizeof(tile_info));
        tile_index_.push_back(tile_info);
    }
    
    file.close();
    return true;
}

void MapDataLoader::createDummyTileIndex() {
    // Create a grid of dummy tiles for demonstration
    const double tile_size = 0.01; // Approximately 1km x 1km tiles
    const double min_lat = 21.0;   // Vietnam coordinates
    const double max_lat = 21.1;
    const double min_lon = 105.8;
    const double max_lon = 105.9;
    
    uint32_t tile_id = 1;
    uint64_t file_offset = 0;
    
    for (double lat = min_lat; lat < max_lat; lat += tile_size) {
        for (double lon = min_lon; lon < max_lon; lon += tile_size) {
            TileInfo tile_info;
            tile_info.tile_id = tile_id++;
            tile_info.bounds = BoundingBox(lat, lon, lat + tile_size, lon + tile_size);
            tile_info.file_offset = file_offset;
            tile_info.data_size = 1024; // Dummy size
            
            tile_index_.push_back(tile_info);
            
            file_offset += tile_info.data_size;
        }
    }
}

void MapDataLoader::createDummyTileIndex() {
    // Create a grid of dummy tiles for demonstration
    const double tile_size = 0.01; // Approximately 1km x 1km tiles
    const double min_lat = 21.0;   // Vietnam coordinates
    const double max_lat = 21.1;
    const double min_lon = 105.8;
    const double max_lon = 105.9;
    
    uint32_t tile_id = 1;
    uint64_t file_offset = 0;
    
    for (double lat = min_lat; lat < max_lat; lat += tile_size) {
        for (double lon = min_lon; lon < max_lon; lon += tile_size) {
            TileInfo tile_info;
            tile_info.tile_id = tile_id++;
            tile_info.bounds = BoundingBox(lat, lon, lat + tile_size, lon + tile_size);
            tile_info.file_offset = file_offset;
            tile_info.data_size = 1024; // Dummy size
            
            tile_index_.push_back(tile_info);
            
            file_offset += tile_info.data_size;
        }
    }
}

bool MapDataLoader::loadTileFromFile(uint32_t tile_id, MapTile& tile) {
    // Find tile info
    const TileInfo* tile_info = nullptr;
    for (const auto& info : tile_index_) {
        if (info.tile_id == tile_id) {
            tile_info = &info;
            break;
        }
    }
    
    if (!tile_info) {
        return false;
    }
    
    // Initialize tile
    tile.tile_id = tile_id;
    tile.bounds = tile_info->bounds;
    tile.is_loaded = true;
    tile.last_accessed = NavUtils::getCurrentTimestampMs();
    
    // Create dummy nodes and edges for demonstration
    createDummyTileData(tile);
    
    return true;
}

void MapDataLoader::createDummyTileData(MapTile& tile) {
    // Create a simple grid of nodes within the tile
    const double step = 0.001; // Smaller step for more nodes
    uint32_t node_id = tile.tile_id * 1000; // Base node ID for this tile
    
    // Create nodes
    for (double lat = tile.bounds.minLat; lat <= tile.bounds.maxLat; lat += step) {
        for (double lon = tile.bounds.minLon; lon <= tile.bounds.maxLon; lon += step) {
            MapNode node;
            node.id = node_id++;
            node.position = Point(lat, lon);
            node.node_type = 0; // Regular waypoint
            tile.nodes.push_back(node);
        }
    }
    
    // Create edges connecting adjacent nodes
    for (size_t i = 0; i < tile.nodes.size(); ++i) {
        for (size_t j = i + 1; j < tile.nodes.size(); ++j) {
            double distance = NavUtils::haversineDistance(
                tile.nodes[i].position, tile.nodes[j].position);
            
            // Connect nodes that are close to each other
            if (distance < 150.0) { // 150 meters
                MapEdge edge;
                edge.from_node = tile.nodes[i].id;
                edge.to_node = tile.nodes[j].id;
                edge.length_meters = distance;
                edge.road_type = 1; // Street
                edge.speed_limit = 50; // 50 km/h
                edge.flags = 0; // No special flags
                
                tile.edges.push_back(edge);
                
                // Create reverse edge for bidirectional roads
                MapEdge reverse_edge = edge;
                reverse_edge.from_node = edge.to_node;
                reverse_edge.to_node = edge.from_node;
                tile.edges.push_back(reverse_edge);
            }
        }
    }
}

void MapDataLoader::createDummyTileData(MapTile& tile) {
    // Create a simple grid of nodes within the tile
    const double step = 0.001; // Smaller step for more nodes
    uint32_t node_id = tile.tile_id * 1000; // Base node ID for this tile
    
    // Create nodes
    for (double lat = tile.bounds.minLat; lat <= tile.bounds.maxLat; lat += step) {
        for (double lon = tile.bounds.minLon; lon <= tile.bounds.maxLon; lon += step) {
            MapNode node;
            node.id = node_id++;
            node.position = Point(lat, lon);
            node.node_type = 0; // Regular waypoint
            tile.nodes.push_back(node);
        }
    }
    
    // Create edges connecting adjacent nodes
    for (size_t i = 0; i < tile.nodes.size(); ++i) {
        for (size_t j = i + 1; j < tile.nodes.size(); ++j) {
            double distance = NavUtils::haversineDistance(
                tile.nodes[i].position, tile.nodes[j].position);
            
            // Connect nodes that are close to each other
            if (distance < 150.0) { // 150 meters
                MapEdge edge;
                edge.from_node = tile.nodes[i].id;
                edge.to_node = tile.nodes[j].id;
                edge.length_meters = distance;
                edge.road_type = 1; // Street
                edge.speed_limit = 50; // 50 km/h
                edge.flags = 0; // No special flags
                
                tile.edges.push_back(edge);
                
                // Create reverse edge for bidirectional roads
                MapEdge reverse_edge = edge;
                reverse_edge.from_node = edge.to_node;
                reverse_edge.to_node = edge.from_node;
                tile.edges.push_back(reverse_edge);
            }
        }
    }
}

} // namespace nav