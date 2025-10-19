#ifndef SPATIAL_INDEX_H
#define SPATIAL_INDEX_H

#include "../../../../common/include/nav_types.h"
#include "address_components.h"
#include <vector>
#include <memory>
#include <string>

// Forward declarations for Boost.Geometry types (avoid heavy header includes)
namespace boost { namespace geometry { namespace index {
    template<typename Value, typename Parameters, typename IndexableGetter,
             typename EqualTo, typename Allocator>
    class rtree;
}}}

namespace nav {
namespace geocoding {

/**
 * @brief Address record for spatial indexing
 */
struct AddressRecord {
    uint64_t id;                      // Unique ID
    Point location;                   // Lat/Lon
    std::string formatted_address;    // Full address string
    std::string normalized_address;   // Searchable format (uppercase, standardized)
    std::string postal_code;          // For quick filtering
    std::string city;
    std::string state;
    
    // Metadata
    std::string data_source;          // "TIGER", "OSM", "POI_DB"
    GeocodingQuality quality;
    uint64_t last_updated_ms;
    
    AddressRecord()
        : id(0)
        , quality(GeocodingQuality::UNKNOWN)
        , last_updated_ms(0) {}
};

/**
 * @brief Spatial index using R*-tree for fast geographic queries
 * Thread-safe for read operations after building
 * 
 * @note Requires Boost.Geometry library (header-only)
 * Performance: O(log n) query time vs O(n) linear search
 */
class SpatialIndex {
public:
    /**
     * @brief Constructor with R-tree parameters
     * @param max_elements Maximum elements per node (default: 16)
     *        Higher = more memory, faster build, slightly slower query
     * @param min_elements Minimum elements per node (default: 4)
     *        Lower = deeper tree, more nodes
     */
    explicit SpatialIndex(size_t max_elements = 16, size_t min_elements = 4);
    
    ~SpatialIndex();
    
    /**
     * @brief Build index from address database
     * @param addresses Vector of all addresses
     * @return true if successful
     * 
     * @note This is a bulk-load operation. For 1M addresses, takes ~2-5 seconds
     */
    bool buildIndex(const std::vector<AddressRecord>& addresses);
    
    /**
     * @brief Query addresses within bounding box
     * @param bbox Geographic bounding box
     * @return Addresses within box (unsorted)
     */
    std::vector<AddressRecord> queryRegion(const BoundingBox& bbox) const;
    
    /**
     * @brief Query addresses within radius
     * @param center Center point
     * @param radius_meters Radius in meters
     * @return Addresses within radius (sorted by distance ascending)
     */
    std::vector<AddressRecord> queryRadius(
        const Point& center, 
        double radius_meters) const;
    
    /**
     * @brief Find k-nearest neighbors
     * @param location Query point
     * @param k Number of neighbors
     * @return k nearest addresses (sorted by distance ascending)
     * 
     * @note Typical usage: k=10 for geocoding candidates
     */
    std::vector<AddressRecord> findNearestAddresses(
        const Point& location, 
        size_t k = 10) const;
    
    /**
     * @brief Get index statistics
     */
    struct IndexStats {
        size_t total_addresses;
        size_t tree_depth;
        size_t memory_bytes_estimate;
        double build_time_ms;
    };
    IndexStats getStats() const;
    
    /**
     * @brief Check if index is ready
     */
    bool isReady() const { return m_index_built; }
    
    /**
     * @brief Clear index and free memory
     */
    void clear();
    
private:
    // Opaque pointer to R-tree (PIMPL idiom to hide Boost types)
    class Impl;
    std::unique_ptr<Impl> m_impl;
    
    bool m_index_built;
    size_t m_address_count;
    double m_build_time_ms;
    
    // Helper: Calculate geodetic distance (Haversine formula)
    double calculateDistance(const Point& p1, const Point& p2) const;
};

} // namespace geocoding
} // namespace nav

#endif // SPATIAL_INDEX_H
