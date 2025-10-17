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
#include <QCheckBox>
#include <QMenu>
#include <QCursor>

#include "nav_types.h"
#include "nav_messages.h"
#include "nav_utils.h"
#include "../../controllers/include/integrated_navigation_controller.h"
#include "../../services/include/poi_service.h"
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
    //Needtodo
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
    void onSetClickedAsStartPoint();
    void onSetClickedAsEndPoint();
    void onSimulationTimer();
    void onMousePositionChanged(const Point& position);
    
    // POI test panel slots
    void onPOILoadData();
    void onPOISearch();
    void onPOINearbySearch();
    void onGeocodeAddress();
    void onPOIDataLoaded();
    void onPOISearchCompleted(const std::vector<POI>& results);
    void onAddressGeocoded(const GeocodingResult& result);
    void onPOIResultSelected(int index);  // Handle POI selection from list
    
    // Panel collapse/expand slots
    void onToggleLeftPanel();
    void onToggleRightPanel();

private:
    void setupUI();
    void setupMenuBar();
    void setupStatusBar();
    void setupMapView();
    void setupControlPanel(QVBoxLayout* parentLayout = nullptr);
    void setupInfoPanel(QVBoxLayout* parentLayout = nullptr);
    void setupManualControlPanel(QVBoxLayout* parentLayout = nullptr);
    void setupGuidanceControlPanel(QVBoxLayout* parentLayout = nullptr);
    void setupPOITestPanel(QVBoxLayout* parentLayout = nullptr);
    void createGuidancePopup();
    void showGuidancePopup();
    void hideGuidancePopup();
    
    void updateStatusBar();
    void updatePositionDisplay();
    void updateGuidanceDisplay();
    void updateRouteInfo();
    void updateServiceStatus();
    
    void onServiceModeChanged();
    
    // Helper functions
    void setControlsEnabled(bool enabled);
    void checkAndStopGuidance();
    double calculateDistance(double lat1, double lon1, double lat2, double lon2);
    double calculateBearing(double lat1, double lon1, double lat2, double lon2);
    bool checkDestinationReached();
    void showPOIOnMap(const POI& poi);  // Show selected POI on map
    
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
    
    // Manual Control Panel for Speed & Heading
    QGroupBox *m_manualControlGroup;
    QSlider *m_manualSpeedSlider;
    QLabel *m_manualSpeedLabel;
    QSlider *m_manualHeadingSlider;
    QLabel *m_manualHeadingLabel;
    
    // Guidance Control Panel  
    QGroupBox *m_guidanceControlGroup;
    QPushButton *m_startGuidanceButton;
    QPushButton *m_stopGuidanceButton;
    QLabel *m_guidanceStatusLabel;
    QCheckBox *m_autoHeadingCheckBox;
    
    // Guidance popup overlay
    QWidget *m_guidancePopup;
    QLabel *m_guidancePopupTitle;
    QLabel *m_guidancePopupStatus;
    QPushButton *m_guidancePopupStopButton;
    
    // Information panel
    QGroupBox *m_positionGroup;
    QLabel *m_latitudeLabel;
    QLabel *m_longitudeLabel;
    QLabel *m_speedValueLabel;
    QLabel *m_headingLabel;
    
    // Mouse position display
    QLabel *m_mouseLatLabel;
    QLabel *m_mouseLonLabel;
    
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
    
    // POI Test Panel
    QGroupBox *m_poiTestGroup;
    QLabel *m_poiStatusLabel;
    QPushButton *m_poiLoadDataButton;
    QLineEdit *m_poiSearchEdit;
    QPushButton *m_poiSearchButton;
    QComboBox *m_poiCategoryCombo;
    QSpinBox *m_poiRadiusSpinBox;
    QPushButton *m_poiNearbyButton;
    QLineEdit *m_addressEdit;
    QPushButton *m_geocodeButton;
    QTextEdit *m_poiResultsText;
    QListWidget *m_poiResultsList;  // List widget for clickable POI results
    std::vector<POI> m_lastSearchResults;  // Store last search results
    
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
    POIService *m_poiService;
    
    // State
    Point m_startPoint;
    Point m_endPoint;
    bool m_hasStartPoint;
    bool m_hasEndPoint;
    bool m_simulationRunning;
    bool m_autoHeadingEnabled;
    
    // Destination detection tracking
    Point m_previousPosition;
    bool m_hasPreviousPosition;
    double m_previousDistanceToEnd;
    bool m_guidanceRunning;
    
    // Current position simulation
    Point m_currentPosition;
    bool m_hasCurrentPosition;
    QTimer *m_simulationTimer;
    
    // Current manual control values
    double m_manualSpeed;
    double m_manualHeading;
    
    // Last clicked position for context menu
    Point m_lastClickedPosition;
    bool m_hasLastClickedPosition;
    
    // Panel collapse state
    QWidget *m_leftPanelContainer;
    QWidget *m_rightPanelContainer;
    QPushButton *m_toggleLeftPanelButton;
    QPushButton *m_toggleRightPanelButton;
    bool m_leftPanelCollapsed;
    bool m_rightPanelCollapsed;
    
    // Static methods to get constants
    static double getDefaultLat() { return 21.028511; }  // Hanoi, Vietnam
    static double getDefaultLon() { return 105.804817; }
    static double getMapZoom() { return 0.01; }          // Map zoom level
};

} // namespace nav