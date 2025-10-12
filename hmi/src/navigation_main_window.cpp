#include "../include/navigation_main_window.h"
#include "../include/navigation_models.h"
#include "../include/map_widget.h"
#include "../include/service_clients_qt.h"
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
{
    // Initialize integrated navigation controller
    m_navController = new IntegratedNavigationController(this);
    
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

    // Setup control panel
    setupControlPanel();
    qDebug() << "Control panel setup completed";
    
    // Setup info panel  
    setupInfoPanel();
    qDebug() << "Info panel setup completed";

    // Setup menu and status bar
    setupMenuBar();
    setupStatusBar();

    qDebug() << "Main UI setup completed";
}

void NavigationMainWindow::setupMapView()
{
    qDebug() << "Creating MapWidget...";
    m_mapRenderer = new MapWidget(this);
    m_mapRenderer->setMinimumSize(400, 300);
    qDebug() << "MapWidget created with size:" << m_mapRenderer->size();
}

void NavigationMainWindow::setupControlPanel()
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
    m_calculateButton = new QPushButton("Calculate Shortest Route");
    m_calculateButton->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; font-weight: bold; padding: 8px; }");
    m_clearButton = new QPushButton("Clear Route");
    m_clearButton->setStyleSheet("QPushButton { background-color: #f44336; color: white; font-weight: bold; padding: 8px; }");
    
    buttonLayout->addWidget(m_calculateButton);
    buttonLayout->addWidget(m_clearButton);
    
    // Route Information
    QGroupBox *infoGroup = new QGroupBox("Route Information");
    QVBoxLayout *infoLayout = new QVBoxLayout(infoGroup);
    
    m_totalDistanceLabel = new QLabel("Total Distance: --");
    m_totalTimeLabel = new QLabel("Estimated Time: --");
    m_remainingDistanceLabel = new QLabel("Remaining: --");
    
    infoLayout->addWidget(m_totalDistanceLabel);
    infoLayout->addWidget(m_totalTimeLabel);
    infoLayout->addWidget(m_remainingDistanceLabel);
    
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
    
    // Add to central widget layout
    QVBoxLayout *mainLayout = qobject_cast<QVBoxLayout*>(m_centralWidget->layout());
    if (mainLayout) {
        mainLayout->addWidget(routeGroup);
    }
}

void NavigationMainWindow::setupInfoPanel()
{
    // Basic info panel implementation
    QGroupBox *infoGroup = new QGroupBox("Current Position");
    QVBoxLayout *layout = new QVBoxLayout(infoGroup);
    
    // Current position display
    m_latitudeLabel = new QLabel("Latitude: --");
    m_longitudeLabel = new QLabel("Longitude: --");
    m_speedValueLabel = new QLabel("Speed: --");
    m_headingLabel = new QLabel("Heading: --");
    
    layout->addWidget(m_latitudeLabel);
    layout->addWidget(m_longitudeLabel);
    layout->addWidget(m_speedValueLabel);
    layout->addWidget(m_headingLabel);
    
    // Button to get current position
    QPushButton *getCurrentPosButton = new QPushButton("Get Current Position");
    getCurrentPosButton->setStyleSheet("QPushButton { background-color: #2196F3; color: white; font-weight: bold; padding: 8px; }");
    layout->addWidget(getCurrentPosButton);
    
    // Connect button to get current position
    connect(getCurrentPosButton, &QPushButton::clicked, this, &NavigationMainWindow::onGetCurrentPosition);
    
    // Add to central widget layout
    QVBoxLayout *mainLayout = qobject_cast<QVBoxLayout*>(m_centralWidget->layout());
    if (mainLayout) {
        mainLayout->addWidget(infoGroup);
    }
    
    // Add Manual Control Panel
    setupManualControlPanel();
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

// Slot implementations - Route calculation functionality
void NavigationMainWindow::onCalculateRoute()
{
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
        statusBar()->showMessage("❌ ERROR: Navigation services are not ready!", 10000);
        QMessageBox::critical(this, "Service Error", 
            "❌ Navigation Services Not Ready!\n\n"
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
    
    // Enable start guidance button
    if (m_startSimButton) {
        m_startSimButton->setEnabled(true);
    }
    
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

void NavigationMainWindow::onMapClicked(const Point& position)
{
    qDebug() << "Map clicked at" << position.latitude << "," << position.longitude;
}

void NavigationMainWindow::onSimulationSpeedChanged(int speed)
{
    qDebug() << "Simulation speed changed to" << speed;
}

void NavigationMainWindow::onSpeedChanged(int speed)
{
    qDebug() << "Manual speed changed to" << speed;
    m_speedLabel->setText(QString("Speed: %1 km/h").arg(speed));
    m_speedValueLabel->setText(QString("Speed: %1 km/h").arg(speed));
    statusBar()->showMessage(QString("Speed set to %1 km/h").arg(speed), 2000);
}

void NavigationMainWindow::onHeadingChanged(int heading)
{
    qDebug() << "Manual heading changed to" << heading;
    m_headingSliderLabel->setText(QString("Heading: %1°").arg(heading));
    m_headingLabel->setText(QString("Heading: %1°").arg(heading));
    statusBar()->showMessage(QString("Heading set to %1°").arg(heading), 2000);
}

void NavigationMainWindow::setupGuidanceControlPanel()
{
    // Guidance Control Panel
    QGroupBox *guidanceGroup = new QGroupBox("Navigation Guidance");
    QVBoxLayout *guidanceLayout = new QVBoxLayout(guidanceGroup);
    
    // Start/Stop Guidance buttons
    QHBoxLayout *guidanceButtonLayout = new QHBoxLayout();
    
    m_startSimButton = new QPushButton("Start Guidance");
    m_startSimButton->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; font-weight: bold; padding: 8px; }");
    m_startSimButton->setEnabled(false); // Disabled until route is calculated
    
    m_stopSimButton = new QPushButton("Stop Guidance");
    m_stopSimButton->setStyleSheet("QPushButton { background-color: #f44336; color: white; font-weight: bold; padding: 8px; }");
    m_stopSimButton->setEnabled(false); // Disabled until guidance starts
    
    guidanceButtonLayout->addWidget(m_startSimButton);
    guidanceButtonLayout->addWidget(m_stopSimButton);
    guidanceLayout->addLayout(guidanceButtonLayout);
    
    // Guidance Information
    QLabel *guidanceInfoLabel = new QLabel("Guidance Information:");
    guidanceLayout->addWidget(guidanceInfoLabel);
    
    m_instructionLabel = new QLabel("No active guidance");
    m_instructionLabel->setWordWrap(true);
    m_instructionLabel->setStyleSheet("QLabel { border: 1px solid #ccc; padding: 8px; background-color: #f9f9f9; }");
    m_instructionLabel->setMinimumHeight(60);
    guidanceLayout->addWidget(m_instructionLabel);
    
    m_distanceLabel = new QLabel("Distance to next turn: --");
    guidanceLayout->addWidget(m_distanceLabel);
    
    // Auto heading checkbox
    QCheckBox *autoHeadingCheckbox = new QCheckBox("Auto-adjust heading to destination");
    autoHeadingCheckbox->setChecked(true);
    guidanceLayout->addWidget(autoHeadingCheckbox);
    
    // Connect signals
    connect(m_startSimButton, &QPushButton::clicked, this, &NavigationMainWindow::onStartGuidance);
    connect(m_stopSimButton, &QPushButton::clicked, this, &NavigationMainWindow::onStopGuidance);
    connect(autoHeadingCheckbox, &QCheckBox::toggled, this, &NavigationMainWindow::onAutoHeadingToggled);
    
    m_autoHeadingEnabled = true; // Store auto heading state
    
    // Add to main layout
    QVBoxLayout *mainLayout = qobject_cast<QVBoxLayout*>(m_centralWidget->layout());
    if (mainLayout) {
        mainLayout->addWidget(guidanceGroup);
    }
}

void NavigationMainWindow::onSetStartPoint()
{
    qDebug() << "Set start point to current position";
    
    // Get current position from labels
    QString latText = m_latitudeLabel->text();
    QString lonText = m_longitudeLabel->text();
    
    // Extract coordinates from labels
    if (latText.contains(":") && lonText.contains(":")) {
        bool ok1, ok2;
        double lat = latText.split(":")[1].trimmed().toDouble(&ok1);
        double lon = lonText.split(":")[1].trimmed().toDouble(&ok2);
        
        if (ok1 && ok2) {
            m_startLatEdit->setText(QString::number(lat, 'f', 6));
            m_startLonEdit->setText(QString::number(lon, 'f', 6));
            statusBar()->showMessage("Start point set to current position", 3000);
        } else {
            statusBar()->showMessage("Please get current position first", 3000);
        }
    } else {
        statusBar()->showMessage("Please get current position first", 3000);
    }
}

void NavigationMainWindow::onSetEndPoint()
{
    qDebug() << "Set end point requested";
}

void NavigationMainWindow::onGetCurrentPosition()
{
    qDebug() << "Getting current position...";
    
    // For demo, use default position (Hanoi, Vietnam)
    // In real implementation, this would get GPS coordinates
    double currentLat = 21.028511;
    double currentLon = 105.804817;
    double currentSpeed = 0.0;
    double currentHeading = 0.0;
    
    // Update labels
    m_latitudeLabel->setText(QString("Latitude: %1").arg(currentLat, 0, 'f', 6));
    m_longitudeLabel->setText(QString("Longitude: %1").arg(currentLon, 0, 'f', 6));
    m_speedValueLabel->setText(QString("Speed: %1 km/h").arg(currentSpeed, 0, 'f', 1));
    m_headingLabel->setText(QString("Heading: %1°").arg(currentHeading, 0, 'f', 1));
    
    // Show current position on map
    MapWidget* mapWidget = qobject_cast<MapWidget*>(m_mapRenderer);
    if (mapWidget) {
        Point currentPos(currentLat, currentLon);
        mapWidget->setCurrentPosition(currentPos);
        mapWidget->centerMap(currentLat, currentLon);
        qDebug() << "Current position displayed on map";
    }
    
    statusBar()->showMessage("Current position updated", 3000);
}

void NavigationMainWindow::setupManualControlPanel()
{
    // Manual Control Panel
    QGroupBox *controlGroup = new QGroupBox("Manual Control");
    QVBoxLayout *controlLayout = new QVBoxLayout(controlGroup);
    
    // Speed Control
    QLabel *speedLabel = new QLabel("Speed Control:");
    controlLayout->addWidget(speedLabel);
    
    m_speedSlider = new QSlider(Qt::Horizontal);
    m_speedSlider->setRange(0, 120); // 0-120 km/h
    m_speedSlider->setValue(0);
    m_speedSlider->setTickPosition(QSlider::TicksBelow);
    m_speedSlider->setTickInterval(20);
    controlLayout->addWidget(m_speedSlider);
    
    m_speedLabel = new QLabel("Speed: 0 km/h");
    controlLayout->addWidget(m_speedLabel);
    
    // Heading Control
    QLabel *headingLabel = new QLabel("Heading Control:");
    controlLayout->addWidget(headingLabel);
    
    m_headingSlider = new QSlider(Qt::Horizontal);
    m_headingSlider->setRange(0, 359); // 0-359 degrees
    m_headingSlider->setValue(0);
    m_headingSlider->setTickPosition(QSlider::TicksBelow);
    m_headingSlider->setTickInterval(45);
    controlLayout->addWidget(m_headingSlider);
    
    m_headingSliderLabel = new QLabel("Heading: 0°");
    controlLayout->addWidget(m_headingSliderLabel);
    
    // Connect sliders
    connect(m_speedSlider, &QSlider::valueChanged, this, &NavigationMainWindow::onSpeedChanged);
    connect(m_headingSlider, &QSlider::valueChanged, this, &NavigationMainWindow::onHeadingChanged);
    
    // Add to main layout
    QVBoxLayout *mainLayout = qobject_cast<QVBoxLayout*>(m_centralWidget->layout());
    if (mainLayout) {
        mainLayout->addWidget(controlGroup);
    }
    
    // Add Guidance Control Panel
    setupGuidanceControlPanel();
}
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

// Helper function to calculate bearing between two coordinates
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
    
    double bearing_rad = atan2(y, x);
    double bearing_deg = bearing_rad * 180.0 / M_PI;
    
    // Normalize to 0-360 degrees
    bearing_deg = fmod(bearing_deg + 360.0, 360.0);
    
    return bearing_deg;
}

void NavigationMainWindow::onStartGuidance()
{
    qDebug() << "Starting guidance...";
    
    if (!m_hasStartPoint || !m_hasEndPoint) {
        statusBar()->showMessage("Please calculate a route first", 3000);
        return;
    }
    
    // Start guidance simulation
    m_simulationRunning = true;
    m_startSimButton->setEnabled(false);
    m_stopSimButton->setEnabled(true);
    
    // If auto-heading is enabled, calculate heading to destination
    if (m_autoHeadingEnabled) {
        double currentLat = m_startPoint.latitude;
        double currentLon = m_startPoint.longitude;
        double destLat = m_endPoint.latitude;
        double destLon = m_endPoint.longitude;
        
        // Calculate bearing to destination
        double bearing = calculateBearing(currentLat, currentLon, destLat, destLon);
        
        // Update heading slider and display
        m_headingSlider->setValue((int)bearing);
        m_headingSliderLabel->setText(QString("Heading: %1°").arg(bearing, 0, 'f', 0));
        m_headingLabel->setText(QString("Heading: %1°").arg(bearing, 0, 'f', 0));
    }
    
    // Update guidance display
    m_instructionLabel->setText("Following route to destination...");
    double distance = calculateDistance(m_startPoint.latitude, m_startPoint.longitude, 
                                      m_endPoint.latitude, m_endPoint.longitude);
    m_distanceLabel->setText(QString("Distance to destination: %1 km").arg(distance, 0, 'f', 2));
    
    statusBar()->showMessage("Guidance started", 3000);
    qDebug() << "Guidance started with auto-heading";
}

void NavigationMainWindow::onStopGuidance()
{
    qDebug() << "Stopping guidance...";
    
    m_simulationRunning = false;
    m_startSimButton->setEnabled(true);
    m_stopSimButton->setEnabled(false);
    
    // Reset guidance display
    m_instructionLabel->setText("No active guidance");
    m_distanceLabel->setText("Distance to next turn: --");
    
    statusBar()->showMessage("Guidance stopped", 3000);
    qDebug() << "Guidance stopped";
}

void NavigationMainWindow::onAutoHeadingToggled(bool enabled)
{
    m_autoHeadingEnabled = enabled;
    qDebug() << "Auto-heading" << (enabled ? "enabled" : "disabled");
    
    if (enabled && m_simulationRunning && m_hasStartPoint && m_hasEndPoint) {
        // Recalculate heading to destination
        double bearing = calculateBearing(m_startPoint.latitude, m_startPoint.longitude, 
                                        m_endPoint.latitude, m_endPoint.longitude);
        m_headingSlider->setValue((int)bearing);
        statusBar()->showMessage("Auto-heading enabled", 2000);
    } else {
        statusBar()->showMessage("Auto-heading disabled", 2000);
    }
}

} // namespace nav
