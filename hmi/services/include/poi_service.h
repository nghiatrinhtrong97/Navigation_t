#ifndef POI_SERVICE_H
#define POI_SERVICE_H
#include <cstdint>
#include <QObject>
#include <QString>
#include <QJsonArray>
#include <QJsonObject>
#include <vector>
#include "../../models/include/navigation_models.h"  // Use Point from navigation_models

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
 * 
 */
struct AddressRequest {  // fixed typo: Adress -> Address
    std::string street_number;
    std::string street_name;
    std::string city;
    std::string state;
    std::string zip_code;
    std::string country;
};

struct GeocodingResult {
    bool success;
    double latitude;
    double longitude;
    std::string standard_address;
    double accuracy; // e.g., in meters
    std::string match_type; // e.g., "exact", "approximate"

    GeocodingResult() 
        : success(false), latitude(0.0), longitude(0.0), standard_address(""), accuracy(0.0), match_type("") {}
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

    // Address geocoding services
    GeocodingResult geocodeAddress(const AddressRequest& address);  // fixed typo: Adress -> Address
    GeocodingResult reverseGeocode(double latitude, double longitude);

private:
    //Core service state
    bool m_initialized;
    bool m_serviceReady;
    mutable double m_lastQueryTimeMs;

    //POI data storage and indexing
    std::vector<POI> m_poiDatabase;
    
    // Helper methods
    void loadPOIDatabase();
    double calculateDistance(const Point& p1, const Point& p2) const;
};

} // namespace nav

#endif // POI_SERVICE_H
