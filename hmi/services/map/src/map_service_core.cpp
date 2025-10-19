#include "../include/map_service_core.h"
#include <QDebug>
#include <QRandomGenerator>
#include <algorithm>

namespace nav {

MapServiceCore::MapServiceCore(QObject* parent)
    : QObject(parent)
    , m_initialized(false)
    , m_serviceReady(false)
    , m_tileLoadTimer(new QTimer(this))
    , m_dataUpdateTimer(new QTimer(this))
{
    // Setup tile loading timer
    connect(m_tileLoadTimer, &QTimer::timeout, this, &MapServiceCore::performTileLoading);
    m_tileLoadTimer->setSingleShot(true);
    
    // Setup data update timer
    connect(m_dataUpdateTimer, &QTimer::timeout, this, &MapServiceCore::loadSampleData);
    m_dataUpdateTimer->setSingleShot(true);
}

MapServiceCore::~MapServiceCore()
{
    shutdown();
}

bool MapServiceCore::initialize()
{
    if (m_initialized) {
        return true;
    }
    
    qDebug() << "🗺️ [MAP CORE] Initializing map service...";
    
    // Initialize map data
    m_pois.clear();
    m_categoryIndex.clear();
    m_tileCache.clear();
    m_pendingTileLoads.clear();
    
    // Load sample data after short delay
    m_dataUpdateTimer->start(100);
    
    m_initialized = true;
    m_serviceReady = true;
    
    qDebug() << "✅ [MAP CORE] Map service initialized successfully";
    emit serviceStatusChanged(true);
    
    return true;
}

void MapServiceCore::shutdown()
{
    if (!m_initialized) {
        return;
    }
    
    qDebug() << "🗺️ [MAP CORE] Shutting down map service...";
    
    m_tileLoadTimer->stop();
    m_dataUpdateTimer->stop();
    clearTileCache();
    m_pois.clear();
    m_categoryIndex.clear();
    
    m_initialized = false;
    m_serviceReady = false;
    
    emit serviceStatusChanged(false);
    qDebug() << "✅ [MAP CORE] Map service shut down";
}

std::vector<POI> MapServiceCore::findPOINearLocation(const Point& location, double radiusMeters, const QString& category) const
{
    std::vector<POI> nearbyPOIs;
    
    for (const auto& poi : m_pois) {
        Point poiLocation{poi.latitude, poi.longitude};
        double distance = calculateDistance(location, poiLocation);
        
        if (distance <= radiusMeters) {
            // Check category filter
            if (category.isEmpty() || QString::fromStdString(poi.category) == category) {
                nearbyPOIs.push_back(poi);
            }
        }
    }
    
    // Sort by distance
    std::sort(nearbyPOIs.begin(), nearbyPOIs.end(), 
              [&location, this](const POI& a, const POI& b) {
                  Point locA{a.latitude, a.longitude};
                  Point locB{b.latitude, b.longitude};
                  return calculateDistance(location, locA) < calculateDistance(location, locB);
              });
    
    qDebug() << "🔍 [MAP CORE] Found" << nearbyPOIs.size() << "POIs near location within" << radiusMeters << "m";
    
    return nearbyPOIs;
}

std::vector<POI> MapServiceCore::searchPOI(const QString& searchTerm) const
{
    std::vector<POI> results;
    QString lowerSearchTerm = searchTerm.toLower();
    
    for (const auto& poi : m_pois) {
        QString poiName = QString::fromStdString(poi.name);
        QString poiCategory = QString::fromStdString(poi.category);
        
        if (poiName.toLower().contains(lowerSearchTerm) ||
            poiCategory.toLower().contains(lowerSearchTerm)) {
            results.push_back(poi);
        }
    }
    
    qDebug() << "🔍 [MAP CORE] POI search for '" << searchTerm << "' returned" << results.size() << "results";
    
    return results;
}

POI MapServiceCore::getPOIById(uint64_t poiId) const
{
    for (const auto& poi : m_pois) {
        if (poi.poi_id == poiId) {
            return poi;
        }
    }
    
    // Return empty POI if not found
    return POI{};
}

MapTile MapServiceCore::getMapTile(const Point& location, int zoomLevel)
{
    uint32_t tileId = generateTileId(location, zoomLevel);
    
    auto it = m_tileCache.find(tileId);
    if (it != m_tileCache.end()) {
        qDebug() << "📍 [MAP CORE] Tile" << tileId << "found in cache";
        return it->second;
    }
    
    // Create new tile and schedule loading
    MapTile tile;
    tile.tileId = tileId;
    tile.topLeft = Point(location.latitude + 0.01, location.longitude - 0.01);
    tile.bottomRight = Point(location.latitude - 0.01, location.longitude + 0.01);
    tile.zoomLevel = zoomLevel;
    tile.loaded = false;
    
    m_tileCache[tileId] = tile;
    m_pendingTileLoads.push_back(tileId);
    
    // Start loading timer if not already running
    if (!m_tileLoadTimer->isActive()) {
        m_tileLoadTimer->start(TILE_LOAD_DELAY_MS);
    }
    
    qDebug() << "📍 [MAP CORE] Scheduled loading for tile" << tileId;
    
    return tile;
}

void MapServiceCore::preloadTilesForArea(const Point& center, double radiusMeters, int zoomLevel)
{
    qDebug() << "📍 [MAP CORE] Preloading tiles for area around" 
             << center.latitude << "," << center.longitude 
             << "radius:" << radiusMeters << "m";
    
    // Calculate grid of tiles to preload
    double degreesPerMeter = 1.0 / 111320.0; // Approximate
    double radiusDegrees = radiusMeters * degreesPerMeter;
    
    int tilesPerSide = qMax(1, static_cast<int>(radiusDegrees / 0.01)); // 0.01 degree tiles
    
    for (int x = -tilesPerSide; x <= tilesPerSide; ++x) {
        for (int y = -tilesPerSide; y <= tilesPerSide; ++y) {
            Point tileCenter(
                center.latitude + (y * 0.01),
                center.longitude + (x * 0.01)
            );
            getMapTile(tileCenter, zoomLevel);
        }
    }
}

void MapServiceCore::clearTileCache()
{
    qDebug() << "🗑️ [MAP CORE] Clearing tile cache (" << m_tileCache.size() << "tiles)";
    m_tileCache.clear();
    m_pendingTileLoads.clear();
}

std::vector<QString> MapServiceCore::getAvailableCategories() const
{
    std::vector<QString> categories;
    for (const auto& pair : m_categoryIndex) {
        categories.push_back(pair.first);
    }
    return categories;
}

int MapServiceCore::getTotalPOICount() const
{
    return static_cast<int>(m_pois.size());
}

int MapServiceCore::getLoadedTileCount() const
{
    int loadedCount = 0;
    for (const auto& pair : m_tileCache) {
        if (pair.second.loaded) {
            loadedCount++;
        }
    }
    return loadedCount;
}

bool MapServiceCore::isServiceReady() const
{
    return m_serviceReady;
}

QString MapServiceCore::getServiceStatus() const
{
    if (!m_initialized) {
        return "Not Initialized";
    }
    if (!m_serviceReady) {
        return "Not Ready";
    }
    return QString("Ready - %1 POIs, %2 tiles loaded")
           .arg(m_pois.size())
           .arg(getLoadedTileCount());
}

void MapServiceCore::loadSampleData()
{
    qDebug() << "📊 [MAP CORE] Loading sample POI data...";
    
    initializeSamplePOIs();
    initializeSampleTiles();
    
    emit poiDataUpdated();
    emit mapDataChanged();
    
    qDebug() << "✅ [MAP CORE] Sample data loaded:" << m_pois.size() << "POIs";
}

void MapServiceCore::performTileLoading()
{
    if (m_pendingTileLoads.empty()) {
        return;
    }
    
    // Load one tile per timer tick to simulate async loading
    uint32_t tileId = m_pendingTileLoads.front();
    m_pendingTileLoads.erase(m_pendingTileLoads.begin());
    
    auto it = m_tileCache.find(tileId);
    if (it != m_tileCache.end()) {
        MapTile& tile = it->second;
        
        // Simulate tile data loading (in real system would fetch from map server)
        tile.imageData = QByteArray(1024, static_cast<char>(QRandomGenerator::global()->bounded(256)));
        tile.loaded = true;
        
        qDebug() << "📍 [MAP CORE] Tile" << tileId << "loaded successfully";
        emit mapTileLoaded(tile);
    }
    
    // Continue loading if more tiles pending
    if (!m_pendingTileLoads.empty()) {
        m_tileLoadTimer->start(TILE_LOAD_DELAY_MS);
    }
    
    // Clean up cache if too large
    if (static_cast<int>(m_tileCache.size()) > MAX_TILE_CACHE_SIZE) {
        qDebug() << "🗑️ [MAP CORE] Tile cache full, cleaning up oldest tiles";
        // In a real implementation, would use LRU eviction
        auto oldest = m_tileCache.begin();
        m_tileCache.erase(oldest);
    }
}

void MapServiceCore::initializeSamplePOIs()
{
    m_pois.clear();
    m_categoryIndex.clear();
    
    // Sample POIs around Hanoi, Vietnam
    struct SamplePOI {
        double lat, lon;
        QString name, category, description;
        double rating;
    };
    
    std::vector<SamplePOI> sampleData = {
        // Restaurants
        {21.0285, 105.8048, "Pho Gia Truyen", "Restaurant", "Traditional Vietnamese pho restaurant", 4.5},
        {21.0290, 105.8055, "Bun Cha Huong Lien", "Restaurant", "Famous bun cha restaurant", 4.3},
        {21.0275, 105.8040, "Quan An Ngon", "Restaurant", "Vietnamese street food collection", 4.2},
        
        // Attractions
        {21.0245, 105.8412, "Hoan Kiem Lake", "Attraction", "Historic lake in Hanoi center", 4.6},
        {21.0336, 105.8448, "Temple of Literature", "Attraction", "Vietnam's first university", 4.4},
        {21.0583, 105.8347, "Ho Chi Minh Mausoleum", "Attraction", "Historic mausoleum", 4.3},
        
        // Hotels
        {21.0280, 105.8050, "Hanoi La Siesta Hotel", "Hotel", "Boutique hotel in Old Quarter", 4.4},
        {21.0270, 105.8045, "Golden Sun Palace Hotel", "Hotel", "Luxury hotel with spa", 4.2},
        
        // Shopping
        {21.0295, 105.8060, "Dong Xuan Market", "Shopping", "Traditional Vietnamese market", 4.0},
        {21.0320, 105.8080, "Weekend Night Market", "Shopping", "Street market on weekends", 3.8},
        
        // Gas Stations
        {21.0300, 105.8100, "Petrolimex Station", "Gas Station", "Fuel and convenience store", 3.5},
        {21.0250, 105.8020, "Shell Station", "Gas Station", "International fuel brand", 3.7},
    };
    
    for (size_t i = 0; i < sampleData.size(); ++i) {
        const auto& sample = sampleData[i];
        
        POI poi;
        poi.poi_id = static_cast<uint64_t>(i + 1);
        poi.latitude = sample.lat;
        poi.longitude = sample.lon;
        poi.name = sample.name.toStdString();
        poi.category = sample.category.toStdString();
        poi.address = "Hanoi, Vietnam";  // Default address
        
        m_pois.push_back(poi);
        
        // Update category index
        m_categoryIndex[sample.category].push_back(poi.poi_id);
    }
    
    qDebug() << "📊 [MAP CORE] Initialized" << m_pois.size() << "sample POIs";
}

void MapServiceCore::initializeSampleTiles()
{
    // Pre-populate some tiles around Hanoi
    std::vector<Point> tileLocations = {
        Point(21.0285, 105.8048), // Hanoi center
        Point(21.0300, 105.8100), // North
        Point(21.0270, 105.8000), // South
        Point(21.0285, 105.8148), // East
        Point(21.0285, 105.7948), // West
    };
    
    for (const auto& location : tileLocations) {
        getMapTile(location, 10); // Zoom level 10
    }
    
    qDebug() << "📊 [MAP CORE] Initialized sample map tiles";
}

uint32_t MapServiceCore::generateTileId(const Point& location, int zoomLevel) const
{
    // Simple tile ID generation based on location and zoom
    uint32_t latHash = static_cast<uint32_t>(location.latitude * 10000) % 10000;
    uint32_t lonHash = static_cast<uint32_t>(location.longitude * 10000) % 10000;
    uint32_t zoomHash = static_cast<uint32_t>(zoomLevel) % 100;
    
    return (latHash << 16) | (lonHash << 8) | zoomHash;
}

double MapServiceCore::calculateDistance(const Point& p1, const Point& p2) const
{
    // Haversine formula
    const double R = 6371000; // Earth radius in meters
    
    double lat1Rad = p1.latitude * M_PI / 180.0;
    double lat2Rad = p2.latitude * M_PI / 180.0;
    double deltaLatRad = (p2.latitude - p1.latitude) * M_PI / 180.0;
    double deltaLonRad = (p2.longitude - p1.longitude) * M_PI / 180.0;
    
    double a = sin(deltaLatRad / 2) * sin(deltaLatRad / 2) +
               cos(lat1Rad) * cos(lat2Rad) *
               sin(deltaLonRad / 2) * sin(deltaLonRad / 2);
    double c = 2 * atan2(sqrt(a), sqrt(1 - a));
    
    return R * c;
}

} // namespace nav