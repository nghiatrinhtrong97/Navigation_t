# Phase 1 Implementation Plan: Foundation Enhancement (Weeks 1-4)

**Project:** Geocoder Modernization - Address Data Model & Spatial Indexing  
**Duration:** 4 Weeks (160 hours)  
**Team Size:** 2-3 Engineers  
**Date:** October 18, 2025

---

## 📋 EXECUTIVE SUMMARY

### Goals
1. **Enhanced Address Data Model** - Support complex US addresses (apartments, buildings, intersections)
2. **Address Parser & Normalizer** - Handle fuzzy inputs, abbreviations, typos
3. **Spatial Indexing (R-tree)** - Reduce geocoding latency from 50ms → <5ms

### Success Metrics
| Metric | Current | Target | Measure |
|--------|---------|--------|---------|
| Address fields supported | 6 | 14 | Struct size |
| Address parsing accuracy | 0% | >85% | Unit tests |
| Geocoding latency (10K addresses) | 50ms | <5ms | Benchmark |
| Memory usage | N/A | <500MB for 1M addresses | Profiler |

---

## 🗓️ WEEK-BY-WEEK BREAKDOWN

### **Week 1: Enhanced Address Data Model**
**Focus:** Extend existing structs, add new enums, update POI service

#### Day 1-2: Design & Data Structure Definition
**Owner:** Senior Engineer  
**Deliverables:**
- [ ] Updated `poi_service.h` with enhanced structs
- [ ] New enums for address types and quality tiers
- [ ] Migration plan for existing POI database

#### Day 3-4: Implementation
**Owner:** Mid-level Engineer  
**Deliverables:**
- [ ] Implement all new struct fields
- [ ] Update POI service constructors/destructors
- [ ] Add JSON serialization/deserialization

#### Day 5: Testing & Documentation
**Owner:** Both Engineers  
**Deliverables:**
- [ ] Unit tests for new data structures (>90% coverage)
- [ ] Update API documentation
- [ ] Code review and merge

---

### **Week 2: Address Parser & Normalizer (Foundation)**
**Focus:** Integrate libpostal, build custom parser layer

#### Day 1: Environment Setup & Library Integration
**Owner:** DevOps/Senior Engineer  
**Deliverables:**
- [ ] Install libpostal library (compile from source)
- [ ] Update CMakeLists.txt with new dependencies
- [ ] Verify build on Windows/Linux

#### Day 2-3: Address Parser Implementation
**Owner:** Mid-level Engineer  
**Deliverables:**
- [ ] `AddressParser` class with libpostal integration
- [ ] Wrapper functions for C++ compatibility
- [ ] Basic parsing tests (50+ test cases)

#### Day 4-5: Address Normalizer Implementation
**Owner:** Senior Engineer  
**Deliverables:**
- [ ] `AddressNormalizer` class
- [ ] USPS abbreviation database
- [ ] Normalization rules engine
- [ ] Integration tests

---

### **Week 3: Spatial Indexing (R-tree Design)**
**Focus:** Design R-tree architecture, integrate Boost.Geometry

#### Day 1-2: Library Integration & Proof of Concept
**Owner:** Senior Engineer  
**Deliverables:**
- [ ] Integrate Boost.Geometry library
- [ ] Simple R-tree POC with 100 addresses
- [ ] Performance benchmark vs linear search

#### Day 3-4: Full Implementation
**Owner:** Both Engineers  
**Deliverables:**
- [ ] `SpatialIndex` class implementation
- [ ] Build index from POI database
- [ ] Query methods (region, radius, k-NN)

#### Day 5: Optimization & Tuning
**Owner:** Senior Engineer  
**Deliverables:**
- [ ] Benchmark with 1M+ addresses
- [ ] Memory profiling
- [ ] Index parameter tuning (node size, split algorithm)

---

### **Week 4: Integration & Performance Testing**
**Focus:** Connect all components, end-to-end testing, documentation

#### Day 1-2: Service Integration
**Owner:** Both Engineers  
**Deliverables:**
- [ ] Update `POIService::geocodeAddress()` to use new components
- [ ] Add spatial index to geocoding pipeline
- [ ] Integration tests

#### Day 3: Performance & Load Testing
**Owner:** Senior Engineer  
**Deliverables:**
- [ ] Benchmark suite (1K, 10K, 100K, 1M addresses)
- [ ] Latency percentile analysis (p50, p95, p99)
- [ ] Memory leak testing (Valgrind)

#### Day 4: Documentation & Knowledge Transfer
**Owner:** Team Lead  
**Deliverables:**
- [ ] Architecture diagrams (Mermaid/PlantUML)
- [ ] Developer guide
- [ ] API reference documentation

#### Day 5: Code Review & Deployment Prep
**Owner:** All  
**Deliverables:**
- [ ] Final code review
- [ ] CI/CD pipeline updates
- [ ] Deployment runbook

---

## 🏗️ HIGH-LEVEL ARCHITECTURE

### System Overview
```
┌─────────────────────────────────────────────────────────────┐
│                   Geocoding Service API                      │
│  (NavigationMainWindow → POIService::geocodeAddress)        │
└───────────────────────┬─────────────────────────────────────┘
                        │
                        ▼
┌─────────────────────────────────────────────────────────────┐
│                  Enhanced POI Service                        │
│  ┌─────────────────────────────────────────────────────┐    │
│  │  1. Address Parser (libpostal)                      │    │
│  │     - Parse freeform text → structured components   │    │
│  │     - Extract: street, city, state, zip, etc.       │    │
│  └─────────────────────────────────────────────────────┘    │
│                        │                                     │
│                        ▼                                     │
│  ┌─────────────────────────────────────────────────────┐    │
│  │  2. Address Normalizer                              │    │
│  │     - Expand abbreviations (St → Street)            │    │
│  │     - Normalize case, punctuation                   │    │
│  │     - Standardize directionals (North, N, etc.)     │    │
│  └─────────────────────────────────────────────────────┘    │
│                        │                                     │
│                        ▼                                     │
│  ┌─────────────────────────────────────────────────────┐    │
│  │  3. Spatial Index Query (R-tree)                    │    │
│  │     - Input: Normalized address → Bounding Box      │    │
│  │     - Query: Find candidates within 1km radius      │    │
│  │     - Output: Top-K address candidates              │    │
│  └─────────────────────────────────────────────────────┘    │
│                        │                                     │
│                        ▼                                     │
│  ┌─────────────────────────────────────────────────────┐    │
│  │  4. String Matching & Ranking                       │    │
│  │     - Compare normalized input vs candidates        │    │
│  │     - Score by: edit distance, component match      │    │
│  │     - Return best match with confidence score       │    │
│  └─────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────┘
                        │
                        ▼
┌─────────────────────────────────────────────────────────────┐
│              EnhancedGeocodingResult                         │
│  {lat, lon, confidence, quality, formatted_address, ...}     │
└─────────────────────────────────────────────────────────────┘
```

### Component Interactions
```mermaid
graph TB
    A[User Input: "123 Main St, Apt 4B"] --> B[AddressParser]
    B --> C[Parsed Components]
    C --> D[AddressNormalizer]
    D --> E[Normalized Address]
    E --> F[BoundingBox Estimator]
    F --> G[SpatialIndex R-tree Query]
    G --> H[Candidate Addresses]
    H --> I[String Matcher]
    I --> J[Ranked Results]
    J --> K[EnhancedGeocodingResult]
    
    L[POI Database 1M records] --> M[Build R-tree Index]
    M --> G
```

### Data Flow
```
Input: "123 Main Street, Apartment 4B, San Francisco, CA 94102"
  ↓
[Parser] → {
  street_number: "123",
  street_name: "Main Street",
  unit_number: "4B",
  city: "San Francisco",
  state: "CA",
  zip_code: "94102"
}
  ↓
[Normalizer] → {
  street_number: "123",
  street_name: "MAIN ST",      // Normalized
  unit_number: "APT 4B",         // Standardized
  city: "SAN FRANCISCO",
  state: "CA",
  zip_code: "94102"
}
  ↓
[Bounding Box] → {
  center: (37.7749, -122.4194),  // SF centroid
  radius: 1000 meters
}
  ↓
[R-tree Query] → [
  {addr: "123 MAIN ST APT 4B", lat: 37.7750, lon: -122.4195, score: 0.98},
  {addr: "123 MAIN ST APT 4A", lat: 37.7750, lon: -122.4195, score: 0.92},
  {addr: "125 MAIN ST APT 4B", lat: 37.7751, lon: -122.4196, score: 0.85}
]
  ↓
[Best Match] → {
  success: true,
  latitude: 37.7750,
  longitude: -122.4195,
  confidence: 0.98,
  formatted_address: "123 Main St Apt 4B, San Francisco, CA 94102"
}
```

---

## 🔧 LOW-LEVEL DESIGN

## Task 1: Enhanced Address Data Model

### 1.1 File Structure
```
hmi/services/
├── include/
│   ├── poi_service.h              [MODIFY]
│   ├── enhanced_geocoding.h       [NEW]
│   └── address_components.h       [NEW]
└── src/
    ├── poi_service.cpp            [MODIFY]
    ├── enhanced_geocoding.cpp     [NEW]
    └── address_components.cpp     [NEW]
```

### 1.2 Data Structures Implementation

#### File: `hmi/services/include/address_components.h`
```cpp
#ifndef ADDRESS_COMPONENTS_H
#define ADDRESS_COMPONENTS_H

#include <string>
#include <optional>
#include <vector>
#include <unordered_map>

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
     */
    std::string toFormattedString(const std::string& format = "usps") const;
    
    /**
     * @brief Check if address is complete enough for geocoding
     */
    bool isGeocodable() const;
    
    /**
     * @brief Get all non-empty components as key-value map
     */
    std::unordered_map<std::string, std::string> toMap() const;
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
     */
    double calculateRelevanceScore(const Point& reference_point) const;
};

} // namespace geocoding
} // namespace nav

#endif // ADDRESS_COMPONENTS_H
```

#### File: `hmi/services/include/enhanced_geocoding.h`
```cpp
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
        all.push_back(primary_result);
        all.insert(all.end(), alternatives.begin(), alternatives.end());
        return all;
    }
};

} // namespace geocoding
} // namespace nav

#endif // ENHANCED_GEOCODING_H
```

### 1.3 Migration Plan for Existing Code

#### Step 1: Add backward compatibility wrapper
```cpp
// In poi_service.h - keep old struct for now
struct AddressRequest {  // DEPRECATED - use EnhancedAddressRequest
    std::string street_number;
    std::string street_name;
    std::string city;
    std::string state;
    std::string zip_code;
    std::string country;
    
    // Convert to new format
    geocoding::EnhancedAddressRequest toEnhanced() const {
        geocoding::EnhancedAddressRequest req;
        geocoding::AddressComponents comp;
        comp.house_number = street_number;
        comp.street_name = street_name;
        comp.city = city;
        comp.state = state;
        comp.postal_code = zip_code;
        comp.country = country;
        req.components = comp;
        return req;
    }
};

struct GeocodingResult {  // DEPRECATED
    bool success;
    double latitude;
    double longitude;
    std::string standard_address;
    double accuracy;
    std::string match_type;
    
    // Convert from new format
    static GeocodingResult fromEnhanced(
        const geocoding::EnhancedGeocodingResult& enhanced) {
        GeocodingResult old;
        old.success = enhanced.success;
        old.latitude = enhanced.primary_result.latitude;
        old.longitude = enhanced.primary_result.longitude;
        old.standard_address = enhanced.primary_result.formatted_address;
        old.accuracy = enhanced.primary_result.accuracy_meters;
        old.match_type = enhanced.primary_result.match_type;
        return old;
    }
};
```

#### Step 2: Update POIService class
```cpp
class POIService : public QObject {
    Q_OBJECT
    
public:
    // OLD API (deprecated but still works)
    GeocodingResult geocodeAddress(const AddressRequest& address);
    
    // NEW API
    geocoding::EnhancedGeocodingResult geocodeAddressEnhanced(
        const geocoding::EnhancedAddressRequest& request);
    
    // Implementation delegates old → new
    GeocodingResult POIService::geocodeAddress(const AddressRequest& address) {
        auto enhanced_req = address.toEnhanced();
        auto enhanced_result = geocodeAddressEnhanced(enhanced_req);
        return GeocodingResult::fromEnhanced(enhanced_result);
    }
};
```

---

## Task 2: Address Parser & Normalizer

### 2.1 libpostal Integration

#### Installation Script: `scripts/install_libpostal.sh`
```bash
#!/bin/bash
# Install libpostal on Ubuntu/Debian

set -e

echo "Installing libpostal dependencies..."
sudo apt-get update
sudo apt-get install -y curl autoconf automake libtool pkg-config

echo "Cloning libpostal..."
cd /tmp
git clone https://github.com/openvenues/libpostal
cd libpostal

echo "Downloading language models (~2GB)..."
./bootstrap.sh
./configure --datadir=/usr/local/share/libpostal
make -j$(nproc)
sudo make install
sudo ldconfig

echo "libpostal installed successfully!"
libpostal --version
```

#### CMake Integration: Update `CMakeLists.txt`
```cmake
# Find libpostal
find_library(POSTAL_LIBRARY NAMES postal PATHS /usr/local/lib)
find_path(POSTAL_INCLUDE_DIR libpostal/libpostal.h PATHS /usr/local/include)

if(POSTAL_LIBRARY AND POSTAL_INCLUDE_DIR)
    message(STATUS "Found libpostal: ${POSTAL_LIBRARY}")
    set(LIBPOSTAL_FOUND TRUE)
else()
    message(FATAL_ERROR "libpostal not found. Please install: https://github.com/openvenues/libpostal")
endif()

# Link to POI service
target_include_directories(nav_poi_service PRIVATE ${POSTAL_INCLUDE_DIR})
target_link_libraries(nav_poi_service PRIVATE ${POSTAL_LIBRARY})
```

### 2.2 Address Parser Implementation

#### File: `hmi/services/include/address_parser.h`
```cpp
#ifndef ADDRESS_PARSER_H
#define ADDRESS_PARSER_H

#include "address_components.h"
#include <string>
#include <memory>

// Forward declaration to avoid exposing libpostal types
extern "C" {
    struct libpostal_address_parser_options;
    struct libpostal_address_parser_response;
}

namespace nav {
namespace geocoding {

/**
 * @brief Address parsing service using libpostal
 * Thread-safe, singleton pattern
 */
class AddressParser {
public:
    static AddressParser& getInstance();
    
    // Delete copy/move constructors (singleton)
    AddressParser(const AddressParser&) = delete;
    AddressParser& operator=(const AddressParser&) = delete;
    
    /**
     * @brief Parse freeform address text into components
     * @param address_text Raw input (e.g., "123 Main St Apt 4B SF CA 94102")
     * @param country_hint Country code hint for better accuracy ("US", "CA")
     * @return Parsed address components
     */
    AddressComponents parse(const std::string& address_text,
                           const std::string& country_hint = "US");
    
    /**
     * @brief Check if parser is initialized and ready
     */
    bool isReady() const { return m_initialized; }
    
private:
    AddressParser();
    ~AddressParser();
    
    bool m_initialized;
    
    // Helper to convert libpostal response to our format
    AddressComponents convertLibpostalResponse(
        libpostal_address_parser_response* response);
};

} // namespace geocoding
} // namespace nav

#endif // ADDRESS_PARSER_H
```

#### File: `hmi/services/src/address_parser.cpp`
```cpp
#include "../include/address_parser.h"
#include <libpostal/libpostal.h>
#include <QDebug>
#include <stdexcept>

namespace nav {
namespace geocoding {

AddressParser& AddressParser::getInstance() {
    static AddressParser instance;
    return instance;
}

AddressParser::AddressParser() : m_initialized(false) {
    qDebug() << "[ADDRESS PARSER] Initializing libpostal...";
    
    if (!libpostal_setup() || !libpostal_setup_parser()) {
        qWarning() << "[ADDRESS PARSER] Failed to initialize libpostal";
        throw std::runtime_error("libpostal initialization failed");
    }
    
    m_initialized = true;
    qDebug() << "[ADDRESS PARSER] Initialized successfully";
}

AddressParser::~AddressParser() {
    if (m_initialized) {
        libpostal_teardown();
        libpostal_teardown_parser();
        qDebug() << "[ADDRESS PARSER] Shut down";
    }
}

AddressComponents AddressParser::parse(
    const std::string& address_text,
    const std::string& country_hint) {
    
    if (!m_initialized) {
        throw std::runtime_error("AddressParser not initialized");
    }
    
    // Configure libpostal options
    libpostal_address_parser_options_t options = 
        libpostal_get_address_parser_default_options();
    options.language = nullptr;  // Auto-detect
    options.country = country_hint.empty() ? nullptr : country_hint.c_str();
    
    // Parse address
    libpostal_address_parser_response_t* response = 
        libpostal_parse_address(address_text.c_str(), options);
    
    if (!response) {
        qWarning() << "[ADDRESS PARSER] Failed to parse:" << address_text.c_str();
        return AddressComponents(); // Empty result
    }
    
    // Convert to our format
    AddressComponents components = convertLibpostalResponse(response);
    
    // Cleanup
    libpostal_address_parser_response_destroy(response);
    
    return components;
}

AddressComponents AddressParser::convertLibpostalResponse(
    libpostal_address_parser_response_t* response) {
    
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
    
    return comp;
}

} // namespace geocoding
} // namespace nav
```

### 2.3 Address Normalizer Implementation

#### File: `hmi/services/include/address_normalizer.h`
```cpp
#ifndef ADDRESS_NORMALIZER_H
#define ADDRESS_NORMALIZER_H

#include "address_components.h"
#include <string>
#include <unordered_map>
#include <regex>

namespace nav {
namespace geocoding {

/**
 * @brief Normalize and standardize address components
 * Handles abbreviations, case, punctuation, etc.
 */
class AddressNormalizer {
public:
    /**
     * @brief Normalize all components in-place
     */
    static void normalize(AddressComponents& components);
    
    /**
     * @brief Expand street abbreviations (St → Street)
     */
    static std::string expandStreetType(const std::string& abbrev);
    
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
     * @brief Clean text: uppercase, remove extra spaces, punctuation
     */
    static std::string cleanText(const std::string& text);
    
private:
    // Abbreviation lookup tables (initialized once)
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
```

#### File: `hmi/services/src/address_normalizer.cpp`
```cpp
#include "../include/address_normalizer.h"
#include <algorithm>
#include <cctype>
#include <sstream>

namespace nav {
namespace geocoding {

// USPS-approved abbreviations
const std::unordered_map<std::string, std::string> 
AddressNormalizer::STREET_TYPES = {
    {"STREET", "ST"}, {"ST", "ST"}, {"STR", "ST"},
    {"AVENUE", "AVE"}, {"AVE", "AVE"}, {"AV", "AVE"},
    {"BOULEVARD", "BLVD"}, {"BLVD", "BLVD"},
    {"ROAD", "RD"}, {"RD", "RD"},
    {"DRIVE", "DR"}, {"DR", "DR"},
    {"LANE", "LN"}, {"LN", "LN"},
    {"COURT", "CT"}, {"CT", "CT"},
    {"PLACE", "PL"}, {"PL", "PL"},
    {"CIRCLE", "CIR"}, {"CIR", "CIR"},
    {"PARKWAY", "PKWY"}, {"PKWY", "PKWY"},
    {"HIGHWAY", "HWY"}, {"HWY", "HWY"},
    {"TERRACE", "TER"}, {"TER", "TER"}
};

const std::unordered_map<std::string, std::string> 
AddressNormalizer::DIRECTIONALS = {
    {"NORTH", "N"}, {"N", "N"},
    {"SOUTH", "S"}, {"S", "S"},
    {"EAST", "E"}, {"E", "E"},
    {"WEST", "W"}, {"W", "W"},
    {"NORTHEAST", "NE"}, {"NE", "NE"},
    {"NORTHWEST", "NW"}, {"NW", "NW"},
    {"SOUTHEAST", "SE"}, {"SE", "SE"},
    {"SOUTHWEST", "SW"}, {"SW", "SW"}
};

const std::unordered_map<std::string, std::string> 
AddressNormalizer::UNIT_TYPES = {
    {"APARTMENT", "APT"}, {"APT", "APT"}, {"UNIT", "UNIT"},
    {"SUITE", "STE"}, {"STE", "STE"},
    {"FLOOR", "FL"}, {"FL", "FL"},
    {"ROOM", "RM"}, {"RM", "RM"},
    {"BUILDING", "BLDG"}, {"BLDG", "BLDG"}
};

const std::unordered_map<std::string, std::string> 
AddressNormalizer::STATE_CODES = {
    {"ALABAMA", "AL"}, {"AL", "AL"},
    {"CALIFORNIA", "CA"}, {"CA", "CA"}, {"CALIF", "CA"},
    {"TEXAS", "TX"}, {"TX", "TX"},
    {"NEW YORK", "NY"}, {"NY", "NY"},
    {"FLORIDA", "FL"}, {"FL", "FL"},
    // ... (add all 50 states)
};

void AddressNormalizer::normalize(AddressComponents& comp) {
    // Normalize street components
    if (comp.street_name) {
        comp.street_name = cleanText(*comp.street_name);
    }
    if (comp.street_type) {
        comp.street_type = expandStreetType(*comp.street_type);
    }
    if (comp.street_prefix) {
        comp.street_prefix = normalizeDirectional(*comp.street_prefix);
    }
    if (comp.street_suffix) {
        comp.street_suffix = normalizeDirectional(*comp.street_suffix);
    }
    
    // Normalize unit
    if (comp.unit_type) {
        comp.unit_type = normalizeUnitType(*comp.unit_type);
    }
    if (comp.unit_number) {
        comp.unit_number = cleanText(*comp.unit_number);
    }
    
    // Normalize locality
    if (comp.city) {
        comp.city = cleanText(*comp.city);
    }
    if (comp.state) {
        std::string state_code = normalizeState(*comp.state);
        comp.state = state_code;
        comp.state_code = state_code;
    }
    if (comp.postal_code) {
        comp.postal_code = cleanText(*comp.postal_code);
    }
}

std::string AddressNormalizer::expandStreetType(const std::string& abbrev) {
    std::string upper = toUpper(abbrev);
    auto it = STREET_TYPES.find(upper);
    return (it != STREET_TYPES.end()) ? it->second : upper;
}

std::string AddressNormalizer::normalizeDirectional(const std::string& dir) {
    std::string upper = toUpper(dir);
    auto it = DIRECTIONALS.find(upper);
    return (it != DIRECTIONALS.end()) ? it->second : upper;
}

std::string AddressNormalizer::normalizeUnitType(const std::string& unit) {
    std::string upper = toUpper(unit);
    auto it = UNIT_TYPES.find(upper);
    return (it != UNIT_TYPES.end()) ? it->second : upper;
}

std::string AddressNormalizer::normalizeState(const std::string& state) {
    std::string upper = toUpper(state);
    auto it = STATE_CODES.find(upper);
    return (it != STATE_CODES.end()) ? it->second : upper;
}

std::string AddressNormalizer::cleanText(const std::string& text) {
    std::string cleaned = trim(text);
    cleaned = toUpper(cleaned);
    
    // Remove extra spaces
    std::regex multiple_spaces("\\s+");
    cleaned = std::regex_replace(cleaned, multiple_spaces, " ");
    
    // Remove punctuation except hyphens in addresses
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
    size_t first = str.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, last - first + 1);
}

} // namespace geocoding
} // namespace nav
```

---

## Task 3: Spatial Indexing (R-tree)

### 3.1 Boost.Geometry Integration

#### Update `CMakeLists.txt`
```cmake
# Find Boost with Geometry component
find_package(Boost 1.70 REQUIRED COMPONENTS system)

if(Boost_FOUND)
    message(STATUS "Found Boost ${Boost_VERSION}: ${Boost_INCLUDE_DIRS}")
    include_directories(${Boost_INCLUDE_DIRS})
    link_libraries(${Boost_LIBRARIES})
else()
    message(FATAL_ERROR "Boost not found. Install: sudo apt-get install libboost-all-dev")
endif()
```

### 3.2 Spatial Index Implementation

#### File: `hmi/services/include/spatial_index.h`
```cpp
#ifndef SPATIAL_INDEX_H
#define SPATIAL_INDEX_H

#include "../../common/include/nav_types.h"
#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/box.hpp>
#include <boost/geometry/index/rtree.hpp>
#include <vector>
#include <memory>

namespace nav {
namespace geocoding {

// Boost.Geometry type aliases
namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

using BoostPoint = bg::model::point<double, 2, bg::cs::cartesian>;
using BoostBox = bg::model::box<BoostPoint>;

/**
 * @brief Address record for spatial indexing
 */
struct AddressRecord {
    uint64_t id;                      // Unique ID
    Point location;                   // Lat/Lon
    std::string formatted_address;    // Full address string
    std::string normalized_address;   // Searchable format
    std::string postal_code;          // For quick filtering
    std::string city;
    std::string state;
    
    // Metadata
    std::string data_source;          // "TIGER", "OSM"
    GeocodingQuality quality;
    uint64_t last_updated_ms;
    
    AddressRecord()
        : id(0)
        , quality(GeocodingQuality::UNKNOWN)
        , last_updated_ms(0) {}
    
    /**
     * @brief Convert to Boost point for R-tree
     */
    BoostPoint toBoostPoint() const {
        // Note: R-tree uses Cartesian, not geodetic
        // For better accuracy, convert to local projection
        return BoostPoint(location.longitude, location.latitude);
    }
};

// R-tree value type: pair of point and record
using RTreeValue = std::pair<BoostPoint, AddressRecord>;

/**
 * @brief Spatial index using R*-tree for fast geographic queries
 * Thread-safe for read operations after building
 */
class SpatialIndex {
public:
    /**
     * @brief Constructor with R-tree parameters
     * @param max_elements Maximum elements per node (default: 16)
     * @param min_elements Minimum elements per node (default: 4)
     */
    explicit SpatialIndex(size_t max_elements = 16, size_t min_elements = 4);
    
    ~SpatialIndex();
    
    /**
     * @brief Build index from address database
     * @param addresses Vector of all addresses
     * @return true if successful
     */
    bool buildIndex(const std::vector<AddressRecord>& addresses);
    
    /**
     * @brief Query addresses within bounding box
     * @param bbox Geographic bounding box
     * @return Addresses within box
     */
    std::vector<AddressRecord> queryRegion(const BoundingBox& bbox) const;
    
    /**
     * @brief Query addresses within radius
     * @param center Center point
     * @param radius_meters Radius in meters
     * @return Addresses within radius (sorted by distance)
     */
    std::vector<AddressRecord> queryRadius(
        const Point& center, 
        double radius_meters) const;
    
    /**
     * @brief Find k-nearest neighbors
     * @param location Query point
     * @param k Number of neighbors
     * @return k nearest addresses (sorted by distance)
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
        size_t leaf_nodes;
        size_t internal_nodes;
        size_t memory_bytes;
    };
    IndexStats getStats() const;
    
    /**
     * @brief Check if index is ready
     */
    bool isReady() const { return m_index_built; }
    
private:
    // R*-tree with quadratic split algorithm
    using RTree = bgi::rtree<
        RTreeValue,
        bgi::rstar<16>  // Max 16 elements per node
    >;
    
    std::unique_ptr<RTree> m_rtree;
    bool m_index_built;
    size_t m_address_count;
    
    // Helper: Convert lat/lon box to Boost box
    BoostBox toBoundingBox(const BoundingBox& bbox) const;
    
    // Helper: Calculate geodetic distance
    double calculateDistance(const Point& p1, const Point& p2) const;
};

} // namespace geocoding
} // namespace nav

#endif // SPATIAL_INDEX_H
```

#### File: `hmi/services/src/spatial_index.cpp`
```cpp
#include "../include/spatial_index.h"
#include "../../common/include/nav_utils.h"
#include <QDebug>
#include <algorithm>
#include <chrono>

namespace nav {
namespace geocoding {

SpatialIndex::SpatialIndex(size_t max_elements, size_t min_elements)
    : m_index_built(false)
    , m_address_count(0) {
    
    // Create R-tree with custom parameters
    m_rtree = std::make_unique<RTree>(bgi::rstar<16>());
    qDebug() << "[SPATIAL INDEX] Created with max_elements=" << max_elements;
}

SpatialIndex::~SpatialIndex() {
    qDebug() << "[SPATIAL INDEX] Destroyed";
}

bool SpatialIndex::buildIndex(const std::vector<AddressRecord>& addresses) {
    auto start = std::chrono::high_resolution_clock::now();
    
    qDebug() << "[SPATIAL INDEX] Building index for" << addresses.size() << "addresses...";
    
    // Clear existing index
    m_rtree->clear();
    m_index_built = false;
    
    // Bulk load addresses into R-tree
    std::vector<RTreeValue> values;
    values.reserve(addresses.size());
    
    for (const auto& addr : addresses) {
        BoostPoint point = addr.toBoostPoint();
        values.emplace_back(point, addr);
    }
    
    // Rebuild tree (bulk loading is faster than incremental)
    m_rtree = std::make_unique<RTree>(values.begin(), values.end());
    
    m_address_count = addresses.size();
    m_index_built = true;
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    qDebug() << "[SPATIAL INDEX] Built index in" << duration.count() << "ms";
    qDebug() << "[SPATIAL INDEX] Index ready with" << m_address_count << "addresses";
    
    return true;
}

std::vector<AddressRecord> SpatialIndex::queryRegion(const BoundingBox& bbox) const {
    if (!m_index_built) {
        qWarning() << "[SPATIAL INDEX] Index not built!";
        return {};
    }
    
    BoostBox boost_box = toBoundingBox(bbox);
    std::vector<AddressRecord> results;
    
    // Query R-tree
    m_rtree->query(
        bgi::intersects(boost_box),
        std::back_inserter(results)
    );
    
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
    
    // Convert radius to approximate degrees (rough estimate)
    // 1 degree latitude ≈ 111km
    double radius_degrees = radius_meters / 111000.0;
    
    // Create bounding box for initial filter
    BoundingBox bbox;
    bbox.min_lat = center.latitude - radius_degrees;
    bbox.max_lat = center.latitude + radius_degrees;
    bbox.min_lon = center.longitude - radius_degrees;
    bbox.max_lon = center.longitude + radius_degrees;
    
    // Query candidates in bounding box
    auto candidates = queryRegion(bbox);
    
    // Filter by exact geodetic distance
    std::vector<std::pair<AddressRecord, double>> scored;
    for (const auto& addr : candidates) {
        double distance = calculateDistance(center, addr.location);
        if (distance <= radius_meters) {
            scored.emplace_back(addr, distance);
        }
    }
    
    // Sort by distance
    std::sort(scored.begin(), scored.end(),
        [](const auto& a, const auto& b) { return a.second < b.second; });
    
    // Extract addresses
    std::vector<AddressRecord> results;
    for (const auto& pair : scored) {
        results.push_back(pair.first);
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
    
    BoostPoint query_point(location.longitude, location.latitude);
    std::vector<RTreeValue> nearest;
    
    // k-NN query
    m_rtree->query(
        bgi::nearest(query_point, k),
        std::back_inserter(nearest)
    );
    
    // Extract addresses and calculate exact distances
    std::vector<std::pair<AddressRecord, double>> scored;
    for (const auto& value : nearest) {
        const AddressRecord& addr = value.second;
        double distance = calculateDistance(location, addr.location);
        scored.emplace_back(addr, distance);
    }
    
    // Sort by exact geodetic distance
    std::sort(scored.begin(), scored.end(),
        [](const auto& a, const auto& b) { return a.second < b.second; });
    
    std::vector<AddressRecord> results;
    for (const auto& pair : scored) {
        results.push_back(pair.first);
    }
    
    qDebug() << "[SPATIAL INDEX] k-NN query returned" << results.size() << "results";
    return results;
}

SpatialIndex::IndexStats SpatialIndex::getStats() const {
    IndexStats stats;
    stats.total_addresses = m_address_count;
    // Note: Boost.Geometry R-tree doesn't expose internal stats easily
    // These would need custom calculation
    stats.tree_depth = 0;  // TODO: calculate
    stats.leaf_nodes = 0;
    stats.internal_nodes = 0;
    stats.memory_bytes = m_address_count * sizeof(RTreeValue);  // Rough estimate
    return stats;
}

BoostBox SpatialIndex::toBoundingBox(const BoundingBox& bbox) const {
    BoostPoint min_corner(bbox.min_lon, bbox.min_lat);
    BoostPoint max_corner(bbox.max_lon, bbox.max_lat);
    return BoostBox(min_corner, max_corner);
}

double SpatialIndex::calculateDistance(const Point& p1, const Point& p2) const {
    return NavUtils::haversineDistance(p1, p2);
}

} // namespace geocoding
} // namespace nav
```

---

## 🧪 TESTING STRATEGY

### Unit Tests (Week 1-4, ongoing)

#### File: `tests/test_address_parser.cpp`
```cpp
#include <gtest/gtest.h>
#include "../hmi/services/include/address_parser.h"

using namespace nav::geocoding;

TEST(AddressParserTest, ParseSimpleAddress) {
    auto& parser = AddressParser::getInstance();
    
    auto result = parser.parse("123 Main Street, San Francisco, CA 94102");
    
    ASSERT_TRUE(result.house_number.has_value());
    EXPECT_EQ(*result.house_number, "123");
    EXPECT_EQ(*result.street_name, "Main");
    EXPECT_EQ(*result.city, "San Francisco");
    EXPECT_EQ(*result.state, "CA");
    EXPECT_EQ(*result.postal_code, "94102");
}

TEST(AddressParserTest, ParseApartmentAddress) {
    auto& parser = AddressParser::getInstance();
    
    auto result = parser.parse("456 Oak Ave Apt 4B, New York, NY 10001");
    
    EXPECT_EQ(*result.house_number, "456");
    EXPECT_EQ(*result.unit_number, "4B");
    EXPECT_EQ(*result.city, "New York");
}

TEST(AddressParserTest, ParsePOBox) {
    auto& parser = AddressParser::getInstance();
    
    auto result = parser.parse("PO Box 1234, Austin, TX 78701");
    
    EXPECT_TRUE(result.is_po_box);
    EXPECT_EQ(*result.po_box, "1234");
}
```

#### File: `tests/test_spatial_index.cpp`
```cpp
#include <gtest/gtest.h>
#include "../hmi/services/include/spatial_index.h"

using namespace nav::geocoding;

class SpatialIndexTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test addresses
        addresses = {
            {1, Point(37.7749, -122.4194), "123 Market St", "123 MARKET ST", "94102", "San Francisco", "CA"},
            {2, Point(37.7750, -122.4195), "125 Market St", "125 MARKET ST", "94102", "San Francisco", "CA"},
            {3, Point(37.7900, -122.4000), "456 Mission St", "456 MISSION ST", "94103", "San Francisco", "CA"}
        };
        
        index.buildIndex(addresses);
    }
    
    SpatialIndex index;
    std::vector<AddressRecord> addresses;
};

TEST_F(SpatialIndexTest, BuildIndex) {
    EXPECT_TRUE(index.isReady());
    EXPECT_EQ(index.getStats().total_addresses, 3);
}

TEST_F(SpatialIndexTest, RadiusQuery) {
    Point center(37.7749, -122.4194);
    auto results = index.queryRadius(center, 100.0); // 100m radius
    
    EXPECT_GE(results.size(), 1);
    EXPECT_LE(results.size(), 3);
    
    // First result should be closest
    EXPECT_EQ(results[0].id, 1);
}

TEST_F(SpatialIndexTest, KNNQuery) {
    Point query(37.7749, -122.4194);
    auto results = index.findNearestAddresses(query, 2);
    
    EXPECT_EQ(results.size(), 2);
    EXPECT_EQ(results[0].id, 1); // Exact match
    EXPECT_EQ(results[1].id, 2); // Next closest
}
```

### Performance Benchmarks

#### File: `benchmarks/benchmark_spatial_index.cpp`
```cpp
#include <benchmark/benchmark.h>
#include "../hmi/services/include/spatial_index.h"

using namespace nav::geocoding;

static void BM_SpatialIndex_Build(benchmark::State& state) {
    std::vector<AddressRecord> addresses;
    for (int i = 0; i < state.range(0); ++i) {
        AddressRecord addr;
        addr.id = i;
        addr.location = Point(37.0 + (i * 0.0001), -122.0 + (i * 0.0001));
        addresses.push_back(addr);
    }
    
    for (auto _ : state) {
        SpatialIndex index;
        index.buildIndex(addresses);
    }
}
BENCHMARK(BM_SpatialIndex_Build)->Range(1000, 1000000);

static void BM_SpatialIndex_Query(benchmark::State& state) {
    // Pre-build index
    std::vector<AddressRecord> addresses;
    for (int i = 0; i < 100000; ++i) {
        AddressRecord addr;
        addr.id = i;
        addr.location = Point(37.0 + (i * 0.0001), -122.0 + (i * 0.0001));
        addresses.push_back(addr);
    }
    
    SpatialIndex index;
    index.buildIndex(addresses);
    
    Point query(37.5, -122.5);
    
    for (auto _ : state) {
        auto results = index.findNearestAddresses(query, 10);
        benchmark::DoNotOptimize(results);
    }
}
BENCHMARK(BM_SpatialIndex_Query);

BENCHMARK_MAIN();
```

---

## 📊 DELIVERABLES CHECKLIST

### Week 1 Deliverables
- [ ] `address_components.h` - Enhanced data structures
- [ ] `enhanced_geocoding.h` - Request/response types
- [ ] `address_components.cpp` - Component implementations
- [ ] Unit tests with >90% coverage
- [ ] Documentation (Doxygen comments)

### Week 2 Deliverables
- [ ] libpostal integration (CMake, build scripts)
- [ ] `address_parser.h/cpp` - Parser implementation
- [ ] `address_normalizer.h/cpp` - Normalizer with USPS rules
- [ ] 50+ parsing test cases
- [ ] Performance benchmarks

### Week 3 Deliverables
- [ ] Boost.Geometry integration
- [ ] `spatial_index.h/cpp` - R-tree implementation
- [ ] Query methods (region, radius, k-NN)
- [ ] Benchmark: 1M addresses in <5ms query time
- [ ] Memory profiling report

### Week 4 Deliverables
- [ ] Updated `POIService::geocodeAddressEnhanced()`
- [ ] End-to-end integration tests
- [ ] Performance report (latency percentiles)
- [ ] Architecture diagrams (Mermaid)
- [ ] Developer documentation
- [ ] Deployment runbook

---

## 🎯 SUCCESS CRITERIA

### Functional Requirements
- ✅ Parse 95%+ of standard US addresses correctly
- ✅ Support apartments, suites, PO boxes, intersections
- ✅ Normalize addresses to USPS standard format
- ✅ Spatial index queries in <5ms for 1M addresses

### Non-Functional Requirements
- ✅ Thread-safe operations (read-only after build)
- ✅ Memory usage <500MB for 1M addresses
- ✅ Unit test coverage >90%
- ✅ Zero memory leaks (Valgrind clean)
- ✅ Build time <5 minutes on standard hardware

### Documentation Requirements
- ✅ API reference (Doxygen)
- ✅ Architecture diagrams
- ✅ Developer setup guide
- ✅ Performance tuning guide

---

**Document Version:** 1.0  
**Last Updated:** October 18, 2025  
**Next Review:** Week 5 (Phase 2 Planning)
