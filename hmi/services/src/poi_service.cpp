#include "../include/poi_service.h"
#include <QFile>
#include <QIODevice>
#include <QJsonDocument>
#include <QDebug>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace nav {

POIService::POIService(QObject* parent)
    : QObject(parent)
    , m_initialized(false)
    , m_serviceReady(false)
    , m_lastQueryTimeMs(0.0)
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
    return POI(); // Return empty POI if not found
}

GeocodingResult POIService::geocodeAddress(const AddressRequest& address) {
    // Query geocoding database for address m_poiDatabase
    //Needtodo
    GeocodingResult result;
    return GeocodingResult(); // Return empty result if not found   
}
} // namespace nav


