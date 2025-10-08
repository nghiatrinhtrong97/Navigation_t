#pragma once

#include <QWidget>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QPainter>
#include <QPoint>
#include <QPointF>
#include <QRect>
#include <QRectF>
#include <QColor>
#include <QBrush>
#include <QPen>
#include <QFont>
#include <QFontMetrics>
#include <QPixmap>
#include <QTimer>

#include "nav_types.h"

namespace nav {

class MapRenderer : public QWidget
{
    Q_OBJECT

public:
    explicit MapRenderer(QWidget *parent = nullptr);
    ~MapRenderer();

    // Map control
    void setCenter(const Point& center);
    void setZoom(double zoom);
    void zoomIn();
    void zoomOut();
    void resetView();
    
    // Data display
    void setCurrentPosition(const Point& position, double heading = 0.0);
    void setRoute(const Route& route, const std::vector<Point>& routePoints);
    void clearRoute();
    void setStartPoint(const Point& point);
    void setEndPoint(const Point& point);
    void clearWaypoints();
    
    // Map data
    void setMapNodes(const std::vector<MapNode>& nodes);
    void setMapEdges(const std::vector<MapEdges>& edges);
    
    // Conversion functions
    QPointF geoToScreen(const Point& geoPoint) const;
    Point screenToGeo(const QPointF& screenPoint) const;

signals:
    void mapClicked(const Point& geoPosition);
    void mapRightClicked(const Point& geoPosition);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void drawBackground(QPainter& painter);
    void drawGrid(QPainter& painter);
    void drawMapData(QPainter& painter);
    void drawRoute(QPainter& painter);
    void drawWaypoints(QPainter& painter);
    void drawCurrentPosition(QPainter& painter);
    void drawScale(QPainter& painter);
    void drawCompass(QPainter& painter);
    
    void updateViewport();
    double calculateScale() const;  // Meters per pixel
    
    // Map state
    Point m_center;                 // Map center in geo coordinates
    double m_zoom;                  // Zoom level (degrees per pixel)
    QRectF m_viewport;              // Current viewport in geo coordinates
    
    // Display data
    Point m_currentPosition;
    double m_currentHeading;        // Vehicle heading in degrees
    bool m_hasCurrentPosition;
    
    std::vector<Point> m_routePoints;
    bool m_hasRoute;
    
    Point m_startPoint;
    Point m_endPoint;
    bool m_hasStartPoint;
    bool m_hasEndPoint;
    
    std::vector<MapNode> m_mapNodes;
    std::vector<MapEdge> m_mapEdges;
    
    // Interaction state
    bool m_dragging;
    QPoint m_lastMousePos;
    
    // Visual settings
    QColor m_backgroundColor;
    QColor m_gridColor;
    QColor m_roadColor;
    QColor m_routeColor;
    QColor m_positionColor;
    QColor m_waypointColor;
    
    int m_roadWidth;
    int m_routeWidth;
    int m_positionRadius;
    int m_waypointRadius;
    
    // Performance
    QPixmap m_backgroundCache;
    bool m_backgroundDirty;
    QTimer *m_redrawTimer;
    
    // Constants
    static constexpr double MIN_ZOOM = 0.0001;   // Very zoomed in
    static constexpr double MAX_ZOOM = 1.0;      // Very zoomed out
    static constexpr double ZOOM_FACTOR = 1.2;
    static constexpr double DEFAULT_ZOOM = 0.01;
    static constexpr double DEFAULT_LAT = 21.028511;  // Hanoi
    static constexpr double DEFAULT_LON = 105.804817;
};

} // namespace nav