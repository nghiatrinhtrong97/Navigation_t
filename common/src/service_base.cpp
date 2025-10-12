#include "service_base.h"
#include <QDebug>
#include <QCommandLineParser>
#include <QJsonDocument>
#include <QJsonObject>
#include <QCoreApplication>
#include <QUuid>

namespace nav {

ServiceBase::ServiceBase(const QString& serviceName, QObject *parent)
    : QObject(parent)
    , m_serviceName(serviceName)
    , m_parentSocket(std::make_unique<QLocalSocket>(this))
    , m_heartbeatTimer(std::make_unique<QTimer>(this))
    , m_parentServerName("nav_system_ipc")
    , m_connected(false)
    , m_registrationSent(false)
    , m_verboseLogging(false)
{
    // Setup parent socket connections
    connect(m_parentSocket.get(), &QLocalSocket::connected,
            this, &ServiceBase::onConnectedToParent);
    connect(m_parentSocket.get(), &QLocalSocket::disconnected,
            this, &ServiceBase::onDisconnectedFromParent);
    connect(m_parentSocket.get(), &QLocalSocket::readyRead,
            this, &ServiceBase::onParentDataReady);
    connect(m_parentSocket.get(), QOverload<QLocalSocket::LocalSocketError>::of(&QLocalSocket::errorOccurred),
            this, &ServiceBase::onParentError);
    
    // Setup heartbeat timer
    connect(m_heartbeatTimer.get(), &QTimer::timeout,
            this, &ServiceBase::onHeartbeatTimeout);
    m_heartbeatTimer->setInterval(10000); // 10 seconds
}

ServiceBase::~ServiceBase()
{
    shutdown();
}

bool ServiceBase::initialize()
{
    parseCommandLineArguments();
    
    qInfo() << "Initializing service:" << m_serviceName;
    
    // Initialize the specific service
    if (!initializeService()) {
        qCritical() << "Failed to initialize service implementation";
        return false;
    }
    
    // Connect to parent process
    if (!connectToParent(m_ipcServerName.isEmpty() ? m_parentServerName : m_ipcServerName)) {
        qWarning() << "Failed to connect to parent process. Service will run standalone.";
        // Don't fail initialization - service can run standalone
    }
    
    emit serviceReady();
    qInfo() << "Service" << m_serviceName << "initialized successfully";
    return true;
}

void ServiceBase::shutdown()
{
    qInfo() << "Shutting down service:" << m_serviceName;
    
    emit serviceShuttingDown();
    
    // Stop heartbeat
    m_heartbeatTimer->stop();
    
    // Shutdown specific service implementation
    shutdownService();
    
    // Disconnect from parent
    disconnectFromParent();
    
    qInfo() << "Service" << m_serviceName << "shutdown complete";
}

bool ServiceBase::connectToParent(const QString& serverName)
{
    if (m_connected) {
        qDebug() << "🔗 [SERVICE BASE] Already connected to parent";
        return true;
    }
    
    m_parentServerName = serverName;
    
    qDebug() << "🔗 [SERVICE BASE] Connecting to parent IPC server:" << serverName;
    qDebug() << "🔗 [SERVICE BASE] Current socket state:" << m_parentSocket->state();
    
    m_parentSocket->connectToServer(serverName);
    
    qDebug() << "🔗 [SERVICE BASE] Waiting for connection (5 seconds timeout)...";
    if (!m_parentSocket->waitForConnected(5000)) {
        qWarning() << "❌ [SERVICE BASE] Failed to connect to parent:" << m_parentSocket->errorString();
        qWarning() << "❌ [SERVICE BASE] Socket error:" << m_parentSocket->error();
        qWarning() << "❌ [SERVICE BASE] Socket state:" << m_parentSocket->state();
        return false;
    }
    
    qDebug() << "✅ [SERVICE BASE] Successfully connected to parent IPC server!";
    return true;
}

void ServiceBase::disconnectFromParent()
{
    if (!m_connected) {
        return;
    }
    
    qDebug() << "Disconnecting from parent";
    
    if (m_parentSocket->state() != QLocalSocket::UnconnectedState) {
        m_parentSocket->disconnectFromServer();
        m_parentSocket->waitForDisconnected(3000);
    }
}

bool ServiceBase::isConnectedToParent() const
{
    return m_connected;
}

void ServiceBase::sendMessage(const QJsonObject& messageData, const QString& messageType)
{
    if (!m_connected) {
        qWarning() << "Cannot send message: not connected to parent";
        return;
    }
    
    QJsonObject message;
    message["messageType"] = messageType;
    message["serviceType"] = m_serviceName;
    message["data"] = messageData;
    message["requestId"] = QUuid::createUuid().toString();
    
    QByteArray data = QJsonDocument(message).toJson(QJsonDocument::Compact) + "\n";
    
    qint64 written = m_parentSocket->write(data);
    if (written != data.size()) {
        qWarning() << "Failed to send complete message to parent";
    } else {
        m_parentSocket->flush();
        if (m_verboseLogging) {
            qDebug() << "Sent message:" << messageType << "to parent";
        }
    }
}

void ServiceBase::sendRegistrationMessage()
{
    QJsonObject data;
    data["serviceType"] = m_serviceName;
    data["version"] = getServiceVersion();
    data["pid"] = static_cast<qint64>(QCoreApplication::applicationPid());
    
    sendMessage(data, "service_registration");
    m_registrationSent = true;
    
    qInfo() << "Registration message sent for service:" << m_serviceName;
}

void ServiceBase::sendHeartbeat()
{
    QJsonObject data;
    data["serviceType"] = m_serviceName;
    data["status"] = "ok";
    
    sendMessage(data, "heartbeat");
    
    if (m_verboseLogging) {
        qDebug() << "Heartbeat sent";
    }
}

void ServiceBase::sendErrorMessage(const QString& error, int errorCode)
{
    QJsonObject data;
    data["error"] = error;
    data["errorCode"] = errorCode;
    
    sendMessage(data, "service_error");
    
    qWarning() << "Error message sent:" << error;
}

// Slot implementations
void ServiceBase::onConnectedToParent()
{
    m_connected = true;
    qInfo() << "Connected to parent process";
    
    // Send registration message
    sendRegistrationMessage();
    
    // Start heartbeat
    m_heartbeatTimer->start();
    
    emit connectedToParent();
}

void ServiceBase::onDisconnectedFromParent()
{
    m_connected = false;
    m_registrationSent = false;
    
    // Stop heartbeat
    m_heartbeatTimer->stop();
    
    qWarning() << "Disconnected from parent process";
    emit disconnectedFromParent();
}

void ServiceBase::onParentDataReady()
{
    while (m_parentSocket->canReadLine()) {
        QByteArray data = m_parentSocket->readLine().trimmed();
        if (!data.isEmpty()) {
            processIncomingMessage(data);
        }
    }
}

void ServiceBase::onParentError(QLocalSocket::LocalSocketError error)
{
    QString errorString;
    switch (error) {
        case QLocalSocket::ConnectionRefusedError:
            errorString = "Connection refused";
            break;
        case QLocalSocket::PeerClosedError:
            errorString = "Parent closed connection";
            break;
        case QLocalSocket::ServerNotFoundError:
            errorString = "Parent server not found";
            break;
        case QLocalSocket::SocketAccessError:
            errorString = "Socket access error";
            break;
        case QLocalSocket::SocketResourceError:
            errorString = "Socket resource error";
            break;
        case QLocalSocket::SocketTimeoutError:
            errorString = "Socket timeout";
            break;
        default:
            errorString = "Unknown error";
            break;
    }
    
    qWarning() << "Parent connection error:" << errorString;
    emit parentConnectionError(errorString);
}

void ServiceBase::onHeartbeatTimeout()
{
    sendHeartbeat();
}

// Protected methods
void ServiceBase::parseCommandLineArguments()
{
    QCommandLineParser parser;
    parser.setApplicationDescription(QString("%1 Navigation Service").arg(m_serviceName));
    parser.addHelpOption();
    parser.addVersionOption();
    
    QCommandLineOption ipcOption(QStringList() << "i" << "ipc-server",
                                "IPC server name to connect to.",
                                "server-name");
    parser.addOption(ipcOption);
    
    QCommandLineOption verboseOption(QStringList() << "v" << "verbose",
                                   "Enable verbose logging.");
    parser.addOption(verboseOption);
    
    parser.process(*QCoreApplication::instance());
    
    if (parser.isSet(ipcOption)) {
        m_ipcServerName = parser.value(ipcOption);
        qDebug() << "Using IPC server:" << m_ipcServerName;
    }
    
    if (parser.isSet(verboseOption)) {
        m_verboseLogging = true;
        qDebug() << "Verbose logging enabled";
    }
}

void ServiceBase::processIncomingMessage(const QByteArray& data)
{
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    
    if (error.error != QJsonParseError::NoError) {
        qWarning() << "Failed to parse message from parent:" << error.errorString();
        return;
    }
    
    if (!doc.isObject()) {
        qWarning() << "Received non-object message from parent";
        return;
    }
    
    QJsonObject message = doc.object();
    QString messageType = message["messageType"].toString();
    QJsonObject messageData = message["data"].toObject();
    
    if (m_verboseLogging) {
        qDebug() << "Received message:" << messageType;
    }
    
    // Handle system commands
    if (messageType == "system_command") {
        handleSystemCommand(messageData);
        return;
    }
    
    // Forward to service-specific handler
    handleMessage(messageType, messageData);
    emit messageReceived(messageType, messageData);
}

void ServiceBase::handleSystemCommand(const QJsonObject& data)
{
    QString command = data["command"].toString();
    
    if (command == "shutdown") {
        qInfo() << "Received shutdown command from parent";
        QTimer::singleShot(100, this, &ServiceBase::shutdown);
    }
    else if (command == "ping") {
        // Respond to ping with pong
        QJsonObject response;
        response["command"] = "pong";
        response["serviceType"] = m_serviceName;
        sendMessage(response, "system_response");
    }
    else {
        qWarning() << "Unknown system command:" << command;
    }
}

} // namespace nav