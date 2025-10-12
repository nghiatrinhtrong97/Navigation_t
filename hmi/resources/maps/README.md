# Vietnam Map Images for Navigation System

This folder contains map images for the automotive navigation system:

## Map Files:
- hanoi_satellite.jpg - Satellite view of Hanoi area (800x600)
- hanoi_street.jpg - Street map view of Hanoi area (800x600)  
- vietnam_overview.jpg - Overview map of Vietnam (800x600)

## Usage:
These images are loaded by the MapWidget class and displayed based on the selected map style:
- Satellite View: Uses hanoi_satellite.jpg
- Street Map View: Uses hanoi_street.jpg
- Hybrid View: Uses vietnam_overview.jpg

## Image Requirements:
- Format: JPEG
- Size: 800x600 pixels
- Geographic Coverage: Northern Vietnam / Hanoi area
- Coordinate Bounds: 
  - North: 21.15°N
  - South: 20.90°N  
  - West: 105.70°E
  - East: 105.95°E

## Note:
Current implementation uses placeholder/dummy images generated programmatically if the actual image files are not found. Real satellite/street map images would be provided by map tile services or offline map data providers.

## Integration:
Images are referenced in the Qt resource system (resources.qrc) and loaded by MapWidget::loadMapImages() method.