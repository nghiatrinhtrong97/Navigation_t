#pragma once

#include <QMainWindow>
#include <QTimer>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QProgressBar>
#include <QSlider>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QListWidget>
#include <QTabWidget>
#include <QSplitter>
#include <QPaintEvent>
#include <QPainter>
#include <QMouseEvent>
#include <QPointF>
#include <QApplication>
#include <QMessageBox>
#include <QFileDialog>
#include <QMenuBar>
#include <QStatusBar>
#include <QThread>
#include <QMutex>
#include <QMutexLocker>

#include "nav_types.h"
#include "nav_messages.h"
#include "nav_utils.h"
#include "integrated_navigation_controller.h"
#include "map_widget.h"
#include "service_interfaces.h"

QT_BEGIN_NAMESPACE
class QAction;
class QMenu;
QT_END_NAMESPACE

namespace nav {

class NavigationMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    NavigationMainWindow(QWidget *parent = nullptr);
    ~NavigationMainWindow();

private slots:
    void onCalculateRoute();
    void onClearRoute();
    void onStartSimulation();
    void onStopSimulation();
    void onPositionChanged(const Point& position, double heading, double speed);
    void onRouteCalculated(const Route& route);
    void onGuidanceUpdated(const GuidanceInstruction& instruction);
    void onMapClicked(const Point& position);
    void onSimulationSpeedChanged(int speed);
    void onSetStartPoint();
    void onSetEndPoint();
    void onGetCurrentPosition();
    void onSpeedChanged(int speed);
    void onHeadingChanged(int heading);
    void onStartGuidance();
    void onStopGuidance();
    void onAutoHeadingToggled(bool enabled);

    // POI Service Test Slots
    void onPOISearch();
    void onPOILoadData();
    void onPOINearbySearch();
    void onGeocodeAddress();
    void onPOIDataLoaded(int count);
    void onPOISearchCompleted(const std::vector<POI>& results);
    void onAddressGeocoded(const GeocodingResult& result);

private:
    void setupUI();
    void setupMenuBar();
    void setupStatusBar();
    void setupMapView();
    void setupControlPanel();
    void setupInfoPanel();
    void setupManualControlPanel();
    void setupGuidanceControlPanel();
    
    void updateStatusBar();
    void updatePositionDisplay();
    void updateGuidanceDisplay();
    void updateRouteInfo();
    void updateServiceStatus();
    
    void onServiceModeChanged();
    
    // Helper functions
    double calculateDistance(double lat1, double lon1, double lat2, double lon2);
    double calculateBearing(double lat1, double lon1, double lat2, double lon2);
    
    // UI Components
    QWidget *m_centralWidget;
    QSplitter *m_mainSplitter;
    QTabWidget *m_controlTabs;
    
    // Map view
    MapWidget *m_mapRenderer;
    QGroupBox *m_mapGroup;
    
    // Control panel
    QGroupBox *m_routeGroup;
    QLineEdit *m_startLatEdit;
    QLineEdit *m_startLonEdit;
    QLineEdit *m_endLatEdit;
    QLineEdit *m_endLonEdit;
    QPushButton *m_calculateButton;
    QPushButton *m_clearButton;
    QPushButton *m_setStartButton;
    QPushButton *m_setEndButton;
    
    // Simulation controls
    QGroupBox *m_simulationGroup;
    QPushButton *m_startSimButton;
    QPushButton *m_stopSimButton;
    QSlider *m_speedSlider;
    QLabel *m_speedLabel;
    QSlider *m_headingSlider;
    QLabel *m_headingSliderLabel;
    QProgressBar *m_routeProgress;
    
    // Information panel
    QGroupBox *m_positionGroup;
    QLabel *m_latitudeLabel;
    QLabel *m_longitudeLabel;
    QLabel *m_speedValueLabel;
    QLabel *m_headingLabel;
    
    QGroupBox *m_guidanceGroup;
    QLabel *m_instructionLabel;
    QLabel *m_distanceLabel;
    QLabel *m_timeLabel;
    
    QGroupBox *m_routeInfoGroup;
    QLabel *m_totalDistanceLabel;
    QLabel *m_totalTimeLabel;
    QLabel *m_remainingDistanceLabel;
    QLabel *m_remainingTimeLabel;
    
    // Log display
    QTextEdit *m_logTextEdit;
    
    // Service control
    QGroupBox *m_serviceGroup;
    QLabel *m_serviceModeLabel;
    QPushButton *m_toggleModeButton;
    QPushButton *m_reconnectButton;
    QTextEdit *m_serviceStatusText;
    
    // Menu and status
    QAction *m_exitAction;
    QAction *m_aboutAction;
    QLabel *m_statusLabel;
    
    // Backend components
    IntegratedNavigationController *m_navController;
    
    // State
    Point m_startPoint;
    Point m_endPoint;
    bool m_hasStartPoint;
    bool m_hasEndPoint;
    bool m_simulationRunning;
    bool m_autoHeadingEnabled;

    // POI Service Testing Components
    QGroupBox* m_poiTestGroup;
    QLineEdit* m_poiSearchEdit;
    QPushButton* m_poiSearchButton;
    QPushButton* m_poiLoadDataButton;
    QPushButton* m_poiNearbyButton;
    QComboBox* m_poiCategoryCombo;
    QTextEdit* m_poiResultsText;
    QLabel* m_poiStatusLabel;
    QSpinBox* m_poiRadiusSpinBox;
    QPushButton* m_geocodeButton;
    QLineEdit* m_addressEdit;
    std::unique_ptr<POIService> m_poiService;
    
    // Static methods to get constants
    static double getDefaultLat() { return 21.028511; }  // Hanoi, Vietnam
    static double getDefaultLon() { return 105.804817; }
    static double getMapZoom() { return 0.01; }          // Map zoom level
};

} // namespace nav