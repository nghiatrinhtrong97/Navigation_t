#ifndef ADDRESS_COMPONENTS_H
#define ADDRESS_COMPONENTS_H

#include <string>
#include <optional>
#include <vector>
#include <unordered_map>
#include "../../common/include/nav_types.h"

namespace nav {
namespace geocoding {

/**
 * @brief Comprehensive address component structure
 * Supports US, international, and complex addresses
 */
struct AddressComponents {
    // Street address components
    std::optional<std::string> house_number;      // "123", "456A"
    std::optional<std::string> street_prefix;     // "North", "East"
    std::optional<std::string> street_name;       // "Main"
    std::optional<std::string> street_type;       // "Street", "Avenue"
    std::optional<std::string> street_suffix;     // "Northwest"
    
    // Building/Unit components
    std::optional<std::string> unit_type;         // "Apartment", "Suite"
    std::optional<std::string> unit_number;       // "4B", "200"
    std::optional<std::string> building_name;     // "Empire State Building"
    std::optional<std::string> floor;             // "5th Floor"
    
    // Intersection addresses
    std::optional<std::string> intersection_street1;
    std::optional<std::string> intersection_street2;
    
    // Locality components
    std::optional<std::string> sublocality;       // "SoMa", "Downtown"
    std::optional<std::string> city;              // "San Francisco"
    std::optional<std::string> county;            // "San Francisco County"
    std::optional<std::string> state;             // "California"
    std::optional<std::string> state_code;        // "CA"
    
    // Postal components
    std::optional<std::string> postal_code;       // "94102"
    std::optional<std::string> postal_code_suffix; // "1234"
    std::optional<std::string> po_box;            // "PO Box 1234"
    
    // Country
    std::optional<std::string> country;           // "United States"
    std::optional<std::string> country_code;      // "US" (ISO 3166-1)
    
    // Metadata
    std::string input_language;                    // "en", "es", etc.
    bool is_intersection;                          // true if corner address
    bool is_po_box;                                // true if PO Box
    
    AddressComponents() 
        : input_language("en")
        , is_intersection(false)
        , is_po_box(false) {}
    
    /**
     * @brief Convert to formatted string
     * @param format Format type: "usps", "google", "custom"
     * @return Formatted address string
     */
    std::string toFormattedString(const std::string& format = "usps") const;
    
    /**
     * @brief Check if address is complete enough for geocoding
     * @return true if address has minimum required fields
     */
    bool isGeocodable() const;
    
    /**
     * @brief Get all non-empty components as key-value map
     * @return Map of component name to value
     */
    std::unordered_map<std::string, std::string> toMap() const;
    
    /**
     * @brief Create address components from simple fields
     * @param house_num House number
     * @param street Street name
     * @param city_name City
     * @param state_name State
     * @param zip ZIP code
     * @return Populated AddressComponents
     */
    static AddressComponents fromBasicFields(
        const std::string& house_num,
        const std::string& street,
        const std::string& city_name,
        const std::string& state_name,
        const std::string& zip);

private:
    /**
     * @brief Format address in USPS standard
     * @return USPS formatted address string
     */
    std::string toUSPSFormat() const;
    
    /**
     * @brief Format address for display
     * @return Display formatted address string
     */
    std::string toDisplayFormat() const;
};

/**
 * @brief Geocoding quality levels (matches Google Maps tiers)
 */
enum class GeocodingQuality : uint8_t {
    ROOFTOP = 0,              // Building-specific (accuracy: 1-10m)
    RANGE_INTERPOLATED = 1,   // Interpolated from address range (10-50m)
    GEOMETRIC_CENTER = 2,     // Center of street segment (50-200m)
    APPROXIMATE = 3,          // ZIP/city centroid (200-5000m)
    UNKNOWN = 4
};

/**
 * @brief Convert quality enum to string
 */
inline std::string qualityToString(GeocodingQuality quality) {
    switch (quality) {
        case GeocodingQuality::ROOFTOP: return "ROOFTOP";
        case GeocodingQuality::RANGE_INTERPOLATED: return "RANGE_INTERPOLATED";
        case GeocodingQuality::GEOMETRIC_CENTER: return "GEOMETRIC_CENTER";
        case GeocodingQuality::APPROXIMATE: return "APPROXIMATE";
        default: return "UNKNOWN";
    }
}

/**
 * @brief Convert quality to accuracy radius in meters
 */
inline double qualityToAccuracyMeters(GeocodingQuality quality) {
    switch (quality) {
        case GeocodingQuality::ROOFTOP: return 5.0;
        case GeocodingQuality::RANGE_INTERPOLATED: return 30.0;
        case GeocodingQuality::GEOMETRIC_CENTER: return 100.0;
        case GeocodingQuality::APPROXIMATE: return 1000.0;
        default: return 5000.0;
    }
}

/**
 * @brief Geocoding bias preferences
 */
enum class GeocodingBias : uint8_t {
    NONE = 0,
    FAVOR_ROOFTOP = 1,        // Only accept building-level
    FAVOR_STREET = 2,         // Street segment acceptable
    FAVOR_POSTAL = 3,         // ZIP code centroid OK
    FAVOR_LOCALITY = 4        // City-level acceptable
};

/**
 * @brief Single geocoding result candidate
 */
struct GeocodingCandidate {
    double latitude;
    double longitude;
    double altitude;                      // Elevation in meters
    
    AddressComponents components;         // Parsed address components
    std::string formatted_address;        // Human-readable full address
    
    double confidence_score;              // 0.0 - 1.0 (match quality)
    GeocodingQuality quality;             // Quality tier
    double accuracy_meters;               // Horizontal uncertainty
    
    std::string match_type;               // "exact", "partial", "fuzzy"
    std::string data_source;              // "TIGER", "OSM", "HERE"
    std::string geocoding_method;         // "parcel", "interpolation"
    
    uint64_t timestamp_ms;                // When geocoded
    
    GeocodingCandidate()
        : latitude(0.0)
        , longitude(0.0)
        , altitude(0.0)
        , confidence_score(0.0)
        , quality(GeocodingQuality::UNKNOWN)
        , accuracy_meters(5000.0)
        , match_type("unknown")
        , data_source("unknown")
        , geocoding_method("unknown")
        , timestamp_ms(0) {}
    
    /**
     * @brief Calculate relevance score based on distance and confidence
     * @param reference_point User's location for proximity bias
     * @return Relevance score (0.0 - 1.0)
     */
    double calculateRelevanceScore(const Point& reference_point) const;
};

} // namespace geocoding
} // namespace nav

#endif // ADDRESS_COMPONENTS_H
