#include "../include/poi_service.h"
#include "address_parser.h"  // Will be found via geocoding include path
#include "address_normalizer.h"  // Will be found via geocoding include path
#include "spatial_index.h"  // Will be found via geocoding include path
#include <QFile>
#include <QIODevice>
#include <QJsonDocument>
#include <QDebug>
#include <cmath>
#include <chrono>
#include <limits>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace nav {

POIService::POIService(QObject* parent)
    : QObject(parent)
    , m_initialized(false)
    , m_serviceReady(false)
    , m_lastQueryTimeMs(0.0)
    , m_spatialIndex(nullptr)
    , m_useEnhancedGeocoding(true)
{
    // Constructor implementation
}

POIService::~POIService()
{
    // Destructor implementation
}

bool POIService::initialize()
{
    if (m_initialized) {
        return true;
    }
            
    // Load POI database
    loadPOIDatabase();
    
    // Build spatial index for enhanced geocoding
    if (m_useEnhancedGeocoding && !m_poiDatabase.empty()) {
        buildSpatialIndex();
    }
    
    m_initialized = true;
    m_serviceReady = true;

    return true;
}

void POIService::shutdown()
{
    if (!m_initialized) {
        return;
    }
    
    // Cleanup resources if needed
    
    m_initialized = false;
    m_serviceReady = false;
}

bool POIService::isServiceReady() const
{
    return m_serviceReady;
}

std::string POIService::getServiceStatus() const
{
    if (!m_initialized) {
        return "Not Initialized";
    }   
    return "Ready";
}

void POIService::loadPOIDatabase()
{
    //Needtodo
//    Load POI database from JSON file
    QString filePath = "D:/Data/My job/C++/Automotive/hmi/maps/poi_database.json";
    
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "[POI SERVICE] Failed to open POI database file:" << filePath;
        qWarning() << "[POI SERVICE] Error:" << file.errorString();
        return;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isArray()) {
        qWarning() << "[POI SERVICE] Invalid POI database format";
        return;
    }
    
    std::vector<POI> poiList;
    QJsonArray poiArray = doc.array();
    
    for (const QJsonValue& value : poiArray) {
        if (value.isObject()) {
            QJsonObject obj = value.toObject();
            POI poi;
            poi.poi_id = obj["id"].toInt();
            poi.name = obj["name"].toString().toStdString();
            poi.category = obj["category"].toString().toStdString();
            poi.latitude = obj["latitude"].toDouble();
            poi.longitude = obj["longitude"].toDouble();
            poi.address = obj["address"].toString().toStdString();
            poiList.push_back(poi);
        }
    }
    
    m_poiDatabase = std::move(poiList);
    
    qDebug() << "[POI SERVICE] Loaded" << m_poiDatabase.size() << "POIs from database";
}

void POIService::buildSpatialIndex() {
    qDebug() << "[POI SERVICE] Building spatial index...";
    
    // Create spatial index
    m_spatialIndex = std::make_unique<geocoding::SpatialIndex>(16, 4);
    
    // Convert POI database to AddressRecord format
    std::vector<geocoding::AddressRecord> records;
    records.reserve(m_poiDatabase.size());
    
    for (const auto& poi : m_poiDatabase) {
        geocoding::AddressRecord record;
        record.id = poi.poi_id;
        record.location = Point{poi.latitude, poi.longitude};
        record.formatted_address = poi.address;
        record.normalized_address = poi.address;  // TODO: Normalize
        record.city = poi.name;  // Using name as proxy for city
        record.data_source = "POI_DB";
        record.quality = geocoding::GeocodingQuality::APPROXIMATE;
        record.last_updated_ms = static_cast<uint64_t>(
            std::chrono::system_clock::now().time_since_epoch().count() / 1000000);
        
        records.push_back(record);
    }
    
    // Build index
    if (m_spatialIndex->buildIndex(records)) {
        qDebug() << "[POI SERVICE] Spatial index built successfully";
    } else {
        qWarning() << "[POI SERVICE] Failed to build spatial index";
    }
}

double POIService::calculateDistance(const Point& p1, const Point& p2) const
{
    // Haversine formula for distance calculation
    const double R = 6371000.0; // Earth radius in meters
    
    double lat1_rad = p1.latitude * M_PI / 180.0;
    double lat2_rad = p2.latitude * M_PI / 180.0;
    double dlat = (p2.latitude - p1.latitude) * M_PI / 180.0;
    double dlon = (p2.longitude - p1.longitude) * M_PI / 180.0;
    
    double a = std::sin(dlat/2) * std::sin(dlat/2) +
               std::cos(lat1_rad) * std::cos(lat2_rad) * std::sin(dlon/2) * std::sin(dlon/2);
    double c = 2 * std::atan2(std::sqrt(a), std::sqrt(1-a));
    
    return R * c;
}

std::vector<POI> POIService::findNearbyPOIs(const Point& location, double radiusMeters, const QString& category) {
    //Needtodo
    std::vector<POI> results;
    
    for (const auto& poi : m_poiDatabase) {
        Point poiLocation{poi.latitude, poi.longitude};
        double distance = calculateDistance(location, poiLocation);
        
        if (distance <= radiusMeters) {
            if (category.isEmpty() || QString::fromStdString(poi.category) == category) {
                results.push_back(poi);
            }
        }
    }
    
    return results;
}

std::vector<POI> POIService::searchPOIsByName(const QString& name) {
    std::vector<POI> results;
    QString lowerName = name.toLower();
    
    for (const auto& poi : m_poiDatabase) {
        if (QString::fromStdString(poi.name).toLower().contains(lowerName)) {
            results.push_back(poi);
        }
    }
    
    return results;
}

POI POIService::getPOIFromAddress(const AddressRequest& address) {
    // Query POI database for matching address
    //Needtodo
    QString targetAddress = QString::fromStdString(address.street_number + " " + address.street_name + ", " +
                                                  address.city + ", " + address.state + " " +
                                                  address.zip_code + ", " + address.country);
    for (const auto& poi : m_poiDatabase) {
        if (QString::fromStdString(poi.address) == targetAddress) {
            return poi;
        }
    }
    return POI(); // Return empty POI if not found
}

GeocodingResult POIService::geocodeAddress(const AddressRequest& address) {
    // Query geocoding database for address m_poiDatabase
    //Needtodo
    GeocodingResult result;
    QString targetAddress = QString::fromStdString(address.street_number + " " + address.street_name + ", " +
                                                  address.city + ", " + address.state + " " +
                                                  address.zip_code + ", " + address.country);
    for (const auto& poi : m_poiDatabase) {
        if (QString::fromStdString(poi.address) == targetAddress) {
            result.success = true;
            result.latitude = poi.latitude;
            result.longitude = poi.longitude;
            result.standard_address = poi.address;
            result.accuracy = 5.0; // Sample accuracy in meters
            result.match_type = "exact";
            return result;
        }
    }
    return GeocodingResult(); // Return empty result if not found   
}

// ============================================================================
// Backward Compatibility Conversion Methods
// ============================================================================

geocoding::AddressComponents AddressRequest::toEnhanced() const {
    geocoding::AddressComponents addr;
    
    if (!street_number.empty()) addr.house_number = street_number;
    if (!street_name.empty()) addr.street_name = street_name;
    if (!city.empty()) addr.city = city;
    if (!state.empty()) addr.state = state;
    if (!zip_code.empty()) addr.postal_code = zip_code;
    if (!country.empty()) {
        addr.country = country;
        if (country == "USA" || country == "United States") {
            addr.country_code = "US";
        }
    } else {
        addr.country_code = "US";
        addr.country = "United States";
    }
    
    addr.is_intersection = false;
    addr.is_po_box = false;
    
    return addr;
}

GeocodingResult GeocodingResult::fromEnhanced(
    const geocoding::EnhancedGeocodingResult& enhanced) {
    GeocodingResult result;
    
    result.success = enhanced.success;
    if (enhanced.success) {
        const auto& primary = enhanced.primary_result;
        result.latitude = primary.latitude;
        result.longitude = primary.longitude;
        result.standard_address = primary.formatted_address;
        result.accuracy = primary.accuracy_meters;
        result.match_type = primary.match_type;
    }
    
    return result;
}

// ============================================================================
// Enhanced Geocoding API Implementation (Stub for Week 2-4)
// ============================================================================

geocoding::EnhancedGeocodingResult POIService::geocodeAddressEnhanced(
    const geocoding::EnhancedAddressRequest& request) {
    
    auto start_time = std::chrono::high_resolution_clock::now();
    geocoding::EnhancedGeocodingResult result;
    
    // Step 1: Parse address (if freeform text)
    geocoding::AddressComponents parsed_components;
    
    if (request.freeform_address.has_value()) {
        qDebug() << "[POI SERVICE] Parsing freeform address:" 
                 << request.freeform_address->c_str();
        
        try {
            auto& parser = geocoding::AddressParser::getInstance();
            parsed_components = parser.parse(
                *request.freeform_address,
                request.country_hint
            );
        } catch (const std::exception& e) {
            result.success = false;
            result.error_message = std::string("Parser error: ") + e.what();
            return result;
        }
    } else if (request.components.has_value()) {
        parsed_components = *request.components;
    } else {
        result.success = false;
        result.error_message = "Neither freeform address nor components provided";
        return result;
    }
    
    // Step 2: Normalize address
    geocoding::AddressNormalizer::normalize(parsed_components);
    
    qDebug() << "[POI SERVICE] Normalized address:"
             << parsed_components.toFormattedString("display").c_str();
    
    // Step 3: Validate
    if (!parsed_components.isGeocodable()) {
        result.success = false;
        result.error_message = "Address does not contain enough information for geocoding";
        return result;
    }
    
    // Step 4: Query spatial index
    if (m_spatialIndex && m_spatialIndex->isReady()) {
        result.primary_result = matchAddressWithSpatialIndex(
            parsed_components, request);
        result.success = (result.primary_result.confidence_score > 0.0);
    } else {
        // Fallback to legacy method
        qWarning() << "[POI SERVICE] Spatial index not available, using legacy geocoding";
        
        AddressRequest legacyReq;
        if (parsed_components.house_number) legacyReq.street_number = *parsed_components.house_number;
        if (parsed_components.street_name) legacyReq.street_name = *parsed_components.street_name;
        if (parsed_components.city) legacyReq.city = *parsed_components.city;
        if (parsed_components.state) legacyReq.state = *parsed_components.state;
        if (parsed_components.postal_code) legacyReq.zip_code = *parsed_components.postal_code;
        
        GeocodingResult legacyResult = geocodeAddress(legacyReq);
        result.success = legacyResult.success;
        
        if (legacyResult.success) {
            result.primary_result.latitude = legacyResult.latitude;
            result.primary_result.longitude = legacyResult.longitude;
            result.primary_result.formatted_address = legacyResult.standard_address;
            result.primary_result.confidence_score = 1.0;
            result.primary_result.quality = geocoding::GeocodingQuality::ROOFTOP;
            result.primary_result.accuracy_meters = legacyResult.accuracy;
            result.primary_result.match_type = legacyResult.match_type;
            result.primary_result.data_source = "legacy_poi_database";
            result.primary_result.geocoding_method = "exact_string_match";
        }
    }
    
    // Step 5: Calculate performance metrics
    auto end_time = std::chrono::high_resolution_clock::now();
    result.processing_time_ms = std::chrono::duration<double, std::milli>(
        end_time - start_time).count();
    result.used_cache = false;
    result.candidates_evaluated = m_poiDatabase.size();
    
    m_lastQueryTimeMs = result.processing_time_ms;
    
    if (result.success) {
        qDebug() << "[POI SERVICE] Geocoding succeeded in" << result.processing_time_ms 
                 << "ms, confidence:" << result.primary_result.confidence_score;
    } else {
        qDebug() << "[POI SERVICE] Geocoding failed:" << result.error_message.c_str();
    }
    
    return result;
}

geocoding::GeocodingCandidate POIService::matchAddressWithSpatialIndex(
    const geocoding::AddressComponents& normalized_address,
    const geocoding::EnhancedAddressRequest& request) const {
    
    geocoding::GeocodingCandidate best_candidate;
    
    // Strategy 1: If we have postal code, query by location
    if (normalized_address.postal_code.has_value()) {
        // Estimate location from postal code (simplified - would use ZIP centroid database in production)
        Point estimated_center{37.7749, -122.4194};  // TODO: Real ZIP lookup
        
        if (request.reference_location.has_value()) {
            estimated_center = *request.reference_location;
        }
        
        // Query nearest addresses
        auto candidates = m_spatialIndex->findNearestAddresses(
            estimated_center, 
            request.max_results
        );
        
        if (!candidates.empty()) {
            // Convert first match to GeocodingCandidate
            const auto& record = candidates[0];
            best_candidate.latitude = record.location.latitude;
            best_candidate.longitude = record.location.longitude;
            best_candidate.formatted_address = record.formatted_address;
            best_candidate.components = normalized_address;
            best_candidate.confidence_score = 0.85;  // TODO: Calculate based on string match
            best_candidate.quality = record.quality;
            best_candidate.accuracy_meters = geocoding::qualityToAccuracyMeters(record.quality);
            best_candidate.match_type = "spatial_proximity";
            best_candidate.data_source = record.data_source;
            best_candidate.geocoding_method = "spatial_index_knn";
            best_candidate.timestamp_ms = static_cast<uint64_t>(
                std::chrono::system_clock::now().time_since_epoch().count() / 1000000);
        }
    }
    
    return best_candidate;
}

geocoding::AddressComponents POIService::reverseGeocodeEnhanced(
    double latitude, double longitude) {
    // TODO: Full implementation in Week 3-4 with spatial index
    // For now, use nearest POI as fallback
    
    geocoding::AddressComponents result;
    
    Point queryPoint{latitude, longitude};
    double minDistance = std::numeric_limits<double>::max();
    const POI* nearestPOI = nullptr;
    
    for (const auto& poi : m_poiDatabase) {
        Point poiPoint{poi.latitude, poi.longitude};
        double dist = calculateDistance(queryPoint, poiPoint);
        if (dist < minDistance) {
            minDistance = dist;
            nearestPOI = &poi;
        }
    }
    
    if (nearestPOI && minDistance < 100.0) { // Within 100 meters
        // Parse address string (crude implementation)
        // TODO: Replace with proper reverse geocoder in Week 3
        result.street_name = nearestPOI->name;
        result.city = "Unknown"; // POI database doesn't have structured address
        result.country_code = "US";
        result.country = "United States";
    }
    
    return result;
}

} // namespace nav



