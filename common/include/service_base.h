#ifndef SERVICE_BASE_H
#define SERVICE_BASE_H

#include <QObject>
#include <QLocalSocket>
#include <QTimer>
#include <QCoreApplication>
#include <QCommandLineParser>
#include <QJsonObject>
#include <QJsonDocument>
#include <QString>
#include <memory>

namespace nav {

/**
 * @brief Base class for all navigation services providing IPC functionality
 * 
 * This class provides common IPC communication functionality that all
 * service executables can inherit from to communicate with the main
 * navigation application.
 */
class ServiceBase : public QObject
{
    Q_OBJECT

public:
    explicit ServiceBase(const QString& serviceName, QObject *parent = nullptr);
    virtual ~ServiceBase();
    
    // Service lifecycle
    virtual bool initialize();
    virtual void shutdown();
    
    // IPC connection management
    bool connectToParent(const QString& serverName = "nav_system_ipc");
    void disconnectFromParent();
    bool isConnectedToParent() const;
    
    // Message handling
    void sendMessage(const QJsonObject& messageData, const QString& messageType);
    void sendRegistrationMessage();
    void sendHeartbeat();
    void sendErrorMessage(const QString& error, int errorCode = -1);

signals:
    // Service lifecycle signals
    void serviceReady();
    void serviceShuttingDown();
    
    // IPC communication signals
    void connectedToParent();
    void disconnectedFromParent();
    void messageReceived(const QString& messageType, const QJsonObject& data);
    void parentConnectionError(const QString& error);

protected slots:
    // IPC event handlers
    void onConnectedToParent();
    void onDisconnectedFromParent();
    void onParentDataReady();
    void onParentError(QLocalSocket::LocalSocketError error);
    
    // Lifecycle handlers
    void onHeartbeatTimeout();

protected:
    // Virtual methods for subclasses to implement
    virtual QString getServiceVersion() const { return "1.0"; }
    virtual bool initializeService() = 0;
    virtual void shutdownService() = 0;
    virtual void handleMessage(const QString& messageType, const QJsonObject& data) = 0;
    
    // Utility methods
    void parseCommandLineArguments();
    QString getServiceName() const { return m_serviceName; }
    
    // IPC helper methods
    void processIncomingMessage(const QByteArray& data);
    void handleSystemCommand(const QJsonObject& data);

private:
    QString m_serviceName;
    std::unique_ptr<QLocalSocket> m_parentSocket;
    std::unique_ptr<QTimer> m_heartbeatTimer;
    QString m_parentServerName;
    bool m_connected;
    bool m_registrationSent;
    
    // Command line options
    QString m_ipcServerName;
    bool m_verboseLogging;
};

/**
 * @brief Macro to help create main function for services
 */
#define IMPLEMENT_SERVICE_MAIN(ServiceClass, serviceName) \
int main(int argc, char *argv[]) \
{ \
    QCoreApplication app(argc, argv); \
    app.setApplicationName(serviceName); \
    app.setApplicationVersion("1.0"); \
    \
    ServiceClass service; \
    if (!service.initialize()) { \
        qCritical() << "Failed to initialize" << serviceName; \
        return -1; \
    } \
    \
    QObject::connect(&service, &ServiceClass::serviceShuttingDown, \
                     &app, &QCoreApplication::quit); \
    \
    qInfo() << serviceName << "started successfully"; \
    return app.exec(); \
}

} // namespace nav

#endif // SERVICE_BASE_H