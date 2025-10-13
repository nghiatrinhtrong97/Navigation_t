#ifndef NAV_MODELS_H
#define NAV_MODELS_H

#include <QObject>
#include <QAbstractListModel>
#include <QSettings>
#include <QTimer>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <vector>
#include <memory>
#include "../../common/include/nav_types.h"

namespace nav {

// Forward declarations
class ServiceManager;

/**
 * @brief Model for managing navigation settings and configuration
 */
class NavigationSettingsModel : public QObject
{
    Q_OBJECT
    
    // General Settings
    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY languageChanged)
    Q_PROPERTY(QString units READ units WRITE setUnits NOTIFY unitsChanged)
    Q_PROPERTY(QString theme READ theme WRITE setTheme NOTIFY themeChanged)
    Q_PROPERTY(bool autoSave READ autoSave WRITE setAutoSave NOTIFY autoSaveChanged)
    
    // Map Settings
    Q_PROPERTY(QString mapStyle READ mapStyle WRITE setMapStyle NOTIFY mapStyleChanged)
    Q_PROPERTY(bool showTraffic READ showTraffic WRITE setShowTraffic NOTIFY showTrafficChanged)
    Q_PROPERTY(bool show3D READ show3D WRITE setShow3D NOTIFY show3DChanged)
    Q_PROPERTY(bool followPosition READ followPosition WRITE setFollowPosition NOTIFY followPositionChanged)
    Q_PROPERTY(int defaultZoom READ defaultZoom WRITE setDefaultZoom NOTIFY defaultZoomChanged)
    
    // Navigation Settings
    Q_PROPERTY(QString routingMode READ routingMode WRITE setRoutingMode NOTIFY routingModeChanged)
    Q_PROPERTY(bool avoidTolls READ avoidTolls WRITE setAvoidTolls NOTIFY avoidTollsChanged)
    Q_PROPERTY(bool avoidHighways READ avoidHighways WRITE setAvoidHighways NOTIFY avoidHighwaysChanged)
    Q_PROPERTY(bool voiceGuidance READ voiceGuidance WRITE setVoiceGuidance NOTIFY voiceGuidanceChanged)
    Q_PROPERTY(int voiceVolume READ voiceVolume WRITE setVoiceVolume NOTIFY voiceVolumeChanged)
    
    // Service Settings
    Q_PROPERTY(bool autoStart READ autoStart WRITE setAutoStart NOTIFY autoStartChanged)
    Q_PROPERTY(int reconnectInterval READ reconnectInterval WRITE setReconnectInterval NOTIFY reconnectIntervalChanged)
    Q_PROPERTY(int serviceTimeout READ serviceTimeout WRITE setServiceTimeout NOTIFY serviceTimeoutChanged)

public:
    explicit NavigationSettingsModel(QObject *parent = nullptr);
    ~NavigationSettingsModel();
    
    // Load and save settings
    Q_INVOKABLE void loadSettings();
    Q_INVOKABLE void saveSettings();
    Q_INVOKABLE void resetToDefaults();
    Q_INVOKABLE void exportSettings(const QString& filePath);
    Q_INVOKABLE bool importSettings(const QString& filePath);
    
    // General Settings
    QString language() const { return m_language; }
    QString units() const { return m_units; }
    QString theme() const { return m_theme; }
    bool autoSave() const { return m_autoSave; }
    
    // Map Settings
    QString mapStyle() const { return m_mapStyle; }
    bool showTraffic() const { return m_showTraffic; }
    bool show3D() const { return m_show3D; }
    bool followPosition() const { return m_followPosition; }
    int defaultZoom() const { return m_defaultZoom; }
    
    // Navigation Settings
    QString routingMode() const { return m_routingMode; }
    bool avoidTolls() const { return m_avoidTolls; }
    bool avoidHighways() const { return m_avoidHighways; }
    bool voiceGuidance() const { return m_voiceGuidance; }
    int voiceVolume() const { return m_voiceVolume; }
    
    // Service Settings
    bool autoStart() const { return m_autoStart; }
    int reconnectInterval() const { return m_reconnectInterval; }
    int serviceTimeout() const { return m_serviceTimeout; }
    
    // Available options
    Q_INVOKABLE QStringList availableLanguages() const;
    Q_INVOKABLE QStringList availableUnits() const;
    Q_INVOKABLE QStringList availableThemes() const;
    Q_INVOKABLE QStringList availableMapStyles() const;
    Q_INVOKABLE QStringList availableRoutingModes() const;

public slots:
    // General Settings
    void setLanguage(const QString& language);
    void setUnits(const QString& units);
    void setTheme(const QString& theme);
    void setAutoSave(bool autoSave);
    
    // Map Settings
    void setMapStyle(const QString& mapStyle);
    void setShowTraffic(bool showTraffic);
    void setShow3D(bool show3D);
    void setFollowPosition(bool followPosition);
    void setDefaultZoom(int defaultZoom);
    
    // Navigation Settings
    void setRoutingMode(const QString& routingMode);
    void setAvoidTolls(bool avoidTolls);
    void setAvoidHighways(bool avoidHighways);
    void setVoiceGuidance(bool voiceGuidance);
    void setVoiceVolume(int voiceVolume);
    
    // Service Settings
    void setAutoStart(bool autoStart);
    void setReconnectInterval(int reconnectInterval);
    void setServiceTimeout(int serviceTimeout);

signals:
    // General Settings
    void languageChanged(const QString& language);
    void unitsChanged(const QString& units);
    void themeChanged(const QString& theme);
    void autoSaveChanged(bool autoSave);
    
    // Map Settings
    void mapStyleChanged(const QString& mapStyle);
    void showTrafficChanged(bool showTraffic);
    void show3DChanged(bool show3D);
    void followPositionChanged(bool followPosition);
    void defaultZoomChanged(int defaultZoom);
    
    // Navigation Settings
    void routingModeChanged(const QString& routingMode);
    void avoidTollsChanged(bool avoidTolls);
    void avoidHighwaysChanged(bool avoidHighways);
    void voiceGuidanceChanged(bool voiceGuidance);
    void voiceVolumeChanged(int voiceVolume);
    
    // Service Settings
    void autoStartChanged(bool autoStart);
    void reconnectIntervalChanged(int reconnectInterval);
    void serviceTimeoutChanged(int serviceTimeout);
    
    // General signals
    void settingsChanged();
    void settingsLoaded();
    void settingsSaved();

private slots:
    void onAutoSaveTimer();

private:
    void initializeDefaults();
    QJsonObject settingsToJson() const;
    void settingsFromJson(const QJsonObject& json);
    
    // General Settings
    QString m_language;
    QString m_units;
    QString m_theme;
    bool m_autoSave;
    
    // Map Settings
    QString m_mapStyle;
    bool m_showTraffic;
    bool m_show3D;
    bool m_followPosition;
    int m_defaultZoom;
    
    // Navigation Settings
    QString m_routingMode;
    bool m_avoidTolls;
    bool m_avoidHighways;
    bool m_voiceGuidance;
    int m_voiceVolume;
    
    // Service Settings
    bool m_autoStart;
    int m_reconnectInterval;
    int m_serviceTimeout;
    
    // Internal
    QTimer* m_autoSaveTimer;
    bool m_settingsLoaded;
};

/**
 * @brief Model for managing navigation data and current state
 */
class NavigationDataModel : public QObject
{
    Q_OBJECT
    
    // Current Position
    Q_PROPERTY(double latitude READ latitude NOTIFY positionChanged)
    Q_PROPERTY(double longitude READ longitude NOTIFY positionChanged)
    Q_PROPERTY(double heading READ heading NOTIFY headingChanged)
    Q_PROPERTY(double speed READ speed NOTIFY speedChanged)
    Q_PROPERTY(double altitude READ altitude NOTIFY altitudeChanged)
    Q_PROPERTY(QString positionSource READ positionSource NOTIFY positionSourceChanged)
    Q_PROPERTY(QDateTime lastUpdate READ lastUpdate NOTIFY lastUpdateChanged)
    
    // Navigation State
    Q_PROPERTY(bool navigating READ navigating NOTIFY navigatingChanged)
    Q_PROPERTY(QString currentInstruction READ currentInstruction NOTIFY currentInstructionChanged)
    Q_PROPERTY(double distanceToNextManeuver READ distanceToNextManeuver NOTIFY distanceToNextManeuverChanged)
    Q_PROPERTY(double distanceRemaining READ distanceRemaining NOTIFY distanceRemainingChanged)
    Q_PROPERTY(int timeRemaining READ timeRemaining NOTIFY timeRemainingChanged)
    Q_PROPERTY(QDateTime estimatedArrival READ estimatedArrival NOTIFY estimatedArrivalChanged)
    
    // Route Information
    Q_PROPERTY(bool routeActive READ routeActive NOTIFY routeActiveChanged)
    Q_PROPERTY(double routeDistance READ routeDistance NOTIFY routeDistanceChanged)
    Q_PROPERTY(int routeDuration READ routeDuration NOTIFY routeDurationChanged)
    Q_PROPERTY(QString routeName READ routeName NOTIFY routeNameChanged)
    
    // Service Status
    Q_PROPERTY(bool positioningServiceConnected READ positioningServiceConnected NOTIFY positioningServiceConnectedChanged)
    Q_PROPERTY(bool routingServiceConnected READ routingServiceConnected NOTIFY routingServiceConnectedChanged)
    Q_PROPERTY(bool guidanceServiceConnected READ guidanceServiceConnected NOTIFY guidanceServiceConnectedChanged)
    Q_PROPERTY(bool mapServiceConnected READ mapServiceConnected NOTIFY mapServiceConnectedChanged)

public:
    explicit NavigationDataModel(QObject *parent = nullptr);
    ~NavigationDataModel();
    
    void setServiceManager(ServiceManager* serviceManager);
    
    // Current Position
    double latitude() const { return m_latitude; }
    double longitude() const { return m_longitude; }
    double heading() const { return m_heading; }
    double speed() const { return m_speed; }
    double altitude() const { return m_altitude; }
    QString positionSource() const { return m_positionSource; }
    QDateTime lastUpdate() const { return m_lastUpdate; }
    
    // Navigation State
    bool navigating() const { return m_navigating; }
    QString currentInstruction() const { return m_currentInstruction; }
    double distanceToNextManeuver() const { return m_distanceToNextManeuver; }
    double distanceRemaining() const { return m_distanceRemaining; }
    int timeRemaining() const { return m_timeRemaining; }
    QDateTime estimatedArrival() const { return m_estimatedArrival; }
    
    // Route Information
    bool routeActive() const { return m_routeActive; }
    double routeDistance() const { return m_routeDistance; }
    int routeDuration() const { return m_routeDuration; }
    QString routeName() const { return m_routeName; }
    
    // Service Status
    bool positioningServiceConnected() const { return m_positioningServiceConnected; }
    bool routingServiceConnected() const { return m_routingServiceConnected; }
    bool guidanceServiceConnected() const { return m_guidanceServiceConnected; }
    bool mapServiceConnected() const { return m_mapServiceConnected; }
    
    // Current route points
    std::vector<Point> getCurrentRoute() const { return m_currentRoute; }
    Point getCurrentPosition() const { return Point(m_latitude, m_longitude); }

public slots:
    void updatePosition(double lat, double lon, double heading, double speed, double altitude, const QString& source);
    void updateNavigationState(bool navigating, const QString& instruction, double distanceToNext, 
                              double distanceRemaining, int timeRemaining);
    void updateRoute(const std::vector<Point>& route, const QString& routeName, double distance, int duration);
    void clearRoute();
    void updateServiceStatus(const QString& service, bool connected);

signals:
    // Position signals
    void positionChanged(double latitude, double longitude);
    void headingChanged(double heading);
    void speedChanged(double speed);
    void altitudeChanged(double altitude);
    void positionSourceChanged(const QString& source);
    void lastUpdateChanged(const QDateTime& lastUpdate);
    
    // Navigation signals
    void navigatingChanged(bool navigating);
    void currentInstructionChanged(const QString& instruction);
    void distanceToNextManeuverChanged(double distance);
    void distanceRemainingChanged(double distance);
    void timeRemainingChanged(int timeRemaining);
    void estimatedArrivalChanged(const QDateTime& arrival);
    
    // Route signals
    void routeActiveChanged(bool active);
    void routeDistanceChanged(double distance);
    void routeDurationChanged(int duration);
    void routeNameChanged(const QString& name);
    void routeUpdated(const std::vector<Point>& route);
    
    // Service status signals
    void positioningServiceConnectedChanged(bool connected);
    void routingServiceConnectedChanged(bool connected);
    void guidanceServiceConnectedChanged(bool connected);
    void mapServiceConnectedChanged(bool connected);

private:
    void updateLastUpdate();
    
    // Current Position
    double m_latitude;
    double m_longitude;
    double m_heading;
    double m_speed;
    double m_altitude;
    QString m_positionSource;
    QDateTime m_lastUpdate;
    
    // Navigation State
    bool m_navigating;
    QString m_currentInstruction;
    double m_distanceToNextManeuver;
    double m_distanceRemaining;
    int m_timeRemaining;
    QDateTime m_estimatedArrival;
    
    // Route Information
    bool m_routeActive;
    double m_routeDistance;
    int m_routeDuration;
    QString m_routeName;
    std::vector<Point> m_currentRoute;
    
    // Service Status
    bool m_positioningServiceConnected;
    bool m_routingServiceConnected;
    bool m_guidanceServiceConnected;
    bool m_mapServiceConnected;
    
    ServiceManager* m_serviceManager;
};

/**
 * @brief Model for managing route history and favorites
 */
class RouteHistoryModel : public QAbstractListModel
{
    Q_OBJECT

public:
    struct RouteEntry {
        QString name;
        Point startPoint;
        Point endPoint;
        QDateTime timestamp;
        double distance;
        int duration;
        bool favorite;
        
        RouteEntry() : distance(0.0), duration(0), favorite(false) {}
    };
    
    enum RouteRoles {
        NameRole = Qt::UserRole + 1,
        StartLatRole,
        StartLonRole,
        EndLatRole,
        EndLonRole,
        TimestampRole,
        DistanceRole,
        DurationRole,
        FavoriteRole
    };

public:
    explicit RouteHistoryModel(QObject *parent = nullptr);
    ~RouteHistoryModel();
    
    // QAbstractListModel interface
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QHash<int, QByteArray> roleNames() const override;
    
    // Route management
    Q_INVOKABLE void addRoute(const QString& name, double startLat, double startLon, 
                             double endLat, double endLon, double distance, int duration);
    Q_INVOKABLE void removeRoute(int index);
    Q_INVOKABLE void clearHistory();
    Q_INVOKABLE void toggleFavorite(int index);
    Q_INVOKABLE void loadHistory();
    Q_INVOKABLE void saveHistory();
    
    // Filtering and searching
    Q_INVOKABLE void setShowFavoritesOnly(bool favoritesOnly);
    Q_INVOKABLE void setSearchFilter(const QString& filter);
    Q_INVOKABLE QVariantMap getRoute(int index) const;

signals:
    void routeSelected(double startLat, double startLon, double endLat, double endLon);
    void historyChanged();

private:
    void filterRoutes();
    void sortRoutes();
    
    std::vector<RouteEntry> m_routes;
    std::vector<RouteEntry> m_filteredRoutes;
    bool m_showFavoritesOnly;
    QString m_searchFilter;
};

} // namespace nav

#endif // NAV_MODELS_H
