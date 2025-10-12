#pragma once

#include "../../models/include/navigation_models.h"
#include <QObject>
#include <QTimer>
#include <vector>
#include <map>

namespace nav {

/**
 * @brief POI (Point of Interest) data structure
 */
struct POI {
    uint32_t id;
    Point location;
    QString name;
    QString category;
    QString description;
    double rating;
};

/**
 * @brief Map tile data structure
 */
struct MapTile {
    uint32_t tileId;
    Point topLeft;
    Point bottomRight;
    int zoomLevel;
    QByteArray imageData;
    bool loaded;
};

/**
 * @brief Core map service integrated into HMI
 * Provides map data, POI information, and tile management
 */
class MapServiceCore : public QObject
{
    Q_OBJECT

public:
    explicit MapServiceCore(QObject* parent = nullptr);
    ~MapServiceCore();

    // Main service interface
    bool initialize();
    void shutdown();
    
    // Map data queries
    std::vector<POI> findPOINearLocation(const Point& location, double radiusMeters, const QString& category = QString()) const;
    std::vector<POI> searchPOI(const QString& searchTerm) const;
    POI getPOIById(uint32_t poiId) const;
    
    // Map tile management
    MapTile getMapTile(const Point& location, int zoomLevel);
    void preloadTilesForArea(const Point& center, double radiusMeters, int zoomLevel);
    void clearTileCache();
    
    // Map information
    std::vector<QString> getAvailableCategories() const;
    int getTotalPOICount() const;
    int getLoadedTileCount() const;
    
    // Service status
    bool isServiceReady() const;
    QString getServiceStatus() const;

signals:
    void poiDataUpdated();
    void mapTileLoaded(const MapTile& tile);
    void mapDataChanged();
    void serviceStatusChanged(bool ready);

private slots:
    void loadSampleData();
    void performTileLoading();

private:
    // Map data management
    void initializeSamplePOIs();
    void initializeSampleTiles();
    uint32_t generateTileId(const Point& location, int zoomLevel) const;
    double calculateDistance(const Point& p1, const Point& p2) const;
    
    // Service state
    bool m_initialized;
    bool m_serviceReady;
    
    // POI data
    std::vector<POI> m_pois;
    std::map<QString, std::vector<uint32_t>> m_categoryIndex; // category -> POI IDs
    
    // Map tile data
    std::map<uint32_t, MapTile> m_tileCache;
    QTimer* m_tileLoadTimer;
    std::vector<uint32_t> m_pendingTileLoads;
    
    // Data management
    QTimer* m_dataUpdateTimer;
    
    static constexpr int MAX_TILE_CACHE_SIZE = 100;
    static constexpr int TILE_LOAD_DELAY_MS = 100;
};

} // namespace nav