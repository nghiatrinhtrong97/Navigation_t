#include "../include/navigation_models.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include <algorithm>

namespace nav {

// =============================================================================
// NavigationSettingsModel Implementation
// =============================================================================

NavigationSettingsModel::NavigationSettingsModel(QObject *parent)
    : QObject(parent)
    , m_autoSaveTimer(new QTimer(this))
    , m_settingsLoaded(false)
{
    initializeDefaults();
    
    // Setup auto-save timer
    m_autoSaveTimer->setSingleShot(true);
    m_autoSaveTimer->setInterval(5000); // 5 seconds delay
    connect(m_autoSaveTimer, &QTimer::timeout, this, &NavigationSettingsModel::onAutoSaveTimer);
    
    // Connect all property changes to auto-save trigger
    connect(this, &NavigationSettingsModel::settingsChanged, [this]() {
        if (m_autoSave && m_settingsLoaded) {
            m_autoSaveTimer->start();
        }
    });
    
    loadSettings();
    
    qDebug() << "NavigationSettingsModel initialized";
}

NavigationSettingsModel::~NavigationSettingsModel()
{
    if (m_autoSave) {
        saveSettings();
    }
}

void NavigationSettingsModel::initializeDefaults()
{
    // General Settings
    m_language = "Vietnamese";
    m_units = "Metric";
    m_theme = "Dark";
    m_autoSave = true;
    
    // Map Settings
    m_mapStyle = "Satellite";
    m_showTraffic = true;
    m_show3D = false;
    m_followPosition = true;
    m_defaultZoom = 15;
    
    // Navigation Settings
    m_routingMode = "Fastest";
    m_avoidTolls = false;
    m_avoidHighways = false;
    m_voiceGuidance = true;
    m_voiceVolume = 80;
    
    // Service Settings
    m_autoStart = true;
    m_reconnectInterval = 5000;
    m_serviceTimeout = 10000;
}

void NavigationSettingsModel::loadSettings()
{
    QSettings settings;
    settings.beginGroup("NavigationSettings");
    
    // General Settings
    setLanguage(settings.value("language", m_language).toString());
    setUnits(settings.value("units", m_units).toString());
    setTheme(settings.value("theme", m_theme).toString());
    setAutoSave(settings.value("autoSave", m_autoSave).toBool());
    
    // Map Settings
    setMapStyle(settings.value("mapStyle", m_mapStyle).toString());
    setShowTraffic(settings.value("showTraffic", m_showTraffic).toBool());
    setShow3D(settings.value("show3D", m_show3D).toBool());
    setFollowPosition(settings.value("followPosition", m_followPosition).toBool());
    setDefaultZoom(settings.value("defaultZoom", m_defaultZoom).toInt());
    
    // Navigation Settings
    setRoutingMode(settings.value("routingMode", m_routingMode).toString());
    setAvoidTolls(settings.value("avoidTolls", m_avoidTolls).toBool());
    setAvoidHighways(settings.value("avoidHighways", m_avoidHighways).toBool());
    setVoiceGuidance(settings.value("voiceGuidance", m_voiceGuidance).toBool());
    setVoiceVolume(settings.value("voiceVolume", m_voiceVolume).toInt());
    
    // Service Settings
    setAutoStart(settings.value("autoStart", m_autoStart).toBool());
    setReconnectInterval(settings.value("reconnectInterval", m_reconnectInterval).toInt());
    setServiceTimeout(settings.value("serviceTimeout", m_serviceTimeout).toInt());
    
    settings.endGroup();
    
    m_settingsLoaded = true;
    emit settingsLoaded();
    
    qDebug() << "Navigation settings loaded";
}

void NavigationSettingsModel::saveSettings()
{
    QSettings settings;
    settings.beginGroup("NavigationSettings");
    
    // General Settings
    settings.setValue("language", m_language);
    settings.setValue("units", m_units);
    settings.setValue("theme", m_theme);
    settings.setValue("autoSave", m_autoSave);
    
    // Map Settings
    settings.setValue("mapStyle", m_mapStyle);
    settings.setValue("showTraffic", m_showTraffic);
    settings.setValue("show3D", m_show3D);
    settings.setValue("followPosition", m_followPosition);
    settings.setValue("defaultZoom", m_defaultZoom);
    
    // Navigation Settings
    settings.setValue("routingMode", m_routingMode);
    settings.setValue("avoidTolls", m_avoidTolls);
    settings.setValue("avoidHighways", m_avoidHighways);
    settings.setValue("voiceGuidance", m_voiceGuidance);
    settings.setValue("voiceVolume", m_voiceVolume);
    
    // Service Settings
    settings.setValue("autoStart", m_autoStart);
    settings.setValue("reconnectInterval", m_reconnectInterval);
    settings.setValue("serviceTimeout", m_serviceTimeout);
    
    settings.endGroup();
    settings.sync();
    
    emit settingsSaved();
    
    qDebug() << "Navigation settings saved";
}

void NavigationSettingsModel::resetToDefaults()
{
    initializeDefaults();
    emit settingsChanged();
    qDebug() << "Settings reset to defaults";
}

void NavigationSettingsModel::exportSettings(const QString& filePath)
{
    QJsonObject json = settingsToJson();
    QJsonDocument doc(json);
    
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        file.close();
        qDebug() << "Settings exported to:" << filePath;
    } else {
        qWarning() << "Failed to export settings to:" << filePath;
    }
}

bool NavigationSettingsModel::importSettings(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open settings file:" << filePath;
        return false;
    }
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
    file.close();
    
    if (error.error != QJsonParseError::NoError) {
        qWarning() << "Failed to parse settings JSON:" << error.errorString();
        return false;
    }
    
    settingsFromJson(doc.object());
    emit settingsChanged();
    
    qDebug() << "Settings imported from:" << filePath;
    return true;
}

QStringList NavigationSettingsModel::availableLanguages() const
{
    return QStringList() << "Vietnamese" << "English" << "Chinese" << "Japanese" << "Korean";
}

QStringList NavigationSettingsModel::availableUnits() const
{
    return QStringList() << "Metric" << "Imperial";
}

QStringList NavigationSettingsModel::availableThemes() const
{
    return QStringList() << "Light" << "Dark" << "Auto";
}

QStringList NavigationSettingsModel::availableMapStyles() const
{
    return QStringList() << "Satellite" << "Street Map" << "Hybrid";
}

QStringList NavigationSettingsModel::availableRoutingModes() const
{
    return QStringList() << "Fastest" << "Shortest" << "Eco" << "Avoid Highways";
}

// Setters with change notifications
void NavigationSettingsModel::setLanguage(const QString& language)
{
    if (m_language != language) {
        m_language = language;
        emit languageChanged(m_language);
        emit settingsChanged();
    }
}

void NavigationSettingsModel::setUnits(const QString& units)
{
    if (m_units != units) {
        m_units = units;
        emit unitsChanged(m_units);
        emit settingsChanged();
    }
}

void NavigationSettingsModel::setTheme(const QString& theme)
{
    if (m_theme != theme) {
        m_theme = theme;
        emit themeChanged(m_theme);
        emit settingsChanged();
    }
}

void NavigationSettingsModel::setAutoSave(bool autoSave)
{
    if (m_autoSave != autoSave) {
        m_autoSave = autoSave;
        emit autoSaveChanged(m_autoSave);
        emit settingsChanged();
    }
}

void NavigationSettingsModel::setMapStyle(const QString& mapStyle)
{
    if (m_mapStyle != mapStyle) {
        m_mapStyle = mapStyle;
        emit mapStyleChanged(m_mapStyle);
        emit settingsChanged();
    }
}

void NavigationSettingsModel::setShowTraffic(bool showTraffic)
{
    if (m_showTraffic != showTraffic) {
        m_showTraffic = showTraffic;
        emit showTrafficChanged(m_showTraffic);
        emit settingsChanged();
    }
}

void NavigationSettingsModel::setShow3D(bool show3D)
{
    if (m_show3D != show3D) {
        m_show3D = show3D;
        emit show3DChanged(m_show3D);
        emit settingsChanged();
    }
}

void NavigationSettingsModel::setFollowPosition(bool followPosition)
{
    if (m_followPosition != followPosition) {
        m_followPosition = followPosition;
        emit followPositionChanged(m_followPosition);
        emit settingsChanged();
    }
}

void NavigationSettingsModel::setDefaultZoom(int defaultZoom)
{
    if (m_defaultZoom != defaultZoom) {
        m_defaultZoom = defaultZoom;
        emit defaultZoomChanged(m_defaultZoom);
        emit settingsChanged();
    }
}

void NavigationSettingsModel::setRoutingMode(const QString& routingMode)
{
    if (m_routingMode != routingMode) {
        m_routingMode = routingMode;
        emit routingModeChanged(m_routingMode);
        emit settingsChanged();
    }
}

void NavigationSettingsModel::setAvoidTolls(bool avoidTolls)
{
    if (m_avoidTolls != avoidTolls) {
        m_avoidTolls = avoidTolls;
        emit avoidTollsChanged(m_avoidTolls);
        emit settingsChanged();
    }
}

void NavigationSettingsModel::setAvoidHighways(bool avoidHighways)
{
    if (m_avoidHighways != avoidHighways) {
        m_avoidHighways = avoidHighways;
        emit avoidHighwaysChanged(m_avoidHighways);
        emit settingsChanged();
    }
}

void NavigationSettingsModel::setVoiceGuidance(bool voiceGuidance)
{
    if (m_voiceGuidance != voiceGuidance) {
        m_voiceGuidance = voiceGuidance;
        emit voiceGuidanceChanged(m_voiceGuidance);
        emit settingsChanged();
    }
}

void NavigationSettingsModel::setVoiceVolume(int voiceVolume)
{
    if (m_voiceVolume != voiceVolume) {
        m_voiceVolume = voiceVolume;
        emit voiceVolumeChanged(m_voiceVolume);
        emit settingsChanged();
    }
}

void NavigationSettingsModel::setAutoStart(bool autoStart)
{
    if (m_autoStart != autoStart) {
        m_autoStart = autoStart;
        emit autoStartChanged(m_autoStart);
        emit settingsChanged();
    }
}

void NavigationSettingsModel::setReconnectInterval(int reconnectInterval)
{
    if (m_reconnectInterval != reconnectInterval) {
        m_reconnectInterval = reconnectInterval;
        emit reconnectIntervalChanged(m_reconnectInterval);
        emit settingsChanged();
    }
}

void NavigationSettingsModel::setServiceTimeout(int serviceTimeout)
{
    if (m_serviceTimeout != serviceTimeout) {
        m_serviceTimeout = serviceTimeout;
        emit serviceTimeoutChanged(m_serviceTimeout);
        emit settingsChanged();
    }
}

void NavigationSettingsModel::onAutoSaveTimer()
{
    saveSettings();
}

QJsonObject NavigationSettingsModel::settingsToJson() const
{
    QJsonObject json;
    
    // General Settings
    json["language"] = m_language;
    json["units"] = m_units;
    json["theme"] = m_theme;
    json["autoSave"] = m_autoSave;
    
    // Map Settings
    json["mapStyle"] = m_mapStyle;
    json["showTraffic"] = m_showTraffic;
    json["show3D"] = m_show3D;
    json["followPosition"] = m_followPosition;
    json["defaultZoom"] = m_defaultZoom;
    
    // Navigation Settings
    json["routingMode"] = m_routingMode;
    json["avoidTolls"] = m_avoidTolls;
    json["avoidHighways"] = m_avoidHighways;
    json["voiceGuidance"] = m_voiceGuidance;
    json["voiceVolume"] = m_voiceVolume;
    
    // Service Settings
    json["autoStart"] = m_autoStart;
    json["reconnectInterval"] = m_reconnectInterval;
    json["serviceTimeout"] = m_serviceTimeout;
    
    return json;
}

void NavigationSettingsModel::settingsFromJson(const QJsonObject& json)
{
    // General Settings
    if (json.contains("language")) setLanguage(json["language"].toString());
    if (json.contains("units")) setUnits(json["units"].toString());
    if (json.contains("theme")) setTheme(json["theme"].toString());
    if (json.contains("autoSave")) setAutoSave(json["autoSave"].toBool());
    
    // Map Settings
    if (json.contains("mapStyle")) setMapStyle(json["mapStyle"].toString());
    if (json.contains("showTraffic")) setShowTraffic(json["showTraffic"].toBool());
    if (json.contains("show3D")) setShow3D(json["show3D"].toBool());
    if (json.contains("followPosition")) setFollowPosition(json["followPosition"].toBool());
    if (json.contains("defaultZoom")) setDefaultZoom(json["defaultZoom"].toInt());
    
    // Navigation Settings
    if (json.contains("routingMode")) setRoutingMode(json["routingMode"].toString());
    if (json.contains("avoidTolls")) setAvoidTolls(json["avoidTolls"].toBool());
    if (json.contains("avoidHighways")) setAvoidHighways(json["avoidHighways"].toBool());
    if (json.contains("voiceGuidance")) setVoiceGuidance(json["voiceGuidance"].toBool());
    if (json.contains("voiceVolume")) setVoiceVolume(json["voiceVolume"].toInt());
    
    // Service Settings
    if (json.contains("autoStart")) setAutoStart(json["autoStart"].toBool());
    if (json.contains("reconnectInterval")) setReconnectInterval(json["reconnectInterval"].toInt());
    if (json.contains("serviceTimeout")) setServiceTimeout(json["serviceTimeout"].toInt());
}

// =============================================================================
// NavigationDataModel Implementation
// =============================================================================

NavigationDataModel::NavigationDataModel(QObject *parent)
    : QObject(parent)
    , m_latitude(21.0285)   // Default to Hanoi
    , m_longitude(105.8542)
    , m_heading(0.0)
    , m_speed(0.0)
    , m_altitude(0.0)
    , m_positionSource("GPS")
    , m_navigating(false)
    , m_distanceToNextManeuver(0.0)
    , m_distanceRemaining(0.0)
    , m_timeRemaining(0)
    , m_routeActive(false)
    , m_routeDistance(0.0)
    , m_routeDuration(0)
    , m_positioningServiceConnected(false)
    , m_routingServiceConnected(false)
    , m_guidanceServiceConnected(false)
    , m_mapServiceConnected(false)
    , m_serviceManager(nullptr)
{
    updateLastUpdate();
    qDebug() << "NavigationDataModel initialized";
}

NavigationDataModel::~NavigationDataModel()
{
}

void NavigationDataModel::setServiceManager(ServiceManager* serviceManager)
{
    m_serviceManager = serviceManager;
}

void NavigationDataModel::updatePosition(double lat, double lon, double heading, double speed, double altitude, const QString& source)
{
    bool positionChanged = (m_latitude != lat || m_longitude != lon);
    bool headingChanged = (m_heading != heading);
    bool speedChanged = (m_speed != speed);
    bool altitudeChanged = (m_altitude != altitude);
    bool sourceChanged = (m_positionSource != source);
    
    m_latitude = lat;
    m_longitude = lon;
    m_heading = heading;
    m_speed = speed;
    m_altitude = altitude;
    m_positionSource = source;
    
    updateLastUpdate();
    
    if (positionChanged) {
        emit this->positionChanged(m_latitude, m_longitude);
    }
    if (headingChanged) {
        emit this->headingChanged(m_heading);
    }
    if (speedChanged) {
        emit this->speedChanged(m_speed);
    }
    if (altitudeChanged) {
        emit this->altitudeChanged(m_altitude);
    }
    if (sourceChanged) {
        emit this->positionSourceChanged(m_positionSource);
    }
}

void NavigationDataModel::updateNavigationState(bool navigating, const QString& instruction, double distanceToNext, double distanceRemaining, int timeRemaining)
{
    bool navigatingChanged = (m_navigating != navigating);
    bool instructionChanged = (m_currentInstruction != instruction);
    bool distanceToNextChanged = (m_distanceToNextManeuver != distanceToNext);
    bool distanceRemainingChanged = (m_distanceRemaining != distanceRemaining);
    bool timeRemainingChanged = (m_timeRemaining != timeRemaining);
    
    m_navigating = navigating;
    m_currentInstruction = instruction;
    m_distanceToNextManeuver = distanceToNext;
    m_distanceRemaining = distanceRemaining;
    m_timeRemaining = timeRemaining;
    
    // Calculate estimated arrival
    if (timeRemaining > 0) {
        m_estimatedArrival = QDateTime::currentDateTime().addSecs(timeRemaining);
        emit estimatedArrivalChanged(m_estimatedArrival);
    }
    
    if (navigatingChanged) {
        emit this->navigatingChanged(m_navigating);
    }
    if (instructionChanged) {
        emit this->currentInstructionChanged(m_currentInstruction);
    }
    if (distanceToNextChanged) {
        emit this->distanceToNextManeuverChanged(m_distanceToNextManeuver);
    }
    if (distanceRemainingChanged) {
        emit this->distanceRemainingChanged(m_distanceRemaining);
    }
    if (timeRemainingChanged) {
        emit this->timeRemainingChanged(m_timeRemaining);
    }
}

void NavigationDataModel::updateRoute(const std::vector<Point>& route, const QString& routeName, double distance, int duration)
{
    m_currentRoute = route;
    m_routeName = routeName;
    m_routeDistance = distance;
    m_routeDuration = duration;
    m_routeActive = !route.empty();
    
    emit routeUpdated(route);
    emit routeNameChanged(m_routeName);
    emit routeDistanceChanged(m_routeDistance);
    emit routeDurationChanged(m_routeDuration);
    emit routeActiveChanged(m_routeActive);
}

void NavigationDataModel::clearRoute()
{
    m_currentRoute.clear();
    m_routeName.clear();
    m_routeDistance = 0.0;
    m_routeDuration = 0;
    m_routeActive = false;
    m_navigating = false;
    m_currentInstruction.clear();
    m_distanceToNextManeuver = 0.0;
    m_distanceRemaining = 0.0;
    m_timeRemaining = 0;
    
    emit routeUpdated(m_currentRoute);
    emit routeNameChanged(m_routeName);
    emit routeDistanceChanged(m_routeDistance);
    emit routeDurationChanged(m_routeDuration);
    emit routeActiveChanged(m_routeActive);
    emit navigatingChanged(m_navigating);
    emit currentInstructionChanged(m_currentInstruction);
}

void NavigationDataModel::updateServiceStatus(const QString& service, bool connected)
{
    if (service == "positioning" && m_positioningServiceConnected != connected) {
        m_positioningServiceConnected = connected;
        emit positioningServiceConnectedChanged(connected);
    } else if (service == "routing" && m_routingServiceConnected != connected) {
        m_routingServiceConnected = connected;
        emit routingServiceConnectedChanged(connected);
    } else if (service == "guidance" && m_guidanceServiceConnected != connected) {
        m_guidanceServiceConnected = connected;
        emit guidanceServiceConnectedChanged(connected);
    } else if (service == "map" && m_mapServiceConnected != connected) {
        m_mapServiceConnected = connected;
        emit mapServiceConnectedChanged(connected);
    }
}

void NavigationDataModel::updateLastUpdate()
{
    m_lastUpdate = QDateTime::currentDateTime();
    emit lastUpdateChanged(m_lastUpdate);
}

// =============================================================================
// RouteHistoryModel Implementation
// =============================================================================

RouteHistoryModel::RouteHistoryModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_showFavoritesOnly(false)
{
    loadHistory();
    qDebug() << "RouteHistoryModel initialized";
}

RouteHistoryModel::~RouteHistoryModel()
{
    saveHistory();
}

int RouteHistoryModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return static_cast<int>(m_filteredRoutes.size());
}

QVariant RouteHistoryModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= static_cast<int>(m_filteredRoutes.size())) {
        return QVariant();
    }
    
    const RouteEntry &route = m_filteredRoutes[index.row()];
    
    switch (role) {
    case NameRole:
        return route.name;
    case StartLatRole:
        return route.startPoint.latitude;
    case StartLonRole:
        return route.startPoint.longitude;
    case EndLatRole:
        return route.endPoint.latitude;
    case EndLonRole:
        return route.endPoint.longitude;
    case TimestampRole:
        return route.timestamp;
    case DistanceRole:
        return route.distance;
    case DurationRole:
        return route.duration;
    case FavoriteRole:
        return route.favorite;
    default:
        return QVariant();
    }
}

bool RouteHistoryModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || index.row() >= static_cast<int>(m_filteredRoutes.size())) {
        return false;
    }
    
    RouteEntry &route = m_filteredRoutes[index.row()];
    
    switch (role) {
    case NameRole:
        route.name = value.toString();
        break;
    case FavoriteRole:
        route.favorite = value.toBool();
        break;
    default:
        return false;
    }
    
    emit dataChanged(index, index, {role});
    return true;
}

Qt::ItemFlags RouteHistoryModel::flags(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return Qt::NoItemFlags;
    }
    
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
}

QHash<int, QByteArray> RouteHistoryModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[NameRole] = "name";
    roles[StartLatRole] = "startLat";
    roles[StartLonRole] = "startLon";
    roles[EndLatRole] = "endLat";
    roles[EndLonRole] = "endLon";
    roles[TimestampRole] = "timestamp";
    roles[DistanceRole] = "distance";
    roles[DurationRole] = "duration";
    roles[FavoriteRole] = "favorite";
    return roles;
}

void RouteHistoryModel::addRoute(const QString& name, double startLat, double startLon, double endLat, double endLon, double distance, int duration)
{
    RouteEntry entry;
    entry.name = name;
    entry.startPoint = Point(startLat, startLon);
    entry.endPoint = Point(endLat, endLon);
    entry.timestamp = QDateTime::currentDateTime();
    entry.distance = distance;
    entry.duration = duration;
    entry.favorite = false;
    
    m_routes.insert(m_routes.begin(), entry); // Add to beginning for most recent first
    
    // Limit history to 100 entries
    if (m_routes.size() > 100) {
        m_routes.resize(100);
    }
    
    filterRoutes();
    emit historyChanged();
    
    qDebug() << "Route added to history:" << name;
}

void RouteHistoryModel::removeRoute(int index)
{
    if (index < 0 || index >= static_cast<int>(m_filteredRoutes.size())) {
        return;
    }
    
    // Find the route in the main list
    const RouteEntry &toRemove = m_filteredRoutes[index];
    auto it = std::find_if(m_routes.begin(), m_routes.end(), 
                          [&toRemove](const RouteEntry &entry) {
                              return entry.name == toRemove.name && 
                                     entry.timestamp == toRemove.timestamp;
                          });
    
    if (it != m_routes.end()) {
        beginRemoveRows(QModelIndex(), index, index);
        m_routes.erase(it);
        filterRoutes();
        endRemoveRows();
        
        emit historyChanged();
        qDebug() << "Route removed from history";
    }
}

void RouteHistoryModel::clearHistory()
{
    beginResetModel();
    m_routes.clear();
    m_filteredRoutes.clear();
    endResetModel();
    
    emit historyChanged();
    qDebug() << "Route history cleared";
}

void RouteHistoryModel::toggleFavorite(int index)
{
    if (index < 0 || index >= static_cast<int>(m_filteredRoutes.size())) {
        return;
    }
    
    RouteEntry &route = m_filteredRoutes[index];
    route.favorite = !route.favorite;
    
    // Update in main list as well
    auto it = std::find_if(m_routes.begin(), m_routes.end(), 
                          [&route](RouteEntry &entry) {
                              return entry.name == route.name && 
                                     entry.timestamp == route.timestamp;
                          });
    
    if (it != m_routes.end()) {
        it->favorite = route.favorite;
    }
    
    QModelIndex modelIndex = createIndex(index, 0);
    emit dataChanged(modelIndex, modelIndex, {FavoriteRole});
    emit historyChanged();
    
    qDebug() << "Route favorite toggled:" << route.name << route.favorite;
}

void RouteHistoryModel::loadHistory()
{
    QSettings settings;
    settings.beginGroup("RouteHistory");
    
    int size = settings.beginReadArray("routes");
    m_routes.clear();
    
    for (int i = 0; i < size; ++i) {
        settings.setArrayIndex(i);
        
        RouteEntry entry;
        entry.name = settings.value("name").toString();
        entry.startPoint.latitude = settings.value("startLat").toDouble();
        entry.startPoint.longitude = settings.value("startLon").toDouble();
        entry.endPoint.latitude = settings.value("endLat").toDouble();
        entry.endPoint.longitude = settings.value("endLon").toDouble();
        entry.timestamp = settings.value("timestamp").toDateTime();
        entry.distance = settings.value("distance").toDouble();
        entry.duration = settings.value("duration").toInt();
        entry.favorite = settings.value("favorite").toBool();
        
        m_routes.push_back(entry);
    }
    
    settings.endArray();
    settings.endGroup();
    
    sortRoutes();
    filterRoutes();
    
    qDebug() << "Route history loaded:" << m_routes.size() << "entries";
}

void RouteHistoryModel::saveHistory()
{
    QSettings settings;
    settings.beginGroup("RouteHistory");
    
    settings.beginWriteArray("routes");
    for (size_t i = 0; i < m_routes.size(); ++i) {
        settings.setArrayIndex(static_cast<int>(i));
        
        const RouteEntry &entry = m_routes[i];
        settings.setValue("name", entry.name);
        settings.setValue("startLat", entry.startPoint.latitude);
        settings.setValue("startLon", entry.startPoint.longitude);
        settings.setValue("endLat", entry.endPoint.latitude);
        settings.setValue("endLon", entry.endPoint.longitude);
        settings.setValue("timestamp", entry.timestamp);
        settings.setValue("distance", entry.distance);
        settings.setValue("duration", entry.duration);
        settings.setValue("favorite", entry.favorite);
    }
    settings.endArray();
    settings.endGroup();
    
    qDebug() << "Route history saved:" << m_routes.size() << "entries";
}

void RouteHistoryModel::setShowFavoritesOnly(bool favoritesOnly)
{
    if (m_showFavoritesOnly != favoritesOnly) {
        m_showFavoritesOnly = favoritesOnly;
        filterRoutes();
    }
}

void RouteHistoryModel::setSearchFilter(const QString& filter)
{
    if (m_searchFilter != filter) {
        m_searchFilter = filter;
        filterRoutes();
    }
}

QVariantMap RouteHistoryModel::getRoute(int index) const
{
    QVariantMap route;
    if (index >= 0 && index < static_cast<int>(m_filteredRoutes.size())) {
        const RouteEntry &entry = m_filteredRoutes[index];
        route["name"] = entry.name;
        route["startLat"] = entry.startPoint.latitude;
        route["startLon"] = entry.startPoint.longitude;
        route["endLat"] = entry.endPoint.latitude;
        route["endLon"] = entry.endPoint.longitude;
        route["distance"] = entry.distance;
        route["duration"] = entry.duration;
        route["favorite"] = entry.favorite;
    }
    return route;
}

void RouteHistoryModel::filterRoutes()
{
    beginResetModel();
    
    m_filteredRoutes.clear();
    
    for (const RouteEntry &entry : m_routes) {
        bool includeRoute = true;
        
        // Filter by favorites
        if (m_showFavoritesOnly && !entry.favorite) {
            includeRoute = false;
        }
        
        // Filter by search text
        if (includeRoute && !m_searchFilter.isEmpty()) {
            if (!entry.name.contains(m_searchFilter, Qt::CaseInsensitive)) {
                includeRoute = false;
            }
        }
        
        if (includeRoute) {
            m_filteredRoutes.push_back(entry);
        }
    }
    
    endResetModel();
}

void RouteHistoryModel::sortRoutes()
{
    // Sort by timestamp, most recent first
    std::sort(m_routes.begin(), m_routes.end(), 
              [](const RouteEntry &a, const RouteEntry &b) {
                  return a.timestamp > b.timestamp;
              });
}

} // namespace nav
