#pragma once

#include <QWidget>
#include <QPixmap>
#include <QPoint>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QTimer>
#include <QPropertyAnimation>
#include <QGraphicsEffect>
#include <QSettings>
#include <vector>

#include "nav_types.h"

namespace nav {

/**
 * @brief Enhanced map widget with real image support and MVC architecture
 */
class MapWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(double centerLatitude READ centerLatitude WRITE setCenterLatitude)
    Q_PROPERTY(double centerLongitude READ centerLongitude WRITE setCenterLongitude)
    Q_PROPERTY(int zoomLevel READ zoomLevel WRITE setZoomLevel)
    Q_PROPERTY(QString mapStyle READ mapStyle WRITE setMapStyle)

public:
    enum MapStyle {
        SATELLITE,
        STREET_MAP,
        HYBRID
    };
    Q_ENUM(MapStyle)

    explicit MapWidget(QWidget *parent = nullptr);
    ~MapWidget();

    // Property accessors
    double centerLatitude() const { return m_centerLat; }
    double centerLongitude() const { return m_centerLon; }
    int zoomLevel() const { return m_zoomLevel; }
    QString mapStyle() const;

    // Map control
    void setCenterLatitude(double lat);
    void setCenterLongitude(double lon);
    void setZoomLevel(int zoom);
    void setMapStyle(const QString& style);
    void setMapStyle(MapStyle style);
    void centerMap(double lat, double lon);

    // Navigation elements
    void setCurrentPosition(const Point& position, double heading = 0.0);
    void setStartPoint(const Point& point);
    void setEndPoint(const Point& point);
    void setRoute(const std::vector<Point>& route);
    void clearRoute();
    void clearStartPoint();
    void clearEndPoint();
    void clearWaypoints();
    void setClickedPoint(const Point& point);
    void clearClickedPoint();

    // Animation
    void animateToPosition(const Point& position);
    void animateZoomTo(int targetZoom);

    // Settings
    void loadSettings();
    void saveSettings();

signals:
    void mapClicked(const Point& position);
    void mapDoubleClicked(const Point& position);
    void mousePositionChanged(const Point& position);
    void zoomChanged(int newZoom);
    void centerChanged(double lat, double lon);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onAnimationFinished();

private:
    // Map rendering
    void drawMap(QPainter& painter);
    void drawGrid(QPainter& painter);
    void drawRoute(QPainter& painter);
    void drawWaypoints(QPainter& painter);
    void drawCurrentPosition(QPainter& painter);
    void drawClickedPoint(QPainter& painter);
    void drawHeadingIndicator(QPainter& painter, const QPoint& center);
    void drawDirectionArrows(QPainter& painter);
    void drawMapOverlays(QPainter& painter);
    void drawScaleBar(QPainter& painter);
    void drawCoordinateInfo(QPainter& painter);
    void drawCopyright(QPainter& painter);

    // Map image management
    void loadMapImages();
    QPixmap getCurrentMapImage() const;
    QPixmap getMapImageForStyle(MapStyle style) const;
    void updateMapCache();

    // Coordinate conversion
    QPoint geoToScreen(const Point& geoPoint) const;
    Point screenToGeo(const QPoint& screenPoint) const;
    QPoint geoToMap(const Point& geoPoint) const;
    Point mapToGeo(const QPoint& mapPoint) const;

    // Utility functions
    double calculateDistance(const Point& p1, const Point& p2) const;
    double calculateBearing(const Point& from, const Point& to) const;
    QRect getVisibleBounds() const;
    bool isPointVisible(const Point& point) const;

    // Map data
    double m_centerLat;
    double m_centerLon;
    int m_zoomLevel;
    MapStyle m_currentMapStyle;
    
    // Map images
    QPixmap m_satelliteMap;
    QPixmap m_streetMap;
    QPixmap m_hybridMap;
    QPixmap m_currentMapCache;
    bool m_mapImagesLoaded;

    // Navigation elements
    Point m_currentPosition;
    double m_currentHeading;
    bool m_hasCurrentPosition;
    
    Point m_startPoint;
    Point m_endPoint;
    bool m_hasStartPoint;
    bool m_hasEndPoint;
    
    Point m_clickedPoint;
    bool m_hasClickedPoint;
    
    std::vector<Point> m_routePoints;

    // Map bounds (for map images)
    Point m_mapTopLeft;
    Point m_mapBottomRight;
    
    // Mouse interaction
    bool m_dragging;
    QPoint m_lastPanPoint;
    
    // Animation
    QPropertyAnimation* m_centerAnimation;
    QPropertyAnimation* m_zoomAnimation;
    
    // Visual settings
    bool m_showGrid;
    bool m_showCoordinates;
    bool m_showScale;
    bool m_showCopyright;
    
    // Constants
    static const int MIN_ZOOM = 5;
    static const int MAX_ZOOM = 20;
    static const int DEFAULT_ZOOM = 12;
    
    // Static methods to get constants
    static double getDefaultLat() { return 21.028511; }  // Hanoi
    static double getDefaultLon() { return 105.804817; }
    static double getMapBoundsNorth() { return 21.1; }
    static double getMapBoundsSouth() { return 20.9; }
    static double getMapBoundsEast() { return 105.9; }
    static double getMapBoundsWest() { return 105.7; }
};

} // namespace nav