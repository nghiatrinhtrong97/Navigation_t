#ifndef ADDRESS_NORMALIZER_H
#define ADDRESS_NORMALIZER_H

#include "address_components.h"
#include <string>
#include <unordered_map>

namespace nav {
namespace geocoding {

/**
 * @brief Normalize and standardize address components
 * Handles abbreviations, case, punctuation per USPS Publication 28
 */
class AddressNormalizer {
public:
    /**
     * @brief Normalize all components in-place
     * Applies USPS standardization rules
     */
    static void normalize(AddressComponents& components);
    
    /**
     * @brief Expand/standardize street type abbreviation (St → ST, Avenue → AVE)
     */
    static std::string normalizeStreetType(const std::string& abbrev);
    
    /**
     * @brief Normalize directionals (N, North, NORTH → N)
     */
    static std::string normalizeDirectional(const std::string& dir);
    
    /**
     * @brief Normalize unit type (Apt, Apartment, APT → APT)
     */
    static std::string normalizeUnitType(const std::string& unit);
    
    /**
     * @brief Normalize state name (California, CA, calif → CA)
     */
    static std::string normalizeState(const std::string& state);
    
    /**
     * @brief Clean text: uppercase, remove extra spaces, standardize punctuation
     */
    static std::string cleanText(const std::string& text);
    
private:
    // USPS-approved abbreviation lookup tables
    static const std::unordered_map<std::string, std::string> STREET_TYPES;
    static const std::unordered_map<std::string, std::string> DIRECTIONALS;
    static const std::unordered_map<std::string, std::string> UNIT_TYPES;
    static const std::unordered_map<std::string, std::string> STATE_CODES;
    
    static std::string toUpper(const std::string& str);
    static std::string trim(const std::string& str);
};

} // namespace geocoding
} // namespace nav

#endif // ADDRESS_NORMALIZER_H
