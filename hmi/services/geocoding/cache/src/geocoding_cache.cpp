#include "geocoding_cache.h"
#include <algorithm>

namespace geocoding {
namespace cache {

InMemoryGeocodingCache::InMemoryGeocodingCache(size_t max_size, int ttl_seconds)
    : m_max_size(max_size)
    , m_ttl_seconds(ttl_seconds)
    , m_hits(0)
    , m_misses(0)
    , m_evictions(0) {
}

void InMemoryGeocodingCache::put(const std::string& query, const nav::geocoding::EnhancedGeocodingResult& result) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::string key = generateCacheKey(query);
    
    // Check if key exists
    auto it = m_cache_map.find(key);
    if (it != m_cache_map.end()) {
        // Update existing entry
        auto& [cache_entry, lru_iterator] = it->second;
        cache_entry.result = std::make_shared<nav::geocoding::EnhancedGeocodingResult>(result);
        cache_entry.timestamp = std::chrono::steady_clock::now();
        cache_entry.access_count++;
        
        // Move to front (most recent)
        m_lru_list.erase(lru_iterator);
        m_lru_list.push_front(key);
        lru_iterator = m_lru_list.begin();
    } else {
        // Check if cache is full
        if (m_cache_map.size() >= m_max_size) {
            evictLRU();
        }
        
        // Insert new entry
        CacheEntry entry;
        entry.result = std::make_shared<nav::geocoding::EnhancedGeocodingResult>(result);
        entry.timestamp = std::chrono::steady_clock::now();
        entry.access_count = 1;
        
        m_lru_list.push_front(key);
        m_cache_map[key] = {entry, m_lru_list.begin()};
    }
}

std::optional<nav::geocoding::EnhancedGeocodingResult> InMemoryGeocodingCache::get(const std::string& query) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::string key = generateCacheKey(query);
    
    auto it = m_cache_map.find(key);
    if (it == m_cache_map.end()) {
        ++m_misses;
        return std::nullopt;
    }
    
    // Check if expired
    auto& [cache_entry, lru_iterator] = it->second;
    if (isExpired(cache_entry)) {
        // Remove expired entry
        m_lru_list.erase(lru_iterator);
        m_cache_map.erase(it);
        ++m_misses;
        return std::nullopt;
    }
    
    // Update access info
    cache_entry.access_count++;
    
    // Move to front (most recent)
    m_lru_list.erase(lru_iterator);
    m_lru_list.push_front(key);
    lru_iterator = m_lru_list.begin();
    
    ++m_hits;
    return *(cache_entry.result);
}

bool InMemoryGeocodingCache::contains(const std::string& query) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::string key = generateCacheKey(query);
    auto it = m_cache_map.find(key);
    
    if (it == m_cache_map.end()) {
        return false;
    }
    
    const auto& [cache_entry, lru_iterator] = it->second;
    return !isExpired(cache_entry);
}

void InMemoryGeocodingCache::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    m_cache_map.clear();
    m_lru_list.clear();
    m_hits = 0;
    m_misses = 0;
    m_evictions = 0;
}

size_t InMemoryGeocodingCache::evictExpired() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    size_t evicted = 0;
    auto it = m_cache_map.begin();
    
    while (it != m_cache_map.end()) {
        auto& [cache_entry, lru_iterator] = it->second;
        if (isExpired(cache_entry)) {
            m_lru_list.erase(lru_iterator);
            it = m_cache_map.erase(it);
            ++evicted;
            ++m_evictions;
        } else {
            ++it;
        }
    }
    
    return evicted;
}

CacheStats InMemoryGeocodingCache::getStats() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    CacheStats stats;
    stats.hits = m_hits;
    stats.misses = m_misses;
    stats.evictions = m_evictions;
    stats.current_size = m_cache_map.size();
    
    size_t total_requests = m_hits + m_misses;
    stats.hit_rate = total_requests > 0 
        ? static_cast<double>(m_hits) / total_requests 
        : 0.0;
    
    return stats;
}

size_t InMemoryGeocodingCache::size() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_cache_map.size();
}

bool InMemoryGeocodingCache::empty() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_cache_map.empty();
}

void InMemoryGeocodingCache::setMaxSize(size_t max_size) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_max_size = max_size;
    
    // Evict excess entries if needed
    while (m_cache_map.size() > m_max_size) {
        evictLRU();
    }
}

void InMemoryGeocodingCache::setTTL(int ttl_seconds) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_ttl_seconds = ttl_seconds;
}

bool InMemoryGeocodingCache::isExpired(const CacheEntry& entry) const {
    if (m_ttl_seconds <= 0) {
        return false;  // No expiration
    }
    
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - entry.timestamp);
    
    return elapsed.count() >= m_ttl_seconds;
}

void InMemoryGeocodingCache::evictLRU() {
    if (m_lru_list.empty()) {
        return;
    }
    
    // Remove least recently used (back of list)
    std::string lru_key = m_lru_list.back();
    m_lru_list.pop_back();
    m_cache_map.erase(lru_key);
    ++m_evictions;
}

void InMemoryGeocodingCache::updateLRU(const std::string& query) {
    std::string key = generateCacheKey(query);
    auto it = m_cache_map.find(key);
    
    if (it != m_cache_map.end()) {
        // Move to front
        auto& [cache_entry, lru_iterator] = it->second;
        m_lru_list.erase(lru_iterator);
        m_lru_list.push_front(key);
        lru_iterator = m_lru_list.begin();
    }
}

std::string InMemoryGeocodingCache::generateCacheKey(const std::string& query) {
    // Normalize query for consistent caching
    std::string key = query;
    
    // Convert to lowercase
    std::transform(key.begin(), key.end(), key.begin(), ::tolower);
    
    // Remove extra whitespace
    key.erase(std::unique(key.begin(), key.end(),
        [](char a, char b) { return std::isspace(a) && std::isspace(b); }),
        key.end());
    
    return key;
}

}} // namespace geocoding::cache
