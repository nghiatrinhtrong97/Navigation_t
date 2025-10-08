#include "../include/map_renderer.h"
#include <QPainter>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QDebug>
#include <cmath>

namespace nav {

MapRenderer::MapRenderer(QWidget *parent)
    : QWidget(parent)
    , m_centerLat(DEFAULT_LAT)
    , m_centerLon(DEFAULT_LON)
    , m_zoomLevel(DEFAULT_ZOOM)
    , m_hasCurrentPosition(false)
    , m_hasStartPoint(false)
    , m_hasEndPoint(false)
    , m_currentHeading(0.0)
{
    setMinimumSize(600, 400);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMouseTracking(true);
    
    // Enable focus for keyboard events
    setFocusPolicy(Qt::StrongFocus);
    
    qDebug() << "Map renderer initialized";
}

MapRenderer::~MapRenderer()
{
}

void MapRenderer::setCurrentPosition(const Point& position, double heading)
{
    m_currentPosition = position;
    m_currentHeading = heading;
    m_hasCurrentPosition = true;
    
    // Center map on current position
    m_centerLat = position.latitude;
    m_centerLon = position.longitude;
    
    update();
}

void MapRenderer::setStartPoint(const Point& point)
{
    m_startPoint = point;
    m_hasStartPoint = true;
    update();
}

void MapRenderer::setEndPoint(const Point& point)
{
    m_endPoint = point;
    m_hasEndPoint = true;
    update();
}

void MapRenderer::setRoute(const std::vector<Point>& route)
{
    m_routePoints = route;
    update();
}

void MapRenderer::clearRoute()
{
    m_routePoints.clear();
    update();
}

void MapRenderer::clearWaypoints()
{
    m_hasStartPoint = false;
    m_hasEndPoint = false;
    update();
}

void MapRenderer::setZoomLevel(int zoom)
{
    m_zoomLevel = qBound(MIN_ZOOM, zoom, MAX_ZOOM);
    update();
}

void MapRenderer::centerOnPosition(const Point& position)
{
    m_centerLat = position.latitude;
    m_centerLon = position.longitude;
    update();
}

void MapRenderer::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Clear background
    painter.fillRect(rect(), QColor(240, 248, 255)); // Light blue background
    
    // Draw grid
    drawGrid(painter);
    
    // Draw route
    if (!m_routePoints.empty()) {
        drawRoute(painter);
    }
    
    // Draw waypoints
    if (m_hasStartPoint) {
        drawWaypoint(painter, m_startPoint, Qt::green, "S");
    }
    
    if (m_hasEndPoint) {
        drawWaypoint(painter, m_endPoint, Qt::red, "E");
    }
    
    // Draw current position
    if (m_hasCurrentPosition) {
        drawCurrentPosition(painter);
    }
    
    // Draw scale and info
    drawScaleBar(painter);
    drawCoordinateInfo(painter);
}

void MapRenderer::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        QPoint screenPos = event->pos();
        Point geoPos = screenToGeo(screenPos);
        emit mapClicked(geoPos);
    }
    
    QWidget::mousePressEvent(event);
}

void MapRenderer::wheelEvent(QWheelEvent *event)
{
    // Zoom in/out with mouse wheel
    int delta = event->angleDelta().y();
    if (delta > 0) {
        setZoomLevel(m_zoomLevel + 1);
    } else {
        setZoomLevel(m_zoomLevel - 1);
    }
    
    QWidget::wheelEvent(event);
}

void MapRenderer::keyPressEvent(QKeyEvent *event)
{
    const double moveStep = 0.001; // Degrees
    
    switch (event->key()) {
    case Qt::Key_Up:
        m_centerLat += moveStep;
        update();
        break;
    case Qt::Key_Down:
        m_centerLat -= moveStep;
        update();
        break;
    case Qt::Key_Left:
        m_centerLon -= moveStep;
        update();
        break;
    case Qt::Key_Right:
        m_centerLon += moveStep;
        update();
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

void MapRenderer::drawGrid(QPainter& painter)
{
    painter.setPen(QPen(QColor(200, 200, 200), 1));
    
    // Calculate grid spacing based on zoom level
    double gridSpacing = 0.01 / pow(2, m_zoomLevel - 10); // Adjust grid density
    
    // Calculate visible bounds
    Point topLeft = screenToGeo(QPoint(0, 0));
    Point bottomRight = screenToGeo(QPoint(width(), height()));
    
    // Draw latitude lines
    double startLat = floor(topLeft.latitude / gridSpacing) * gridSpacing;
    double endLat = ceil(bottomRight.latitude / gridSpacing) * gridSpacing;
    
    for (double lat = startLat; lat <= endLat; lat += gridSpacing) {
        QPoint p1 = geoToScreen(Point(lat, topLeft.longitude));
        QPoint p2 = geoToScreen(Point(lat, bottomRight.longitude));
        painter.drawLine(p1, p2);
    }
    
    // Draw longitude lines
    double startLon = floor(bottomRight.longitude / gridSpacing) * gridSpacing;
    double endLon = ceil(topLeft.longitude / gridSpacing) * gridSpacing;
    
    for (double lon = startLon; lon <= endLon; lon += gridSpacing) {
        QPoint p1 = geoToScreen(Point(topLeft.latitude, lon));
        QPoint p2 = geoToScreen(Point(bottomRight.latitude, lon));
        painter.drawLine(p1, p2);
    }
}

void MapRenderer::drawRoute(QPainter& painter)
{
    if (m_routePoints.size() < 2) {
        return;
    }
    
    painter.setPen(QPen(QColor(0, 100, 255), 3)); // Blue route line
    
    QPoint prevPoint = geoToScreen(m_routePoints[0]);
    
    for (size_t i = 1; i < m_routePoints.size(); ++i) {
        QPoint currentPoint = geoToScreen(m_routePoints[i]);
        painter.drawLine(prevPoint, currentPoint);
        prevPoint = currentPoint;
    }
    
    // Draw route direction arrows
    painter.setPen(QPen(QColor(0, 80, 200), 2));
    for (size_t i = 0; i < m_routePoints.size() - 1; i += 3) { // Every 3rd point
        drawDirectionArrow(painter, m_routePoints[i], m_routePoints[i + 1]);
    }
}

void MapRenderer::drawWaypoint(QPainter& painter, const Point& point, const QColor& color, const QString& label)
{
    QPoint screenPos = geoToScreen(point);
    
    // Draw circle
    painter.setPen(QPen(color.darker(), 2));
    painter.setBrush(QBrush(color));
    painter.drawEllipse(screenPos, 8, 8);
    
    // Draw label
    painter.setPen(QPen(Qt::white));
    painter.setFont(QFont("Arial", 10, QFont::Bold));
    QRect textRect = painter.fontMetrics().boundingRect(label);
    textRect.moveCenter(screenPos);
    painter.drawText(textRect, Qt::AlignCenter, label);
}

void MapRenderer::drawCurrentPosition(QPainter& painter)
{
    QPoint screenPos = geoToScreen(m_currentPosition);
    
    // Draw position circle
    painter.setPen(QPen(Qt::blue, 3));
    painter.setBrush(QBrush(QColor(0, 100, 255, 180)));
    painter.drawEllipse(screenPos, 12, 12);
    
    // Draw center dot
    painter.setPen(QPen(Qt::white, 2));
    painter.setBrush(QBrush(Qt::white));
    painter.drawEllipse(screenPos, 4, 4);
    
    // Draw heading indicator
    drawHeadingIndicator(painter, screenPos);
}

void MapRenderer::drawHeadingIndicator(QPainter& painter, const QPoint& center)
{
    painter.save();
    
    painter.translate(center);
    painter.rotate(m_currentHeading);
    
    // Draw arrow
    painter.setPen(QPen(Qt::red, 2));
    painter.setBrush(QBrush(Qt::red));
    
    QPolygon arrow;
    arrow << QPoint(0, -20) << QPoint(-6, -8) << QPoint(6, -8);
    painter.drawPolygon(arrow);
    
    painter.restore();
}

void MapRenderer::drawDirectionArrow(QPainter& painter, const Point& from, const Point& to)
{
    QPoint fromScreen = geoToScreen(from);
    QPoint toScreen = geoToScreen(to);
    
    // Calculate arrow direction
    double dx = toScreen.x() - fromScreen.x();
    double dy = toScreen.y() - fromScreen.y();
    double angle = atan2(dy, dx) * 180.0 / M_PI;
    
    // Find middle point
    QPoint middle((fromScreen.x() + toScreen.x()) / 2, (fromScreen.y() + toScreen.y()) / 2);
    
    painter.save();
    painter.translate(middle);
    painter.rotate(angle);
    
    // Draw small arrow
    painter.setPen(QPen(QColor(0, 80, 200), 2));
    painter.setBrush(QBrush(QColor(0, 80, 200)));
    
    QPolygon arrow;
    arrow << QPoint(8, 0) << QPoint(-4, -3) << QPoint(-4, 3);
    painter.drawPolygon(arrow);
    
    painter.restore();
}

void MapRenderer::drawScaleBar(QPainter& painter)
{
    // Draw scale bar in bottom left corner
    painter.setPen(QPen(Qt::black, 2));
    painter.setFont(QFont("Arial", 9));
    
    // Calculate scale
    Point p1 = screenToGeo(QPoint(10, height() - 30));
    Point p2 = screenToGeo(QPoint(110, height() - 30)); // 100 pixels
    double distance = calculateDistance(p1, p2);
    
    QString scaleText;
    if (distance >= 1000) {
        scaleText = QString("%1 km").arg(distance / 1000.0, 0, 'f', 1);
    } else {
        scaleText = QString("%1 m").arg(distance, 0, 'f', 0);
    }
    
    // Draw scale bar
    painter.drawLine(10, height() - 30, 110, height() - 30);
    painter.drawLine(10, height() - 35, 10, height() - 25);
    painter.drawLine(110, height() - 35, 110, height() - 25);
    
    // Draw scale text
    painter.drawText(15, height() - 35, scaleText);
}

void MapRenderer::drawCoordinateInfo(QPainter& painter)
{
    // Draw coordinate info in top right corner
    painter.setPen(QPen(Qt::black));
    painter.setFont(QFont("Arial", 9));
    
    QString coordText = QString("Center: %1, %2\nZoom: %3")
                       .arg(m_centerLat, 0, 'f', 6)
                       .arg(m_centerLon, 0, 'f', 6)
                       .arg(m_zoomLevel);
    
    QRect textRect = painter.fontMetrics().boundingRect(coordText);
    textRect.moveTopRight(QPoint(width() - 10, 10));
    
    // Draw background
    painter.fillRect(textRect.adjusted(-5, -2, 5, 2), QColor(255, 255, 255, 200));
    
    // Draw text
    painter.drawText(textRect, coordText);
}

QPoint MapRenderer::geoToScreen(const Point& geoPoint) const
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

Point MapRenderer::screenToGeo(const QPoint& screenPoint) const
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

double MapRenderer::calculateDistance(const Point& p1, const Point& p2) const
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