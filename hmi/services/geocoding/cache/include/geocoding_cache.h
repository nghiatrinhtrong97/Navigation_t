#pragma once
#include "enhanced_geocoding.h"  // Need full definition for struct member
#include <string>
#include <unordered_map>
#include <list>
#include <mutex>
#include <chrono>
#include <optional>
#include <memory>

namespace geocoding {
namespace cache {

/**
 * @brief Cache entry with timestamp for TTL support
 */
struct CacheEntry {
    std::shared_ptr<nav::geocoding::EnhancedGeocodingResult> result;
    std::chrono::steady_clock::time_point timestamp;
    size_t access_count = 0;
};

/**
 * @brief Cache statistics for monitoring
 */
struct CacheStats {
    size_t hits = 0;
    size_t misses = 0;
    size_t evictions = 0;
    size_t current_size = 0;
    double hit_rate = 0.0;
};

/**
 * @brief LRU (Least Recently Used) in-memory cache for geocoding results
 * Thread-safe with TTL support
 */
class InMemoryGeocodingCache {
public:
    /**
     * @brief Constructor
     * @param max_size Maximum number of entries in cache
     * @param ttl_seconds Time-to-live in seconds (0 = no expiration)
     */
    explicit InMemoryGeocodingCache(size_t max_size = 1000, 
                                    int ttl_seconds = 3600);

    /**
     * @brief Store geocoding result in cache
     * @param query Query string (cache key)
     * @param result Geocoding result to cache
     */
    void put(const std::string& query, const nav::geocoding::EnhancedGeocodingResult& result);

    /**
     * @brief Retrieve cached result
     * @param query Query string (cache key)
     * @return Cached result if found and not expired, nullopt otherwise
     */
    std::optional<nav::geocoding::EnhancedGeocodingResult> get(const std::string& query);

    /**
     * @brief Check if query exists in cache (without updating LRU)
     * @param query Query string
     * @return True if cached and not expired
     */
    bool contains(const std::string& query) const;

    /**
     * @brief Clear all cached entries
     */
    void clear();

    /**
     * @brief Remove expired entries
     * @return Number of entries removed
     */
    size_t evictExpired();

    /**
     * @brief Get cache statistics
     * @return Current cache statistics
     */
    CacheStats getStats() const;

    /**
     * @brief Get current cache size
     * @return Number of entries in cache
     */
    size_t size() const;

    /**
     * @brief Check if cache is empty
     * @return True if empty
     */
    bool empty() const;

    /**
     * @brief Set maximum cache size
     * @param max_size New maximum size
     */
    void setMaxSize(size_t max_size);

    /**
     * @brief Set time-to-live
     * @param ttl_seconds TTL in seconds (0 = no expiration)
     */
    void setTTL(int ttl_seconds);

private:
    // LRU list: most recent at front, least recent at back
    using CacheList = std::list<std::string>;
    using CacheMap = std::unordered_map<std::string, std::pair<CacheEntry, CacheList::iterator>>;

    size_t m_max_size;
    int m_ttl_seconds;
    
    CacheList m_lru_list;  // LRU ordering
    CacheMap m_cache_map;  // Fast lookup
    
    mutable std::mutex m_mutex;  // Thread safety
    
    // Statistics
    mutable size_t m_hits;
    mutable size_t m_misses;
    size_t m_evictions;

    // Helper: Check if entry is expired
    bool isExpired(const CacheEntry& entry) const;

    // Helper: Evict least recently used entry
    void evictLRU();

    // Helper: Update LRU position
    void updateLRU(const std::string& query);

    // Helper: Generate cache key from query
    static std::string generateCacheKey(const std::string& query);
};

}} // namespace geocoding::cache
