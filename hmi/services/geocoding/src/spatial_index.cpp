#include "../include/spatial_index.h"
#include "../../../../common/include/nav_utils.h"
#include <QDebug>
#include <algorithm>
#include <chrono>
#include <cmath>

// Boost.Geometry headers (conditional)
#ifdef USE_BOOST_GEOMETRY
#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/box.hpp>
#include <boost/geometry/index/rtree.hpp>
#define BOOST_GEOMETRY_AVAILABLE 1
#else
#define BOOST_GEOMETRY_AVAILABLE 0
#endif

namespace nav {
namespace geocoding {

#if BOOST_GEOMETRY_AVAILABLE

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

// Boost.Geometry type definitions
using BoostPoint = bg::model::point<double, 2, bg::cs::cartesian>;
using BoostBox = bg::model::box<BoostPoint>;
using RTreeValue = std::pair<BoostPoint, AddressRecord>;
using RTree = bgi::rtree<RTreeValue, bgi::rstar<16>>;

// PIMPL implementation with Boost R-tree
class SpatialIndex::Impl {
public:
    std::unique_ptr<RTree> rtree;
    
    Impl() : rtree(std::make_unique<RTree>()) {}
    
    BoostPoint toBoostPoint(const Point& p) const {
        return BoostPoint(p.longitude, p.latitude);
    }
    
    BoostBox toBoostBox(const BoundingBox& bbox) const {
        BoostPoint min_corner(bbox.minLon, bbox.minLat);
        BoostPoint max_corner(bbox.maxLon, bbox.maxLat);
        return BoostBox(min_corner, max_corner);
    }
};

#else

// Fallback implementation without Boost (simple linear search)
class SpatialIndex::Impl {
public:
    std::vector<AddressRecord> addresses;
    
    Impl() = default;
};

#endif

// ============================================================================
// SpatialIndex Implementation
// ============================================================================

SpatialIndex::SpatialIndex(size_t max_elements, size_t min_elements)
    : m_impl(std::make_unique<Impl>())
    , m_index_built(false)
    , m_address_count(0)
    , m_build_time_ms(0.0) {
    
#if BOOST_GEOMETRY_AVAILABLE
    qDebug() << "[SPATIAL INDEX] Created with Boost.Geometry R*-tree"
             << "(max_elements=" << max_elements << ")";
#else
    qDebug() << "[SPATIAL INDEX] Created with fallback linear search";
    qDebug() << "[SPATIAL INDEX] For better performance, rebuild with -DUSE_BOOST_GEOMETRY=ON";
    Q_UNUSED(max_elements);
    Q_UNUSED(min_elements);
#endif
}

SpatialIndex::~SpatialIndex() {
    clear();
}

bool SpatialIndex::buildIndex(const std::vector<AddressRecord>& addresses) {
    auto start = std::chrono::high_resolution_clock::now();
    
    qDebug() << "[SPATIAL INDEX] Building index for" << addresses.size() << "addresses...";
    
    clear();
    
#if BOOST_GEOMETRY_AVAILABLE
    // Build Boost R-tree
    std::vector<RTreeValue> values;
    values.reserve(addresses.size());
    
    for (const auto& addr : addresses) {
        BoostPoint point = m_impl->toBoostPoint(addr.location);
        values.emplace_back(point, addr);
    }
    
    // Bulk load (faster than incremental insert)
    m_impl->rtree = std::make_unique<RTree>(values.begin(), values.end());
    
#else
    // Fallback: Store addresses in vector
    m_impl->addresses = addresses;
#endif
    
    m_address_count = addresses.size();
    m_index_built = true;
    
    auto end = std::chrono::high_resolution_clock::now();
    m_build_time_ms = std::chrono::duration<double, std::milli>(end - start).count();
    
    qDebug() << "[SPATIAL INDEX] Built index in" << m_build_time_ms << "ms";
    qDebug() << "[SPATIAL INDEX] Index ready with" << m_address_count << "addresses";
    
    return true;
}

std::vector<AddressRecord> SpatialIndex::queryRegion(const BoundingBox& bbox) const {
    if (!m_index_built) {
        qWarning() << "[SPATIAL INDEX] Index not built!";
        return {};
    }
    
    std::vector<AddressRecord> results;
    
#if BOOST_GEOMETRY_AVAILABLE
    BoostBox boost_box = m_impl->toBoostBox(bbox);
    std::vector<RTreeValue> tree_results;
    
    m_impl->rtree->query(
        bgi::intersects(boost_box),
        std::back_inserter(tree_results)
    );
    
    for (const auto& [point, address_record] : tree_results) {
        results.push_back(address_record);
    }
    
#else
    // Fallback: Linear search
    for (const auto& addr : m_impl->addresses) {
        if (addr.location.latitude >= bbox.minLat &&
            addr.location.latitude <= bbox.maxLat &&
            addr.location.longitude >= bbox.minLon &&
            addr.location.longitude <= bbox.maxLon) {
            results.push_back(addr);
        }
    }
#endif
    
    qDebug() << "[SPATIAL INDEX] Region query returned" << results.size() << "results";
    return results;
}

std::vector<AddressRecord> SpatialIndex::queryRadius(
    const Point& center, 
    double radius_meters) const {
    
    if (!m_index_built) {
        qWarning() << "[SPATIAL INDEX] Index not built!";
        return {};
    }
    
    // Convert radius to approximate degrees (1 degree ≈ 111km at equator)
    double radius_degrees = radius_meters / 111000.0;
    
    // Create bounding box for initial filter
    BoundingBox bbox;
    bbox.minLat = center.latitude - radius_degrees;
    bbox.maxLat = center.latitude + radius_degrees;
    bbox.minLon = center.longitude - radius_degrees;
    bbox.maxLon = center.longitude + radius_degrees;
    
    // Query candidates in bounding box
    auto candidates = queryRegion(bbox);
    
    // Filter by exact geodetic distance and sort
    std::vector<std::pair<AddressRecord, double>> scored;
    for (const auto& addr : candidates) {
        double distance = calculateDistance(center, addr.location);
        if (distance <= radius_meters) {
            scored.emplace_back(addr, distance);
        }
    }
    
    // Sort by distance ascending
    std::sort(scored.begin(), scored.end(),
        [](const auto& a, const auto& b) { 
            return a.second < b.second; 
        });
    
    // Extract addresses
    std::vector<AddressRecord> results;
    results.reserve(scored.size());
    for (const auto& [address, distance] : scored) {
        results.push_back(address);
    }
    
    qDebug() << "[SPATIAL INDEX] Radius query returned" << results.size() 
             << "results within" << radius_meters << "m";
    return results;
}

std::vector<AddressRecord> SpatialIndex::findNearestAddresses(
    const Point& location, 
    size_t k) const {
    
    if (!m_index_built) {
        qWarning() << "[SPATIAL INDEX] Index not built!";
        return {};
    }
    
    std::vector<AddressRecord> results;
    
#if BOOST_GEOMETRY_AVAILABLE
    // Use R-tree k-NN query (efficient)
    BoostPoint query_point = m_impl->toBoostPoint(location);
    std::vector<RTreeValue> nearest;
    
    m_impl->rtree->query(
        bgi::nearest(query_point, k),
        std::back_inserter(nearest)
    );
    
    // Calculate exact distances and sort
    std::vector<std::pair<AddressRecord, double>> scored;
    for (const auto& [point, address_record] : nearest) {
        double distance = calculateDistance(location, address_record.location);
        scored.emplace_back(address_record, distance);
    }
    
    std::sort(scored.begin(), scored.end(),
        [](const auto& a, const auto& b) { 
            return a.second < b.second; 
        });
    
    for (const auto& [address, distance] : scored) {
        results.push_back(address);
    }
    
#else
    // Fallback: Calculate all distances and get top k
    std::vector<std::pair<AddressRecord, double>> scored;
    scored.reserve(m_impl->addresses.size());
    
    for (const auto& addr : m_impl->addresses) {
        double distance = calculateDistance(location, addr.location);
        scored.emplace_back(addr, distance);
    }
    
    // Partial sort to get k smallest
    if (scored.size() > k) {
        std::partial_sort(scored.begin(), scored.begin() + k, scored.end(),
            [](const auto& a, const auto& b) { 
                return a.second < b.second; 
            });
        scored.resize(k);
    } else {
        std::sort(scored.begin(), scored.end(),
            [](const auto& a, const auto& b) { 
                return a.second < b.second; 
            });
    }
    
    for (const auto& [address, distance] : scored) {
        results.push_back(address);
    }
#endif
    
    qDebug() << "[SPATIAL INDEX] k-NN query returned" << results.size() << "results";
    return results;
}

SpatialIndex::IndexStats SpatialIndex::getStats() const {
    IndexStats stats;
    stats.total_addresses = m_address_count;
    stats.tree_depth = 0;  // TODO: Calculate from R-tree if needed
    stats.memory_bytes_estimate = m_address_count * sizeof(AddressRecord);
    stats.build_time_ms = m_build_time_ms;
    return stats;
}

void SpatialIndex::clear() {
    if (m_index_built) {
#if BOOST_GEOMETRY_AVAILABLE
        m_impl->rtree->clear();
#else
        m_impl->addresses.clear();
#endif
        m_index_built = false;
        m_address_count = 0;
        qDebug() << "[SPATIAL INDEX] Cleared";
    }
}

double SpatialIndex::calculateDistance(const Point& p1, const Point& p2) const {
    // Haversine formula for great-circle distance
    const double R = 6371000.0; // Earth radius in meters
    
    double lat1_rad = p1.latitude * M_PI / 180.0;
    double lat2_rad = p2.latitude * M_PI / 180.0;
    double dlat = (p2.latitude - p1.latitude) * M_PI / 180.0;
    double dlon = (p2.longitude - p1.longitude) * M_PI / 180.0;
    
    double a = std::sin(dlat/2) * std::sin(dlat/2) +
               std::cos(lat1_rad) * std::cos(lat2_rad) *
               std::sin(dlon/2) * std::sin(dlon/2);
    double c = 2 * std::atan2(std::sqrt(a), std::sqrt(1-a));
    
    return R * c;
}

} // namespace geocoding
} // namespace nav
