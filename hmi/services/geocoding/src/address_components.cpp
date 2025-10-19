#include "address_components.h"
#include <sstream>
#include <algorithm>

namespace nav {
namespace geocoding {

// ============================================================================
// AddressComponents Implementation
// ============================================================================

std::string AddressComponents::toFormattedString(const std::string& format) const {
    if (format == "usps" || format == "USPS") {
        // USPS 2-line format
        std::string line1, line2;
        return toDisplayFormat();
    } else {
        // Default: display format
        return toDisplayFormat();
    }
}

bool AddressComponents::isGeocodable() const {
    // Minimum requirement: Need at least one of:
    // 1. Street address (house_number + street_name)
    // 2. Intersection (intersection_street1 + intersection_street2)
    // 3. PO Box (po_box)
    // 4. Postal code + city
    
    // Case 1: Street address
    if (house_number.has_value() && street_name.has_value()) {
        // Must have city OR postal_code for geocoding
        if (city.has_value() || postal_code.has_value()) {
            return true;
        }
    }
    
    // Case 2: Intersection
    if (is_intersection && intersection_street1.has_value() && 
        intersection_street2.has_value()) {
        if (city.has_value() || postal_code.has_value()) {
            return true;
        }
    }
    
    // Case 3: PO Box
    if (is_po_box && po_box.has_value()) {
        if (city.has_value() && state.has_value() && postal_code.has_value()) {
            return true;
        }
    }
    
    // Case 4: Postal code + city (for approximate geocoding)
    if (postal_code.has_value() && city.has_value()) {
        return true;
    }
    
    return false;
}

std::unordered_map<std::string, std::string> AddressComponents::toMap() const {
    std::unordered_map<std::string, std::string> result;
    
    // Helper lambda to add optional fields
    auto addOptional = [&](const std::string& key, const std::optional<std::string>& value) {
        if (value.has_value() && !value->empty()) {
            result[key] = *value;
        }
    };
    
    // Street components
    addOptional("house_number", house_number);
    addOptional("street_prefix", street_prefix);
    addOptional("street_name", street_name);
    addOptional("street_type", street_type);
    addOptional("street_suffix", street_suffix);
    
    // Building/Unit
    addOptional("unit_type", unit_type);
    addOptional("unit_number", unit_number);
    addOptional("building_name", building_name);
    addOptional("floor", floor);
    
    // Intersection
    addOptional("intersection_street1", intersection_street1);
    addOptional("intersection_street2", intersection_street2);
    
    // Locality
    addOptional("sublocality", sublocality);
    addOptional("city", city);
    addOptional("county", county);
    addOptional("state", state);
    addOptional("state_code", state_code);
    
    // Postal
    addOptional("postal_code", postal_code);
    addOptional("postal_code_suffix", postal_code_suffix);
    addOptional("po_box", po_box);
    
    // Country
    addOptional("country", country);
    addOptional("country_code", country_code);
    
    // Metadata
    result["input_language"] = input_language;
    result["is_intersection"] = is_intersection ? "true" : "false";
    result["is_po_box"] = is_po_box ? "true" : "false";
    
    return result;
}

AddressComponents AddressComponents::fromBasicFields(
    const std::string& house_num,
    const std::string& street,
    const std::string& city_name,
    const std::string& state_name,
    const std::string& zip_code
) {
    AddressComponents addr;
    
    if (!house_num.empty()) addr.house_number = house_num;
    if (!street.empty()) addr.street_name = street;
    if (!city_name.empty()) addr.city = city_name;
    if (!state_name.empty()) addr.state = state_name;
    if (!zip_code.empty()) addr.postal_code = zip_code;
    
    addr.country_code = "US";
    addr.country = "United States";
    addr.is_intersection = false;
    addr.is_po_box = false;
    
    return addr;
}

// ============================================================================
// Private Helper Methods
// ============================================================================

std::string AddressComponents::toUSPSFormat() const {
    std::ostringstream oss;
    
    // Line 1: Primary address
    if (is_po_box && po_box.has_value()) {
        oss << *po_box;
    } else if (is_intersection) {
        // Intersection format: "ST1 & ST2"
        if (intersection_street1) oss << *intersection_street1;
        oss << " & ";
        if (intersection_street2) oss << *intersection_street2;
    } else {
        // Standard street address
        if (house_number) oss << *house_number << " ";
        if (street_prefix) oss << *street_prefix << " ";
        if (street_name) oss << *street_name;
        if (street_type) oss << " " << *street_type;
        if (street_suffix) oss << " " << *street_suffix;
        
        // Unit/Apartment (secondary address)
        if (unit_type && unit_number) {
            oss << " " << *unit_type << " " << *unit_number;
        } else if (unit_number) {
            oss << " #" << *unit_number;
        }
    }
    
    std::string line1 = oss.str();
    oss.str(""); // Clear
    
    // Line 2: City, State ZIP
    if (city) oss << *city;
    if (state_code) {
        oss << (city ? ", " : "") << *state_code;
    }
    if (postal_code) {
        oss << " " << *postal_code;
        if (postal_code_suffix) oss << "-" << *postal_code_suffix;
    }
    
    std::string line2 = oss.str();
    
    // Combine
    if (!line1.empty() && !line2.empty()) {
        return line1 + "\n" + line2;
    } else if (!line1.empty()) {
        return line1;
    } else {
        return line2;
    }
}

std::string AddressComponents::toDisplayFormat() const {
    std::ostringstream oss;
    
    // Building name (if present)
    if (building_name) {
        oss << *building_name << ", ";
    }
    
    // Primary address
    if (is_po_box && po_box.has_value()) {
        oss << *po_box;
    } else if (is_intersection) {
        if (intersection_street1) oss << *intersection_street1;
        oss << " & ";
        if (intersection_street2) oss << *intersection_street2;
    } else {
        // Street address
        if (house_number) oss << *house_number << " ";
        if (street_prefix) oss << *street_prefix << " ";
        if (street_name) oss << *street_name;
        if (street_type) oss << " " << *street_type;
        if (street_suffix) oss << " " << *street_suffix;
    }
    
    // Unit/Floor
    if (unit_type && unit_number) {
        oss << ", " << *unit_type << " " << *unit_number;
    } else if (unit_number) {
        oss << ", #" << *unit_number;
    }
    if (floor) {
        oss << ", Floor " << *floor;
    }
    
    // Locality
    if (sublocality) oss << ", " << *sublocality;
    if (city) oss << ", " << *city;
    if (county) oss << ", " << *county;
    if (state) oss << ", " << *state;
    
    // Postal code
    if (postal_code) {
        oss << " " << *postal_code;
        if (postal_code_suffix) oss << "-" << *postal_code_suffix;
    }
    
    // Country (if not US)
    if (country_code && *country_code != "US") {
        if (country) {
            oss << ", " << *country;
        }
    }
    
    return oss.str();
}

} // namespace geocoding
} // namespace nav
