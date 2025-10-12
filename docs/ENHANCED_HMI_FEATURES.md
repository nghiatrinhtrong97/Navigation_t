# Enhanced Automotive Navigation System with Qt Designer and Real Maps

## Overview
This enhanced version of the automotive navigation system now includes:
- **Real map image support** with satellite/street/hybrid views
- **Qt Designer UI integration** with .ui files
- **Complete MVC architecture** with models for settings and navigation data
- **Professional map widget** with animations and overlays
- **Settings persistence** with file-based configuration
- **Route history management** with favorites support

## New Components Added

### 1. Map Widget with Real Images (`map_widget.h/cpp`)
**Features:**
- Real satellite, street, and hybrid map image display
- Coordinate-based rendering with proper projection
- Interactive navigation (pan, zoom, click handling)
- Route visualization with direction arrows
- Position tracking with heading indicator
- Animated transitions and zoom effects
- Scale bar, coordinate display, and copyright info

**Usage:**
```cpp
MapWidget* mapWidget = new MapWidget(parent);
mapWidget->setMapStyle("Satellite");
mapWidget->setCurrentPosition(Point(21.0285, 105.8542), 45.0);
mapWidget->setRoute(routePoints);
```

### 2. Qt Designer UI Integration (`navigation_main_window.ui`)
**Features:**
- Professional tabbed interface layout
- Map view with integrated controls
- Route planning panel
- Service monitoring tab
- Settings configuration tab
- Responsive design with splitter layouts

**UI Structure:**
```
MainWindow
├── Central Splitter
│   ├── Map Widget (75% width)
│   └── Control Tabs (25% width)
│       ├── Navigation Tab
│       ├── Routing Tab  
│       ├── Services Tab
│       └── Settings Tab
```

### 3. MVC Models (`navigation_models.h/cpp`)

#### NavigationSettingsModel
**Manages:**
- General settings (language, units, theme)
- Map preferences (style, traffic, 3D mode)
- Navigation options (routing mode, voice guidance)
- Service configuration (auto-start, timeouts)
- Auto-save functionality with QSettings persistence

#### NavigationDataModel  
**Tracks:**
- Current position and movement data
- Active navigation state and instructions
- Route information and progress
- Service connection status
- Real-time updates with Qt signals

#### RouteHistoryModel
**Provides:**
- Route history with QAbstractListModel
- Favorites management
- Search and filtering capabilities
- Persistent storage with QSettings

### 4. Resource System (`resources.qrc`)
**Contains:**
- Map images (satellite, street, hybrid views)
- UI icons and graphics
- Qt Designer .ui files
- Embedded resources with :/prefix access

## File Structure
```
hmi/
├── include/
│   ├── map_widget.h              # Enhanced map widget
│   ├── navigation_models.h       # MVC data models
│   └── navigation_main_window.h  # Updated main window
├── src/
│   ├── map_widget.cpp            # Map implementation
│   ├── navigation_models.cpp     # Model implementations
│   └── navigation_main_window.cpp # Enhanced UI
├── ui/
│   └── navigation_main_window.ui # Qt Designer file
├── resources/
│   ├── maps/                     # Map image files
│   └── icons/                    # UI icons
└── resources.qrc                 # Resource definition
```

## Key Features

### Real Map Display
- **Multiple map styles**: Satellite, Street Map, Hybrid
- **Coordinate projection**: Proper lat/lon to screen conversion
- **Image caching**: Efficient map rendering
- **Fallback support**: Generates placeholder maps if images missing

### Qt Designer Integration
- **Visual UI design**: Professional interface layout
- **Resource embedding**: Maps and icons in compiled binary
- **Runtime loading**: QUiLoader support for dynamic UI
- **Consistent styling**: Professional automotive appearance

### MVC Architecture
- **Data separation**: Models handle all navigation data
- **Signal-based updates**: Real-time UI synchronization  
- **Settings persistence**: Automatic save/load functionality
- **History management**: Route tracking with favorites

### Professional Features
- **Animated transitions**: Smooth map movement and zoom
- **Interactive controls**: Click/drag/wheel navigation
- **Status indicators**: Service connection monitoring
- **Keyboard shortcuts**: Full menu and shortcut support

## Build Requirements
```cmake
find_package(Qt5 REQUIRED COMPONENTS
    Core
    Widgets  
    Network
    UiTools    # For Qt Designer support
)

# Enable Qt features
set(CMAKE_AUTOMOC ON)   # Meta-object compilation
set(CMAKE_AUTORCC ON)   # Resource compilation  
set(CMAKE_AUTOUIC ON)   # UI file compilation
```

## Usage Examples

### Setting Map Style
```cpp
// From settings model
settingsModel->setMapStyle("Satellite");

// Direct to map widget  
mapWidget->setMapStyle(MapWidget::SATELLITE);
```

### Route Management
```cpp
// Plan new route
std::vector<Point> route = {
    Point(21.0285, 105.8542),  // Start
    Point(21.0335, 105.8602),  // Waypoint
    Point(21.0385, 105.8662)   // End
};

dataModel->updateRoute(route, "Route to Downtown", 5.2, 720);
mapWidget->setRoute(route);
```

### Settings Persistence
```cpp
// Auto-save settings
settingsModel->setAutoSave(true);
settingsModel->setLanguage("Vietnamese");
settingsModel->setMapStyle("Hybrid");
// Settings automatically saved after 5 second delay

// Manual save/load
settingsModel->saveSettings();
settingsModel->loadSettings();
```

## Integration with Services
The enhanced HMI maintains full compatibility with the existing service architecture:
- **QLocalSocket communication** for cross-platform IPC
- **Service status monitoring** with automatic reconnection
- **Real-time position updates** from positioning service
- **Route calculation integration** with routing service
- **Turn-by-turn guidance** from guidance service

## Future Enhancements
- **Online map tiles** integration (OSM, Google Maps)
- **GPS tracking visualization** with breadcrumb trails
- **Voice command interface** integration
- **Multi-language UI** with translation support
- **Theme customization** with day/night modes
- **Advanced route options** (traffic avoidance, POI search)

This enhanced system provides a professional, automotive-grade navigation interface with modern Qt framework features and real map visualization capabilities.