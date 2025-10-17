#include "../include/map_widget.h"
#include <QPainter>
#include <QApplication>
#include <QDebug>
#include <QStandardPaths>
#include <QDir>
#include <cmath>

namespace nav {

/**
 * @brief Constructor for MapWidget - Initializes interactive map component
 * @param parent Parent widget for Qt widget hierarchy
 * 
 * Creates a fully interactive map widget with the following UI components:
 * - Multi-style map display (Satellite, Street, Hybrid)
 * - Zoom controls and pan functionality
 * - Coordinate display and grid overlay
 * - Route visualization with waypoint markers
 * - Current position tracking with heading indicator
 * - Click-to-select location functionality
 * - Smooth animations for map transitions
 */
MapWidget::MapWidget(QWidget *parent)
    : QWidget(parent)
    , m_centerLat(getDefaultLat())           // Default center: Hanoi coordinates (21.0285°N)
    , m_centerLon(getDefaultLon())           // Default center: Hanoi coordinates (105.8542°E)
    , m_zoomLevel(DEFAULT_ZOOM)              // Default zoom level for city-wide view
    , m_currentMapStyle(SATELLITE)           // Start with satellite imagery style
    , m_mapImagesLoaded(false)               // Flag for resource loading status
    , m_currentHeading(0.0)                  // Vehicle heading direction (degrees)
    , m_hasCurrentPosition(false)            // GPS position availability flag
    , m_hasStartPoint(false)                 // Route start point marker flag
    , m_hasEndPoint(false)                   // Route destination marker flag
    , m_hasClickedPoint(false)               // User-clicked location marker flag
    , m_hasPOILabel(false)                   // POI text label display flag
    , m_dragging(false)                      // Mouse drag state for map panning
    , m_centerAnimation(nullptr)             // Smooth map centering animation
    , m_zoomAnimation(nullptr)               // Smooth zoom transition animation
    , m_showGrid(true)                       // Geographic coordinate grid overlay
    , m_showCoordinates(true)                // Current position coordinate display
    , m_showScale(true)                      // Distance scale bar indicator
    , m_showCopyright(true)                  // Map attribution/copyright notice
    , m_mapTopLeft(getMapBoundsNorth(), getMapBoundsWest())      // Geographic bounds
    , m_mapBottomRight(getMapBoundsSouth(), getMapBoundsEast())  // Geographic bounds
{
    // Configure widget properties for optimal map interaction
    setMinimumSize(400, 300);                           // Minimum size for usability
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);  // Fill available space
    setMouseTracking(true);                             // Enable mouse coordinate tracking
    setFocusPolicy(Qt::StrongFocus);                    // Accept keyboard input for navigation
    
    // Initialize map image resources from Qt Resource System
    loadMapImages();
    
    // Setup smooth animation controllers for user experience
    m_centerAnimation = new QPropertyAnimation(this, "centerLatitude");
    m_centerAnimation->setDuration(1000);               // 1 second smooth centering
    connect(m_centerAnimation, &QPropertyAnimation::finished, this, &MapWidget::onAnimationFinished);
    
    m_zoomAnimation = new QPropertyAnimation(this, "zoomLevel");
    m_zoomAnimation->setDuration(500);                  // 0.5 second zoom transition
    
    // Restore user preferences from previous session
    loadSettings();
    
    qDebug() << "MapWidget initialized with real image support";
}

MapWidget::~MapWidget()
{
    // Persist user preferences for next session
    saveSettings();
}

/**
 * @brief Load map background images from Qt Resource System
 * 
 * Loads three map styles with fallback to programmatically generated placeholder images:
 * 1. Satellite view: Real satellite imagery of Hanoi area
 * 2. Street map: Road network with street names and landmarks
 * 3. Hybrid view: Vietnam country overview for context
 * 
 * If resource loading fails, creates dummy images with visual indicators
 * to maintain functionality during development/testing.
 */
void MapWidget::loadMapImages()
{
    qDebug() << "Loading map images...";
    
    // Define resource paths from resources.qrc
    QString satellitePath = ":/maps/hanoi_satellite.jpg";    // Detailed Hanoi satellite imagery
    QString streetPath = ":/maps/hanoi_street.jpg";          // Street map with road networks
    QString hybridPath = ":/maps/vietnam_overview.jpg";      // Country-level overview map
    
    // Load satellite map with fallback placeholder generation
    if (!m_satelliteMap.load(satellitePath)) {
        qDebug() << "Failed to load satellite map, creating placeholder";
        m_satelliteMap = QPixmap(800, 600);
        m_satelliteMap.fill(QColor(120, 150, 80));          // Earth-tone green background
        
        QPainter painter(&m_satelliteMap);
        // Create visual placeholder with geographic features
        painter.setPen(QPen(Qt::darkGreen, 2));
        painter.setFont(QFont("Arial", 16));
        painter.drawText(m_satelliteMap.rect(), Qt::AlignCenter, 
                        "Satellite View\n(Hanoi Area)\nDummy Image");
        
        // Draw simulated roads and water features for visual reference
        painter.setPen(QPen(Qt::gray, 3));                  // Major roads
        painter.drawLine(100, 200, 700, 400);              // Diagonal highway
        painter.drawLine(200, 100, 600, 500);              // Cross street
        painter.setPen(QPen(Qt::blue, 2));
        painter.drawEllipse(300, 200, 200, 100);           // Hoan Kiem Lake representation
    }
    
    // Load street map with grid-pattern fallback
    if (!m_streetMap.load(streetPath)) {
        qDebug() << "Failed to load street map, creating placeholder";
        m_streetMap = QPixmap(800, 600);
        m_streetMap.fill(QColor(245, 245, 220));            // Light beige paper map color
        
        QPainter painter(&m_streetMap);
        painter.setPen(QPen(Qt::black, 1));
        painter.setFont(QFont("Arial", 16));
        painter.drawText(m_streetMap.rect(), Qt::AlignCenter,
                        "Street Map\n(Hanoi Area)\nDummy Image");
        
        // Create grid pattern simulating city street layout
        painter.setPen(QPen(Qt::darkGray, 2));
        for (int x = 50; x < 800; x += 100) {              // Vertical streets
            painter.drawLine(x, 0, x, 600);
        }
        for (int y = 50; y < 600; y += 100) {              // Horizontal streets
            painter.drawLine(0, y, 800, y);
        }
        
        // Highlight main arterial roads
        painter.setPen(QPen(Qt::red, 4));
        painter.drawLine(0, 300, 800, 300);                // Main horizontal avenue
        painter.drawLine(400, 0, 400, 600);                // Main vertical boulevard
    }
    
    // Load hybrid/overview map with coastline simulation
    if (!m_hybridMap.load(hybridPath)) {
        qDebug() << "Failed to load hybrid map, creating placeholder";
        m_hybridMap = QPixmap(800, 600);
        m_hybridMap.fill(QColor(100, 120, 140));            // Ocean blue-gray background
        
        QPainter painter(&m_hybridMap);
        painter.setPen(QPen(Qt::white, 2));
        painter.setFont(QFont("Arial", 16));
        painter.drawText(m_hybridMap.rect(), Qt::AlignCenter,
                        "Hybrid View\n(Vietnam Overview)\nDummy Image");
        
        // Draw simplified Vietnam coastline for geographic context
        painter.setPen(QPen(Qt::yellow, 3));
        QPolygon coastline;
        coastline << QPoint(100, 100) << QPoint(200, 80) << QPoint(300, 120)
                  << QPoint(500, 150) << QPoint(700, 200) << QPoint(750, 400)
                  << QPoint(700, 500) << QPoint(500, 480) << QPoint(300, 450)
                  << QPoint(100, 400);
        painter.drawPolygon(coastline);
    }
    
    m_mapImagesLoaded = true;
    updateMapCache();                                       // Prepare initial display cache
    
    qDebug() << "Map images loaded successfully";
}

/**
 * @brief Get current map background image based on selected style
 * @return QPixmap containing the current map background
 */
QPixmap MapWidget::getCurrentMapImage() const
{
    return getMapImageForStyle(m_currentMapStyle);
}

/**
 * @brief Retrieve map image for specific display style
 * @param style Map style enum (SATELLITE, STREET_MAP, HYBRID)
 * @return QPixmap for the requested map style
 */
QPixmap MapWidget::getMapImageForStyle(MapStyle style) const
{
    switch (style) {
        case SATELLITE:
            return m_satelliteMap;                          // High-resolution satellite imagery
        case STREET_MAP:
            return m_streetMap;                             // Road network with labels
        case HYBRID:
        default:
            return m_hybridMap;                             // Overview/context map
    }
}

/**
 * @brief Update cached map image and trigger UI refresh
 * 
 * Caches the current map style image for efficient rendering
 * and triggers a repaint event to update the display.
 */
void MapWidget::updateMapCache()
{
    m_currentMapCache = getCurrentMapImage();
    update();                                               // Trigger paintEvent()
}

/**
 * @brief Get human-readable name for current map style
 * @return QString containing style name for UI display
 */
QString MapWidget::mapStyle() const
{
    switch (m_currentMapStyle) {
        case SATELLITE: return "Satellite";                 // Satellite imagery view
        case STREET_MAP: return "Street Map";               // Road-focused view
        case HYBRID: return "Hybrid";                       // Mixed/overview view
        default: return "Unknown";
    }
}

/**
 * @brief Set map center latitude with change detection
 * @param lat New latitude in decimal degrees
 * 
 * Updates map center position and emits change signal for UI updates.
 * Triggers map repaint if coordinate actually changed.
 */
void MapWidget::setCenterLatitude(double lat)
{
    if (lat != m_centerLat) {
        m_centerLat = lat;
        update();                                           // Redraw map at new position
        emit centerChanged(m_centerLat, m_centerLon);       // Notify coordinate displays
    }
}

/**
 * @brief Set map center longitude with change detection
 * @param lon New longitude in decimal degrees
 */
void MapWidget::setCenterLongitude(double lon)
{
    if (lon != m_centerLon) {
        m_centerLon = lon;
        update();
        emit centerChanged(m_centerLat, m_centerLon);
    }
}

/**
 * @brief Set zoom level with bounds checking
 * @param zoom New zoom level (MIN_ZOOM to MAX_ZOOM)
 * 
 * Controls map detail level and coordinate scaling.
 * Higher zoom = more detail, smaller geographic area.
 */
void MapWidget::setZoomLevel(int zoom)
{
    int newZoom = qBound(MIN_ZOOM, zoom, MAX_ZOOM);         // Enforce valid range
    if (newZoom != m_zoomLevel) {
        m_zoomLevel = newZoom;
        update();                                           // Redraw with new scale
        emit zoomChanged(m_zoomLevel);                      // Update zoom indicators
    }
}

/**
 * @brief Set map style from string identifier
 * @param style Style name ("Satellite", "Street Map", "Hybrid")
 */
void MapWidget::setMapStyle(const QString& style)
{
    if (style == "Satellite") {
        setMapStyle(SATELLITE);
    } else if (style == "Street Map") {
        setMapStyle(STREET_MAP);
    } else if (style == "Hybrid") {
        setMapStyle(HYBRID);
    }
}

/**
 * @brief Set map style from enum with visual update
 * @param style MapStyle enum value
 */
void MapWidget::setMapStyle(MapStyle style)
{
    if (style != m_currentMapStyle) {
        m_currentMapStyle = style;
        updateMapCache();                                   // Switch background image
        qDebug() << "Map style changed to:" << mapStyle();
    }
}

/**
 * @brief Update current GPS position with heading indicator
 * @param position Geographic coordinates (latitude, longitude)
 * @param heading Vehicle direction in degrees (0° = North)
 * 
 * Displays blue circular marker with white crosshairs and directional arrow.
 * Used for real-time vehicle tracking and navigation guidance.
 */
void MapWidget::setCurrentPosition(const Point& position, double heading)
{
    m_currentPosition = position;
    m_currentHeading = heading;
    m_hasCurrentPosition = true;
    update();                                               // Redraw position marker
}

/**
 * @brief Set route starting point with green square marker
 * @param point Geographic coordinates for route origin
 */
void MapWidget::setStartPoint(const Point& point)
{
    m_startPoint = point;
    m_hasStartPoint = true;
    update();                                               // Show green "S" marker
}

/**
 * @brief Set route destination with red triangle marker
 * @param point Geographic coordinates for route destination
 */
void MapWidget::setEndPoint(const Point& point)
{
    m_endPoint = point;
    m_hasEndPoint = true;
    update();                                               // Show red "E" marker
}

/**
 * @brief Display calculated route as connected blue line
 * @param route Vector of geographic waypoints defining the path
 * 
 * Renders route as thick blue line with directional arrows.
 * Connects all waypoints in sequence for visual navigation guidance.
 */
void MapWidget::setRoute(const std::vector<Point>& route)
{
    m_routePoints = route;
    update();                                               // Draw route path
}

/**
 * @brief Remove route visualization from map display
 */
void MapWidget::clearRoute()
{
    m_routePoints.clear();
    update();
}

/**
 * @brief Remove all waypoint markers (start and end points)
 */
void MapWidget::clearWaypoints()
{
    m_hasStartPoint = false;
    m_hasEndPoint = false;
    update();
}

/**
 * @brief Set user-clicked location with prominent red marker
 * @param point Geographic coordinates of click location
 * 
 * Displays large red circle with crosshairs for user-selected points.
 * Used for destination selection and point-of-interest marking.
 */
void MapWidget::setClickedPoint(const Point& point)
{
    m_clickedPoint = point;
    m_hasClickedPoint = true;
    qDebug() << "Clicked point set at:" << point.latitude << "," << point.longitude;
    update();
}

/**
 * @brief Remove clicked point marker from display
 */
void MapWidget::clearClickedPoint()
{
    m_hasClickedPoint = false;
    m_hasPOILabel = false;  // Also clear label when clearing point
    update();
}

/**
 * @brief Set POI text label to display on map
 * @param label Text label to display (e.g., POI name)
 */
void MapWidget::setPOILabel(const QString& label)
{
    m_poiLabel = label;
    m_hasPOILabel = true;
    qDebug() << "POI label set:" << label;
    update();
}

/**
 * @brief Clear POI text label from map
 */
void MapWidget::clearPOILabel()
{
    m_hasPOILabel = false;
    m_poiLabel.clear();
    update();
}

/**
 * @brief Remove route starting point marker
 */
void MapWidget::clearStartPoint()
{
    m_hasStartPoint = false;
    update();
}

/**
 * @brief Remove route destination marker
 */
void MapWidget::clearEndPoint()
{
    m_hasEndPoint = false;
    update();
}

/**
 * @brief Instantly center map on specified coordinates
 * @param lat Target latitude in decimal degrees
 * @param lon Target longitude in decimal degrees
 */
void MapWidget::centerMap(double lat, double lon)
{
    m_centerLat = lat;
    m_centerLon = lon;
    update();
}

/**
 * @brief Smoothly animate map center to new position
 * @param position Target geographic coordinates
 * 
 * Provides smooth 1-second animation for better user experience
 * when jumping to distant locations or following GPS updates.
 */
void MapWidget::animateToPosition(const Point& position)
{
    if (m_centerAnimation->state() == QPropertyAnimation::Running) {
        m_centerAnimation->stop();                          // Cancel existing animation
    }
    
    m_centerAnimation->setStartValue(m_centerLat);
    m_centerAnimation->setEndValue(position.latitude);
    m_centerAnimation->start();
    
    // Simplified longitude animation (could be improved for great circle paths)
    m_centerLon = position.longitude;
}

/**
 * @brief Smoothly animate zoom level transition
 * @param targetZoom Desired final zoom level
 * 
 * Provides smooth 0.5-second zoom animation for better visual continuity
 * compared to instant zoom level changes.
 */
void MapWidget::animateZoomTo(int targetZoom)
{
    if (m_zoomAnimation->state() == QPropertyAnimation::Running) {
        m_zoomAnimation->stop();
    }
    
    m_zoomAnimation->setStartValue(m_zoomLevel);
    m_zoomAnimation->setEndValue(targetZoom);
    m_zoomAnimation->start();
}

/**
 * @brief Main rendering function - draws all map components
 * @param event Paint event with clipping information
 * 
 * Renders map in layered order:
 * 1. Background color fill
 * 2. Map background image (satellite/street/hybrid)
 * 3. Geographic coordinate grid (if enabled)
 * 4. Route path with directional arrows
 * 5. Waypoint markers (start/end points)
 * 6. Current position indicator with heading
 * 7. User-clicked point marker
 * 8. UI overlays (scale bar, coordinates, copyright)
 */
void MapWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);          // Smooth lines and curves
    painter.setRenderHint(QPainter::SmoothPixmapTransform); // High-quality image scaling
    
    // Clear background with sky blue color
    painter.fillRect(rect(), QColor(200, 220, 255));        // Light blue background
    
    // Draw base map layer
    drawMap(painter);
    
    // Draw optional coordinate grid overlay
    if (m_showGrid) {
        drawGrid(painter);
    }
    
    // Draw navigation route if available
    if (!m_routePoints.empty()) {
        drawRoute(painter);
    }
    
    // Draw all waypoint markers
    drawWaypoints(painter);
    
    // Draw current GPS position if available
    if (m_hasCurrentPosition) {
        drawCurrentPosition(painter);
    }
    
    // Draw user-selected point if available
    if (m_hasClickedPoint) {
        drawClickedPoint(painter);
    }
    
    // Draw UI information overlays
    drawMapOverlays(painter);
}

/**
 * @brief Render background map image with geographic positioning
 * @param painter QPainter instance for drawing operations
 * 
 * Scales and positions the background map image based on current
 * zoom level and center coordinates. Includes border decoration.
 */
void MapWidget::drawMap(QPainter& painter)
{
    if (!m_mapImagesLoaded || m_currentMapCache.isNull()) {
        return;                                             // Skip if images not ready
    }
    
    // Calculate display scale based on zoom level
    double scale = pow(2, m_zoomLevel - 10);                // Exponential zoom scaling
    
    // Convert geographic bounds to screen coordinates
    QPoint mapTopLeftScreen = geoToScreen(m_mapTopLeft);
    QPoint mapBottomRightScreen = geoToScreen(m_mapBottomRight);
    
    QRect mapRect(mapTopLeftScreen, mapBottomRightScreen);
    
    // Draw the background map image fitted to geographic bounds
    painter.drawPixmap(mapRect, m_currentMapCache);
    
    // Draw decorative border around map area
    painter.setPen(QPen(Qt::darkGray, 2));
    painter.drawRect(mapRect);
}

/**
 * @brief Draw geographic coordinate grid overlay
 * @param painter QPainter instance for drawing operations
 * 
 * Renders latitude and longitude grid lines with spacing
 * that adapts to current zoom level for optimal readability.
 */
void MapWidget::drawGrid(QPainter& painter)
{
    painter.setPen(QPen(QColor(255, 255, 255, 100), 1));    // Semi-transparent white lines
    
    // Calculate grid spacing based on zoom level (closer lines at higher zoom)
    double gridSpacing = 0.01 / pow(2, m_zoomLevel - 10);
    
    // Draw latitude lines (horizontal)
    double startLat = floor(m_centerLat / gridSpacing) * gridSpacing;
    for (int i = -10; i <= 10; ++i) {
        double lat = startLat + i * gridSpacing;
        QPoint p1 = geoToScreen(Point(lat, m_centerLon - 0.1));
        QPoint p2 = geoToScreen(Point(lat, m_centerLon + 0.1));
        painter.drawLine(p1, p2);
    }
    
    // Draw longitude lines (vertical)
    double startLon = floor(m_centerLon / gridSpacing) * gridSpacing;
    for (int i = -10; i <= 10; ++i) {
        double lon = startLon + i * gridSpacing;
        QPoint p1 = geoToScreen(Point(m_centerLat - 0.1, lon));
        QPoint p2 = geoToScreen(Point(m_centerLat + 0.1, lon));
        painter.drawLine(p1, p2);
    }
}

/**
 * @brief Draw navigation route as connected line segments
 * @param painter QPainter instance for drawing operations
 * 
 * Renders route as thick blue line connecting all waypoints
 * with directional arrows indicating travel direction.
 */
void MapWidget::drawRoute(QPainter& painter)
{
    if (m_routePoints.size() < 2) {
        return;                                             // Need at least two points
    }
    
    painter.setPen(QPen(QColor(0, 100, 255), 4));          // Thick blue route line
    painter.setBrush(QBrush(QColor(0, 100, 255, 100)));
    
    QPoint prevPoint = geoToScreen(m_routePoints[0]);
    
    // Connect all route waypoints with line segments
    for (size_t i = 1; i < m_routePoints.size(); ++i) {
        QPoint currentPoint = geoToScreen(m_routePoints[i]);
        painter.drawLine(prevPoint, currentPoint);
        prevPoint = currentPoint;
    }
    
    // Add directional arrows for navigation guidance
    drawDirectionArrows(painter);
}

/**
 * @brief Draw route start and end point markers
 * @param painter QPainter instance for drawing operations
 * 
 * Start point: Green square with "S" label
 * End point: Red triangle with "E" label
 * Different shapes help distinguish waypoint types at a glance.
 */
void MapWidget::drawWaypoints(QPainter& painter)
{
    // Draw start point as green square marker
    if (m_hasStartPoint) {
        QPoint startScreen = geoToScreen(m_startPoint);
        
        // Outer green square border
        painter.setPen(QPen(Qt::darkGreen, 3));
        painter.setBrush(QBrush(Qt::green));
        QRect startRect(startScreen.x() - 10, startScreen.y() - 10, 20, 20);
        painter.drawRect(startRect);
        
        // Inner white square for contrast
        painter.setPen(QPen(Qt::white, 2));
        painter.setBrush(QBrush(Qt::white));
        QRect innerRect(startScreen.x() - 6, startScreen.y() - 6, 12, 12);
        painter.drawRect(innerRect);
        
        // "S" text label for Start point identification
        painter.setPen(QPen(Qt::darkGreen, 2));
        painter.setFont(QFont("Arial", 10, QFont::Bold));
        painter.drawText(startScreen + QPoint(-4, 4), "S");
    }
    
    // Draw end point as red triangle marker
    if (m_hasEndPoint) {
        QPoint endScreen = geoToScreen(m_endPoint);
        
        // Red triangle shape pointing upward
        painter.setPen(QPen(Qt::darkRed, 3));
        painter.setBrush(QBrush(Qt::red));
        
        QPolygon triangle;
        triangle << QPoint(endScreen.x(), endScreen.y() - 12)        // Top vertex
                << QPoint(endScreen.x() - 10, endScreen.y() + 8)     // Bottom left
                << QPoint(endScreen.x() + 10, endScreen.y() + 8);    // Bottom right
        
        painter.drawPolygon(triangle);
        
        // "E" text label for End point identification
        painter.setPen(QPen(Qt::white, 2));
        painter.setFont(QFont("Arial", 10, QFont::Bold));
        painter.drawText(endScreen + QPoint(-4, 4), "E");
    }
}

/**
 * @brief Draw current GPS position with heading indicator
 * @param painter QPainter instance for drawing operations
 * 
 * Renders distinctive blue circle with white crosshairs and
 * red directional arrow showing vehicle orientation.
 */
void MapWidget::drawCurrentPosition(QPainter& painter)
{
    QPoint posScreen = geoToScreen(m_currentPosition);
    
    // Large blue circle background for high visibility
    painter.setPen(QPen(Qt::blue, 4));
    painter.setBrush(QBrush(QColor(30, 144, 255, 220)));    // Bright blue with transparency
    painter.drawEllipse(posScreen, 16, 16);
    
    // White crosshairs for precise position indication
    painter.setPen(QPen(Qt::white, 3));
    // Horizontal crosshair line
    painter.drawLine(posScreen.x() - 8, posScreen.y(), posScreen.x() + 8, posScreen.y());
    // Vertical crosshair line
    painter.drawLine(posScreen.x(), posScreen.y() - 8, posScreen.x(), posScreen.y() + 8);
    
    // Center dot for exact position
    painter.setPen(QPen(Qt::blue, 2));
    painter.setBrush(QBrush(Qt::blue));
    painter.drawEllipse(posScreen, 3, 3);
    
    // "C" text label for Current position identification
    painter.setPen(QPen(Qt::white, 2));
    painter.setFont(QFont("Arial", 10, QFont::Bold));
    painter.drawText(posScreen + QPoint(-4, 4), "C");
    
    // Draw vehicle heading direction arrow
    drawHeadingIndicator(painter, posScreen);
}

/**
 * @brief Draw user-clicked location marker
 * @param painter QPainter instance for drawing operations
 * 
 * Large red circle with white center and crosshairs
 * for highly visible user-selected point indication.
 */
void MapWidget::drawClickedPoint(QPainter& painter)
{
    QPoint posScreen = geoToScreen(m_clickedPoint);
    qDebug() << "Drawing clicked point at screen pos:" << posScreen << "for geo:" << m_clickedPoint.latitude << "," << m_clickedPoint.longitude;
    
    // Large red circle for high visibility
    painter.setPen(QPen(Qt::red, 4));
    painter.setBrush(QBrush(QColor(255, 0, 0, 200)));       // Bright red with transparency
    painter.drawEllipse(posScreen.x() - 12, posScreen.y() - 12, 24, 24);
    
    // White center dot for precise location
    painter.setPen(QPen(Qt::white, 3));
    painter.setBrush(QBrush(Qt::white));
    painter.drawEllipse(posScreen.x() - 4, posScreen.y() - 4, 8, 8);
    
    // Extended crosshairs for enhanced visibility
    painter.setPen(QPen(Qt::black, 3));
    painter.drawLine(posScreen.x() - 20, posScreen.y(), posScreen.x() + 20, posScreen.y());
    painter.drawLine(posScreen.x(), posScreen.y() - 20, posScreen.x(), posScreen.y() + 20);
    
    // Draw POI label if available
    if (m_hasPOILabel && !m_poiLabel.isEmpty()) {
        QFont font = painter.font();
        font.setPointSize(11);
        font.setBold(true);
        painter.setFont(font);
        
        QFontMetrics fm(font);
        QString displayText = m_poiLabel;
        int textWidth = fm.horizontalAdvance(displayText);
        int textHeight = fm.height();
        
        // Position label above the marker
        int labelX = posScreen.x() - textWidth / 2;
        int labelY = posScreen.y() - 35;  // 35 pixels above marker
        
        // Draw background rectangle for better readability
        QRect textRect(labelX - 5, labelY - textHeight, textWidth + 10, textHeight + 5);
        painter.setPen(QPen(QColor(0, 0, 0, 180), 2));
        painter.setBrush(QBrush(QColor(255, 255, 255, 230)));  // Semi-transparent white
        painter.drawRoundedRect(textRect, 5, 5);
        
        // Draw text label
        painter.setPen(QPen(QColor(0, 0, 0), 1));
        painter.drawText(labelX, labelY, displayText);
        
        qDebug() << "POI label drawn:" << displayText << "at" << labelX << "," << labelY;
    }
}

/**
 * @brief Draw vehicle heading direction arrow
 * @param painter QPainter instance for drawing operations
 * @param center Screen coordinates of position marker center
 * 
 * Rotated red arrow indicating vehicle direction for navigation context.
 */
void MapWidget::drawHeadingIndicator(QPainter& painter, const QPoint& center)
{
    painter.save();
    
    // Rotate coordinate system to vehicle heading
    painter.translate(center);
    painter.rotate(m_currentHeading);                       // Heading in degrees from North
    
    // Draw directional arrow pointing in heading direction
    painter.setPen(QPen(Qt::red, 3));
    painter.setBrush(QBrush(Qt::red));
    
    QPolygon arrow;
    arrow << QPoint(0, -20)                                 // Arrow tip pointing forward
          << QPoint(-6, -8)                                 // Left base
          << QPoint(6, -8);                                 // Right base
    painter.drawPolygon(arrow);
    
    painter.restore();
}

/**
 * @brief Draw directional arrows along route path
 * @param painter QPainter instance for drawing operations
 * 
 * Places small arrows at intervals along route to indicate
 * travel direction for navigation guidance.
 */
void MapWidget::drawDirectionArrows(QPainter& painter)
{
    if (m_routePoints.size() < 2) return;
    
    painter.setPen(QPen(QColor(0, 80, 200), 2));            // Darker blue for arrows
    painter.setBrush(QBrush(QColor(0, 80, 200)));
    
    // Place arrows every 3rd route segment to avoid clutter
    for (size_t i = 0; i < m_routePoints.size() - 1; i += 3) {
        QPoint fromScreen = geoToScreen(m_routePoints[i]);
        QPoint toScreen = geoToScreen(m_routePoints[i + 1]);
        
        // Calculate arrow direction angle
        double dx = toScreen.x() - fromScreen.x();
        double dy = toScreen.y() - fromScreen.y();
        double angle = atan2(dy, dx) * 180.0 / M_PI;
        
        // Position arrow at segment midpoint
        QPoint middle((fromScreen.x() + toScreen.x()) / 2, 
                     (fromScreen.y() + toScreen.y()) / 2);
        
        painter.save();
        painter.translate(middle);
        painter.rotate(angle);                              // Align with route direction
        
        // Draw small directional arrow
        QPolygon arrow;
        arrow << QPoint(8, 0)                               // Arrow tip
              << QPoint(-4, -3)                             // Left base
              << QPoint(-4, 3);                             // Right base
        painter.drawPolygon(arrow);
        
        painter.restore();
    }
}

/**
 * @brief Draw all UI overlay elements
 * @param painter QPainter instance for drawing operations
 * 
 * Conditionally renders UI information overlays based on user preferences:
 * - Distance scale bar
 * - Current coordinate display
 * - Map attribution/copyright
 */
void MapWidget::drawMapOverlays(QPainter& painter)
{
    if (m_showScale) {
        drawScaleBar(painter);
    }
    
    if (m_showCoordinates) {
        drawCoordinateInfo(painter);
    }
    
    if (m_showCopyright) {
        drawCopyright(painter);
    }
}

/**
 * @brief Draw distance scale bar in bottom-left corner
 * @param painter QPainter instance for drawing operations
 * 
 * Calculates real-world distance for 100-pixel line at current
 * zoom level and displays with appropriate units (m/km).
 */
void MapWidget::drawScaleBar(QPainter& painter)
{
    painter.setPen(QPen(Qt::black, 2));
    painter.setFont(QFont("Arial", 10));
    
    // Calculate real-world distance for 100-pixel reference line
    Point p1 = screenToGeo(QPoint(10, height() - 30));
    Point p2 = screenToGeo(QPoint(110, height() - 30));
    double distance = calculateDistance(p1, p2);
    
    // Format distance with appropriate units
    QString scaleText;
    if (distance >= 1000) {
        scaleText = QString("%1 km").arg(distance / 1000.0, 0, 'f', 1);
    } else {
        scaleText = QString("%1 m").arg(distance, 0, 'f', 0);
    }
    
    // Draw semi-transparent background
    QRect scaleRect(5, height() - 45, 120, 20);
    painter.fillRect(scaleRect, QColor(255, 255, 255, 200));
    painter.drawRect(scaleRect);
    
    // Draw scale bar with end markers
    painter.drawLine(10, height() - 30, 110, height() - 30);   // Main scale line
    painter.drawLine(10, height() - 35, 10, height() - 25);    // Left end marker
    painter.drawLine(110, height() - 35, 110, height() - 25);  // Right end marker
    
    // Display calculated distance
    painter.drawText(15, height() - 35, scaleText);
}

/**
 * @brief Draw coordinate and map information in top-right corner
 * @param painter QPainter instance for drawing operations
 * 
 * Shows current map center coordinates, zoom level, and active map style
 * for user reference and debugging purposes.
 */
void MapWidget::drawCoordinateInfo(QPainter& painter)
{
    painter.setPen(QPen(Qt::black));
    painter.setFont(QFont("Arial", 10));
    
    // Format comprehensive map state information
    QString coordText = QString("Center: %1, %2\nZoom: %3\nStyle: %4")
                       .arg(m_centerLat, 0, 'f', 6)         // 6 decimal places for precision
                       .arg(m_centerLon, 0, 'f', 6)
                       .arg(m_zoomLevel)
                       .arg(mapStyle());
    
    // Calculate text bounding box for background
    QRect textRect = painter.fontMetrics().boundingRect(coordText);
    textRect.moveTopRight(QPoint(width() - 10, 10));
    
    // Draw semi-transparent background for readability
    painter.fillRect(textRect.adjusted(-5, -2, 5, 2), QColor(255, 255, 255, 200));
    painter.drawRect(textRect.adjusted(-5, -2, 5, 2));
    
    // Display coordinate information
    painter.drawText(textRect, coordText);
}

/**
 * @brief Draw copyright/attribution notice in bottom-right corner
 * @param painter QPainter instance for drawing operations
 * 
 * Displays map attribution information as required by mapping services
 * and to identify the navigation system.
 */
void MapWidget::drawCopyright(QPainter& painter)
{
    painter.setPen(QPen(Qt::gray));
    painter.setFont(QFont("Arial", 8));
    
    QString copyright = "© Navigation System - Demo Maps";
    QRect copyrightRect = painter.fontMetrics().boundingRect(copyright);
    copyrightRect.moveBottomRight(QPoint(width() - 10, height() - 10));
    
    // Semi-transparent background for text visibility
    painter.fillRect(copyrightRect.adjusted(-5, -2, 5, 2), QColor(255, 255, 255, 150));
    painter.drawText(copyrightRect, copyright);
}

/**
 * @brief Handle mouse press events for map interaction
 * @param event Mouse event containing button and position information
 * 
 * Initiates map panning on left-click and emits geographic coordinates
 * of click location for destination selection or point marking.
 */
void MapWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;                                  // Enable drag-to-pan mode
        m_lastPanPoint = event->pos();                      // Store drag start position
        
        // Convert screen coordinates to geographic and emit signal
        Point geoPos = screenToGeo(event->pos());
        emit mapClicked(geoPos);                            // Notify listeners of click location
    }
    
    QWidget::mousePressEvent(event);
}

/**
 * @brief Handle mouse double-click events for quick zoom
 * @param event Mouse event containing position information
 * 
 * Centers map on double-click location and increases zoom level
 * for detailed view of the selected area.
 */
void MapWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        Point geoPos = screenToGeo(event->pos());
        emit mapDoubleClicked(geoPos);                      // Notify of double-click location
        
        // Zoom in for detailed view
        setZoomLevel(m_zoomLevel + 1);
    }
    
    QWidget::mouseDoubleClickEvent(event);
}

/**
 * @brief Handle mouse movement for panning and coordinate tracking
 * @param event Mouse event with current position and button state
 * 
 * Enables map panning when left button is held and continuously
 * emits current mouse geographic coordinates for UI updates.
 */
void MapWidget::mouseMoveEvent(QMouseEvent *event)
{
    // Always emit current mouse position for coordinate display
    Point mousePos = screenToGeo(event->pos());
    emit mousePositionChanged(mousePos);                    // Update coordinate displays
    
    // Handle map panning if left button is pressed
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        QPoint delta = event->pos() - m_lastPanPoint;
        
        // Convert pixel movement to geographic coordinate changes
        // Movement scale inversely related to zoom level
        double latDelta = -delta.y() * 0.0001 * (21 - m_zoomLevel);
        double lonDelta = -delta.x() * 0.0001 * (21 - m_zoomLevel);
        
        // Update map center position
        setCenterLatitude(m_centerLat + latDelta);
        setCenterLongitude(m_centerLon + lonDelta);
        
        m_lastPanPoint = event->pos();                      // Update for next movement
    }
    
    QWidget::mouseMoveEvent(event);
}

/**
 * @brief Handle mouse wheel events for zoom control
 * @param event Wheel event containing scroll direction and amount
 * 
 * Zoom in on forward scroll, zoom out on backward scroll.
 * Provides intuitive zoom control similar to web mapping services.
 */
void MapWidget::wheelEvent(QWheelEvent *event)
{
    int delta = event->angleDelta().y();
    if (delta > 0) {
        setZoomLevel(m_zoomLevel + 1);                      // Zoom in
    } else {
        setZoomLevel(m_zoomLevel - 1);                      // Zoom out
    }
    
    QWidget::wheelEvent(event);
}

/**
 * @brief Handle keyboard events for map navigation
 * @param event Key event containing pressed key information
 * 
 * Arrow keys: Pan map in corresponding direction
 * +/= keys: Zoom in for more detail
 * - key: Zoom out for wider view
 */
void MapWidget::keyPressEvent(QKeyEvent *event)
{
    // Calculate movement step based on zoom level (larger steps at lower zoom)
    const double moveStep = 0.001 * (21 - m_zoomLevel);
    
    switch (event->key()) {
    case Qt::Key_Up:
        setCenterLatitude(m_centerLat + moveStep);          // Pan north
        break;
    case Qt::Key_Down:
        setCenterLatitude(m_centerLat - moveStep);          // Pan south
        break;
    case Qt::Key_Left:
        setCenterLongitude(m_centerLon - moveStep);         // Pan west
        break;
    case Qt::Key_Right:
        setCenterLongitude(m_centerLon + moveStep);         // Pan east
        break;
    case Qt::Key_Plus:
    case Qt::Key_Equal:
        setZoomLevel(m_zoomLevel + 1);                      // Zoom in
        break;
    case Qt::Key_Minus:
        setZoomLevel(m_zoomLevel - 1);                      // Zoom out
        break;
    default:
        QWidget::keyPressEvent(event);                      // Pass unhandled keys to parent
        break;
    }
}

/**
 * @brief Handle widget resize events
 * @param event Resize event with old and new dimensions
 * 
 * Triggers map repaint to adjust coordinate calculations
 * and UI overlay positions for new widget size.
 */
void MapWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    update();                                               // Redraw with new dimensions
}

/**
 * @brief Animation completion callback
 * 
 * Triggers final map repaint when smooth animations complete
 * to ensure consistent visual state.
 */
void MapWidget::onAnimationFinished()
{
    update();                                               // Final repaint after animation
}

/**
 * @brief Convert geographic coordinates to screen pixel coordinates
 * @param geoPoint Geographic position (latitude, longitude)
 * @return QPoint Screen coordinates (x, y pixels)
 * 
 * Uses simplified Mercator-like projection with zoom-based scaling.
 * Suitable for local area navigation (Hanoi city scale).
 */
QPoint MapWidget::geoToScreen(const Point& geoPoint) const
{
    // Exponential zoom scaling factor
    double scale = pow(2, m_zoomLevel);
    
    // Calculate offset from map center
    double deltaLat = geoPoint.latitude - m_centerLat;
    double deltaLon = geoPoint.longitude - m_centerLon;
    
    // Convert to screen coordinates with zoom scaling
    int x = width() / 2 + static_cast<int>(deltaLon * scale * width() / 360.0);
    int y = height() / 2 - static_cast<int>(deltaLat * scale * height() / 180.0);
    
    return QPoint(x, y);
}

/**
 * @brief Convert screen pixel coordinates to geographic coordinates
 * @param screenPoint Screen coordinates (x, y pixels)
 * @return Point Geographic position (latitude, longitude)
 * 
 * Reverse transformation of geoToScreen() for click-to-coordinate conversion.
 */
Point MapWidget::screenToGeo(const QPoint& screenPoint) const
{
    // Zoom scaling factor (inverse of geoToScreen)
    double scale = pow(2, m_zoomLevel);
    
    // Calculate offset from screen center
    int deltaX = screenPoint.x() - width() / 2;
    int deltaY = height() / 2 - screenPoint.y();
    
    // Convert to geographic coordinate offsets
    double deltaLon = static_cast<double>(deltaX) * 360.0 / (scale * width());
    double deltaLat = static_cast<double>(deltaY) * 180.0 / (scale * height());
    
    // Add offsets to map center coordinates
    double lat = m_centerLat + deltaLat;
    double lon = m_centerLon + deltaLon;
    
    return Point(lat, lon);
}

/**
 * @brief Calculate great-circle distance between two geographic points
 * @param p1 First geographic point
 * @param p2 Second geographic point
 * @return Distance in meters using Haversine formula
 * 
 * Provides accurate distance calculation for navigation and scale bar display.
 */
double MapWidget::calculateDistance(const Point& p1, const Point& p2) const
{
    const double R = 6371000;                               // Earth radius in meters
    
    // Convert degrees to radians
    double lat1Rad = p1.latitude * M_PI / 180.0;
    double lat2Rad = p2.latitude * M_PI / 180.0;
    double deltaLatRad = (p2.latitude - p1.latitude) * M_PI / 180.0;
    double deltaLonRad = (p2.longitude - p1.longitude) * M_PI / 180.0;
    
    // Haversine formula for great-circle distance
    double a = sin(deltaLatRad / 2) * sin(deltaLatRad / 2) +
               cos(lat1Rad) * cos(lat2Rad) *
               sin(deltaLonRad / 2) * sin(deltaLonRad / 2);
    double c = 2 * atan2(sqrt(a), sqrt(1 - a));
    
    return R * c;                                           // Distance in meters
}

/**
 * @brief Calculate bearing angle between two geographic points
 * @param from Starting geographic point
 * @param to Target geographic point
 * @return Bearing in degrees (0° = North, 90° = East)
 * 
 * Used for heading calculations and directional arrow positioning.
 */
double MapWidget::calculateBearing(const Point& from, const Point& to) const
{
    // Convert to radians
    double lat1 = from.latitude * M_PI / 180.0;
    double lat2 = to.latitude * M_PI / 180.0;
    double deltaLon = (to.longitude - from.longitude) * M_PI / 180.0;
    
    // Calculate bearing using spherical trigonometry
    double y = sin(deltaLon) * cos(lat2);
    double x = cos(lat1) * sin(lat2) - sin(lat1) * cos(lat2) * cos(deltaLon);
    
    double bearing = atan2(y, x) * 180.0 / M_PI;
    return fmod(bearing + 360.0, 360.0);                    // Normalize to 0-360°
}

/**
 * @brief Load user preferences from persistent storage
 * 
 * Restores map position, zoom level, style, and UI overlay preferences
 * from previous application session using QSettings.
 */
void MapWidget::loadSettings()
{
    QSettings settings;
    settings.beginGroup("MapWidget");
    
    // Restore map view state
    m_centerLat = settings.value("centerLatitude", getDefaultLat()).toDouble();
    m_centerLon = settings.value("centerLongitude", getDefaultLon()).toDouble();
    m_zoomLevel = settings.value("zoomLevel", DEFAULT_ZOOM).toInt();
    
    // Restore map style preference
    QString styleString = settings.value("mapStyle", "Satellite").toString();
    setMapStyle(styleString);
    
    // Restore UI overlay preferences
    m_showGrid = settings.value("showGrid", true).toBool();
    m_showCoordinates = settings.value("showCoordinates", true).toBool();
    m_showScale = settings.value("showScale", true).toBool();
    m_showCopyright = settings.value("showCopyright", true).toBool();
    
    settings.endGroup();
    
    qDebug() << "Map settings loaded";
}

/**
 * @brief Save user preferences to persistent storage
 * 
 * Stores current map state and user preferences for restoration
 * in next application session using QSettings.
 */
void MapWidget::saveSettings()
{
    QSettings settings;
    settings.beginGroup("MapWidget");
    
    // Save current map view state
    settings.setValue("centerLatitude", m_centerLat);
    settings.setValue("centerLongitude", m_centerLon);
    settings.setValue("zoomLevel", m_zoomLevel);
    settings.setValue("mapStyle", mapStyle());
    
    // Save UI overlay preferences
    settings.setValue("showGrid", m_showGrid);
    settings.setValue("showCoordinates", m_showCoordinates);
    settings.setValue("showScale", m_showScale);
    settings.setValue("showCopyright", m_showCopyright);
    
    settings.endGroup();
    
    qDebug() << "Map settings saved";
}

} // namespace nav