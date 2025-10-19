#ifndef POI_SERVICE_H
#define POI_SERVICE_H
#include <cstdint>
#include <QObject>
#include <QString>
#include <QJsonArray>
#include <QJsonObject>
#include <vector>
#include <memory>
#include "../../models/include/navigation_models.h"  // Use Point from navigation_models
#include "address_components.h"  // Will be found via geocoding include path
#include "enhanced_geocoding.h"  // Will be found via geocoding include path

// Forward declarations for Phase 1 components
namespace nav {
namespace geocoding {
    class AddressParser;
    class SpatialIndex;
}
}

namespace nav {

// Point struct is now from navigation_models.h
// Remove duplicate definition to avoid conflicts

/**
 * @brief Point of Interest data structure
 */
struct POI {
    uint64_t poi_id;
    double latitude;
    double longitude;
    std::string name;
    std::string category;
    std::string address;

    POI() : poi_id(0), latitude(0.0), longitude(0.0), name(""), category(""), address("") {}
};

/**
 * @brief 
 * Format for address struct
 * @deprecated Use nav::geocoding::AddressComponents for new code.
 * This struct is maintained for backward compatibility only.
 */
struct AddressRequest {  // fixed typo: Adress -> Address
    std::string street_number;
    std::string street_name;
    std::string city;
    std::string state;
    std::string zip_code;
    std::string country;
    
    /**
     * @brief Convert to enhanced address components
     */
    geocoding::AddressComponents toEnhanced() const;
};

/**
 * @deprecated Use nav::geocoding::EnhancedGeocodingResult for new code.
 * This struct is maintained for backward compatibility only.
 */
struct GeocodingResult {
    bool success;
    double latitude;
    double longitude;
    std::string standard_address;
    double accuracy; // e.g., in meters
    std::string match_type; // e.g., "exact", "approximate"

    GeocodingResult() 
        : success(false), latitude(0.0), longitude(0.0), standard_address(""), accuracy(0.0), match_type("") {}
    
    /**
     * @brief Create from enhanced result (uses primary candidate)
     */
    static GeocodingResult fromEnhanced(const geocoding::EnhancedGeocodingResult& enhanced);
};

class POIService : public QObject
{
    Q_OBJECT

public:
    explicit POIService(QObject* parent = nullptr);
    ~POIService();

    // Service lifecycle management
    bool initialize();
    void shutdown();
    bool isServiceReady() const;
    std::string getServiceStatus() const;

    // POI retrieval
    std::vector<POI> findNearbyPOIs(const Point& location, double radiusMeters, const QString& category = QString());
    std::vector<POI> searchPOIsByName(const QString& name);
    
    // Address to POI conversion
    POI getPOIFromAddress(const AddressRequest& address);  // fixed typo: Adress -> Address

    // === Legacy geocoding API (backward compatibility) ===
    // @deprecated Use geocodeAddressEnhanced() for better accuracy and features
    GeocodingResult geocodeAddress(const AddressRequest& address);  // fixed typo: Adress -> Address
    GeocodingResult reverseGeocode(double latitude, double longitude);
    
    // === Enhanced geocoding API (recommended for new code) ===
    /**
     * @brief Geocode address with enhanced features
     * @param request Address request with preferences and hints
     * @return Enhanced result with multiple candidates and metadata
     */
    geocoding::EnhancedGeocodingResult geocodeAddressEnhanced(
        const geocoding::EnhancedAddressRequest& request);
    
    /**
     * @brief Reverse geocode with enhanced address breakdown
     * @param latitude Latitude coordinate
     * @param longitude Longitude coordinate
     * @return Enhanced address components
     */
    geocoding::AddressComponents reverseGeocodeEnhanced(
        double latitude, double longitude);

private:
    //Core service state
    bool m_initialized;
    bool m_serviceReady;
    mutable double m_lastQueryTimeMs;

    //POI data storage and indexing
    std::vector<POI> m_poiDatabase;
    
    // Phase 1: Enhanced Geocoding Components
    std::unique_ptr<geocoding::SpatialIndex> m_spatialIndex;
    bool m_useEnhancedGeocoding;
    
    // Helper methods
    void loadPOIDatabase();
    void buildSpatialIndex();
    double calculateDistance(const Point& p1, const Point& p2) const;
    
    // Enhanced geocoding helpers
    geocoding::GeocodingCandidate matchAddressWithSpatialIndex(
        const geocoding::AddressComponents& normalized_address,
        const geocoding::EnhancedAddressRequest& request) const;
};

} // namespace nav

#endif // POI_SERVICE_H
