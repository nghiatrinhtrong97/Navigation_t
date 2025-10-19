#ifndef ENHANCED_GEOCODING_H
#define ENHANCED_GEOCODING_H

#include "address_components.h"
#include "../../common/include/nav_types.h"
#include <vector>
#include <optional>

namespace nav {
namespace geocoding {

/**
 * @brief Enhanced geocoding request with preferences
 */
struct EnhancedAddressRequest {
    // Option 1: Structured address components
    std::optional<AddressComponents> components;
    
    // Option 2: Freeform text (will be parsed)
    std::optional<std::string> freeform_address;
    
    // Geocoding preferences
    GeocodingBias bias;                   // Quality preference
    double search_radius_meters;          // Limit search area (0 = no limit)
    std::optional<Point> reference_location; // For ambiguity resolution
    
    // Parsing hints
    std::string country_hint;             // "US", "CA", "MX"
    std::string language_hint;            // "en", "es"
    
    // Result limits
    size_t max_results;                   // Max candidates to return
    double min_confidence_threshold;      // Filter low-quality matches
    
    EnhancedAddressRequest()
        : bias(GeocodingBias::NONE)
        , search_radius_meters(0.0)
        , country_hint("US")
        , language_hint("en")
        , max_results(1)
        , min_confidence_threshold(0.5) {}
    
    /**
     * @brief Check if request is valid
     */
    bool isValid() const {
        return components.has_value() || freeform_address.has_value();
    }
    
    /**
     * @brief Get address string for logging
     */
    std::string getAddressString() const {
        if (freeform_address) {
            return *freeform_address;
        } else if (components) {
            return components->toFormattedString();
        }
        return "[empty]";
    }
};

/**
 * @brief Enhanced geocoding response with multiple candidates
 */
struct EnhancedGeocodingResult {
    bool success;
    std::string error_message;            // If success=false
    
    // Primary result (best match)
    GeocodingCandidate primary_result;
    
    // Alternative matches (ranked by relevance)
    std::vector<GeocodingCandidate> alternatives;
    
    // Performance metrics
    double processing_time_ms;
    size_t candidates_evaluated;
    bool used_cache;
    
    // Viewport for map display
    std::optional<BoundingBox> viewport;
    
    EnhancedGeocodingResult()
        : success(false)
        , processing_time_ms(0.0)
        , candidates_evaluated(0)
        , used_cache(false) {}
    
    /**
     * @brief Get all results (primary + alternatives)
     */
    std::vector<GeocodingCandidate> getAllCandidates() const {
        std::vector<GeocodingCandidate> all;
        if (success && primary_result.confidence_score > 0.0) {
            all.push_back(primary_result);
        }
        all.insert(all.end(), alternatives.begin(), alternatives.end());
        return all;
    }
    
    /**
     * @brief Get best candidate (alias for primary_result)
     */
    const GeocodingCandidate& getBest() const {
        return primary_result;
    }
    
    /**
     * @brief Check if result has any valid candidates
     */
    bool hasCandidates() const {
        return success && primary_result.confidence_score > 0.0;
    }
};

} // namespace geocoding
} // namespace nav

#endif // ENHANCED_GEOCODING_H
