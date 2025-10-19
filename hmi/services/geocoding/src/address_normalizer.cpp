#include "../include/address_normalizer.h"
#include <algorithm>
#include <cctype>
#include <regex>

namespace nav {
namespace geocoding {

// USPS Publication 28 - Official Street Suffix Abbreviations
const std::unordered_map<std::string, std::string> 
AddressNormalizer::STREET_TYPES = {
    // Common types
    {"STREET", "ST"}, {"ST", "ST"}, {"STR", "ST"}, {"STRT", "ST"},
    {"AVENUE", "AVE"}, {"AVE", "AVE"}, {"AV", "AVE"}, {"AVEN", "AVE"},
    {"BOULEVARD", "BLVD"}, {"BLVD", "BLVD"}, {"BOUL", "BLVD"}, {"BOULV", "BLVD"},
    {"ROAD", "RD"}, {"RD", "RD"},
    {"DRIVE", "DR"}, {"DR", "DR"}, {"DRIV", "DR"}, {"DRV", "DR"},
    {"LANE", "LN"}, {"LN", "LN"},
    {"COURT", "CT"}, {"CT", "CT"}, {"CRT", "CT"},
    {"PLACE", "PL"}, {"PL", "PL"},
    {"CIRCLE", "CIR"}, {"CIR", "CIR"}, {"CIRC", "CIR"}, {"CIRCL", "CIR"},
    {"PARKWAY", "PKWY"}, {"PKWY", "PKWY"}, {"PARKWY", "PKWY"}, {"PKY", "PKWY"},
    {"HIGHWAY", "HWY"}, {"HWY", "HWY"}, {"HIGHWY", "HWY"}, {"HIWAY", "HWY"},
    {"TERRACE", "TER"}, {"TER", "TER"}, {"TERR", "TER"},
    {"WAY", "WAY"},
    {"TRAIL", "TRL"}, {"TRL", "TRL"}, {"TRAILS", "TRL"},
    {"SQUARE", "SQ"}, {"SQ", "SQ"}, {"SQR", "SQ"}, {"SQRE", "SQ"},
    {"PATH", "PATH"},
    {"ALLEY", "ALY"}, {"ALY", "ALY"}, {"ALLEE", "ALY"},
    {"BRIDGE", "BRG"}, {"BRG", "BRG"}, {"BRDGE", "BRG"},
    {"CREEK", "CRK"}, {"CRK", "CRK"},
    {"EXPRESSWAY", "EXPY"}, {"EXPY", "EXPY"}, {"EXPRESS", "EXPY"}, {"EXPW", "EXPY"},
    {"FREEWAY", "FWY"}, {"FWY", "FWY"}, {"FREEWY", "FWY"}, {"FRWY", "FWY"},
    {"GROVE", "GRV"}, {"GRV", "GRV"}, {"GROV", "GRV"},
    {"HEIGHTS", "HTS"}, {"HTS", "HTS"}, {"HEIGHT", "HTS"},
    {"MANOR", "MNR"}, {"MNR", "MNR"},
    {"PLAZA", "PLZ"}, {"PLZ", "PLZ"},
    {"RIDGE", "RDG"}, {"RDG", "RDG"},
    {"RUN", "RUN"},
    {"SPRING", "SPG"}, {"SPG", "SPG"}, {"SPNG", "SPG"},
    {"VALLEY", "VLY"}, {"VLY", "VLY"}, {"VALLY", "VLY"}, {"VLLY", "VLY"},
    {"VIEW", "VW"}, {"VW", "VW"},
    {"WALK", "WALK"}
};

const std::unordered_map<std::string, std::string> 
AddressNormalizer::DIRECTIONALS = {
    {"NORTH", "N"}, {"N", "N"},
    {"SOUTH", "S"}, {"S", "S"},
    {"EAST", "E"}, {"E", "E"},
    {"WEST", "W"}, {"W", "W"},
    {"NORTHEAST", "NE"}, {"NE", "NE"}, {"N E", "NE"},
    {"NORTHWEST", "NW"}, {"NW", "NW"}, {"N W", "NW"},
    {"SOUTHEAST", "SE"}, {"SE", "SE"}, {"S E", "SE"},
    {"SOUTHWEST", "SW"}, {"SW", "SW"}, {"S W", "SW"}
};

const std::unordered_map<std::string, std::string> 
AddressNormalizer::UNIT_TYPES = {
    {"APARTMENT", "APT"}, {"APT", "APT"},
    {"SUITE", "STE"}, {"STE", "STE"},
    {"UNIT", "UNIT"},
    {"FLOOR", "FL"}, {"FL", "FL"}, {"FLR", "FL"},
    {"ROOM", "RM"}, {"RM", "RM"},
    {"BUILDING", "BLDG"}, {"BLDG", "BLDG"}, {"BLDNG", "BLDG"},
    {"DEPARTMENT", "DEPT"}, {"DEPT", "DEPT"},
    {"LOBBY", "LBBY"}, {"LBBY", "LBBY"},
    {"LOT", "LOT"},
    {"PIER", "PIER"},
    {"SLIP", "SLIP"},
    {"SPACE", "SPC"}, {"SPC", "SPC"},
    {"STOP", "STOP"},
    {"TRAILER", "TRLR"}, {"TRLR", "TRLR"}
};

const std::unordered_map<std::string, std::string> 
AddressNormalizer::STATE_CODES = {
    // Full state names to codes
    {"ALABAMA", "AL"}, {"AL", "AL"},
    {"ALASKA", "AK"}, {"AK", "AK"},
    {"ARIZONA", "AZ"}, {"AZ", "AZ"},
    {"ARKANSAS", "AR"}, {"AR", "AR"},
    {"CALIFORNIA", "CA"}, {"CA", "CA"}, {"CALIF", "CA"},
    {"COLORADO", "CO"}, {"CO", "CO"}, {"COLO", "CO"},
    {"CONNECTICUT", "CT"}, {"CT", "CT"}, {"CONN", "CT"},
    {"DELAWARE", "DE"}, {"DE", "DE"},
    {"FLORIDA", "FL"}, {"FL", "FL"}, {"FLA", "FL"},
    {"GEORGIA", "GA"}, {"GA", "GA"},
    {"HAWAII", "HI"}, {"HI", "HI"},
    {"IDAHO", "ID"}, {"ID", "ID"},
    {"ILLINOIS", "IL"}, {"IL", "IL"}, {"ILL", "IL"},
    {"INDIANA", "IN"}, {"IN", "IN"}, {"IND", "IN"},
    {"IOWA", "IA"}, {"IA", "IA"},
    {"KANSAS", "KS"}, {"KS", "KS"}, {"KANS", "KS"},
    {"KENTUCKY", "KY"}, {"KY", "KY"},
    {"LOUISIANA", "LA"}, {"LA", "LA"},
    {"MAINE", "ME"}, {"ME", "ME"},
    {"MARYLAND", "MD"}, {"MD", "MD"},
    {"MASSACHUSETTS", "MA"}, {"MA", "MA"}, {"MASS", "MA"},
    {"MICHIGAN", "MI"}, {"MI", "MI"}, {"MICH", "MI"},
    {"MINNESOTA", "MN"}, {"MN", "MN"}, {"MINN", "MN"},
    {"MISSISSIPPI", "MS"}, {"MS", "MS"}, {"MISS", "MS"},
    {"MISSOURI", "MO"}, {"MO", "MO"},
    {"MONTANA", "MT"}, {"MT", "MT"}, {"MONT", "MT"},
    {"NEBRASKA", "NE"}, {"NE", "NE"}, {"NEBR", "NE"},
    {"NEVADA", "NV"}, {"NV", "NV"},
    {"NEW HAMPSHIRE", "NH"}, {"NH", "NH"},
    {"NEW JERSEY", "NJ"}, {"NJ", "NJ"},
    {"NEW MEXICO", "NM"}, {"NM", "NM"},
    {"NEW YORK", "NY"}, {"NY", "NY"},
    {"NORTH CAROLINA", "NC"}, {"NC", "NC"},
    {"NORTH DAKOTA", "ND"}, {"ND", "ND"},
    {"OHIO", "OH"}, {"OH", "OH"},
    {"OKLAHOMA", "OK"}, {"OK", "OK"}, {"OKLA", "OK"},
    {"OREGON", "OR"}, {"OR", "OR"}, {"ORE", "OR"},
    {"PENNSYLVANIA", "PA"}, {"PA", "PA"}, {"PENN", "PA"},
    {"RHODE ISLAND", "RI"}, {"RI", "RI"},
    {"SOUTH CAROLINA", "SC"}, {"SC", "SC"},
    {"SOUTH DAKOTA", "SD"}, {"SD", "SD"},
    {"TENNESSEE", "TN"}, {"TN", "TN"}, {"TENN", "TN"},
    {"TEXAS", "TX"}, {"TX", "TX"}, {"TEX", "TX"},
    {"UTAH", "UT"}, {"UT", "UT"},
    {"VERMONT", "VT"}, {"VT", "VT"},
    {"VIRGINIA", "VA"}, {"VA", "VA"},
    {"WASHINGTON", "WA"}, {"WA", "WA"}, {"WASH", "WA"},
    {"WEST VIRGINIA", "WV"}, {"WV", "WV"},
    {"WISCONSIN", "WI"}, {"WI", "WI"}, {"WISC", "WI"},
    {"WYOMING", "WY"}, {"WY", "WY"}, {"WYO", "WY"},
    {"DISTRICT OF COLUMBIA", "DC"}, {"DC", "DC"}
};

void AddressNormalizer::normalize(AddressComponents& comp) {
    // Normalize street components
    if (comp.street_name) {
        comp.street_name = cleanText(*comp.street_name);
    }
    if (comp.street_type) {
        comp.street_type = normalizeStreetType(*comp.street_type);
    }
    if (comp.street_prefix) {
        comp.street_prefix = normalizeDirectional(*comp.street_prefix);
    }
    if (comp.street_suffix) {
        comp.street_suffix = normalizeDirectional(*comp.street_suffix);
    }
    
    // Normalize building/unit
    if (comp.building_name) {
        comp.building_name = cleanText(*comp.building_name);
    }
    if (comp.unit_type) {
        comp.unit_type = normalizeUnitType(*comp.unit_type);
    }
    if (comp.unit_number) {
        comp.unit_number = cleanText(*comp.unit_number);
    }
    if (comp.floor) {
        comp.floor = cleanText(*comp.floor);
    }
    
    // Normalize locality
    if (comp.sublocality) {
        comp.sublocality = cleanText(*comp.sublocality);
    }
    if (comp.city) {
        comp.city = cleanText(*comp.city);
    }
    if (comp.county) {
        comp.county = cleanText(*comp.county);
    }
    if (comp.state) {
        std::string state_code = normalizeState(*comp.state);
        comp.state = state_code;
        comp.state_code = state_code;
    }
    
    // Normalize postal code (remove spaces, dashes standardized)
    if (comp.postal_code) {
        std::string zip = cleanText(*comp.postal_code);
        // Remove any non-digit characters
        zip.erase(std::remove_if(zip.begin(), zip.end(), 
                  [](char c) { return !std::isdigit(c); }), zip.end());
        comp.postal_code = zip;
    }
    
    // Normalize country
    if (comp.country) {
        comp.country = cleanText(*comp.country);
    }
}

std::string AddressNormalizer::normalizeStreetType(const std::string& abbrev) {
    std::string upper = toUpper(trim(abbrev));
    auto it = STREET_TYPES.find(upper);
    return (it != STREET_TYPES.end()) ? it->second : upper;
}

std::string AddressNormalizer::normalizeDirectional(const std::string& dir) {
    std::string upper = toUpper(trim(dir));
    auto it = DIRECTIONALS.find(upper);
    return (it != DIRECTIONALS.end()) ? it->second : upper;
}

std::string AddressNormalizer::normalizeUnitType(const std::string& unit) {
    std::string upper = toUpper(trim(unit));
    auto it = UNIT_TYPES.find(upper);
    return (it != UNIT_TYPES.end()) ? it->second : upper;
}

std::string AddressNormalizer::normalizeState(const std::string& state) {
    std::string upper = toUpper(trim(state));
    auto it = STATE_CODES.find(upper);
    return (it != STATE_CODES.end()) ? it->second : upper;
}

std::string AddressNormalizer::cleanText(const std::string& text) {
    if (text.empty()) return text;
    
    std::string cleaned = trim(text);
    cleaned = toUpper(cleaned);
    
    // Remove extra spaces
    std::regex multiple_spaces("\\s+");
    cleaned = std::regex_replace(cleaned, multiple_spaces, " ");
    
    // Remove most punctuation except hyphens in addresses
    std::regex punct("[^A-Z0-9\\s\\-]");
    cleaned = std::regex_replace(cleaned, punct, "");
    
    return trim(cleaned);
}

std::string AddressNormalizer::toUpper(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::toupper(c); });
    return result;
}

std::string AddressNormalizer::trim(const std::string& str) {
    if (str.empty()) return str;
    
    size_t first = str.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) return "";
    
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, last - first + 1);
}

} // namespace geocoding
} // namespace nav
