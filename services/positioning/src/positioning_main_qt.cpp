#include "positioning_service.h"
#include "nav_utils.h"
#include <QCoreApplication>
#include <QLocalServer>
#include <QLocalSocket>
#include <QDataStream>
#include <QTimer>
#include <QDebug>
#include <iostream>

class QtPositioningService : public QObject {
    Q_OBJECT

private:
    QLocalServer* m_server;
    QList<QLocalSocket*> m_clients;
    QTimer* m_updateTimer;
    nav::Point m_currentPosition;
    double m_currentHeading;
    double m_currentSpeed;
    bool m_positioningActive;

public:
    QtPositioningService(QObject* parent = nullptr) 
        : QObject(parent)
        , m_server(nullptr)
        , m_updateTimer(nullptr)
        , m_currentPosition(21.028511, 105.804817)
        , m_currentHeading(0.0)
        , m_currentSpeed(0.0)
        , m_positioningActive(false)
    {
        // Setup server
        m_server = new QLocalServer(this);
        connect(m_server, &QLocalServer::newConnection, 
                this, &QtPositioningService::onNewConnection);
        
        // Setup update timer
        m_updateTimer = new QTimer(this);
        connect(m_updateTimer, &QTimer::timeout,
                this, &QtPositioningService::sendPositionUpdates);
        m_updateTimer->setInterval(100); // 10 Hz
        
        qDebug() << "Qt Positioning Service initialized";
    }
    
    ~QtPositioningService() {
        stop();
    }
    
    bool start() {
        // Remove any existing server
        QLocalServer::removeServer("nav_positioning");
        
        if (!m_server->listen("nav_positioning")) {
            qDebug() << "Failed to start positioning server:" << m_server->errorString();
            return false;
        }
        
        qDebug() << "✓ Positioning service listening on 'nav_positioning'";
        return true;
    }
    
    void stop() {
        if (m_updateTimer) {
            m_updateTimer->stop();
        }
        
        // Close all client connections
        for (auto client : m_clients) {
            client->disconnectFromServer();
            client->deleteLater();
        }
        m_clients.clear();
        
        if (m_server) {
            m_server->close();
        }
        
        qDebug() << "Positioning service stopped";
    }

private slots:
    void onNewConnection() {
        while (m_server->hasPendingConnections()) {
            QLocalSocket* client = m_server->nextPendingConnection();
            
            connect(client, &QLocalSocket::readyRead, 
                    this, &QtPositioningService::onClientDataReceived);
            connect(client, &QLocalSocket::disconnected,
                    this, &QtPositioningService::onClientDisconnected);
            
            m_clients.append(client);
            
            qDebug() << "✓ New positioning client connected. Total clients:" << m_clients.size();
        }
    }
    
    void onClientDataReceived() {
        QLocalSocket* client = qobject_cast<QLocalSocket*>(sender());
        if (!client) return;
        
        while (client->bytesAvailable()) {
            QByteArray data = client->readAll();
            QDataStream stream(&data, QIODevice::ReadOnly);
            
            QString messageType;
            stream >> messageType;
            
            if (messageType == "START_POSITIONING") {
                nav::PositioningRequest request;
                stream.readRawData(reinterpret_cast<char*>(&request), sizeof(request));
                
                m_positioningActive = true;
                m_updateTimer->start();
                
                qDebug() << "Positioning started for client";
                
            } else if (messageType == "STOP_POSITIONING") {
                nav::PositioningRequest request;
                stream.readRawData(reinterpret_cast<char*>(&request), sizeof(request));
                
                m_positioningActive = false;
                m_updateTimer->stop();
                
                qDebug() << "Positioning stopped for client";
                
            } else if (messageType == "SET_POSITION") {
                nav::PositioningRequest request;
                stream.readRawData(reinterpret_cast<char*>(&request), sizeof(request));
                
                m_currentPosition = request.position;
                qDebug() << "Position set to:" << m_currentPosition.latitude << "," << m_currentPosition.longitude;
            }
        }
    }
    
    void onClientDisconnected() {
        QLocalSocket* client = qobject_cast<QLocalSocket*>(sender());
        if (client) {
            m_clients.removeAll(client);
            client->deleteLater();
            
            qDebug() << "✗ Positioning client disconnected. Remaining clients:" << m_clients.size();
            
            // Stop updates if no clients
            if (m_clients.isEmpty()) {
                m_positioningActive = false;
                m_updateTimer->stop();
            }
        }
    }
    
    void sendPositionUpdates() {
        if (!m_positioningActive || m_clients.isEmpty()) {
            return;
        }
        
        // Simulate some movement (for demo)
        static double time = 0.0;
        time += 0.1;
        
        m_currentPosition.latitude += 0.00001 * sin(time * 0.1);
        m_currentPosition.longitude += 0.00001 * cos(time * 0.1);
        m_currentHeading = fmod(time * 10.0, 360.0);
        m_currentSpeed = 50.0 + 10.0 * sin(time * 0.5); // 40-60 km/h
        
        // Send to all connected clients
        for (auto client : m_clients) {
            if (client->state() == QLocalSocket::ConnectedState) {
                QByteArray data;
                QDataStream stream(&data, QIODevice::WriteOnly);
                
                nav::PositioningResponse response;
                response.position = m_currentPosition;
                response.heading = m_currentHeading;
                response.speed = m_currentSpeed;
                
                stream << QString("POSITION_UPDATE");
                stream.writeRawData(reinterpret_cast<const char*>(&response), sizeof(response));
                
                client->write(data);
                client->flush();
            }
        }
    }
};

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    
    qDebug() << "=== Qt Navigation Positioning Service ===";
    
    QtPositioningService service;
    
    if (!service.start()) {
        qDebug() << "Failed to start positioning service";
        return 1;
    }
    
    qDebug() << "Positioning service running... Press Ctrl+C to stop";
    
    return app.exec();
}

#include "positioning_main_qt.moc"