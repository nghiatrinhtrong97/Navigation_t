#include "../include/navigation_main_window.h"
#include "../include/navigation_controller_v2.h"
#include <QDebug>

namespace nav {

NavigationMainWindow::NavigationMainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_centralWidget(nullptr)
    , m_mainSplitter(nullptr)
    , m_mapRenderer(nullptr)
    , m_navController(nullptr)
    , m_hasStartPoint(false)
    , m_hasEndPoint(false)
    , m_simulationRunning(false)
{
    setWindowTitle("Automotive Navigation System");
    setMinimumSize(1200, 800);
    resize(1400, 900);
    
    // Initialize backend
    m_navController = new NavigationController(this);
    
    // Check service status and show initial mode
    updateServiceStatus();
    
    // Setup UI
    setupUI();
    setupMenuBar();
    setupStatusBar();
    
    // Connect signals
    connect(m_navController, &NavigationController::positionChanged,
            this, &NavigationMainWindow::onPositionChanged);
    connect(m_navController, &NavigationController::routeCalculated,
            this, &NavigationMainWindow::onRouteCalculated);
    connect(m_navController, &NavigationController::guidanceUpdated,
            this, &NavigationMainWindow::onGuidanceUpdated);
    
    // Set default position
    Point defaultPos(DEFAULT_LAT, DEFAULT_LON);
    m_navController->setCurrentPosition(defaultPos);
    m_mapRenderer->setCurrentPosition(defaultPos);
    
    updatePositionDisplay();
    updateStatusBar();
    
    qDebug() << "Navigation window initialized";
}

NavigationMainWindow::~NavigationMainWindow()
{
}

void NavigationMainWindow::setupUI()
{
    m_centralWidget = new QWidget;
    setCentralWidget(m_centralWidget);
    
    // Main horizontal splitter
    m_mainSplitter = new QSplitter(Qt::Horizontal);
    
    // Setup components
    setupMapView();
    setupControlPanel();
    
    // Add to splitter
    m_mainSplitter->addWidget(m_mapGroup);
    m_mainSplitter->addWidget(m_controlTabs);
    m_mainSplitter->setSizes({800, 400});
    
    // Main layout
    QVBoxLayout *mainLayout = new QVBoxLayout(m_centralWidget);
    mainLayout->addWidget(m_mainSplitter);
    mainLayout->setContentsMargins(5, 5, 5, 5);
}

void NavigationMainWindow::setupMapView()
{
    m_mapGroup = new QGroupBox("Map View");
    QVBoxLayout *mapLayout = new QVBoxLayout(m_mapGroup);
    
    m_mapRenderer = new MapRenderer();
    mapLayout->addWidget(m_mapRenderer);
    
    // Connect map signals
    connect(m_mapRenderer, &MapRenderer::mapClicked,
            this, &NavigationMainWindow::onMapClicked);
}

void NavigationMainWindow::setupControlPanel()
{
    m_controlTabs = new QTabWidget();
    
    // Route planning tab
    QWidget *routeTab = new QWidget();
    QVBoxLayout *routeLayout = new QVBoxLayout(routeTab);
    
    // Route input group
    m_routeGroup = new QGroupBox("Route Planning");
    QGridLayout *routeGrid = new QGridLayout(m_routeGroup);
    
    // Start point
    routeGrid->addWidget(new QLabel("Start Latitude:"), 0, 0);
    m_startLatEdit = new QLineEdit();
    m_startLatEdit->setText("21.028511");
    routeGrid->addWidget(m_startLatEdit, 0, 1);
    
    routeGrid->addWidget(new QLabel("Start Longitude:"), 1, 0);
    m_startLonEdit = new QLineEdit();
    m_startLonEdit->setText("105.804817");
    routeGrid->addWidget(m_startLonEdit, 1, 1);
    
    m_setStartButton = new QPushButton("Set as Current Position");
    routeGrid->addWidget(m_setStartButton, 2, 0, 1, 2);
    
    // End point
    routeGrid->addWidget(new QLabel("End Latitude:"), 3, 0);
    m_endLatEdit = new QLineEdit();
    m_endLatEdit->setText("21.035000");
    routeGrid->addWidget(m_endLatEdit, 3, 1);
    
    routeGrid->addWidget(new QLabel("End Longitude:"), 4, 0);
    m_endLonEdit = new QLineEdit();
    m_endLonEdit->setText("105.820000");
    routeGrid->addWidget(m_endLonEdit, 4, 1);
    
    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    m_calculateButton = new QPushButton("Calculate Route");
    m_clearButton = new QPushButton("Clear Route");
    buttonLayout->addWidget(m_calculateButton);
    buttonLayout->addWidget(m_clearButton);
    routeGrid->addLayout(buttonLayout, 5, 0, 1, 2);
    
    routeLayout->addWidget(m_routeGroup);
    
    // Simulation group
    m_simulationGroup = new QGroupBox("Simulation Control");
    QVBoxLayout *simLayout = new QVBoxLayout(m_simulationGroup);
    
    QHBoxLayout *simButtonLayout = new QHBoxLayout();
    m_startSimButton = new QPushButton("Start Simulation");
    m_stopSimButton = new QPushButton("Stop Simulation");
    m_stopSimButton->setEnabled(false);
    simButtonLayout->addWidget(m_startSimButton);
    simButtonLayout->addWidget(m_stopSimButton);
    simLayout->addLayout(simButtonLayout);
    
    // Speed control
    QHBoxLayout *speedLayout = new QHBoxLayout();
    speedLayout->addWidget(new QLabel("Speed:"));
    m_speedSlider = new QSlider(Qt::Horizontal);
    m_speedSlider->setRange(1, 10);
    m_speedSlider->setValue(5);
    m_speedLabel = new QLabel("5x");
    speedLayout->addWidget(m_speedSlider);
    speedLayout->addWidget(m_speedLabel);
    simLayout->addLayout(speedLayout);
    
    // Progress bar
    m_routeProgress = new QProgressBar();
    simLayout->addWidget(new QLabel("Route Progress:"));
    simLayout->addWidget(m_routeProgress);
    
    routeLayout->addWidget(m_simulationGroup);
    routeLayout->addStretch();
    
    m_controlTabs->addTab(routeTab, "Route & Simulation");
    
    // Information tab
    setupInfoPanel();
    
    // Service control tab
    setupServicePanel();
    
    // Connect signals
    connect(m_calculateButton, &QPushButton::clicked, this, &NavigationMainWindow::onCalculateRoute);
    connect(m_clearButton, &QPushButton::clicked, this, &NavigationMainWindow::onClearRoute);
    connect(m_startSimButton, &QPushButton::clicked, this, &NavigationMainWindow::onStartSimulation);
    connect(m_stopSimButton, &QPushButton::clicked, this, &NavigationMainWindow::onStopSimulation);
    connect(m_setStartButton, &QPushButton::clicked, this, &NavigationMainWindow::onSetStartPoint);
    connect(m_speedSlider, &QSlider::valueChanged, this, &NavigationMainWindow::onSimulationSpeedChanged);
}

void NavigationMainWindow::setupServicePanel()
{
    QWidget *serviceTab = new QWidget();
    QVBoxLayout *serviceLayout = new QVBoxLayout(serviceTab);
    
    // Service status group
    m_serviceGroup = new QGroupBox("Service Management");
    QVBoxLayout *serviceGroupLayout = new QVBoxLayout(m_serviceGroup);
    
    // Current mode display
    QHBoxLayout *modeLayout = new QHBoxLayout();
    modeLayout->addWidget(new QLabel("Current Mode:"));
    m_serviceModeLabel = new QLabel("Checking...");
    m_serviceModeLabel->setStyleSheet("font-weight: bold; color: orange;");
    modeLayout->addWidget(m_serviceModeLabel);
    modeLayout->addStretch();
    serviceGroupLayout->addLayout(modeLayout);
    
    // Control buttons
    QHBoxLayout *controlLayout = new QHBoxLayout();
    m_toggleModeButton = new QPushButton("Switch to Service Mode");
    m_reconnectButton = new QPushButton("Reconnect Services");
    
    controlLayout->addWidget(m_toggleModeButton);
    controlLayout->addWidget(m_reconnectButton);
    controlLayout->addStretch();
    serviceGroupLayout->addLayout(controlLayout);
    
    // Service status text
    m_serviceStatusText = new QTextEdit();
    m_serviceStatusText->setMaximumHeight(200);
    m_serviceStatusText->setReadOnly(true);
    serviceGroupLayout->addWidget(new QLabel("Service Status:"));
    serviceGroupLayout->addWidget(m_serviceStatusText);
    
    serviceLayout->addWidget(m_serviceGroup);
    serviceLayout->addStretch();
    
    m_controlTabs->addTab(serviceTab, "Service Control");
    
    // Connect service control signals
    connect(m_toggleModeButton, &QPushButton::clicked, this, &NavigationMainWindow::onServiceModeChanged);
    connect(m_reconnectButton, &QPushButton::clicked, [this]() {
        m_logTextEdit->append("Attempting to reconnect services...");
        updateServiceStatus();
    });
}

void NavigationMainWindow::setupInfoPanel()
{
    QWidget *infoTab = new QWidget();
    QVBoxLayout *infoLayout = new QVBoxLayout(infoTab);
    
    // Position info
    m_positionGroup = new QGroupBox("Current Position");
    QGridLayout *posGrid = new QGridLayout(m_positionGroup);
    
    posGrid->addWidget(new QLabel("Latitude:"), 0, 0);
    m_latitudeLabel = new QLabel("0.000000");
    posGrid->addWidget(m_latitudeLabel, 0, 1);
    
    posGrid->addWidget(new QLabel("Longitude:"), 1, 0);
    m_longitudeLabel = new QLabel("0.000000");
    posGrid->addWidget(m_longitudeLabel, 1, 1);
    
    posGrid->addWidget(new QLabel("Speed:"), 2, 0);
    m_speedValueLabel = new QLabel("0 km/h");
    posGrid->addWidget(m_speedValueLabel, 2, 1);
    
    posGrid->addWidget(new QLabel("Heading:"), 3, 0);
    m_headingLabel = new QLabel("0°");
    posGrid->addWidget(m_headingLabel, 3, 1);
    
    infoLayout->addWidget(m_positionGroup);
    
    // Guidance info
    m_guidanceGroup = new QGroupBox("Current Guidance");
    QVBoxLayout *guidanceLayout = new QVBoxLayout(m_guidanceGroup);
    
    m_instructionLabel = new QLabel("No active guidance");
    m_instructionLabel->setWordWrap(true);
    m_instructionLabel->setStyleSheet("font-weight: bold; font-size: 14px;");
    guidanceLayout->addWidget(m_instructionLabel);
    
    m_distanceLabel = new QLabel("Distance: -");
    guidanceLayout->addWidget(m_distanceLabel);
    
    m_timeLabel = new QLabel("Time: -");
    guidanceLayout->addWidget(m_timeLabel);
    
    infoLayout->addWidget(m_guidanceGroup);
    
    // Route info
    m_routeInfoGroup = new QGroupBox("Route Information");
    QGridLayout *routeInfoGrid = new QGridLayout(m_routeInfoGroup);
    
    routeInfoGrid->addWidget(new QLabel("Total Distance:"), 0, 0);
    m_totalDistanceLabel = new QLabel("-");
    routeInfoGrid->addWidget(m_totalDistanceLabel, 0, 1);
    
    routeInfoGrid->addWidget(new QLabel("Total Time:"), 1, 0);
    m_totalTimeLabel = new QLabel("-");
    routeInfoGrid->addWidget(m_totalTimeLabel, 1, 1);
    
    routeInfoGrid->addWidget(new QLabel("Remaining Distance:"), 2, 0);
    m_remainingDistanceLabel = new QLabel("-");
    routeInfoGrid->addWidget(m_remainingDistanceLabel, 2, 1);
    
    routeInfoGrid->addWidget(new QLabel("Remaining Time:"), 3, 0);
    m_remainingTimeLabel = new QLabel("-");
    routeInfoGrid->addWidget(m_remainingTimeLabel, 3, 1);
    
    infoLayout->addWidget(m_routeInfoGroup);
    
    // Log display
    QGroupBox *logGroup = new QGroupBox("System Log");
    QVBoxLayout *logLayout = new QVBoxLayout(logGroup);
    
    m_logTextEdit = new QTextEdit();
    m_logTextEdit->setMaximumHeight(150);
    m_logTextEdit->append("Navigation system initialized");
    logLayout->addWidget(m_logTextEdit);
    
    infoLayout->addWidget(logGroup);
    infoLayout->addStretch();
    
    m_controlTabs->addTab(infoTab, "Information");
}

void NavigationMainWindow::setupMenuBar()
{
    // File menu
    QMenu *fileMenu = menuBar()->addMenu("&File");
    
    m_exitAction = new QAction("&Exit", this);
    m_exitAction->setShortcut(QKeySequence::Quit);
    connect(m_exitAction, &QAction::triggered, this, &QWidget::close);
    fileMenu->addAction(m_exitAction);
    
    // Help menu
    QMenu *helpMenu = menuBar()->addMenu("&Help");
    
    m_aboutAction = new QAction("&About", this);
    connect(m_aboutAction, &QAction::triggered, [this]() {
        QMessageBox::about(this, "About", 
            "Automotive Navigation System\n"
            "Version 1.0\n\n"
            "A complete navigation system simulation\n"
            "with route planning and guidance.");
    });
    helpMenu->addAction(m_aboutAction);
}

void NavigationMainWindow::setupStatusBar()
{
    m_statusLabel = new QLabel("Ready");
    statusBar()->addWidget(m_statusLabel);
}

void NavigationMainWindow::onCalculateRoute()
{
    bool ok;
    double startLat = m_startLatEdit->text().toDouble(&ok);
    if (!ok) {
        QMessageBox::warning(this, "Error", "Invalid start latitude");
        return;
    }
    
    double startLon = m_startLonEdit->text().toDouble(&ok);
    if (!ok) {
        QMessageBox::warning(this, "Error", "Invalid start longitude");
        return;
    }
    
    double endLat = m_endLatEdit->text().toDouble(&ok);
    if (!ok) {
        QMessageBox::warning(this, "Error", "Invalid end latitude");
        return;
    }
    
    double endLon = m_endLonEdit->text().toDouble(&ok);
    if (!ok) {
        QMessageBox::warning(this, "Error", "Invalid end longitude");
        return;
    }
    
    Point start(startLat, startLon);
    Point end(endLat, endLon);
    
    m_startPoint = start;
    m_endPoint = end;
    m_hasStartPoint = true;
    m_hasEndPoint = true;
    
    m_mapRenderer->setStartPoint(start);
    m_mapRenderer->setEndPoint(end);
    
    m_statusLabel->setText("Calculating route...");
    m_calculateButton->setEnabled(false);
    
    if (m_navController->calculateRoute(start, end)) {
        m_logTextEdit->append(QString("Route calculation started from (%1, %2) to (%3, %4)")
                             .arg(startLat, 0, 'f', 6).arg(startLon, 0, 'f', 6)
                             .arg(endLat, 0, 'f', 6).arg(endLon, 0, 'f', 6));
    } else {
        m_statusLabel->setText("Route calculation failed");
        m_calculateButton->setEnabled(true);
        QMessageBox::warning(this, "Error", "Failed to calculate route");
    }
}

void NavigationMainWindow::onClearRoute()
{
    m_navController->clearRoute();
    m_mapRenderer->clearRoute();
    m_mapRenderer->clearWaypoints();
    
    m_hasStartPoint = false;
    m_hasEndPoint = false;
    
    m_routeProgress->setValue(0);
    updateRouteInfo();
    
    m_logTextEdit->append("Route cleared");
    m_statusLabel->setText("Route cleared");
}

void NavigationMainWindow::onStartSimulation()
{
    if (!m_navController->hasActiveRoute()) {
        QMessageBox::information(this, "Info", "Please calculate a route first");
        return;
    }
    
    m_navController->startSimulation();
    m_simulationRunning = true;
    
    m_startSimButton->setEnabled(false);
    m_stopSimButton->setEnabled(true);
    m_calculateButton->setEnabled(false);
    
    m_logTextEdit->append("Simulation started");
    m_statusLabel->setText("Simulation running");
}

void NavigationMainWindow::onStopSimulation()
{
    m_navController->stopSimulation();
    m_simulationRunning = false;
    
    m_startSimButton->setEnabled(true);
    m_stopSimButton->setEnabled(false);
    m_calculateButton->setEnabled(true);
    
    m_logTextEdit->append("Simulation stopped");
    m_statusLabel->setText("Simulation stopped");
}

void NavigationMainWindow::onPositionChanged(const Point& position, double heading, double speed)
{
    m_mapRenderer->setCurrentPosition(position, heading);
    updatePositionDisplay();
    updateStatusBar();
}

void NavigationMainWindow::onRouteCalculated(const Route& route)
{
    m_calculateButton->setEnabled(true);
    m_statusLabel->setText("Route calculated successfully");
    
    m_logTextEdit->append(QString("Route calculated: %1 waypoints, %2 km, %3 minutes")
                         .arg(route.node_count)
                         .arg(route.total_distance_meters / 1000.0, 0, 'f', 1)
                         .arg(route.estimated_time_seconds / 60.0, 0, 'f', 1));
    
    updateRouteInfo();
    
    // Enable simulation
    m_startSimButton->setEnabled(true);
}

void NavigationMainWindow::onGuidanceUpdated(const GuidanceInstruction& instruction)
{
    updateGuidanceDisplay();
}

void NavigationMainWindow::onMapClicked(const Point& position)
{
    // Set as end point when shift-clicked
    QApplication *app = qobject_cast<QApplication*>(QApplication::instance());
    if (app && (app->keyboardModifiers() & Qt::ShiftModifier)) {
        m_endLatEdit->setText(QString::number(position.latitude, 'f', 6));
        m_endLonEdit->setText(QString::number(position.longitude, 'f', 6));
        m_logTextEdit->append(QString("End point set to (%1, %2)")
                             .arg(position.latitude, 0, 'f', 6)
                             .arg(position.longitude, 0, 'f', 6));
    } else {
        // Set as start point on normal click
        m_startLatEdit->setText(QString::number(position.latitude, 'f', 6));
        m_startLonEdit->setText(QString::number(position.longitude, 'f', 6));
        m_logTextEdit->append(QString("Start point set to (%1, %2)")
                             .arg(position.latitude, 0, 'f', 6)
                             .arg(position.longitude, 0, 'f', 6));
    }
}

void NavigationMainWindow::onSimulationSpeedChanged(int speed)
{
    m_speedLabel->setText(QString("%1x").arg(speed));
    m_navController->setSimulationSpeed(speed);
}

void NavigationMainWindow::onSetStartPoint()
{
    Point currentPos = m_navController->getCurrentPosition();
    m_startLatEdit->setText(QString::number(currentPos.latitude, 'f', 6));
    m_startLonEdit->setText(QString::number(currentPos.longitude, 'f', 6));
    
    m_logTextEdit->append(QString("Start point set to current position (%1, %2)")
                         .arg(currentPos.latitude, 0, 'f', 6)
                         .arg(currentPos.longitude, 0, 'f', 6));
}

void NavigationMainWindow::onSetEndPoint()
{
    // This could be implemented for setting end point to current position
}

void NavigationMainWindow::updateStatusBar()
{
    Point pos = m_navController->getCurrentPosition();
    m_statusLabel->setText(QString("Position: %1, %2")
                          .arg(pos.latitude, 0, 'f', 6)
                          .arg(pos.longitude, 0, 'f', 6));
}

void NavigationMainWindow::updatePositionDisplay()
{
    Point pos = m_navController->getCurrentPosition();
    m_latitudeLabel->setText(QString::number(pos.latitude, 'f', 6));
    m_longitudeLabel->setText(QString::number(pos.longitude, 'f', 6));
    
    // These would be updated with real data
    m_speedValueLabel->setText("50 km/h");
    m_headingLabel->setText("45°");
}

void NavigationMainWindow::updateGuidanceDisplay()
{
    if (m_navController->hasActiveRoute()) {
        GuidanceInstruction instruction = m_navController->getCurrentGuidance();
        m_instructionLabel->setText(QString::fromUtf8(instruction.instruction_text));
        m_distanceLabel->setText(QString("Distance: %1 m").arg(instruction.distance_to_turn_meters, 0, 'f', 0));
        m_timeLabel->setText("Time: 2 min");  // Placeholder
    } else {
        m_instructionLabel->setText("No active guidance");
        m_distanceLabel->setText("Distance: -");
        m_timeLabel->setText("Time: -");
    }
}

void NavigationMainWindow::updateRouteInfo()
{
    if (m_navController->hasActiveRoute()) {
        const Route& route = m_navController->getActiveRoute();
        m_totalDistanceLabel->setText(NavUtils::formatDistance(route.total_distance_meters).c_str());
        m_totalTimeLabel->setText(NavUtils::formatTime(route.estimated_time_seconds).c_str());
        m_remainingDistanceLabel->setText(NavUtils::formatDistance(route.total_distance_meters).c_str());
        m_remainingTimeLabel->setText(NavUtils::formatTime(route.estimated_time_seconds).c_str());
    } else {
        m_totalDistanceLabel->setText("-");
        m_totalTimeLabel->setText("-");
        m_remainingDistanceLabel->setText("-");
        m_remainingTimeLabel->setText("-");
    }
}

void NavigationMainWindow::updateServiceStatus()
{
    if (!m_navController) {
        return;
    }
    
    // Update mode display
    NavigationController::OperationMode mode = m_navController->getOperationMode();
    if (mode == NavigationController::SERVICE_MODE) {
        m_serviceModeLabel->setText("Service Mode");
        m_serviceModeLabel->setStyleSheet("font-weight: bold; color: green;");
        m_toggleModeButton->setText("Switch to Simulation Mode");
    } else {
        m_serviceModeLabel->setText("Simulation Mode");
        m_serviceModeLabel->setStyleSheet("font-weight: bold; color: blue;");
        m_toggleModeButton->setText("Switch to Service Mode");
    }
    
    // Update service status
    std::string status = m_navController->getServiceStatus();
    m_serviceStatusText->setPlainText(QString::fromStdString(status));
    
    // Update connection status in log
    bool connected = m_navController->areServicesConnected();
    if (connected) {
        m_logTextEdit->append("✓ All services connected");
    } else {
        m_logTextEdit->append("⚠ Some services disconnected - using simulation fallback");
    }
}

void NavigationMainWindow::onServiceModeChanged()
{
    if (!m_navController) {
        return;
    }
    
    NavigationController::OperationMode currentMode = m_navController->getOperationMode();
    NavigationController::OperationMode newMode;
    
    if (currentMode == NavigationController::SERVICE_MODE) {
        newMode = NavigationController::SIMULATION_MODE;
        m_logTextEdit->append("Switching to simulation mode");
    } else {
        if (m_navController->areServicesConnected()) {
            newMode = NavigationController::SERVICE_MODE;
            m_logTextEdit->append("Switching to service mode");
        } else {
            QMessageBox::warning(this, "Service Mode", 
                "Cannot switch to service mode: services are not available.\n"
                "Please start the backend services first.");
            return;
        }
    }
    
    m_navController->setOperationMode(newMode);
    updateServiceStatus();
}

} // namespace nav