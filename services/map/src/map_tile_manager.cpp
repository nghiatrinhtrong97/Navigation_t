#include "map_service.h"
#include <algorithm>

namespace nav {

MapTileManager::MapTileManager(size_t max_cache_size)
    : cache_(max_cache_size)
    , max_cache_size_(max_cache_size)
{
}

std::shared_ptr<MapTile> MapTileManager::getTile(uint32_t tile_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    // Check if tile is already in cache
    std::shared_ptr<MapTile> tile;
    if (cache_.get(tile_id, tile)) {
        tile->last_accessed = NavUtils::getCurrentTimestampMs();
        return tile;
    }
    
    // Load tile from data loader
    if (!data_loader_) {
        return nullptr;
    }
    
    tile = std::make_shared<MapTile>();
    if (data_loader_->loadTile(tile_id, *tile)) {
        cache_.put(tile_id, tile);
        return tile;
    }
    
    return nullptr;
}

void MapTileManager::preloadTiles(const BoundingBox& bbox) {
    if (!data_loader_) {
        return;
    }
    
    auto tile_ids = data_loader_->getTilesInBoundingBox(bbox);
    
    for (uint32_t tile_id : tile_ids) {
        getTile(tile_id); // This will load and cache the tile
    }
}

void MapTileManager::clearCache() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    cache_.clear();
}

void MapTileManager::setMapDataLoader(std::shared_ptr<MapDataLoader> loader) {
    data_loader_ = loader;
}

void MapTileManager::evictOldTiles() {
    // The LRU cache automatically handles eviction
    // Additional logic could be added here for specific eviction policies
}

} // namespace nav