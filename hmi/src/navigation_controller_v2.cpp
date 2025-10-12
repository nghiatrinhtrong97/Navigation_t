#include "../include/navigation_controller_v2.h"
#include <QDebug>
#include <QMutexLocker>
#include <cmath>

namespace nav {

NavigationController::NavigationController(QObject *parent)
    : QObject(parent)
    , m_processManager(nullptr)
    , m_operationMode(SIMULATION_MODE)
    , m_simulationTimer(nullptr)
    , m_simulationSpeed(5)
    , m_simulationProgress(0.0)
    , m_simulationRunning(false)
    , m_currentRouteIndex(0)
    , m_currentPosition(DEFAULT_LAT, DEFAULT_LON)
    , m_currentHeading(0.0)
    , m_currentSpeed(0.0)
    , m_hasActiveRoute(false)
{
    // Initialize process manager
    m_processManager = std::make_shared<ProcessManager>(this);
    
    // Connect process manager signals
    connect(m_processManager.get(), &ProcessManager::serviceStarted,
            this, &NavigationController::onServiceStarted);
    connect(m_processManager.get(), &ProcessManager::serviceStopped,
            this, &NavigationController::onServiceStopped);
    connect(m_processManager.get(), &ProcessManager::serviceError,
            this, &NavigationController::onServiceError);
    connect(m_processManager.get(), &ProcessManager::messageReceived,
            this, &NavigationController::onMessageReceived);
    
    // Setup simulation timer (fallback)
    m_simulationTimer = new QTimer(this);
    connect(m_simulationTimer, &QTimer::timeout,
            this, &NavigationController::updateSimulation);
    
    // Try to initialize services
    qDebug() << "🚀 [SERVICE INIT] Attempting to initialize services...";
    
    // Force console output for debugging
    qInstallMessageHandler([](QtMsgType type, const QMessageLogContext &context, const QString &msg) {
        static QFile debugFile("navigation_debug.log");
        if (!debugFile.isOpen()) {
            debugFile.open(QIODevice::WriteOnly | QIODevice::Append);
        }
        QTextStream stream(&debugFile);
        stream << QDateTime::currentDateTime().toString() << " - " << msg << Qt::endl;
        stream.flush();
    });
    
    if (initializeServices()) {
        setOperationMode(SERVICE_MODE);
        qDebug() << "✅ [SERVICE INIT] Navigation controller initialized in SERVICE mode";
        qDebug() << "🔍 [SERVICE INIT] Services running check:" << areServicesConnected();
    } else {
        setOperationMode(SIMULATION_MODE);
        qDebug() << "⚠️ [SERVICE INIT] Navigation controller initialized in SIMULATION mode (services unavailable)";
    }
}

NavigationController::~NavigationController()
{
    if (m_simulationRunning) {
        stopSimulation();
    }
    
    if (m_processManager) {
        m_processManager->shutdownServices();
    }
}

bool NavigationController::initializeServices()
{
    qDebug() << "🚀 [PROCESS MANAGER] initializeServices() called";
    if (!m_processManager->initializeServices()) {
        qDebug() << "❌ [PROCESS MANAGER] Failed to initialize process manager";
        return false;
    }

    // Process manager handles service lifecycle automatically
    // We communicate via IPC messages instead of direct service calls
    qDebug() << "✅ [PROCESS MANAGER] Process manager initialized successfully";
    qDebug() << "🔍 [PROCESS MANAGER] Checking if all services are running...";
    bool allRunning = m_processManager->areAllServicesRunning();
    qDebug() << "🔍 [PROCESS MANAGER] All services running:" << allRunning;
    return true;
}bool NavigationController::areServicesConnected()
{
    if (!m_processManager) {
        return false;
    }
    
    return m_processManager->areAllServicesRunning();
}

std::string NavigationController::getServiceStatus()
{
    if (!m_processManager) {
        return "Process manager not available";
    }
    
    return m_processManager->getServiceStatus().toStdString();
}

void NavigationController::setOperationMode(OperationMode mode)
{
    QMutexLocker locker(&m_mutex);
    
    if (m_operationMode == mode) {
        return;
    }
    
    // Stop current operations
    if (m_simulationRunning) {
        stopSimulation();
    }
    
    m_operationMode = mode;
    
    if (mode == SERVICE_MODE) {
        qDebug() << "Switched to SERVICE mode";
        // Start positioning service if available
        if (m_positioningService && m_positioningService->isConnected()) {
            m_positioningService->startPositioning();
        }
    } else {
        qDebug() << "Switched to SIMULATION mode";
        // Stop services
        if (m_positioningService) {
            m_positioningService->stopPositioning();
        }
        if (m_guidanceService) {
            m_guidanceService->stopGuidance();
        }
    }
}

NavigationController::OperationMode NavigationController::getOperationMode() const
{
    QMutexLocker locker(&m_mutex);
    return m_operationMode;
}

bool NavigationController::calculateRoute(const Point& start, const Point& end)
{
    QMutexLocker locker(&m_mutex);
    
    qDebug() << "🧭 [ROUTE CALC] Calculating route from" << start.latitude << "," << start.longitude
             << "to" << end.latitude << "," << end.longitude;
    qDebug() << "🔍 [ROUTE CALC] Current operation mode:" << (m_operationMode == SERVICE_MODE ? "SERVICE_MODE" : "SIMULATION_MODE");
    qDebug() << "🔍 [ROUTE CALC] ProcessManager exists:" << (m_processManager != nullptr);
    
    if (m_processManager) {
        qDebug() << "🔍 [ROUTE CALC] Are all services running:" << m_processManager->areAllServicesRunning();
    }
    
    if (m_operationMode == SERVICE_MODE && m_processManager && m_processManager->areAllServicesRunning()) {
        // Use IPC to request route from routing service
        qDebug() << "🔥 [ROUTE CALC] Using SERVICE_MODE - sending IPC request to routing service!";
        sendRouteRequest(start, end);
        return true; // Actual response will come via message callback
    } else {
        // ❌ MANDATORY: Routing service MUST be available - no fallback allowed!
        qDebug() << "❌ [ROUTE CALC] ROUTING SERVICE NOT AVAILABLE - CANNOT CALCULATE ROUTE!";
        qDebug() << "❌ [ROUTE CALC] Operation mode:" << (m_operationMode == SERVICE_MODE ? "SERVICE_MODE" : "SIMULATION_MODE");
        qDebug() << "❌ [ROUTE CALC] All services running:" << (m_processManager ? m_processManager->areAllServicesRunning() : false);
        
        // Emit error signal instead of fallback calculation
        emit routeCalculationFailed("Routing service is not available. Cannot calculate route without the routing service.");
        return false;
    }
}

void NavigationController::clearRoute()
{
    QMutexLocker locker(&m_mutex);
    
    if (m_simulationRunning) {
        stopSimulation();
    }
    
    if (m_operationMode == SERVICE_MODE && m_guidanceService) {
        m_guidanceService->stopGuidance();
    }
    
    m_hasActiveRoute = false;
    m_activeRoute = Route{};
    m_routePoints.clear();
    m_currentRouteIndex = 0;
    m_simulationProgress = 0.0;
    
    qDebug() << "Route cleared";
}

void NavigationController::startSimulation()
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_hasActiveRoute || m_simulationRunning) {
        qDebug() << "Cannot start simulation: no route or already running";
        return;
    }
    
    m_simulationRunning = true;
    m_currentRouteIndex = 0;
    m_simulationProgress = 0.0;
    
    if (m_operationMode == SERVICE_MODE) {
        // Start real guidance service
        if (m_guidanceService && m_guidanceService->isConnected()) {
            m_guidanceService->startGuidance(m_activeRoute);
        }
        
        // Set initial position in positioning service
        if (m_positioningService && m_positioningService->isConnected() && !m_routePoints.empty()) {
            m_positioningService->setPosition(m_routePoints[0]);
        }
    } else {
        // Start simulation timer
        m_simulationTimer->start(100); // 10 Hz update rate
    }
    
    qDebug() << "Simulation started with speed" << m_simulationSpeed << "x";
}

void NavigationController::stopSimulation()
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_simulationRunning) {
        return;
    }
    
    m_simulationRunning = false;
    m_simulationTimer->stop();
    m_currentSpeed = 0.0;
    
    if (m_operationMode == SERVICE_MODE && m_guidanceService) {
        m_guidanceService->stopGuidance();
    }
    
    qDebug() << "Simulation stopped";
}

void NavigationController::setSimulationSpeed(int speed)
{
    QMutexLocker locker(&m_mutex);
    m_simulationSpeed = qBound(1, speed, 10);
    qDebug() << "Simulation speed set to" << m_simulationSpeed << "x";
}

void NavigationController::setCurrentPosition(const Point& position)
{
    QMutexLocker locker(&m_mutex);
    
    m_currentPosition = position;
    
    if (m_operationMode == SERVICE_MODE) {
        // Update positioning service
        if (m_positioningService && m_positioningService->isConnected()) {
            m_positioningService->setPosition(position);
        }
        
        // Update guidance service
        if (m_guidanceService && m_guidanceService->isConnected() && m_hasActiveRoute) {
            m_guidanceService->updatePosition(position);
        }
    }
    
    emit positionChanged(position, m_currentHeading, m_currentSpeed);
}

Point NavigationController::getCurrentPosition() const
{
    QMutexLocker locker(&m_mutex);
    return m_currentPosition;
}

bool NavigationController::hasActiveRoute() const
{
    QMutexLocker locker(&m_mutex);
    return m_hasActiveRoute;
}

const Route& NavigationController::getActiveRoute() const
{
    QMutexLocker locker(&m_mutex);
    return m_activeRoute;
}

GuidanceInstruction NavigationController::getCurrentGuidance() const
{
    QMutexLocker locker(&m_mutex);
    
    if (m_operationMode == SERVICE_MODE && m_guidanceService) {
        return m_guidanceService->getCurrentInstruction();
    } else {
        return m_currentGuidance;
    }
}

double NavigationController::getSimulationProgress() const
{
    QMutexLocker locker(&m_mutex);
    return m_simulationProgress;
}

void NavigationController::updateSimulation()
{
    if (!m_simulationRunning || !m_hasActiveRoute || m_routePoints.empty()) {
        return;
    }
    
    // Only update in simulation mode
    if (m_operationMode != SIMULATION_MODE) {
        return;
    }
    
    // Calculate simulation step based on speed
    double stepSize = 0.001 * m_simulationSpeed;
    m_simulationProgress += stepSize;
    
    if (m_simulationProgress >= 1.0) {
        m_simulationProgress = 1.0;
        stopSimulation();
        return;
    }
    
    updatePositionAlongRoute();
    
    // Update guidance in simulation mode
    GuidanceInstruction instruction;
    if (m_currentRouteIndex < m_routePoints.size() - 1) {
        Point nextPoint = m_routePoints[m_currentRouteIndex + 1];
        double distance = calculateDistance(m_currentPosition, nextPoint);
        
        if (distance < 100) {
            snprintf(instruction.instruction_text, sizeof(instruction.instruction_text),
                    "In %d meters, turn right", static_cast<int>(distance));
            instruction.turn_type = GuidanceInstruction::TURN_RIGHT;
        } else {
            strcpy(instruction.instruction_text, "Continue straight");
            instruction.turn_type = GuidanceInstruction::STRAIGHT;
        }
        
        instruction.distance_to_turn_meters = static_cast<uint32_t>(distance);
    } else {
        strcpy(instruction.instruction_text, "You have arrived at your destination");
        instruction.distance_to_turn_meters = 0;
        instruction.turn_type = GuidanceInstruction::DESTINATION;
    }
    
    m_currentGuidance = instruction;
    emit guidanceUpdated(instruction);
}

void NavigationController::onServicePositionUpdate(const Point& position, double heading, double speed)
{
    QMutexLocker locker(&m_mutex);
    
    m_currentPosition = position;
    m_currentHeading = heading;
    m_currentSpeed = speed;
    
    emit positionChanged(position, heading, speed);
}

void NavigationController::onServiceRouteCalculated(const Route& route)
{
    QMutexLocker locker(&m_mutex);
    
    m_activeRoute = route;
    m_hasActiveRoute = true;
    m_simulationProgress = 0.0;
    
    qDebug() << "Route calculated via service:" << route.node_count << "points,"
             << route.total_distance_meters / 1000.0 << "km";
    
    emit routeCalculated(route);
}

void NavigationController::onServiceGuidanceUpdate(const GuidanceInstruction& instruction)
{
    QMutexLocker locker(&m_mutex);
    
    m_currentGuidance = instruction;
    emit guidanceUpdated(instruction);
}

void NavigationController::onServiceRouteError(const std::string& error)
{
    qDebug() << "Route calculation error:" << error.c_str();
    
    // Fallback to simulation mode for this calculation
    if (m_operationMode == SERVICE_MODE) {
        qDebug() << "Falling back to simulation for route calculation";
        // The route calculation will be retried in simulation mode
    }
}

void NavigationController::updatePositionAlongRoute()
{
    if (m_routePoints.empty()) {
        return;
    }
    
    // Find current segment based on simulation progress
    size_t totalSegments = m_routePoints.size() - 1;
    if (totalSegments == 0) {
        return;
    }
    
    double segmentProgress = m_simulationProgress * totalSegments;
    size_t currentSegment = static_cast<size_t>(segmentProgress);
    
    if (currentSegment >= totalSegments) {
        currentSegment = totalSegments - 1;
        m_currentPosition = m_routePoints.back();
        m_currentSpeed = 0.0;
    } else {
        // Interpolate between current and next point
        double t = segmentProgress - currentSegment;
        Point start = m_routePoints[currentSegment];
        Point end = m_routePoints[currentSegment + 1];
        
        m_currentPosition.latitude = start.latitude + t * (end.latitude - start.latitude);
        m_currentPosition.longitude = start.longitude + t * (end.longitude - start.longitude);
        
        // Calculate heading (bearing to next point)
        m_currentHeading = calculateBearing(start, end);
        m_currentSpeed = 50.0; // km/h simulation speed
    }
    
    m_currentRouteIndex = currentSegment;
    
    emit positionChanged(m_currentPosition, m_currentHeading, m_currentSpeed);
}

void NavigationController::generateRoutePoints(const Point& start, const Point& end, Route& route)
{
    m_routePoints.clear();
    
    // Generate intermediate points for a more realistic route
    int numPoints = 20;
    
    m_routePoints.push_back(start);
    
    // Add some curve to make it more interesting
    for (int i = 1; i < numPoints; i++) {
        double t = static_cast<double>(i) / numPoints;
        
        // Linear interpolation with some offset for curves
        double lat = start.latitude + t * (end.latitude - start.latitude);
        double lon = start.longitude + t * (end.longitude - start.longitude);
        
        // Add some curve variation
        double curve = 0.002 * sin(t * M_PI * 3);
        lat += curve;
        lon += curve * 0.5;
        
        m_routePoints.push_back(Point(lat, lon));
    }
    
    m_routePoints.push_back(end);
    
    route.node_count = m_routePoints.size();
    m_currentRouteIndex = 0;
    
    qDebug() << "Generated route with" << m_routePoints.size() << "points";
}

double NavigationController::calculateDistance(const Point& p1, const Point& p2) const
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

double NavigationController::calculateBearing(const Point& from, const Point& to) const
{
    double lat1 = from.latitude * M_PI / 180.0;
    double lat2 = to.latitude * M_PI / 180.0;
    double deltaLon = (to.longitude - from.longitude) * M_PI / 180.0;
    
    double y = sin(deltaLon) * cos(lat2);
    double x = cos(lat1) * sin(lat2) - sin(lat1) * cos(lat2) * cos(deltaLon);
    
    double bearing = atan2(y, x) * 180.0 / M_PI;
    return fmod(bearing + 360.0, 360.0); // Normalize to 0-360 degrees
}

// Process management slots
void NavigationController::onServiceStarted(ServiceType serviceType)
{
    qDebug() << "Service started:" << static_cast<int>(serviceType);
    
    // Request initial position when positioning service starts
    if (serviceType == ServiceType::Positioning) {
        sendPositionRequest();
    }
}

void NavigationController::onServiceStopped(ServiceType serviceType)
{
    qDebug() << "Service stopped:" << static_cast<int>(serviceType);
    
    // Fall back to simulation mode if critical services stop
    if (m_operationMode == SERVICE_MODE && 
        (serviceType == ServiceType::Positioning || serviceType == ServiceType::Routing)) {
        qWarning() << "Critical service stopped, falling back to simulation mode";
        setOperationMode(SIMULATION_MODE);
    }
}

void NavigationController::onServiceError(ServiceType serviceType, const QString& error)
{
    qCritical() << "Service error for" << static_cast<int>(serviceType) << ":" << error;
    
    // Handle service errors appropriately
    if (serviceType == ServiceType::Routing && m_hasActiveRoute) {
        emit routeCalculated(Route()); // Signal empty route to indicate error
    }
}

void NavigationController::onMessageReceived(ServiceType serviceType, const IPCMessage& message)
{
    switch (serviceType) {
        case ServiceType::Positioning:
            handlePositionMessage(message);
            break;
        case ServiceType::Routing:
            handleRouteMessage(message);
            break;
        case ServiceType::Guidance:
            handleGuidanceMessage(message);
            break;
        case ServiceType::Map:
            handleMapMessage(message);
            break;
    }
}

// IPC message handling
void NavigationController::handlePositionMessage(const IPCMessage& message)
{
    if (message.messageType == MessageTypes::POSITION_UPDATE) {
        QJsonObject data = message.data;
        double lat = data["latitude"].toDouble();
        double lon = data["longitude"].toDouble();
        double heading = data.contains("heading") ? data["heading"].toDouble() : 0.0;
        double speed = data.contains("speed") ? data["speed"].toDouble() : 0.0;
        
        Point position(lat, lon);
        onServicePositionUpdate(position, heading, speed);
    }
    else if (message.messageType == MessageTypes::POSITION_ERROR) {
        qWarning() << "Position service error:" << message.data["error"].toString();
    }
}

void NavigationController::handleRouteMessage(const IPCMessage& message)
{
    if (message.messageType == MessageTypes::ROUTE_RESPONSE) {
        QJsonObject data = message.data;
        QJsonArray waypoints = data["waypoints"].toArray();
        double distance = data["distance"].toDouble();
        double duration = data["duration"].toDouble();
        
        // Convert waypoints to Route
        Route route;
        route.total_distance_meters = distance;
        route.estimated_time_seconds = static_cast<uint32_t>(duration);
        
        // For now, we'll store waypoints in a simplified way
        // In a real implementation, you'd convert to node IDs
        route.node_count = qMin(waypoints.size(), static_cast<int>(Route::MAX_NODES));
        for (int i = 0; i < route.node_count && i < Route::MAX_NODES; ++i) {
            route.nodes[i] = static_cast<uint32_t>(i); // Simplified node ID assignment
        }
        
        onServiceRouteCalculated(route);
    }
    else if (message.messageType == MessageTypes::ROUTE_ERROR) {
        QString error = message.data["error"].toString();
        onServiceRouteError(error.toStdString());
    }
}

void NavigationController::handleGuidanceMessage(const IPCMessage& message)
{
    if (message.messageType == MessageTypes::GUIDANCE_UPDATE) {
        QJsonObject data = message.data;
        
        GuidanceInstruction instruction;
        instruction.turn_type = TurnType::STRAIGHT; // Default turn type
        instruction.distance_to_turn_meters = data["distanceToNext"].toDouble();
        
        QString instructionText = data["instruction"].toString();
        strncpy(instruction.instruction_text, instructionText.toLocal8Bit().constData(), 
                sizeof(instruction.instruction_text) - 1);
        instruction.instruction_text[sizeof(instruction.instruction_text) - 1] = '\0';
        
        onServiceGuidanceUpdate(instruction);
    }
    else if (message.messageType == MessageTypes::GUIDANCE_ERROR) {
        qWarning() << "Guidance service error:" << message.data["error"].toString();
    }
}

void NavigationController::handleMapMessage(const IPCMessage& message)
{
    if (message.messageType == MessageTypes::MAP_UPDATE) {
        // Handle map updates if needed
        qDebug() << "Map update received";
    }
    else if (message.messageType == MessageTypes::MAP_ERROR) {
        qWarning() << "Map service error:" << message.data["error"].toString();
    }
}

void NavigationController::sendRouteRequest(const Point& start, const Point& end)
{
    if (!m_processManager || m_operationMode != SERVICE_MODE) {
        qDebug() << "❌ [IPC REQUEST] Cannot send route request - ProcessManager unavailable or not in SERVICE_MODE";
        return;
    }
    
    qDebug() << "🚀 [IPC REQUEST] Sending route request to routing service...";
    qDebug() << "🚀 [IPC REQUEST] Start:" << start.latitude << "," << start.longitude;
    qDebug() << "🚀 [IPC REQUEST] End:" << end.latitude << "," << end.longitude;
    
    IPCMessage message;
    message.messageType = MessageTypes::ROUTE_REQUEST;
    message.serviceType = "Routing";
    message.data = MessageBuilder::createRouteRequest(
        QPointF(start.longitude, start.latitude),
        QPointF(end.longitude, end.latitude)
    );
    message.requestId = QUuid::createUuid().toString();
    
    qDebug() << "📡 [IPC REQUEST] Message created with ID:" << message.requestId;
    qDebug() << "📡 [IPC REQUEST] Sending to ProcessManager...";
    
    m_processManager->sendMessageToService(ServiceType::Routing, message);
    qDebug() << "✅ [IPC REQUEST] Route request sent successfully!";
}

void NavigationController::sendPositionRequest()
{
    if (!m_processManager || m_operationMode != SERVICE_MODE) {
        return;
    }
    
    IPCMessage message;
    message.messageType = MessageTypes::POSITION_REQUEST;
    message.serviceType = "Positioning";
    message.data = MessageBuilder::createPositionRequest();
    message.requestId = QUuid::createUuid().toString();
    
    m_processManager->sendMessageToService(ServiceType::Positioning, message);
}

} // namespace nav
