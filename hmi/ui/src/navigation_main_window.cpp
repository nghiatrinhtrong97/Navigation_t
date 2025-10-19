#include "../../ui/include/navigation_main_window.h"
#include "../../models/include/navigation_models.h"
#include "../../ui/include/map_widget.h"
#include "../../controllers/include/integrated_navigation_controller.h"
#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QMessageBox>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QSlider>
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#include <QProgressBar>
#include <QTextEdit>
#include <QGroupBox>
#include <QSplitter>
#include <QStatusBar>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QToolBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QTimer>
#include <QDateTime>
#include <QDesktopServices>
#include <QUrl>
#include <QFile>
#include <QDebug>
#include <cmath>

#include "../../services/include/poi_service.h"
#include <chrono>
#include <QTimer>
#include <QSpinBox>
#include <QComboBox>
#include <QTextEdit>
#include <QListWidget>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace nav {

NavigationMainWindow::NavigationMainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_centralWidget(nullptr)
    , m_mapRenderer(nullptr)
    , m_startLatEdit(nullptr)
    , m_startLonEdit(nullptr)
    , m_endLatEdit(nullptr)
    , m_endLonEdit(nullptr)
    , m_calculateButton(nullptr)
    , m_clearButton(nullptr)
    , m_setStartButton(nullptr)
    , m_setEndButton(nullptr)
    , m_startSimButton(nullptr)
    , m_stopSimButton(nullptr)
    , m_speedSlider(nullptr)
    , m_speedLabel(nullptr)
    , m_latitudeLabel(nullptr)
    , m_longitudeLabel(nullptr)
    , m_speedValueLabel(nullptr)
    , m_headingLabel(nullptr)
    , m_instructionLabel(nullptr)
    , m_distanceLabel(nullptr)
    , m_logTextEdit(nullptr)
    , m_navController(nullptr)
    , m_hasStartPoint(false)
    , m_hasEndPoint(false)
    , m_simulationRunning(false)
    , m_autoHeadingEnabled(false)
    , m_guidanceRunning(false)
    , m_hasCurrentPosition(false)
    , m_hasPreviousPosition(false)
    , m_previousDistanceToEnd(0.0)
    , m_simulationTimer(nullptr)
    , m_manualSpeed(50.0)
    , m_manualHeading(0.0)
    , m_hasLastClickedPosition(false)
    , m_manualSpeedSlider(nullptr)
    , m_manualSpeedLabel(nullptr)
    , m_manualHeadingSlider(nullptr)
    , m_manualHeadingLabel(nullptr)
    , m_startGuidanceButton(nullptr)
    , m_stopGuidanceButton(nullptr)
    , m_guidanceStatusLabel(nullptr)
    , m_autoHeadingCheckBox(nullptr)
    , m_poiService(nullptr)
    , m_leftPanelContainer(nullptr)
    , m_rightPanelContainer(nullptr)
    , m_toggleLeftPanelButton(nullptr)
    , m_toggleRightPanelButton(nullptr)
    , m_leftPanelCollapsed(false)
    , m_rightPanelCollapsed(false)
{
    // Initialize integrated navigation controller
    m_navController = new IntegratedNavigationController(this);
    
    // Get POI service from controller
    m_poiService = m_navController->getPOIService();
    
    // Initialize simulation timer
    m_simulationTimer = new QTimer(this);
    m_simulationTimer->setInterval(1000); // Update every 1 second
    connect(m_simulationTimer, &QTimer::timeout, this, &NavigationMainWindow::onSimulationTimer);
    
    // Connect signals
    connect(m_navController, &IntegratedNavigationController::positionChanged,
            this, &NavigationMainWindow::onPositionChanged);
    connect(m_navController, &IntegratedNavigationController::routeCalculated,
            this, &NavigationMainWindow::onRouteCalculated);
    connect(m_navController, &IntegratedNavigationController::guidanceUpdated,
            this, &NavigationMainWindow::onGuidanceUpdated);

    // Initialize services
    if (!m_navController->initializeServices()) {
        QMessageBox::critical(this, "Error", "Failed to initialize navigation services");
    }
    
    setupUI();
    qDebug() << "NavigationMainWindow initialized with integrated controller";
}

NavigationMainWindow::~NavigationMainWindow()
{
    // Cleanup will be handled by Qt parent-child system
}

void NavigationMainWindow::setupUI()
{
    qDebug() << "Setting up UI...";
    setWindowTitle("Automotive Navigation System");
    
    // Ensure normal window with title bar and controls
    setWindowFlags(Qt::Window);
    
    setMinimumSize(800, 600);
    resize(1200, 800);

    // Create central widget and layout
    m_centralWidget = new QWidget;
    setCentralWidget(m_centralWidget);
    QVBoxLayout *mainLayout = new QVBoxLayout(m_centralWidget);
    qDebug() << "Central widget and layout created";

    // Create map view
    setupMapView();
    if (m_mapRenderer) {
        mainLayout->addWidget(m_mapRenderer);
        qDebug() << "Map renderer added to layout";
    }

    // Create horizontal layout for 2-column layout below map
    QHBoxLayout *bottomLayout = new QHBoxLayout();
    bottomLayout->setSpacing(10); // Add some spacing between columns
    
    // Create LEFT PANEL with collapse button
    QVBoxLayout *leftPanelLayout = new QVBoxLayout();
    
    // Add collapse button for left panel
    m_toggleLeftPanelButton = new QPushButton("< Hide Controls");
    m_toggleLeftPanelButton->setMaximumHeight(30);
    m_toggleLeftPanelButton->setStyleSheet(
        "QPushButton { "
        "   background-color: #2196F3; "
        "   color: white; "
        "   font-weight: bold; "
        "   border-radius: 5px; "
        "   padding: 5px; "
        "}"
        "QPushButton:hover { background-color: #1976D2; }"
    );
    connect(m_toggleLeftPanelButton, &QPushButton::clicked, this, &NavigationMainWindow::onToggleLeftPanel);
    leftPanelLayout->addWidget(m_toggleLeftPanelButton);
    
    // Create container for left panel content
    m_leftPanelContainer = new QWidget();
    QVBoxLayout *leftColumn = new QVBoxLayout(m_leftPanelContainer);
    leftColumn->setContentsMargins(0, 0, 0, 0);
    
    // Setup control panel in left column
    setupControlPanel(leftColumn);
    qDebug() << "Control panel setup completed";
    
    leftPanelLayout->addWidget(m_leftPanelContainer);
    
    // Create RIGHT PANEL with collapse button
    QVBoxLayout *rightPanelLayout = new QVBoxLayout();
    
    // Add collapse button for right panel
    m_toggleRightPanelButton = new QPushButton("Hide Info >");
    m_toggleRightPanelButton->setMaximumHeight(30);
    m_toggleRightPanelButton->setStyleSheet(
        "QPushButton { "
        "   background-color: #4CAF50; "
        "   color: white; "
        "   font-weight: bold; "
        "   border-radius: 5px; "
        "   padding: 5px; "
        "}"
        "QPushButton:hover { background-color: #45a049; }"
    );
    connect(m_toggleRightPanelButton, &QPushButton::clicked, this, &NavigationMainWindow::onToggleRightPanel);
    rightPanelLayout->addWidget(m_toggleRightPanelButton);
    
    // Create container for right panel content
    m_rightPanelContainer = new QWidget();
    QVBoxLayout *rightColumn = new QVBoxLayout(m_rightPanelContainer);
    rightColumn->setContentsMargins(0, 0, 0, 0);
    
    // Setup info panel in right column
    setupInfoPanel(rightColumn);
    qDebug() << "Info panel setup completed";
    
    rightPanelLayout->addWidget(m_rightPanelContainer);
    
    // Add panels to horizontal layout with stretch factors
    bottomLayout->addLayout(leftPanelLayout, 4);
    bottomLayout->addLayout(rightPanelLayout, 1);
    
    // Add the 2-column layout to main layout
    mainLayout->addLayout(bottomLayout);

    // Setup menu and status bar
    setupMenuBar();
    setupStatusBar();
    createGuidancePopup();

    qDebug() << "Main UI setup completed";
}

void NavigationMainWindow::setupMapView()
{
    qDebug() << "Creating MapWidget...";
    m_mapRenderer = new MapWidget(this);
    m_mapRenderer->setMinimumSize(400, 300);
    
    // Connect map signals
    connect(m_mapRenderer, &MapWidget::mapClicked, this, &NavigationMainWindow::onMapClicked);
    connect(m_mapRenderer, &MapWidget::mousePositionChanged, this, &NavigationMainWindow::onMousePositionChanged);
    
    qDebug() << "MapWidget created with size:" << m_mapRenderer->size();
}

void NavigationMainWindow::setupControlPanel(QVBoxLayout* parentLayout)
{
    // Route Planning Group
    QGroupBox *routeGroup = new QGroupBox("Route Planning");
    QVBoxLayout *routeLayout = new QVBoxLayout(routeGroup);
    
    // Start Point Section
    QGroupBox *startGroup = new QGroupBox("Start Point");
    QGridLayout *startLayout = new QGridLayout(startGroup);
    
    startLayout->addWidget(new QLabel("Latitude:"), 0, 0);
    m_startLatEdit = new QLineEdit("21.028511"); // Default Hanoi
    m_startLatEdit->setPlaceholderText("e.g. 21.028511");
    startLayout->addWidget(m_startLatEdit, 0, 1);
    
    startLayout->addWidget(new QLabel("Longitude:"), 1, 0);
    m_startLonEdit = new QLineEdit("105.804817"); // Default Hanoi
    m_startLonEdit->setPlaceholderText("e.g. 105.804817");
    startLayout->addWidget(m_startLonEdit, 1, 1);
    
    m_setStartButton = new QPushButton("Use Current Position");
    startLayout->addWidget(m_setStartButton, 2, 0, 1, 2);
    
    // End Point Section
    QGroupBox *endGroup = new QGroupBox("Destination");
    QGridLayout *endLayout = new QGridLayout(endGroup);
    
    endLayout->addWidget(new QLabel("Latitude:"), 0, 0);
    m_endLatEdit = new QLineEdit("21.035000"); // Default destination
    m_endLatEdit->setPlaceholderText("e.g. 21.035000");
    endLayout->addWidget(m_endLatEdit, 0, 1);
    
    endLayout->addWidget(new QLabel("Longitude:"), 1, 0);
    m_endLonEdit = new QLineEdit("105.820000"); // Default destination
    m_endLonEdit->setPlaceholderText("e.g. 105.820000");
    endLayout->addWidget(m_endLonEdit, 1, 1);
    
    m_setEndButton = new QPushButton("Set on Map");
    endLayout->addWidget(m_setEndButton, 2, 0, 1, 2);
    
    // Control Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    m_calculateButton = new QPushButton("Calculate Route");
    m_clearButton = new QPushButton("Clear Route");
    QPushButton *getCurrentPosButton = new QPushButton("Get Current Position");
    
    buttonLayout->addWidget(m_calculateButton);
    buttonLayout->addWidget(m_clearButton);
    buttonLayout->addWidget(getCurrentPosButton);
    
    // Connect the new button
    connect(getCurrentPosButton, &QPushButton::clicked, this, &NavigationMainWindow::onGetCurrentPosition);
    
    // Route Information
    QGroupBox *infoGroup = new QGroupBox("Route Information");
    QVBoxLayout *infoLayout = new QVBoxLayout(infoGroup);
    
    m_totalDistanceLabel = new QLabel("Total Distance: --");
    m_totalTimeLabel = new QLabel("Estimated Time: --");
    m_remainingDistanceLabel = new QLabel("Remaining: --");
    
    infoLayout->addWidget(m_totalDistanceLabel);
    infoLayout->addWidget(m_totalTimeLabel);
    infoLayout->addWidget(m_remainingDistanceLabel);

    //Needtodo
    
    // Assemble route group
    routeLayout->addWidget(startGroup);
    routeLayout->addWidget(endGroup);
    routeLayout->addLayout(buttonLayout);
    routeLayout->addWidget(infoGroup);
    
    // Connect signals
    connect(m_calculateButton, &QPushButton::clicked, this, &NavigationMainWindow::onCalculateRoute);
    connect(m_clearButton, &QPushButton::clicked, this, &NavigationMainWindow::onClearRoute);
    connect(m_setStartButton, &QPushButton::clicked, this, &NavigationMainWindow::onSetStartPoint);
    connect(m_setEndButton, &QPushButton::clicked, this, &NavigationMainWindow::onSetEndPoint);
    
    // Add to parent layout
    if (parentLayout) {
        parentLayout->addWidget(routeGroup);
    }
    
    // Add Manual Control Panel
    setupManualControlPanel(parentLayout);
    
    // Add Guidance Control Panel  
    setupGuidanceControlPanel(parentLayout);

//    Add POI Test Panel
    setupPOITestPanel(parentLayout);
}

void NavigationMainWindow::setupInfoPanel(QVBoxLayout* parentLayout)
{
    // Basic info panel implementation
    QGroupBox *infoGroup = new QGroupBox("Information & POI Results");
    infoGroup->setMaximumWidth(400); // Increased width for POI results
    QVBoxLayout *layout = new QVBoxLayout(infoGroup);
    
    // Current position info
    m_latitudeLabel = new QLabel("Latitude: --");
    m_longitudeLabel = new QLabel("Longitude: --");
    m_speedValueLabel = new QLabel("Speed: --");
    m_headingLabel = new QLabel("Heading: --");
    
    // Add separator
    QLabel *separator = new QLabel("--- Mouse Position ---");
    separator->setStyleSheet("QLabel { font-weight: bold; color: blue; }");
    
    // Mouse coordinate display
    m_mouseLatLabel = new QLabel("Mouse Lat: --");
    m_mouseLonLabel = new QLabel("Mouse Lon: --");
    
    // Add some styling
    QString labelStyle = "QLabel { padding: 2px; font-size: 10pt; }";
    m_latitudeLabel->setStyleSheet(labelStyle);
    m_longitudeLabel->setStyleSheet(labelStyle);
    m_speedValueLabel->setStyleSheet(labelStyle);
    m_headingLabel->setStyleSheet(labelStyle);
    m_mouseLatLabel->setStyleSheet(labelStyle + " color: blue;");
    m_mouseLonLabel->setStyleSheet(labelStyle + " color: blue;");
    
    layout->addWidget(m_latitudeLabel);
    layout->addWidget(m_longitudeLabel);
    layout->addWidget(m_speedValueLabel);
    layout->addWidget(m_headingLabel);
    layout->addWidget(separator);
    layout->addWidget(m_mouseLatLabel);
    layout->addWidget(m_mouseLonLabel);

    //Needtodo
    
    // Add POI Results Section
    QLabel *poiSeparator = new QLabel("--- POI Search Results ---");
    poiSeparator->setStyleSheet("QLabel { font-weight: bold; color: green; margin-top: 10px; }");
    layout->addWidget(poiSeparator);
    
    // POI Results Display
    m_poiResultsText = new QTextEdit();
    m_poiResultsText->setMaximumHeight(120);
    m_poiResultsText->setStyleSheet(
        "QTextEdit { "
        "   font-family: 'Consolas', 'Monaco', monospace; "
        "   font-size: 8pt; "
        "   background-color: #f5f5f5; "
        "   color: #000000; "
        "   border: 1px solid #ddd; "
        "   padding: 5px; "
        "}"
    );
    m_poiResultsText->setPlainText("POI Service Ready\nSearch to see results...");
    layout->addWidget(m_poiResultsText);
    
    // POI Results List (clickable)
    QLabel *poiListLabel = new QLabel("Click to view on map:");
    poiListLabel->setStyleSheet("QLabel { font-weight: bold; color: #2196F3; margin-top: 5px; }");
    layout->addWidget(poiListLabel);
    
    m_poiResultsList = new QListWidget();
    m_poiResultsList->setMaximumHeight(150);
    m_poiResultsList->setStyleSheet(
        "QListWidget { "
        "   font-size: 9pt; "
        "   background-color: white; "
        "   color: black; "
        "   border: 1px solid #ccc; "
        "}"
        "QListWidget::item { "
        "   color: black; "
        "}"
        "QListWidget::item:selected { "
        "   background-color: #2196F3; "
        "   color: white; "
        "}"
        "QListWidget::item:hover { "
        "   background-color: #e3f2fd; "
        "   color: black; "
        "}"
    );
    layout->addWidget(m_poiResultsList);
    
    // Connect POI list selection
    connect(m_poiResultsList, &QListWidget::currentRowChanged, this, &NavigationMainWindow::onPOIResultSelected);
    
    // Add a stretch to push content to top
    layout->addStretch();
    
    // Add to parent layout
    if (parentLayout) {
        parentLayout->addWidget(infoGroup);
        // Add stretch to keep info panel compact at top
        parentLayout->addStretch();
    }
}

void NavigationMainWindow::setupMenuBar()
{
    QMenuBar *menuBar = this->menuBar();
    QMenu *fileMenu = menuBar->addMenu("&File");
    QMenu *viewMenu = menuBar->addMenu("&View");
    QMenu *helpMenu = menuBar->addMenu("&Help");
    
    QAction *exitAction = fileMenu->addAction("E&xit");
    connect(exitAction, &QAction::triggered, this, &QWidget::close);
}

void NavigationMainWindow::setupStatusBar()
{
    statusBar()->showMessage("Ready");
}

void NavigationMainWindow::createGuidancePopup()
{
    // Create popup overlay widget
    m_guidancePopup = new QWidget(this);
    m_guidancePopup->setStyleSheet(
        "QWidget {"
        "    background-color: rgba(0, 0, 0, 180);"
        "    border-radius: 10px;"
        "}"
        "QLabel {"
        "    color: white;"
        "    font-size: 16px;"
        "    font-weight: bold;"
        "}"
        "QPushButton {"
        "    background-color: #ff4444;"
        "    color: white;"
        "    border: none;"
        "    padding: 10px 20px;"
        "    font-size: 14px;"
        "    font-weight: bold;"
        "    border-radius: 5px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #ff6666;"
        "}"
    );
    
    // Create layout
    QVBoxLayout *popupLayout = new QVBoxLayout(m_guidancePopup);
    popupLayout->setAlignment(Qt::AlignCenter);
    popupLayout->setSpacing(20);
    
    // Title
    m_guidancePopupTitle = new QLabel("NAVIGATION GUIDANCE ACTIVE", m_guidancePopup);
    m_guidancePopupTitle->setAlignment(Qt::AlignCenter);
    m_guidancePopupTitle->setStyleSheet("font-size: 20px; color: #00ff00;");
    
    // Status
    m_guidancePopupStatus = new QLabel("Following route from start to destination...", m_guidancePopup);
    m_guidancePopupStatus->setAlignment(Qt::AlignCenter);
    
    // Stop button
    m_guidancePopupStopButton = new QPushButton("STOP GUIDANCE", m_guidancePopup);
    m_guidancePopupStopButton->setFixedSize(200, 50);
    
    // Add to layout
    popupLayout->addWidget(m_guidancePopupTitle);
    popupLayout->addWidget(m_guidancePopupStatus);
    popupLayout->addWidget(m_guidancePopupStopButton);
    
    // Connect stop button
    connect(m_guidancePopupStopButton, &QPushButton::clicked, this, &NavigationMainWindow::onStopGuidance);
    
    // Initially hidden
    m_guidancePopup->hide();
}

void NavigationMainWindow::showGuidancePopup()
{
    if (!m_guidancePopup) return;
    
    // Position popup in center of main window
    QSize windowSize = this->size();
    QSize popupSize(350, 200);
    
    int x = (windowSize.width() - popupSize.width()) / 2;
    int y = (windowSize.height() - popupSize.height()) / 2;
    
    m_guidancePopup->setGeometry(x, y, popupSize.width(), popupSize.height());
    m_guidancePopup->raise();
    m_guidancePopup->show();
}

void NavigationMainWindow::hideGuidancePopup()
{
    if (m_guidancePopup) {
        m_guidancePopup->hide();
    }
}

// Slot implementations - Route calculation functionality
void NavigationMainWindow::onCalculateRoute()
{
    // Check if guidance is running and stop it
    checkAndStopGuidance();
    // Get coordinates from input fields
    bool ok1, ok2, ok3, ok4;
    double startLat = m_startLatEdit->text().toDouble(&ok1);
    double startLon = m_startLonEdit->text().toDouble(&ok2);
    double endLat = m_endLatEdit->text().toDouble(&ok3);
    double endLon = m_endLonEdit->text().toDouble(&ok4);
    
    if (!ok1 || !ok2 || !ok3 || !ok4) {
        statusBar()->showMessage("Error: Invalid coordinates format", 5000);
        return;
    }
    
    // Validate coordinate ranges
    if (startLat < -90 || startLat > 90 || endLat < -90 || endLat > 90 ||
        startLon < -180 || startLon > 180 || endLon < -180 || endLon > 180) {
        statusBar()->showMessage("Error: Coordinates out of valid range", 5000);
        return;
    }
    
    // Store route points
    m_startPoint = Point(startLat, startLon);
    m_endPoint = Point(endLat, endLon);
    m_hasStartPoint = true;
    m_hasEndPoint = true;
    
    // Check if integrated navigation controller is ready
    if (!m_navController || !m_navController->areServicesReady()) {
        statusBar()->showMessage("ERROR: Navigation services are not ready!", 10000);
        QMessageBox::critical(this, "Service Error", 
            "Navigation Services Not Ready!\n\n"
            "The navigation services are required for route calculation but are not initialized.\n"
            "Please ensure all navigation services are started before calculating routes.\n\n"
            "Route calculation cannot proceed without the services.");
        m_calculateButton->setEnabled(true);
        return;
    }
    
    // Use integrated navigation controller for route calculation
    statusBar()->showMessage("Calculating route using integrated navigation controller...", 0);
    m_calculateButton->setEnabled(false);
    
    // Calculate route with integrated controller
    if (!m_navController->calculateRouteAsync(m_startPoint, m_endPoint, RoutingCriteria::SHORTEST_TIME)) {
        statusBar()->showMessage("Failed to start route calculation", 5000);
        m_calculateButton->setEnabled(true);
    }
}

void NavigationMainWindow::onClearRoute()
{
    // Check if guidance is running and stop it
    checkAndStopGuidance();
    
    // Reset route state
    m_hasStartPoint = false;
    m_hasEndPoint = false;
    
    // Clear route using integrated controller
    if (m_navController) {
        m_navController->clearRoute();
    }
    
    // Clear route information
    if (m_totalDistanceLabel) m_totalDistanceLabel->setText("Total Distance: --");
    if (m_totalTimeLabel) m_totalTimeLabel->setText("Estimated Time: --");
    if (m_remainingDistanceLabel) m_remainingDistanceLabel->setText("Remaining: --");
    
    // Clear route from map
    MapWidget* mapWidget = qobject_cast<MapWidget*>(m_mapRenderer);
    if (mapWidget) {
        mapWidget->clearRoute();
        mapWidget->clearStartPoint();
        mapWidget->clearEndPoint();
        qDebug() << "Route cleared from map";
    }
    
    // Re-enable calculate button in case it was disabled
    if (m_calculateButton) m_calculateButton->setEnabled(true);
    
    statusBar()->showMessage("Route cleared", 3000);
    qDebug() << "Route cleared";
}

void NavigationMainWindow::onStartSimulation()
{
    if (!m_navController || !m_navController->areServicesReady()) {
        statusBar()->showMessage("Cannot start simulation - services not ready", 5000);
        return;
    }
    
    if (!m_navController->hasActiveRoute()) {
        statusBar()->showMessage("Cannot start simulation - no route calculated", 5000);
        return;
    }
    
    m_navController->startNavigation();
    m_simulationRunning = true;
    
    if (m_startSimButton) m_startSimButton->setEnabled(false);
    if (m_stopSimButton) m_stopSimButton->setEnabled(true);
    
    statusBar()->showMessage("Navigation started", 3000);
    qDebug() << "Navigation simulation started";
}

void NavigationMainWindow::onStopSimulation()
{
    if (m_navController) {
        m_navController->stopNavigation();
    }
    
    m_simulationRunning = false;
    
    if (m_startSimButton) m_startSimButton->setEnabled(true);
    if (m_stopSimButton) m_stopSimButton->setEnabled(false);
    
    statusBar()->showMessage("Navigation stopped", 3000);
    qDebug() << "Navigation simulation stopped";
}

void NavigationMainWindow::onPositionChanged(const Point& position, double heading, double speed)
{
    if (m_latitudeLabel) {
        m_latitudeLabel->setText(QString("Latitude: %1").arg(position.latitude, 0, 'f', 6));
    }
    if (m_longitudeLabel) {
        m_longitudeLabel->setText(QString("Longitude: %1").arg(position.longitude, 0, 'f', 6));
    }
    if (m_speedValueLabel) {
        m_speedValueLabel->setText(QString("Speed: %1 km/h").arg(speed, 0, 'f', 1));
    }
    if (m_headingLabel) {
        m_headingLabel->setText(QString("Heading: %1°").arg(heading, 0, 'f', 1));
    }
}

void NavigationMainWindow::onRouteCalculated(const Route& route)
{
    // Re-enable calculate button
    m_calculateButton->setEnabled(true);
    
    if (route.node_count == 0) {
        statusBar()->showMessage("Error: Route calculation returned empty route", 5000);
        return;
    }
    
    // For simple display, create a straight line from start to end point
    std::vector<Point> routePoints;
    routePoints.push_back(m_startPoint);
    
    // Add intermediate points for smoother line visualization
    for (int i = 1; i <= 10; ++i) {
        double ratio = i / 10.0;
        double lat = m_startPoint.latitude + (m_endPoint.latitude - m_startPoint.latitude) * ratio;
        double lon = m_startPoint.longitude + (m_endPoint.longitude - m_startPoint.longitude) * ratio;
        routePoints.push_back(Point(lat, lon));
    }
    
    // Display route on map
    MapWidget* mapWidget = qobject_cast<MapWidget*>(m_mapRenderer);
    if (mapWidget) {
        // Set start and end points
        mapWidget->setStartPoint(m_startPoint);
        mapWidget->setEndPoint(m_endPoint);
        
        // Set the calculated route
        mapWidget->setRoute(routePoints);
        
        // Center map to show route
        double centerLat = (m_startPoint.latitude + m_endPoint.latitude) / 2.0;
        double centerLon = (m_startPoint.longitude + m_endPoint.longitude) / 2.0;
        mapWidget->centerMap(centerLat, centerLon);
        
        qDebug() << "Route displayed on map with" << routePoints.size() << "points";
    }
    
    // Use route statistics from the Route object
    double totalDistance = route.total_distance_meters / 1000.0; // Convert meters to km
    double estimatedTime = route.estimated_time_seconds / 3600.0; // Convert seconds to hours
    
    // Update UI
    m_totalDistanceLabel->setText(QString("Total Distance: %1 km").arg(totalDistance, 0, 'f', 2));
    m_totalTimeLabel->setText(QString("Estimated Time: %1 hours").arg(estimatedTime, 0, 'f', 1));
    m_remainingDistanceLabel->setText(QString("Remaining: %1 km").arg(totalDistance, 0, 'f', 2));
    
    statusBar()->showMessage(QString("Route calculated: %1 km, %2 nodes")
                           .arg(totalDistance, 0, 'f', 2)
                           .arg(route.node_count), 10000);
    
    qDebug() << "Route calculated:" << totalDistance << "km with" << route.node_count << "nodes";
}

void NavigationMainWindow::onGuidanceUpdated(const GuidanceInstruction& instruction)
{
    if (m_instructionLabel) {
        m_instructionLabel->setText(QString("Instruction: %1").arg(QString::fromStdString(instruction.instruction_text)));
    }
    if (m_distanceLabel) {
        m_distanceLabel->setText(QString("Distance: %1 m").arg(instruction.distance_to_turn_meters));
    }
}

void NavigationMainWindow::setupManualControlPanel(QVBoxLayout* parentLayout)
{
    m_manualControlGroup = new QGroupBox("Manual Control - Speed & Heading");
    QVBoxLayout *layout = new QVBoxLayout(m_manualControlGroup);
    
    // Speed Control
    QHBoxLayout *speedLayout = new QHBoxLayout();
    speedLayout->addWidget(new QLabel("Speed (km/h):"));
    m_manualSpeedSlider = new QSlider(Qt::Horizontal);
    m_manualSpeedSlider->setRange(0, 120);
    m_manualSpeedSlider->setValue(static_cast<int>(m_manualSpeed));
    m_manualSpeedSlider->setTickPosition(QSlider::TicksBelow);
    m_manualSpeedSlider->setTickInterval(20);
    m_manualSpeedLabel = new QLabel(QString::number(m_manualSpeed, 'f', 1));
    speedLayout->addWidget(m_manualSpeedSlider);
    speedLayout->addWidget(m_manualSpeedLabel);
    layout->addLayout(speedLayout);
    
    // Heading Control
    QHBoxLayout *headingLayout = new QHBoxLayout();
    headingLayout->addWidget(new QLabel("Heading (degrees):"));
    m_manualHeadingSlider = new QSlider(Qt::Horizontal);
    m_manualHeadingSlider->setRange(0, 359);
    m_manualHeadingSlider->setValue(static_cast<int>(m_manualHeading));
    m_manualHeadingSlider->setTickPosition(QSlider::TicksBelow);
    m_manualHeadingSlider->setTickInterval(45);
    m_manualHeadingLabel = new QLabel(QString::number(m_manualHeading, 'f', 1));
    headingLayout->addWidget(m_manualHeadingSlider);
    headingLayout->addWidget(m_manualHeadingLabel);
    layout->addLayout(headingLayout);
    
    // Connect signals
    connect(m_manualSpeedSlider, &QSlider::valueChanged, this, &NavigationMainWindow::onSpeedChanged);
    connect(m_manualHeadingSlider, &QSlider::valueChanged, this, &NavigationMainWindow::onHeadingChanged);
    
    if (parentLayout) {
        parentLayout->addWidget(m_manualControlGroup);
    }
}

void NavigationMainWindow::setupGuidanceControlPanel(QVBoxLayout* parentLayout)
{
    m_guidanceControlGroup = new QGroupBox("Navigation Guidance");
    QVBoxLayout *layout = new QVBoxLayout(m_guidanceControlGroup);
    
    // Auto heading checkbox
    m_autoHeadingCheckBox = new QCheckBox("Auto-heading towards destination");
    m_autoHeadingCheckBox->setChecked(m_autoHeadingEnabled);
    layout->addWidget(m_autoHeadingCheckBox);
    
    // Control buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    m_startGuidanceButton = new QPushButton("Start Guidance");
    m_stopGuidanceButton = new QPushButton("Stop Guidance");
    m_stopGuidanceButton->setEnabled(false);
    buttonLayout->addWidget(m_startGuidanceButton);
    buttonLayout->addWidget(m_stopGuidanceButton);
    layout->addLayout(buttonLayout);
    
    // Status label
    m_guidanceStatusLabel = new QLabel("Guidance Status: Stopped");
    m_guidanceStatusLabel->setStyleSheet("QLabel { color: red; }");
    layout->addWidget(m_guidanceStatusLabel);
    
    // Connect signals
    connect(m_autoHeadingCheckBox, &QCheckBox::toggled, this, &NavigationMainWindow::onAutoHeadingToggled);
    connect(m_startGuidanceButton, &QPushButton::clicked, this, &NavigationMainWindow::onStartGuidance);
    connect(m_stopGuidanceButton, &QPushButton::clicked, this, &NavigationMainWindow::onStopGuidance);
    
    if (parentLayout) {
        parentLayout->addWidget(m_guidanceControlGroup);
    }
}

void NavigationMainWindow::onMapClicked(const Point& position)
{
    qDebug() << "Map clicked at" << position.latitude << "," << position.longitude;
    
    // Store the clicked position for later use
    m_lastClickedPosition = position;
    m_hasLastClickedPosition = true;
    
    // Show clicked point on map
    m_mapRenderer->setClickedPoint(position);
    
    // Create context menu with coordinate and options
    QMenu contextMenu(this);
    
    // Add coordinate info
    contextMenu.addAction(QString("Coordinates: %1, %2")
                          .arg(position.latitude, 0, 'f', 6)
                          .arg(position.longitude, 0, 'f', 6))->setEnabled(false);
    contextMenu.addSeparator();
    
    // Add options
    QAction *setStartAction = contextMenu.addAction("Set as Start Point");
    QAction *setEndAction = contextMenu.addAction("Set as End Point");
    
    // Connect actions
    connect(setStartAction, &QAction::triggered, this, &NavigationMainWindow::onSetClickedAsStartPoint);
    connect(setEndAction, &QAction::triggered, this, &NavigationMainWindow::onSetClickedAsEndPoint);
    
    // Show context menu at cursor position
    contextMenu.exec(QCursor::pos());
}

// Thêm method này sau setupGuidanceControlPanel:

void NavigationMainWindow::setupPOITestPanel(QVBoxLayout* parentLayout)
{
    m_poiTestGroup = new QGroupBox("POI Service Testing Panel");
    m_poiTestGroup->setStyleSheet(
        "QGroupBox { font-weight: bold; color: #2E8B57; }"
        "QGroupBox::title { color: #2E8B57; }"
    );
    QVBoxLayout *layout = new QVBoxLayout(m_poiTestGroup);
    
    // POI Service Status
    m_poiStatusLabel = new QLabel("POI Service Status: Initializing...");
    m_poiStatusLabel->setStyleSheet("QLabel { color: #FF6600; font-weight: bold; }");
    layout->addWidget(m_poiStatusLabel);
    
    // Load POI Data Section
    QHBoxLayout *loadLayout = new QHBoxLayout();
    loadLayout->addWidget(new QLabel("Data Management:"));
    m_poiLoadDataButton = new QPushButton("Load Sample POI Data");
    m_poiLoadDataButton->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; font-weight: bold; }");
    loadLayout->addWidget(m_poiLoadDataButton);
    layout->addLayout(loadLayout);
    
    // POI Search by Name Section
    QHBoxLayout *searchLayout = new QHBoxLayout();
    searchLayout->addWidget(new QLabel("Search POI:"));
    m_poiSearchEdit = new QLineEdit();
    m_poiSearchEdit->setPlaceholderText("Enter POI name (e.g., 'Pho 24', 'Shell')");
    m_poiSearchEdit->setMinimumWidth(600);  // Chiều rộng tối thiểu 300 pixels
    m_poiSearchEdit->setStyleSheet(
        "QLineEdit { "
        "   background-color: white; "
        "   color: black; "
        "   border: 1px solid #ccc; "
        "   padding: 5px; "
        "}"
    );
    m_poiSearchButton = new QPushButton("Search by Name");
    searchLayout->addWidget(m_poiSearchEdit);  // Stretch factor 4 để chiếm nhiều không gian
    searchLayout->addWidget(m_poiSearchButton);
    layout->addLayout(searchLayout);
    
    // POI Nearby Search Section
    QHBoxLayout *nearbyLayout = new QHBoxLayout();
    nearbyLayout->addWidget(new QLabel("Category:"));
    m_poiCategoryCombo = new QComboBox();
    m_poiCategoryCombo->addItems({
        "All Categories", "gas_station", "restaurant", "hospital", "bank", "parking"
    });
    nearbyLayout->addWidget(m_poiCategoryCombo);
    
    nearbyLayout->addWidget(new QLabel("Radius (km):"));
    m_poiRadiusSpinBox = new QSpinBox();
    m_poiRadiusSpinBox->setRange(1, 50);
    m_poiRadiusSpinBox->setValue(5);
    m_poiRadiusSpinBox->setSuffix(" km");
    nearbyLayout->addWidget(m_poiRadiusSpinBox);
    
    m_poiNearbyButton = new QPushButton("Find Nearby");
    m_poiNearbyButton->setStyleSheet("QPushButton { background-color: #2196F3; color: white; }");
    nearbyLayout->addWidget(m_poiNearbyButton);
    layout->addLayout(nearbyLayout);

    // Needtodo
    
    // Address Geocoding Section
    QHBoxLayout *geocodeLayout = new QHBoxLayout();
    geocodeLayout->addWidget(new QLabel("Address:"));
    m_addressEdit = new QLineEdit();
    m_addressEdit->setPlaceholderText("Enter address to geocode");
    m_addressEdit->setStyleSheet(
        "QLineEdit { "
        "   background-color: white; "
        "   color: black; "
        "   border: 1px solid #ccc; "
        "   padding: 5px; "
        "}"
    );
    m_geocodeButton = new QPushButton("🏠 Geocode Address");
    m_geocodeButton->setStyleSheet("QPushButton { background-color: #FF9800; color: white; }");
    geocodeLayout->addWidget(m_addressEdit);
    geocodeLayout->addWidget(m_geocodeButton);
    layout->addLayout(geocodeLayout);
    
    // Note: Results are now displayed in Information panel (right column)
    
    // Connect signals
    connect(m_poiLoadDataButton, &QPushButton::clicked, this, &NavigationMainWindow::onPOILoadData);
    connect(m_poiSearchButton, &QPushButton::clicked, this, &NavigationMainWindow::onPOISearch);
    connect(m_poiNearbyButton, &QPushButton::clicked, this, &NavigationMainWindow::onPOINearbySearch);
    connect(m_geocodeButton, &QPushButton::clicked, this, &NavigationMainWindow::onGeocodeAddress);
    connect(m_poiSearchEdit, &QLineEdit::returnPressed, this, &NavigationMainWindow::onPOISearch);
    connect(m_addressEdit, &QLineEdit::returnPressed, this, &NavigationMainWindow::onGeocodeAddress);
    
    // Update POI service status
    if (m_poiService && m_poiService->isServiceReady()) {
        m_poiStatusLabel->setText("POI Service Status: Ready");
        m_poiStatusLabel->setStyleSheet("QLabel { color: #2E8B57; font-weight: bold; }");
    } else {
        m_poiStatusLabel->setText("POI Service Status: Not Ready");
        m_poiStatusLabel->setStyleSheet("QLabel { color: #DC143C; font-weight: bold; }");
    }
    
    if (parentLayout) {
        parentLayout->addWidget(m_poiTestGroup);
    }
}

void NavigationMainWindow::onSimulationSpeedChanged(int speed)
{
    qDebug() << "Simulation speed changed to" << speed;
}

void NavigationMainWindow::onSetStartPoint()
{
    qDebug() << "Set start point requested";
}

void NavigationMainWindow::onSetEndPoint()
{
    qDebug() << "Set end point requested";
}

void NavigationMainWindow::updateStatusBar()
{
    statusBar()->showMessage(QString("Last updated: %1").arg(QDateTime::currentDateTime().toString()));
}

void NavigationMainWindow::updatePositionDisplay()
{
    // Implementation for updating position display
}

void NavigationMainWindow::updateGuidanceDisplay()
{
    // Implementation for updating guidance display
}

void NavigationMainWindow::updateRouteInfo()
{
    // Implementation for updating route info
}

void NavigationMainWindow::updateServiceStatus()
{
    // Implementation for updating service status
}

void NavigationMainWindow::onServiceModeChanged()
{
    qDebug() << "Service mode changed";
}

// Helper function to calculate distance between two coordinates (Haversine formula)
double NavigationMainWindow::calculateDistance(double lat1, double lon1, double lat2, double lon2)
{
    const double R = 6371.0; // Earth's radius in kilometers
    
    // Convert degrees to radians
    double lat1_rad = lat1 * M_PI / 180.0;
    double lon1_rad = lon1 * M_PI / 180.0;
    double lat2_rad = lat2 * M_PI / 180.0;
    double lon2_rad = lon2 * M_PI / 180.0;
    
    // Haversine formula
    double dlat = lat2_rad - lat1_rad;
    double dlon = lon2_rad - lon1_rad;
    
    double a = sin(dlat/2) * sin(dlat/2) + 
               cos(lat1_rad) * cos(lat2_rad) * 
               sin(dlon/2) * sin(dlon/2);
    double c = 2 * atan2(sqrt(a), sqrt(1-a));
    
    return R * c; // Distance in kilometers
}

double NavigationMainWindow::calculateBearing(double lat1, double lon1, double lat2, double lon2)
{
    // Convert degrees to radians
    double lat1_rad = lat1 * M_PI / 180.0;
    double lon1_rad = lon1 * M_PI / 180.0;
    double lat2_rad = lat2 * M_PI / 180.0;
    double lon2_rad = lon2 * M_PI / 180.0;
    
    double dlon = lon2_rad - lon1_rad;
    
    double y = sin(dlon) * cos(lat2_rad);
    double x = cos(lat1_rad) * sin(lat2_rad) - sin(lat1_rad) * cos(lat2_rad) * cos(dlon);
    
    double bearing = atan2(y, x);
    
    // Convert from radians to degrees
    bearing = bearing * 180.0 / M_PI;
    
    // Normalize to 0-360 degrees
    bearing = fmod((bearing + 360.0), 360.0);
    
    return bearing;
}

bool NavigationMainWindow::checkDestinationReached()
{
    // Calculate current distance to destination
    double currentDistanceToEnd = calculateDistance(m_currentPosition.latitude, m_currentPosition.longitude,
                                                  m_endPoint.latitude, m_endPoint.longitude);
    
    // Dynamic detection radius based on speed (larger radius for higher speed)
    double detectionRadius = 0.01; // Base 10 meters in km
    if (m_manualSpeed > 30) {
        detectionRadius = 0.03; // 30m for speed > 30 km/h
    } else if (m_manualSpeed > 60) {
        detectionRadius = 0.05; // 50m for speed > 60 km/h
    } else if (m_manualSpeed > 100) {
        detectionRadius = 0.08; // 80m for speed > 100 km/h
    }
    
    // Method 1: Simple distance check
    if (currentDistanceToEnd < detectionRadius) {
        qDebug() << "Destination reached by distance check:" << currentDistanceToEnd << "km";
        return true;
    }
    
    // Method 2: Check if we crossed over destination (went past it)
    if (m_hasPreviousPosition) {
        // Check if distance was decreasing but now increasing (passed the destination)
        if (m_previousDistanceToEnd > currentDistanceToEnd) {
            // We were getting closer
        } else if (m_previousDistanceToEnd < currentDistanceToEnd && m_previousDistanceToEnd < detectionRadius * 2) {
            // We were close and now getting farther - we might have passed it
            qDebug() << "Destination reached by crossover detection. Previous:" << m_previousDistanceToEnd 
                     << "Current:" << currentDistanceToEnd;
            return true;
        }
        
        // Method 3: Line segment intersection with destination circle
        // Check if the line from previous to current position intersects with destination circle
        double dx = m_currentPosition.longitude - m_previousPosition.longitude;
        double dy = m_currentPosition.latitude - m_previousPosition.latitude;
        double segmentLength = sqrt(dx*dx + dy*dy);
        
        if (segmentLength > 0) {
            // Vector from previous position to destination
            double fx = m_endPoint.longitude - m_previousPosition.longitude;
            double fy = m_endPoint.latitude - m_previousPosition.latitude;
            
            // Project destination onto movement line
            double t = (fx*dx + fy*dy) / (dx*dx + dy*dy);
            t = std::max(0.0, std::min(1.0, t)); // Clamp to [0,1]
            
            // Closest point on line segment to destination
            double closestX = m_previousPosition.longitude + t*dx;
            double closestY = m_previousPosition.latitude + t*dy;
            
            // Distance from destination to closest point on path
            double distanceToPath = calculateDistance(m_endPoint.latitude, m_endPoint.longitude,
                                                     closestY, closestX);
            
            if (distanceToPath < detectionRadius) {
                qDebug() << "Destination reached by path intersection. Distance to path:" << distanceToPath;
                return true;
            }
        }
    }
    
    // Update tracking for next iteration
    m_previousPosition = m_currentPosition;
    m_hasPreviousPosition = true;
    m_previousDistanceToEnd = currentDistanceToEnd;
    
    return false;
}

void NavigationMainWindow::onGetCurrentPosition()
{
    // Get current position - use simulation position if available, otherwise default
    double currentLat, currentLon;
    
    if (m_hasCurrentPosition) {
        // Use current simulation position
        currentLat = m_currentPosition.latitude;
        currentLon = m_currentPosition.longitude;
    } else {
        // Use default position (Hanoi coordinates)
        currentLat = getDefaultLat();
        currentLon = getDefaultLon();
        
        // Set current position for the first time
        m_currentPosition = Point(currentLat, currentLon);
        m_hasCurrentPosition = true;
    }
    
    // Update map display with current position (show C icon)
    if (m_mapRenderer) {
        m_mapRenderer->setCurrentPosition(m_currentPosition, m_manualHeading);
    }
    
    statusBar()->showMessage(QString("Current position displayed: %1, %2")
                            .arg(currentLat, 0, 'f', 6)
                            .arg(currentLon, 0, 'f', 6), 5000);
    
    qDebug() << "Current position displayed:" << currentLat << "," << currentLon;
}

void NavigationMainWindow::onSpeedChanged(int speed)
{
    m_manualSpeed = static_cast<double>(speed);
    m_manualSpeedLabel->setText(QString::number(m_manualSpeed, 'f', 1));
    qDebug() << "Manual speed changed to: " << m_manualSpeed << " km/h";
    
    // If guidance is running, this speed will be used for simulation
    if (m_guidanceRunning) {
        // Update navigation controller with new speed
        qDebug() << "Updating guidance speed to: " << m_manualSpeed;
    }
}

void NavigationMainWindow::onHeadingChanged(int heading)
{
    m_manualHeading = static_cast<double>(heading);
    m_manualHeadingLabel->setText(QString::number(m_manualHeading, 'f', 1));
    qDebug() << "Manual heading changed to: " << m_manualHeading << " degrees";
    
    // If guidance is running and auto-heading is disabled, use manual heading
    if (m_guidanceRunning && !m_autoHeadingEnabled) {
        qDebug() << "Updating guidance heading to: " << m_manualHeading;
    }
}

void NavigationMainWindow::onStartGuidance()
{
    if (!m_hasStartPoint || !m_hasEndPoint) {
        QMessageBox::warning(this, "Warning", "Please set both start and end points before starting guidance.");
        return;
    }
    
    qDebug() << "Starting navigation guidance";
    m_guidanceRunning = true;
    
    // ALWAYS start guidance from Start Position (follow route)
    m_currentPosition = m_startPoint;
    m_hasCurrentPosition = true;
    qDebug() << "Starting guidance from start position:" << m_currentPosition.latitude << "," << m_currentPosition.longitude;
    
    // Reset destination detection tracking
    m_hasPreviousPosition = false;
    m_previousDistanceToEnd = 0.0;
    
    // Enable auto-heading by default when starting guidance
    m_autoHeadingEnabled = true;
    m_autoHeadingCheckBox->setChecked(true);
    
    // Calculate initial heading towards destination
    double initialHeading = calculateBearing(m_currentPosition.latitude, m_currentPosition.longitude,
                                           m_endPoint.latitude, m_endPoint.longitude);
    m_manualHeading = initialHeading;
    m_manualHeadingSlider->setValue(static_cast<int>(initialHeading));
    m_manualHeadingLabel->setText(QString::number(initialHeading, 'f', 1));
    
    // Update map with current position and correct heading
    m_mapRenderer->setCurrentPosition(m_currentPosition, initialHeading);
    
    // Start simulation timer
    m_simulationTimer->start();
    
    // Update UI
    m_startGuidanceButton->setEnabled(false);
    m_stopGuidanceButton->setEnabled(true);
    m_guidanceStatusLabel->setText("Guidance Status: Running");
    m_guidanceStatusLabel->setStyleSheet("QLabel { color: green; }");
    
    // Disable other controls during guidance
    setControlsEnabled(false);
    
    // Start guidance with current speed
    qDebug() << "Guidance started with speed:" << m_manualSpeed << "km/h, auto-heading:" << m_autoHeadingEnabled;
    
    // Show guidance popup overlay
    showGuidancePopup();
    
    statusBar()->showMessage("Navigation guidance started - other controls disabled", 3000);
}

void NavigationMainWindow::onStopGuidance()
{
    qDebug() << "Stopping navigation guidance";
    m_guidanceRunning = false;
    
    // Stop simulation timer
    m_simulationTimer->stop();
    
    // Update UI
    m_startGuidanceButton->setEnabled(true);
    m_stopGuidanceButton->setEnabled(false);
    m_guidanceStatusLabel->setText("Guidance Status: Stopped");
    m_guidanceStatusLabel->setStyleSheet("QLabel { color: red; }");
    
    // Re-enable other controls
    setControlsEnabled(true);
    
    // Hide guidance popup overlay
    hideGuidancePopup();
    
    statusBar()->showMessage("Navigation guidance stopped - controls re-enabled", 3000);
}

void NavigationMainWindow::onAutoHeadingToggled(bool enabled)
{
    qDebug() << "Auto heading toggled: " << (enabled ? "enabled" : "disabled");
    m_autoHeadingEnabled = enabled;
    
    if (m_guidanceRunning) {
        if (enabled) {
            qDebug() << "Switching to auto-heading mode during guidance";
        } else {
            qDebug() << "Switching to manual heading mode during guidance - using current heading:" << m_manualHeading;
        }
    }
}

void NavigationMainWindow::onPOILoadData()
{
    if (!m_poiService || !m_poiService->isServiceReady()) {
        m_poiResultsText->setPlainText("ERROR: POI Service not ready!\n\nCannot load POI data.");
        return;
    }
    
    m_poiResultsText->setPlainText("POI Service is ready!\n\nPOI data has been loaded during service initialization.");
    m_poiLoadDataButton->setEnabled(true);
    
    // Update status
    m_poiStatusLabel->setText("POI Service Status: Ready");
    m_poiStatusLabel->setStyleSheet("QLabel { color: #2E8B57; font-weight: bold; }");
}

void NavigationMainWindow::onPOISearch()
{
    if (!m_poiService || !m_poiService->isServiceReady()) {
        m_poiResultsText->setPlainText("ERROR: POI Service not ready!");
        return;
    }
    
    QString searchName = m_poiSearchEdit->text().trimmed();
    if (searchName.isEmpty()) {
        m_poiResultsText->setPlainText("WARNING: Please enter a POI name to search for.");
        return;
    }
    
    m_poiResultsText->setPlainText(QString("Searching for POIs with name: '%1'...\n").arg(searchName));
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Perform POI search by name
    std::vector<POI> results = m_poiService->searchPOIsByName(searchName);
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    double queryTimeMs = duration.count() / 1000.0;
    
    // Display results
    QString resultText = QString(
        "POI NAME SEARCH RESULTS\n"
        "========================\n"
        "Search term: '%1'\n"
        "Results found: %2\n"
        "Query time: %3 ms\n\n"
    ).arg(searchName).arg(results.size()).arg(queryTimeMs, 0, 'f', 3);
    
    // Store results for later use
    m_lastSearchResults = results;
    
    // Clear and populate list widget
    m_poiResultsList->clear();
    
    if (results.empty()) {
        resultText += "No POIs found matching your search.\n\n";
        resultText += "Try searching for:\n";
        resultText += "- 'Pho' - Vietnamese restaurants\n";
        resultText += "- 'Shell' - Gas stations\n";
        resultText += "- 'Hospital' - Medical facilities\n";
        resultText += "- 'Bank' - Financial services\n";
    } else {
        resultText += QString("Found %1 POIs - Click below to view on map").arg(results.size());
        
        // Add to list widget
        for (size_t i = 0; i < results.size(); ++i) {
            const POI& poi = results[i];
            QString listItem = QString("%1 (%2)")
                .arg(QString::fromStdString(poi.name))
                .arg(QString::fromStdString(poi.category));
            m_poiResultsList->addItem(listItem);
        }
    }
    
    m_poiResultsText->setPlainText(resultText);
}

void NavigationMainWindow::onPOINearbySearch()
{
    if (!m_poiService || !m_poiService->isServiceReady()) {
        m_poiResultsText->setPlainText("ERROR: POI Service not ready!");
        return;
    }
    
    // Get search parameters
    QString category = m_poiCategoryCombo->currentText();
    double radiusKm = m_poiRadiusSpinBox->value();
    
    // Use current position or default position
    double searchLat = m_hasCurrentPosition ? m_currentPosition.latitude : getDefaultLat();
    double searchLon = m_hasCurrentPosition ? m_currentPosition.longitude : getDefaultLon();
    
    m_poiResultsText->setPlainText(QString(
        "Searching for nearby POIs...\n"
        "Center: (%1, %2)\n"
        "Category: %3\n"
        "Radius: %4 km\n"
    ).arg(searchLat, 0, 'f', 6).arg(searchLon, 0, 'f', 6).arg(category).arg(radiusKm));
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Perform proximity search
    Point searchPoint{searchLat, searchLon};
    double radiusMeters = radiusKm * 1000.0;
    QString categoryFilter = (category == "All Categories") ? QString() : category;
    std::vector<POI> results = m_poiService->findNearbyPOIs(searchPoint, radiusMeters, categoryFilter);
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    double queryTimeMs = duration.count() / 1000.0;
    
    // Display results with performance metrics
    QString resultText = QString(
        "PROXIMITY SEARCH RESULTS\n"
        "===========================\n"
        "Search center: (%1, %2)\n"
        "Category filter: %3\n"
        "Search radius: %4 km\n"
        "Results found: %5\n"
        "Query time: %6 ms (Spatial Grid Indexing)\n"
        "Grid cells queried: ~9 (3x3 neighborhood)\n\n"
    ).arg(searchLat, 0, 'f', 6)
     .arg(searchLon, 0, 'f', 6)
     .arg(category)
     .arg(radiusKm)
     .arg(results.size())
     .arg(queryTimeMs, 0, 'f', 3);
    
    // Store results for later use
    m_lastSearchResults = results;
    
    // Clear and populate list widget
    m_poiResultsList->clear();
    
    if (results.empty()) {
        resultText += "No POIs found in the specified area.\n\n";
        resultText += "Try:\n";
        resultText += "- Increasing the search radius\n";
        resultText += "- Selecting 'All Categories'\n";
        resultText += "- Moving to a different location\n";
    } else {
        resultText += QString("Found %1 POIs - Click below to view on map").arg(results.size());
        
        // Add to list widget with distance
        for (size_t i = 0; i < results.size(); ++i) {
            const POI& poi = results[i];
            
            // Calculate distance from search center
            double distance = calculateDistance(searchLat, searchLon, poi.latitude, poi.longitude);
            
            QString listItem = QString("%1 - %2 km (%3)")
                .arg(QString::fromStdString(poi.name))
                .arg(distance, 0, 'f', 2)
                .arg(QString::fromStdString(poi.category));
            m_poiResultsList->addItem(listItem);
        }
        
        resultText += QString("\n⚡ Query time: %1 ms").arg(queryTimeMs, 0, 'f', 3);
    }
    
    m_poiResultsText->setPlainText(resultText);
}

void NavigationMainWindow::onGeocodeAddress()
{
    if (!m_poiService || !m_poiService->isServiceReady()) {
        m_poiResultsText->setPlainText("ERROR: POI Service not ready!");
        return;
    }
    
    QString address = m_addressEdit->text().trimmed();
    if (address.isEmpty()) {
        m_poiResultsText->setPlainText("WARNING: Please enter an address to geocode.");
        return;
    }
    
    m_poiResultsText->setPlainText(QString("Geocoding address: '%1'...\n").arg(address));
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
        // Use Enhanced Geocoding API (Phase 1)
        geocoding::EnhancedAddressRequest enhancedRequest;
        enhancedRequest.freeform_address = address.toStdString();  // Support freeform text!
        enhancedRequest.country_hint = "VN";
        enhancedRequest.max_results = 5;  // Get multiple candidates
        enhancedRequest.min_confidence_threshold = 0.5;  // Accept candidates with 50%+ confidence    // Perform enhanced geocoding
    geocoding::EnhancedGeocodingResult enhancedResult = m_poiService->geocodeAddressEnhanced(enhancedRequest);
    
    // Convert to legacy format for backward compatibility
    GeocodingResult result = GeocodingResult::fromEnhanced(enhancedResult);
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    double queryTimeMs = duration.count() / 1000.0;
    
    // Display enhanced geocoding results
    QString resultText = QString(
        "ENHANCED GEOCODING RESULTS (Phase 1)\n"
        "==========================================\n"
        "Input: '%1'\n"
        "Processing time: %2 ms\n"
        "Candidates evaluated: %3\n\n"
    ).arg(address)
     .arg(enhancedResult.processing_time_ms, 0, 'f', 3)
     .arg(enhancedResult.candidates_evaluated);
    
    if (enhancedResult.success) {
        const auto& primary = enhancedResult.primary_result;
        
        resultText += QString(
            "PRIMARY RESULT:\n\n"
            "Coordinates:\n"
            "   Latitude: %1\n"
            "   Longitude: %2\n"
            "   Altitude: %3 m\n\n"
            "Formatted Address:\n"
            "   %4\n\n"
            "Quality Metrics:\n"
            "   Confidence: %5%\n"
            "   Quality: %6\n"
            "   Accuracy: ±%7 m\n"
            "   Match Type: %8\n"
            "   Method: %9\n"
            "   Source: %10\n\n"
        ).arg(primary.latitude, 0, 'f', 6)
         .arg(primary.longitude, 0, 'f', 6)
         .arg(primary.altitude, 0, 'f', 1)
         .arg(QString::fromStdString(primary.formatted_address))
         .arg(primary.confidence_score * 100, 0, 'f', 1)
         .arg(QString::fromStdString(geocoding::qualityToString(primary.quality)))
         .arg(primary.accuracy_meters, 0, 'f', 1)
         .arg(QString::fromStdString(primary.match_type))
         .arg(QString::fromStdString(primary.geocoding_method))
         .arg(QString::fromStdString(primary.data_source));
        
        // Show alternative candidates if any
        if (!enhancedResult.alternatives.empty()) {
            resultText += QString("\nALTERNATIVE CANDIDATES: %1\n")
                .arg(enhancedResult.alternatives.size());
            
            int idx = 1;
            for (const auto& alt : enhancedResult.alternatives) {
                if (idx > 3) break;  // Show max 3 alternatives
                resultText += QString(
                    "\n%1. %2 (confidence: %3%)\n"
                    "   Location: %4, %5\n"
                ).arg(idx)
                 .arg(QString::fromStdString(alt.formatted_address))
                 .arg(alt.confidence_score * 100, 0, 'f', 1)
                 .arg(alt.latitude, 0, 'f', 6)
                 .arg(alt.longitude, 0, 'f', 6);
                idx++;
            }
        }
        
        resultText += "\n\nFeatures Used:\n"
                      " Freeform text parsing\n"
                      " USPS address normalization\n"
                      " Spatial index (R-tree)\n"
                      " Confidence scoring\n";
        
    } else {
        resultText += QString(
            "GEOCODING FAILED\n\n"
            "Error: %1\n\n"
            "Suggestions:\n"
            "- Try more complete address\n"
            "- Use format: '123 Main St, City, State'\n"
            "- Check spelling\n"
            "- Try known POI names\n"
        ).arg(QString::fromStdString(enhancedResult.error_message));
    }
    
    m_poiResultsText->setPlainText(resultText);
}

// POI Service Signal Handlers - TODO: Implement when POI service signals are ready
void NavigationMainWindow::onPOIDataLoaded()
{
    // TODO: POIService doesn't emit signals yet
    qDebug() << "POI data loaded callback";
}

void NavigationMainWindow::onPOISearchCompleted(const std::vector<POI>& results)
{
    qDebug() << "POI search completed:" << results.size() << "results";
}

void NavigationMainWindow::onAddressGeocoded(const GeocodingResult& result)
{
    qDebug() << "Address geocoded. Success:" << result.success;
}

void NavigationMainWindow::onSetClickedAsStartPoint()
{
    if (!m_hasLastClickedPosition) {
        return;
    }
    
    // Check if guidance is running and stop it
    checkAndStopGuidance();
    
    m_startPoint = m_lastClickedPosition;
    m_hasStartPoint = true;
    
    // Update UI
    m_startLatEdit->setText(QString::number(m_startPoint.latitude, 'f', 6));
    m_startLonEdit->setText(QString::number(m_startPoint.longitude, 'f', 6));
    
    // DON'T update current position when setting start point
    // Current position should only change during guidance simulation
    
    qDebug() << "Start point set to:" << m_startPoint.latitude << "," << m_startPoint.longitude;
    statusBar()->showMessage(QString("Start point set to: %1, %2")
                            .arg(m_startPoint.latitude, 0, 'f', 6)
                            .arg(m_startPoint.longitude, 0, 'f', 6), 3000);
    
    // Clear the clicked point marker 
    m_mapRenderer->clearClickedPoint();
}

void NavigationMainWindow::onSetClickedAsEndPoint()
{
    if (!m_hasLastClickedPosition) {
        return;
    }
    
    // Check if guidance is running and stop it
    checkAndStopGuidance();
    
    m_endPoint = m_lastClickedPosition;
    m_hasEndPoint = true;
    
    // Update UI
    m_endLatEdit->setText(QString::number(m_endPoint.latitude, 'f', 6));
    m_endLonEdit->setText(QString::number(m_endPoint.longitude, 'f', 6));
    
    qDebug() << "End point set to:" << m_endPoint.latitude << "," << m_endPoint.longitude;
    statusBar()->showMessage(QString("End point set to: %1, %2")
                            .arg(m_endPoint.latitude, 0, 'f', 6)
                            .arg(m_endPoint.longitude, 0, 'f', 6), 3000);
    
    // Clear the clicked point marker
    m_mapRenderer->clearClickedPoint();
}

void NavigationMainWindow::onSimulationTimer()
{
    if (!m_guidanceRunning || !m_hasCurrentPosition) {
        return;
    }
    
    // Calculate movement based on speed and heading
    double speedMs = m_manualSpeed / 3.6; // Convert km/h to m/s
    double timeStep = 1.0; // 1 second
    double distance = speedMs * timeStep; // Distance in meters
    
    // Get current heading (use auto-heading if enabled)
    double currentHeading = m_manualHeading;
    if (m_autoHeadingEnabled && m_hasEndPoint) {
        // Calculate bearing from current position to end point
        currentHeading = calculateBearing(m_currentPosition.latitude, m_currentPosition.longitude,
                                        m_endPoint.latitude, m_endPoint.longitude);
        
        // Update manual heading display to show auto-calculated heading
        m_manualHeading = currentHeading;
        m_manualHeadingSlider->setValue(static_cast<int>(currentHeading));
        m_manualHeadingLabel->setText(QString::number(currentHeading, 'f', 1));
    }
    
    // Calculate new position using heading and distance
    double headingRad = currentHeading * M_PI / 180.0;
    
    // Approximate latitude/longitude change (for small distances)
    const double EARTH_RADIUS = 6371000.0; // meters
    double deltaLat = (distance * cos(headingRad)) / EARTH_RADIUS * (180.0 / M_PI);
    double deltaLon = (distance * sin(headingRad)) / (EARTH_RADIUS * cos(m_currentPosition.latitude * M_PI / 180.0)) * (180.0 / M_PI);
    
    // Update current position
    m_currentPosition.latitude += deltaLat;
    m_currentPosition.longitude += deltaLon;
    
    // Update map with new position
    m_mapRenderer->setCurrentPosition(m_currentPosition, currentHeading);
    
    // Update information panel with current position and speed
    if (m_latitudeLabel) {
        m_latitudeLabel->setText(QString("Latitude: %1").arg(m_currentPosition.latitude, 0, 'f', 6));
    }
    if (m_longitudeLabel) {
        m_longitudeLabel->setText(QString("Longitude: %1").arg(m_currentPosition.longitude, 0, 'f', 6));
    }
    if (m_speedValueLabel) {
        m_speedValueLabel->setText(QString("Speed: %1 km/h").arg(m_manualSpeed, 0, 'f', 1));
    }
    if (m_headingLabel) {
        m_headingLabel->setText(QString("Heading: %1°").arg(currentHeading, 0, 'f', 1));
    }
    
    // Check if reached destination using improved detection
    if (checkDestinationReached()) {
        // Reached destination
        onStopGuidance();
        QMessageBox::information(this, "Navigation", 
            QString("Destination Reached!\n\nYou have successfully arrived at your destination.\n"
                   "Final position: %1, %2")
            .arg(m_currentPosition.latitude, 0, 'f', 6)
            .arg(m_currentPosition.longitude, 0, 'f', 6));
        statusBar()->showMessage("Destination reached successfully!", 5000);
    }
    
    qDebug() << "Position updated to:" << m_currentPosition.latitude << "," << m_currentPosition.longitude 
             << "Speed:" << m_manualSpeed << "km/h" << "Heading:" << currentHeading << "degrees";
}

void NavigationMainWindow::onMousePositionChanged(const Point& position)
{
    // Update mouse position display in real-time
    if (m_mouseLatLabel) {
        m_mouseLatLabel->setText(QString("Mouse Lat: %1").arg(position.latitude, 0, 'f', 6));
    }
    if (m_mouseLonLabel) {
        m_mouseLonLabel->setText(QString("Mouse Lon: %1").arg(position.longitude, 0, 'f', 6));
    }
}

void NavigationMainWindow::setControlsEnabled(bool enabled)
{
    // Enable/disable route planning controls
    if (m_startLatEdit) m_startLatEdit->setEnabled(enabled);
    if (m_startLonEdit) m_startLonEdit->setEnabled(enabled);
    if (m_endLatEdit) m_endLatEdit->setEnabled(enabled);
    if (m_endLonEdit) m_endLonEdit->setEnabled(enabled);
    if (m_calculateButton) m_calculateButton->setEnabled(enabled);
    if (m_clearButton) m_clearButton->setEnabled(enabled);
    if (m_setStartButton) m_setStartButton->setEnabled(enabled);
    if (m_setEndButton) m_setEndButton->setEnabled(enabled);
    
    // Enable/disable manual control sliders during guidance
    if (m_manualSpeedSlider) m_manualSpeedSlider->setEnabled(enabled || m_guidanceRunning);
    if (m_manualHeadingSlider) m_manualHeadingSlider->setEnabled(enabled || m_guidanceRunning);
}

void NavigationMainWindow::checkAndStopGuidance()
{
    if (m_guidanceRunning) {
        onStopGuidance();
        statusBar()->showMessage("Guidance stopped due to route change", 3000);
    }
}

//Todo: MapWidget supports POI display

void NavigationMainWindow::onPOIResultSelected(int index)
{
    if (index < 0 || index >= static_cast<int>(m_lastSearchResults.size())) {
        return;
    }
    
    const POI& selectedPOI = m_lastSearchResults[index];
    
    // Show POI on map
    showPOIOnMap(selectedPOI);
    
    // Update results text with detailed info
    QString detailText = QString(
        "SELECTED POI DETAILS\n"
        "======================\n\n"
        "Name: %1\n"
        "Category: %2\n"
        "Coordinates:\n"
        "   Latitude: %3\n"
        "   Longitude: %4\n"
        "Address: %5\n\n"
        "You can set this as destination!"
    ).arg(QString::fromStdString(selectedPOI.name))
     .arg(QString::fromStdString(selectedPOI.category))
     .arg(selectedPOI.latitude, 0, 'f', 6)
     .arg(selectedPOI.longitude, 0, 'f', 6)
     .arg(QString::fromStdString(selectedPOI.address));
    
    m_poiResultsText->setPlainText(detailText);
    
    qDebug() << "POI selected:" << QString::fromStdString(selectedPOI.name) 
             << "at" << selectedPOI.latitude << "," << selectedPOI.longitude;
}

void NavigationMainWindow::showPOIOnMap(const POI& poi)
{
    MapWidget* mapWidget = qobject_cast<MapWidget*>(m_mapRenderer);
    if (!mapWidget) {
        return;
    }
    
    // Create Point from POI
    Point poiLocation(poi.latitude, poi.longitude);
    
    // Center map on POI location with smooth animation
    mapWidget->centerMap(poi.latitude, poi.longitude);
    
    // Show POI as a special marker (reuse clicked point for now)
    mapWidget->setClickedPoint(poiLocation);
    m_endLatEdit->setText(QString::number(poi.latitude, 'f', 6));
    m_endLonEdit->setText(QString::number(poi.longitude, 'f', 6));
    
    // Set POI name label to display on map
    QString poiLabel = QString("%1").arg(QString::fromStdString(poi.name));
    mapWidget->setPOILabel(poiLabel);
    
    // Calculate distance from current position if available
    QString distanceInfo = "";
    if (m_hasCurrentPosition) {
        double distance = calculateDistance(
            m_currentPosition.latitude, m_currentPosition.longitude,
            poi.latitude, poi.longitude
        );
        distanceInfo = QString("\nDistance from current position: %1 km").arg(distance, 0, 'f', 2);
    }
    
    // Update POI results text with detailed information
    QString detailText = QString(
        "POI DISPLAYED ON MAP\n"
        "======================\n\n"
        "Name: %1\n"
        "Category: %2\n"
        "Location:\n"
        "   Latitude: %3\n"
        "   Longitude: %4\n"
        "Address: %5%6\n\n"
        "POI is now centered on the map with a marker.\n"
        "You can set this location as Start or Destination point!"
    ).arg(QString::fromStdString(poi.name))
     .arg(QString::fromStdString(poi.category))
     .arg(poi.latitude, 0, 'f', 6)
     .arg(poi.longitude, 0, 'f', 6)
     .arg(QString::fromStdString(poi.address))
     .arg(distanceInfo);
    
    m_poiResultsText->setPlainText(detailText);
    
    // Show status message with POI name
    statusBar()->showMessage(QString("Showing POI: %1 | Category: %2 | Click to set as waypoint")
                            .arg(QString::fromStdString(poi.name))
                            .arg(QString::fromStdString(poi.category)), 8000);
    
    qDebug() << "POI displayed on map:" << QString::fromStdString(poi.name) 
             << "at (" << poi.latitude << "," << poi.longitude << ")";
}

void NavigationMainWindow::onToggleLeftPanel()
{
    m_leftPanelCollapsed = !m_leftPanelCollapsed;
    
    if (m_leftPanelCollapsed) {
        // Hide left panel
        m_leftPanelContainer->hide();
        m_toggleLeftPanelButton->setText("> Show Controls");
        m_toggleLeftPanelButton->setStyleSheet(
            "QPushButton { "
            "   background-color: #FF9800; "
            "   color: white; "
            "   font-weight: bold; "
            "   border-radius: 5px; "
            "   padding: 5px; "
            "}"
            "QPushButton:hover { background-color: #F57C00; }"
        );
        statusBar()->showMessage("Left panel collapsed - More space for map!", 3000);
        qDebug() << "Left panel collapsed";
    } else {
        // Show left panel
        m_leftPanelContainer->show();
        m_toggleLeftPanelButton->setText("< Hide Controls");
        m_toggleLeftPanelButton->setStyleSheet(
            "QPushButton { "
            "   background-color: #2196F3; "
            "   color: white; "
            "   font-weight: bold; "
            "   border-radius: 5px; "
            "   padding: 5px; "
            "}"
            "QPushButton:hover { background-color: #1976D2; }"
        );
        statusBar()->showMessage("Left panel expanded", 3000);
        qDebug() << "Left panel expanded";
    }
}

void NavigationMainWindow::onToggleRightPanel()
{
    m_rightPanelCollapsed = !m_rightPanelCollapsed;
    
    if (m_rightPanelCollapsed) {
        // Hide right panel
        m_rightPanelContainer->hide();
        m_toggleRightPanelButton->setText("< Show Info");
        m_toggleRightPanelButton->setStyleSheet(
            "QPushButton { "
            "   background-color: #FF9800; "
            "   color: white; "
            "   font-weight: bold; "
            "   border-radius: 5px; "
            "   padding: 5px; "
            "}"
            "QPushButton:hover { background-color: #F57C00; }"
        );
        statusBar()->showMessage("Right panel collapsed - More space for map!", 3000);
        qDebug() << "Right panel collapsed";
    } else {
        // Show right panel
        m_rightPanelContainer->show();
        m_toggleRightPanelButton->setText("Hide Info >");
        m_toggleRightPanelButton->setStyleSheet(
            "QPushButton { "
            "   background-color: #4CAF50; "
            "   color: white; "
            "   font-weight: bold; "
            "   border-radius: 5px; "
            "   padding: 5px; "
            "}"
            "QPushButton:hover { background-color: #45a049; }"
        );
        statusBar()->showMessage("Right panel expanded", 3000);
        qDebug() << "Right panel expanded";
    }
}

} // namespace nav
