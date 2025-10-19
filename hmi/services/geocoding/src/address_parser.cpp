#include "../include/address_parser.h"
#include <QDebug>
#include <stdexcept>
#include <regex>
#include <sstream>

// Conditional libpostal support
#ifdef USE_LIBPOSTAL
extern "C" {
    #include <libpostal/libpostal.h>
}
#define LIBPOSTAL_AVAILABLE 1
#else
#define LIBPOSTAL_AVAILABLE 0
#endif

namespace nav {
namespace geocoding {

AddressParser& AddressParser::getInstance() {
    static AddressParser instance;
    return instance;
}

AddressParser::AddressParser() 
    : m_initialized(false)
    , m_use_libpostal(false) {
    
    qDebug() << "[ADDRESS PARSER] Initializing...";
    
#if LIBPOSTAL_AVAILABLE
    // Try to initialize libpostal
    if (libpostal_setup() && libpostal_setup_parser()) {
        m_use_libpostal = true;
        m_initialized = true;
        qDebug() << "[ADDRESS PARSER] Initialized with libpostal support";
    } else {
        qWarning() << "[ADDRESS PARSER] libpostal initialization failed, using fallback parser";
        m_use_libpostal = false;
        m_initialized = true;
    }
#else
    // Use fallback regex parser
    m_use_libpostal = false;
    m_initialized = true;
    qDebug() << "[ADDRESS PARSER] Using fallback regex parser (libpostal not available)";
    qDebug() << "[ADDRESS PARSER] For better parsing, rebuild with -DUSE_LIBPOSTAL=ON";
#endif
}

AddressParser::~AddressParser() {
#if LIBPOSTAL_AVAILABLE
    if (m_use_libpostal) {
        libpostal_teardown();
        libpostal_teardown_parser();
        qDebug() << "[ADDRESS PARSER] libpostal shut down";
    }
#endif
}

bool AddressParser::hasLibpostalSupport() const {
    return m_use_libpostal;
}

AddressComponents AddressParser::parse(
    const std::string& address_text,
    const std::string& country_hint) {
    
    if (!m_initialized) {
        throw std::runtime_error("AddressParser not initialized");
    }
    
#if LIBPOSTAL_AVAILABLE
    if (m_use_libpostal) {
        // Use libpostal parsing
        libpostal_address_parser_options_t options = 
            libpostal_get_address_parser_default_options();
        options.language = nullptr;  // Auto-detect
        options.country = country_hint.empty() ? nullptr : country_hint.c_str();
        
        libpostal_address_parser_response_t* response = 
            libpostal_parse_address(address_text.c_str(), options);
        
        if (!response) {
            qWarning() << "[ADDRESS PARSER] libpostal parsing failed:" << address_text.c_str();
            return AddressComponents();
        }
        
        AddressComponents comp;
        
        // Map libpostal labels to our fields
        for (size_t i = 0; i < response->num_components; i++) {
            std::string label = response->labels[i];
            std::string value = response->components[i];
            
            if (label == "house_number") {
                comp.house_number = value;
            } else if (label == "road") {
                comp.street_name = value;
            } else if (label == "unit") {
                comp.unit_number = value;
            } else if (label == "city") {
                comp.city = value;
            } else if (label == "state") {
                comp.state = value;
            } else if (label == "postcode") {
                comp.postal_code = value;
            } else if (label == "country") {
                comp.country = value;
            } else if (label == "suburb") {
                comp.sublocality = value;
            } else if (label == "house") {
                comp.building_name = value;
            } else if (label == "po_box") {
                comp.po_box = value;
                comp.is_po_box = true;
            } else if (label == "level") {
                comp.floor = value;
            }
        }
        
        libpostal_address_parser_response_destroy(response);
        return comp;
    }
#endif
    
    // Fallback to regex parser
    return parseWithRegex(address_text, country_hint);
}

AddressComponents AddressParser::parseWithRegex(
    const std::string& address_text,
    const std::string& country_hint) {
    
    AddressComponents comp;
    
    // Regex patterns for US addresses
    // Pattern: "123 Main St Apt 4B, San Francisco, CA 94102"
    
    // Extract postal code (5 or 9 digits at end)
    std::regex zip_regex(R"(\b(\d{5})(?:-(\d{4}))?\s*$)");
    std::smatch zip_match;
    std::string remaining = address_text;
    
    if (std::regex_search(remaining, zip_match, zip_regex)) {
        comp.postal_code = zip_match[1].str();
        if (zip_match[2].matched) {
            comp.postal_code_suffix = zip_match[2].str();
        }
        remaining = std::regex_replace(remaining, zip_regex, "");
    }
    
    // Extract state code (2 capital letters before zip)
    std::regex state_regex(R"(\b([A-Z]{2})\s*,?\s*$)");
    std::smatch state_match;
    
    if (std::regex_search(remaining, state_match, state_regex)) {
        comp.state_code = state_match[1].str();
        comp.state = state_match[1].str();
        remaining = std::regex_replace(remaining, state_regex, "");
    }
    
    // Extract city (word before state, after last comma)
    std::regex city_regex(R"(,\s*([A-Za-z\s]+)\s*,?\s*$)");
    std::smatch city_match;
    
    if (std::regex_search(remaining, city_match, city_regex)) {
        std::string city_str = city_match[1].str();
        // Trim whitespace
        city_str.erase(0, city_str.find_first_not_of(" \t"));
        city_str.erase(city_str.find_last_not_of(" \t") + 1);
        comp.city = city_str;
        remaining = std::regex_replace(remaining, city_regex, "");
    }
    
    // Extract unit/apartment (Apt 4B, Suite 200, #5A)
    std::regex unit_regex(R"(\b(Apt|Suite|Unit|#)\s*([A-Za-z0-9\-]+)\b)", 
                         std::regex_constants::icase);
    std::smatch unit_match;
    
    if (std::regex_search(remaining, unit_match, unit_regex)) {
        comp.unit_type = unit_match[1].str();
        comp.unit_number = unit_match[2].str();
        remaining = std::regex_replace(remaining, unit_regex, "");
    }
    
    // Extract house number and street (everything remaining at start)
    std::regex street_regex(R"(^\s*(\d+[A-Za-z]?)\s+(.+?)\s*,?)");
    std::smatch street_match;
    
    if (std::regex_search(remaining, street_match, street_regex)) {
        comp.house_number = street_match[1].str();
        std::string street_full = street_match[2].str();
        
        // Try to separate street type (St, Ave, Blvd, etc.)
        std::regex type_regex(R"(\b(Street|St|Avenue|Ave|Boulevard|Blvd|Road|Rd|Drive|Dr|Lane|Ln|Court|Ct|Place|Pl)\b\s*$)", 
                             std::regex_constants::icase);
        std::smatch type_match;
        
        if (std::regex_search(street_full, type_match, type_regex)) {
            comp.street_type = type_match[1].str();
            comp.street_name = std::regex_replace(street_full, type_regex, "");
            // Trim trailing space
            if (!comp.street_name->empty() && comp.street_name->back() == ' ') {
                comp.street_name->pop_back();
            }
        } else {
            comp.street_name = street_full;
        }
    }
    
    // Set country
    comp.country_code = country_hint.empty() ? "US" : country_hint;
    if (country_hint == "US" || country_hint.empty()) {
        comp.country = "United States";
    }
    
    comp.is_intersection = false;
    comp.is_po_box = false;
    
    qDebug() << "[ADDRESS PARSER] Parsed (regex):" 
             << (comp.house_number ? comp.house_number->c_str() : "")
             << (comp.street_name ? comp.street_name->c_str() : "")
             << (comp.city ? comp.city->c_str() : "")
             << (comp.state ? comp.state->c_str() : "")
             << (comp.postal_code ? comp.postal_code->c_str() : "");
    
    return comp;
}

} // namespace geocoding
} // namespace nav
