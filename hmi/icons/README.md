# Navigation System Icons

This folder contains icon files for the automotive navigation system:

## Icon Files:
- navigation.ico - Main application icon (32x32, 16x16)
- start_marker.png - Start point marker icon (24x24)
- end_marker.png - Destination marker icon (24x24)
- current_position.png - Current position indicator (24x24)
- route_line.png - Route line style icon (16x16)
- zoom_in.png - Zoom in button icon (16x16)
- zoom_out.png - Zoom out button icon (16x16)
- center_position.png - Center on position icon (16x16)
- settings.png - Settings tab icon (16x16)
- services.png - Services tab icon (16x16)

## Usage:
These icons are used throughout the user interface:
- Toolbar buttons
- Tab icons
- Map markers
- Status indicators
- Menu items

## Integration:
Icons are referenced in the Qt resource system (resources.qrc) and loaded using Qt's resource system with the ":/icons/" prefix.

## Format Requirements:
- PNG format with transparency support
- Multiple sizes available for different UI elements
- High contrast for automotive display visibility
- Consistent visual style

## Note:
Current implementation will work without these icon files present, using text labels as fallbacks. Icons enhance the user experience but are not required for basic functionality.