#include "../include/map_widget.h"
#include <QPainter>
#include <QApplication>
#include <QDebug>
#include <QStandardPaths>
#include <QDir>
#include <cmath>

namespace nav {

MapWidget::MapWidget(QWidget *parent)
    : QWidget(parent)
    , m_centerLat(getDefaultLat())
    , m_centerLon(getDefaultLon())
    , m_zoomLevel(DEFAULT_ZOOM)
    , m_currentMapStyle(SATELLITE)
    , m_mapImagesLoaded(false)
    , m_currentHeading(0.0)
    , m_hasCurrentPosition(false)
    , m_hasStartPoint(false)
    , m_hasEndPoint(false)
    , m_dragging(false)
    , m_centerAnimation(nullptr)
    , m_zoomAnimation(nullptr)
    , m_showGrid(true)
    , m_showCoordinates(true)
    , m_showScale(true)
    , m_showCopyright(true)
    , m_mapTopLeft(getMapBoundsNorth(), getMapBoundsWest())
    , m_mapBottomRight(getMapBoundsSouth(), getMapBoundsEast())
{
    setMinimumSize(400, 300);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    
    // Load map images
    loadMapImages();
    
    // Setup animations
    m_centerAnimation = new QPropertyAnimation(this, "centerLatitude");
    m_centerAnimation->setDuration(1000);
    connect(m_centerAnimation, &QPropertyAnimation::finished, this, &MapWidget::onAnimationFinished);
    
    m_zoomAnimation = new QPropertyAnimation(this, "zoomLevel");
    m_zoomAnimation->setDuration(500);
    
    // Load settings
    loadSettings();
    
    qDebug() << "MapWidget initialized with real image support";
}

MapWidget::~MapWidget()
{
    saveSettings();
}

void MapWidget::loadMapImages()
{
    qDebug() << "Loading map images...";
    
    // Try to load from resources first, then from file system
    QString satellitePath = ":/maps/hanoi_satellite.jpg";
    QString streetPath = ":/maps/hanoi_street.jpg";
    QString hybridPath = ":/maps/vietnam_overview.jpg";
    
    if (!m_satelliteMap.load(satellitePath)) {
        qDebug() << "Failed to load satellite map, creating placeholder";
        m_satelliteMap = QPixmap(800, 600);
        m_satelliteMap.fill(QColor(120, 150, 80)); // Green background
        
        QPainter painter(&m_satelliteMap);
        painter.setPen(QPen(Qt::darkGreen, 2));
        painter.setFont(QFont("Arial", 16));
        painter.drawText(m_satelliteMap.rect(), Qt::AlignCenter, 
                        "Satellite View\n(Hanoi Area)\nDummy Image");
        
        // Draw some fake roads/features
        painter.setPen(QPen(Qt::gray, 3));
        painter.drawLine(100, 200, 700, 400);
        painter.drawLine(200, 100, 600, 500);
        painter.setPen(QPen(Qt::blue, 2));
        painter.drawEllipse(300, 200, 200, 100); // Fake lake
    }
    
    if (!m_streetMap.load(streetPath)) {
        qDebug() << "Failed to load street map, creating placeholder";
        m_streetMap = QPixmap(800, 600);
        m_streetMap.fill(QColor(245, 245, 220)); // Beige background
        
        QPainter painter(&m_streetMap);
        painter.setPen(QPen(Qt::black, 1));
        painter.setFont(QFont("Arial", 16));
        painter.drawText(m_streetMap.rect(), Qt::AlignCenter,
                        "Street Map\n(Hanoi Area)\nDummy Image");
        
        // Draw grid pattern for streets
        painter.setPen(QPen(Qt::darkGray, 2));
        for (int x = 50; x < 800; x += 100) {
            painter.drawLine(x, 0, x, 600);
        }
        for (int y = 50; y < 600; y += 100) {
            painter.drawLine(0, y, 800, y);
        }
        
        // Draw main roads
        painter.setPen(QPen(Qt::red, 4));
        painter.drawLine(0, 300, 800, 300); // Horizontal main road
        painter.drawLine(400, 0, 400, 600); // Vertical main road
    }
    
    if (!m_hybridMap.load(hybridPath)) {
        qDebug() << "Failed to load hybrid map, creating placeholder";
        m_hybridMap = QPixmap(800, 600);
        m_hybridMap.fill(QColor(100, 120, 140)); // Blue-gray background
        
        QPainter painter(&m_hybridMap);
        painter.setPen(QPen(Qt::white, 2));
        painter.setFont(QFont("Arial", 16));
        painter.drawText(m_hybridMap.rect(), Qt::AlignCenter,
                        "Hybrid View\n(Vietnam Overview)\nDummy Image");
        
        // Draw coastline
        painter.setPen(QPen(Qt::yellow, 3));
        QPolygon coastline;
        coastline << QPoint(100, 100) << QPoint(200, 80) << QPoint(300, 120)
                  << QPoint(500, 150) << QPoint(700, 200) << QPoint(750, 400)
                  << QPoint(700, 500) << QPoint(500, 480) << QPoint(300, 450)
                  << QPoint(100, 400);
        painter.drawPolygon(coastline);
    }
    
    m_mapImagesLoaded = true;
    updateMapCache();
    
    qDebug() << "Map images loaded successfully";
}

QPixmap MapWidget::getCurrentMapImage() const
{
    return getMapImageForStyle(m_currentMapStyle);
}

QPixmap MapWidget::getMapImageForStyle(MapStyle style) const
{
    switch (style) {
        case SATELLITE:
            return m_satelliteMap;
        case STREET_MAP:
            return m_streetMap;
        case HYBRID:
        default:
            return m_hybridMap;
    }
}

void MapWidget::updateMapCache()
{
    m_currentMapCache = getCurrentMapImage();
    update();
}

QString MapWidget::mapStyle() const
{
    switch (m_currentMapStyle) {
        case SATELLITE: return "Satellite";
        case STREET_MAP: return "Street Map";
        case HYBRID: return "Hybrid";
        default: return "Unknown";
    }
}

void MapWidget::setCenterLatitude(double lat)
{
    if (lat != m_centerLat) {
        m_centerLat = lat;
        update();
        emit centerChanged(m_centerLat, m_centerLon);
    }
}

void MapWidget::setCenterLongitude(double lon)
{
    if (lon != m_centerLon) {
        m_centerLon = lon;
        update();
        emit centerChanged(m_centerLat, m_centerLon);
    }
}

void MapWidget::setZoomLevel(int zoom)
{
    int newZoom = qBound(MIN_ZOOM, zoom, MAX_ZOOM);
    if (newZoom != m_zoomLevel) {
        m_zoomLevel = newZoom;
        update();
        emit zoomChanged(m_zoomLevel);
    }
}

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

void MapWidget::setMapStyle(MapStyle style)
{
    if (style != m_currentMapStyle) {
        m_currentMapStyle = style;
        updateMapCache();
        qDebug() << "Map style changed to:" << mapStyle();
    }
}

void MapWidget::setCurrentPosition(const Point& position, double heading)
{
    m_currentPosition = position;
    m_currentHeading = heading;
    m_hasCurrentPosition = true;
    update();
}

void MapWidget::setStartPoint(const Point& point)
{
    m_startPoint = point;
    m_hasStartPoint = true;
    update();
}

void MapWidget::setEndPoint(const Point& point)
{
    m_endPoint = point;
    m_hasEndPoint = true;
    update();
}

void MapWidget::setRoute(const std::vector<Point>& route)
{
    m_routePoints = route;
    update();
}

void MapWidget::clearRoute()
{
    m_routePoints.clear();
    update();
}

void MapWidget::clearWaypoints()
{
    m_hasStartPoint = false;
    m_hasEndPoint = false;
    update();
}

void MapWidget::clearStartPoint()
{
    m_hasStartPoint = false;
    update();
}

void MapWidget::clearEndPoint()
{
    m_hasEndPoint = false;
    update();
}

void MapWidget::centerMap(double lat, double lon)
{
    m_centerLat = lat;
    m_centerLon = lon;
    update();
}

void MapWidget::animateToPosition(const Point& position)
{
    if (m_centerAnimation->state() == QPropertyAnimation::Running) {
        m_centerAnimation->stop();
    }
    
    m_centerAnimation->setStartValue(m_centerLat);
    m_centerAnimation->setEndValue(position.latitude);
    m_centerAnimation->start();
    
    // Also animate longitude (simplified)
    m_centerLon = position.longitude;
}

void MapWidget::animateZoomTo(int targetZoom)
{
    if (m_zoomAnimation->state() == QPropertyAnimation::Running) {
        m_zoomAnimation->stop();
    }
    
    m_zoomAnimation->setStartValue(m_zoomLevel);
    m_zoomAnimation->setEndValue(targetZoom);
    m_zoomAnimation->start();
}

void MapWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    
    // Clear background
    painter.fillRect(rect(), QColor(200, 220, 255)); // Light blue
    
    // Draw map
    drawMap(painter);
    
    // Draw overlays
    if (m_showGrid) {
        drawGrid(painter);
    }
    
    // Draw navigation elements
    if (!m_routePoints.empty()) {
        drawRoute(painter);
    }
    
    drawWaypoints(painter);
    
    if (m_hasCurrentPosition) {
        drawCurrentPosition(painter);
    }
    
    // Draw UI overlays
    drawMapOverlays(painter);
}

void MapWidget::drawMap(QPainter& painter)
{
    if (!m_mapImagesLoaded || m_currentMapCache.isNull()) {
        return;
    }
    
    // Calculate map position and scale based on zoom and center
    double scale = pow(2, m_zoomLevel - 10); // Scale factor
    
    // Map image covers the bounds defined by m_mapTopLeft and m_mapBottomRight
    QPoint mapTopLeftScreen = geoToScreen(m_mapTopLeft);
    QPoint mapBottomRightScreen = geoToScreen(m_mapBottomRight);
    
    QRect mapRect(mapTopLeftScreen, mapBottomRightScreen);
    
    // Draw the map image
    painter.drawPixmap(mapRect, m_currentMapCache);
    
    // Draw map border
    painter.setPen(QPen(Qt::darkGray, 2));
    painter.drawRect(mapRect);
}

void MapWidget::drawGrid(QPainter& painter)
{
    painter.setPen(QPen(QColor(255, 255, 255, 100), 1));
    
    // Calculate grid spacing based on zoom
    double gridSpacing = 0.01 / pow(2, m_zoomLevel - 10);
    
    // Draw latitude lines
    double startLat = floor(m_centerLat / gridSpacing) * gridSpacing;
    for (int i = -10; i <= 10; ++i) {
        double lat = startLat + i * gridSpacing;
        QPoint p1 = geoToScreen(Point(lat, m_centerLon - 0.1));
        QPoint p2 = geoToScreen(Point(lat, m_centerLon + 0.1));
        painter.drawLine(p1, p2);
    }
    
    // Draw longitude lines
    double startLon = floor(m_centerLon / gridSpacing) * gridSpacing;
    for (int i = -10; i <= 10; ++i) {
        double lon = startLon + i * gridSpacing;
        QPoint p1 = geoToScreen(Point(m_centerLat - 0.1, lon));
        QPoint p2 = geoToScreen(Point(m_centerLat + 0.1, lon));
        painter.drawLine(p1, p2);
    }
}

void MapWidget::drawRoute(QPainter& painter)
{
    if (m_routePoints.size() < 2) {
        return;
    }
    
    painter.setPen(QPen(QColor(0, 100, 255), 4)); // Blue route line
    painter.setBrush(QBrush(QColor(0, 100, 255, 100)));
    
    QPoint prevPoint = geoToScreen(m_routePoints[0]);
    
    for (size_t i = 1; i < m_routePoints.size(); ++i) {
        QPoint currentPoint = geoToScreen(m_routePoints[i]);
        painter.drawLine(prevPoint, currentPoint);
        prevPoint = currentPoint;
    }
    
    // Draw direction arrows
    drawDirectionArrows(painter);
}

void MapWidget::drawWaypoints(QPainter& painter)
{
    // Draw start point
    if (m_hasStartPoint) {
        QPoint startScreen = geoToScreen(m_startPoint);
        painter.setPen(QPen(Qt::darkGreen, 3));
        painter.setBrush(QBrush(Qt::green));
        painter.drawEllipse(startScreen, 10, 10);
        
        painter.setPen(QPen(Qt::white, 2));
        painter.setFont(QFont("Arial", 12, QFont::Bold));
        painter.drawText(startScreen + QPoint(-5, 5), "S");
    }
    
    // Draw end point
    if (m_hasEndPoint) {
        QPoint endScreen = geoToScreen(m_endPoint);
        painter.setPen(QPen(Qt::darkRed, 3));
        painter.setBrush(QBrush(Qt::red));
        painter.drawEllipse(endScreen, 10, 10);
        
        painter.setPen(QPen(Qt::white, 2));
        painter.setFont(QFont("Arial", 12, QFont::Bold));
        painter.drawText(endScreen + QPoint(-5, 5), "E");
    }
}

void MapWidget::drawCurrentPosition(QPainter& painter)
{
    QPoint posScreen = geoToScreen(m_currentPosition);
    
    // Draw position circle
    painter.setPen(QPen(Qt::blue, 3));
    painter.setBrush(QBrush(QColor(0, 100, 255, 180)));
    painter.drawEllipse(posScreen, 12, 12);
    
    // Draw center dot
    painter.setPen(QPen(Qt::white, 2));
    painter.setBrush(QBrush(Qt::white));
    painter.drawEllipse(posScreen, 4, 4);
    
    // Draw heading indicator
    drawHeadingIndicator(painter, posScreen);
}

void MapWidget::drawHeadingIndicator(QPainter& painter, const QPoint& center)
{
    painter.save();
    
    painter.translate(center);
    painter.rotate(m_currentHeading);
    
    // Draw arrow
    painter.setPen(QPen(Qt::red, 3));
    painter.setBrush(QBrush(Qt::red));
    
    QPolygon arrow;
    arrow << QPoint(0, -20) << QPoint(-6, -8) << QPoint(6, -8);
    painter.drawPolygon(arrow);
    
    painter.restore();
}

void MapWidget::drawDirectionArrows(QPainter& painter)
{
    if (m_routePoints.size() < 2) return;
    
    painter.setPen(QPen(QColor(0, 80, 200), 2));
    painter.setBrush(QBrush(QColor(0, 80, 200)));
    
    for (size_t i = 0; i < m_routePoints.size() - 1; i += 3) {
        QPoint fromScreen = geoToScreen(m_routePoints[i]);
        QPoint toScreen = geoToScreen(m_routePoints[i + 1]);
        
        // Calculate arrow direction
        double dx = toScreen.x() - fromScreen.x();
        double dy = toScreen.y() - fromScreen.y();
        double angle = atan2(dy, dx) * 180.0 / M_PI;
        
        // Find middle point
        QPoint middle((fromScreen.x() + toScreen.x()) / 2, 
                     (fromScreen.y() + toScreen.y()) / 2);
        
        painter.save();
        painter.translate(middle);
        painter.rotate(angle);
        
        // Draw small arrow
        QPolygon arrow;
        arrow << QPoint(8, 0) << QPoint(-4, -3) << QPoint(-4, 3);
        painter.drawPolygon(arrow);
        
        painter.restore();
    }
}

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

void MapWidget::drawScaleBar(QPainter& painter)
{
    // Draw scale bar in bottom left corner
    painter.setPen(QPen(Qt::black, 2));
    painter.setFont(QFont("Arial", 10));
    
    // Calculate scale
    Point p1 = screenToGeo(QPoint(10, height() - 30));
    Point p2 = screenToGeo(QPoint(110, height() - 30));
    double distance = calculateDistance(p1, p2);
    
    QString scaleText;
    if (distance >= 1000) {
        scaleText = QString("%1 km").arg(distance / 1000.0, 0, 'f', 1);
    } else {
        scaleText = QString("%1 m").arg(distance, 0, 'f', 0);
    }
    
    // Draw background
    QRect scaleRect(5, height() - 45, 120, 20);
    painter.fillRect(scaleRect, QColor(255, 255, 255, 200));
    painter.drawRect(scaleRect);
    
    // Draw scale bar
    painter.drawLine(10, height() - 30, 110, height() - 30);
    painter.drawLine(10, height() - 35, 10, height() - 25);
    painter.drawLine(110, height() - 35, 110, height() - 25);
    
    // Draw scale text
    painter.drawText(15, height() - 35, scaleText);
}

void MapWidget::drawCoordinateInfo(QPainter& painter)
{
    painter.setPen(QPen(Qt::black));
    painter.setFont(QFont("Arial", 10));
    
    QString coordText = QString("Center: %1, %2\nZoom: %3\nStyle: %4")
                       .arg(m_centerLat, 0, 'f', 6)
                       .arg(m_centerLon, 0, 'f', 6)
                       .arg(m_zoomLevel)
                       .arg(mapStyle());
    
    QRect textRect = painter.fontMetrics().boundingRect(coordText);
    textRect.moveTopRight(QPoint(width() - 10, 10));
    
    // Draw background
    painter.fillRect(textRect.adjusted(-5, -2, 5, 2), QColor(255, 255, 255, 200));
    painter.drawRect(textRect.adjusted(-5, -2, 5, 2));
    
    // Draw text
    painter.drawText(textRect, coordText);
}

void MapWidget::drawCopyright(QPainter& painter)
{
    painter.setPen(QPen(Qt::gray));
    painter.setFont(QFont("Arial", 8));
    
    QString copyright = "© Navigation System - Demo Maps";
    QRect copyrightRect = painter.fontMetrics().boundingRect(copyright);
    copyrightRect.moveBottomRight(QPoint(width() - 10, height() - 10));
    
    painter.fillRect(copyrightRect.adjusted(-5, -2, 5, 2), QColor(255, 255, 255, 150));
    painter.drawText(copyrightRect, copyright);
}

void MapWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        m_lastPanPoint = event->pos();
        
        Point geoPos = screenToGeo(event->pos());
        emit mapClicked(geoPos);
    }
    
    QWidget::mousePressEvent(event);
}

void MapWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        Point geoPos = screenToGeo(event->pos());
        emit mapDoubleClicked(geoPos);
        
        // Zoom in on double click
        setZoomLevel(m_zoomLevel + 1);
    }
    
    QWidget::mouseDoubleClickEvent(event);
}

void MapWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        QPoint delta = event->pos() - m_lastPanPoint;
        
        // Convert pixel movement to coordinate movement
        double latDelta = -delta.y() * 0.0001 * (21 - m_zoomLevel);
        double lonDelta = -delta.x() * 0.0001 * (21 - m_zoomLevel);
        
        setCenterLatitude(m_centerLat + latDelta);
        setCenterLongitude(m_centerLon + lonDelta);
        
        m_lastPanPoint = event->pos();
    }
    
    QWidget::mouseMoveEvent(event);
}

void MapWidget::wheelEvent(QWheelEvent *event)
{
    int delta = event->angleDelta().y();
    if (delta > 0) {
        setZoomLevel(m_zoomLevel + 1);
    } else {
        setZoomLevel(m_zoomLevel - 1);
    }
    
    QWidget::wheelEvent(event);
}

void MapWidget::keyPressEvent(QKeyEvent *event)
{
    const double moveStep = 0.001 * (21 - m_zoomLevel);
    
    switch (event->key()) {
    case Qt::Key_Up:
        setCenterLatitude(m_centerLat + moveStep);
        break;
    case Qt::Key_Down:
        setCenterLatitude(m_centerLat - moveStep);
        break;
    case Qt::Key_Left:
        setCenterLongitude(m_centerLon - moveStep);
        break;
    case Qt::Key_Right:
        setCenterLongitude(m_centerLon + moveStep);
        break;
    case Qt::Key_Plus:
    case Qt::Key_Equal:
        setZoomLevel(m_zoomLevel + 1);
        break;
    case Qt::Key_Minus:
        setZoomLevel(m_zoomLevel - 1);
        break;
    default:
        QWidget::keyPressEvent(event);
        break;
    }
}

void MapWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    update();
}

void MapWidget::onAnimationFinished()
{
    update();
}

QPoint MapWidget::geoToScreen(const Point& geoPoint) const
{
    // Simple Mercator-like projection
    double scale = pow(2, m_zoomLevel);
    
    // Calculate offset from center
    double deltaLat = geoPoint.latitude - m_centerLat;
    double deltaLon = geoPoint.longitude - m_centerLon;
    
    // Convert to screen coordinates
    int x = width() / 2 + static_cast<int>(deltaLon * scale * width() / 360.0);
    int y = height() / 2 - static_cast<int>(deltaLat * scale * height() / 180.0);
    
    return QPoint(x, y);
}

Point MapWidget::screenToGeo(const QPoint& screenPoint) const
{
    // Reverse of geoToScreen
    double scale = pow(2, m_zoomLevel);
    
    // Calculate offset from center in screen coordinates
    int deltaX = screenPoint.x() - width() / 2;
    int deltaY = height() / 2 - screenPoint.y();
    
    // Convert to geographic coordinates
    double deltaLon = static_cast<double>(deltaX) * 360.0 / (scale * width());
    double deltaLat = static_cast<double>(deltaY) * 180.0 / (scale * height());
    
    double lat = m_centerLat + deltaLat;
    double lon = m_centerLon + deltaLon;
    
    return Point(lat, lon);
}

double MapWidget::calculateDistance(const Point& p1, const Point& p2) const
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

double MapWidget::calculateBearing(const Point& from, const Point& to) const
{
    double lat1 = from.latitude * M_PI / 180.0;
    double lat2 = to.latitude * M_PI / 180.0;
    double deltaLon = (to.longitude - from.longitude) * M_PI / 180.0;
    
    double y = sin(deltaLon) * cos(lat2);
    double x = cos(lat1) * sin(lat2) - sin(lat1) * cos(lat2) * cos(deltaLon);
    
    double bearing = atan2(y, x) * 180.0 / M_PI;
    return fmod(bearing + 360.0, 360.0);
}

void MapWidget::loadSettings()
{
    QSettings settings;
    settings.beginGroup("MapWidget");
    
    m_centerLat = settings.value("centerLatitude", getDefaultLat()).toDouble();
    m_centerLon = settings.value("centerLongitude", getDefaultLon()).toDouble();
    m_zoomLevel = settings.value("zoomLevel", DEFAULT_ZOOM).toInt();
    
    QString styleString = settings.value("mapStyle", "Satellite").toString();
    setMapStyle(styleString);
    
    m_showGrid = settings.value("showGrid", true).toBool();
    m_showCoordinates = settings.value("showCoordinates", true).toBool();
    m_showScale = settings.value("showScale", true).toBool();
    m_showCopyright = settings.value("showCopyright", true).toBool();
    
    settings.endGroup();
    
    qDebug() << "Map settings loaded";
}

void MapWidget::saveSettings()
{
    QSettings settings;
    settings.beginGroup("MapWidget");
    
    settings.setValue("centerLatitude", m_centerLat);
    settings.setValue("centerLongitude", m_centerLon);
    settings.setValue("zoomLevel", m_zoomLevel);
    settings.setValue("mapStyle", mapStyle());
    settings.setValue("showGrid", m_showGrid);
    settings.setValue("showCoordinates", m_showCoordinates);
    settings.setValue("showScale", m_showScale);
    settings.setValue("showCopyright", m_showCopyright);
    
    settings.endGroup();
    
    qDebug() << "Map settings saved";
}

} // namespace nav
