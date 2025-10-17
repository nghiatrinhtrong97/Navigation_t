# Automotive Navigation System - Build Scripts Guide

## Overview
This project includes multiple build and deployment scripts for different purposes.

## Deployment Target
All scripts deploy to: **`D:\NavigationApp_Deploy\`**

## Available Scripts

### 1. `build.bat` - Basic Build
**Purpose:** Standard build without automatic deployment
**Usage:**
```batch
build.bat [Release|Debug] [clean]
```
**Output:** `install\bin\` (local directory)
**When to use:** During development, when you don't need deployment

---

### 2. `build_and_deploy.bat` - Full Build & Deploy ⭐ RECOMMENDED
**Purpose:** Complete build, install, and deploy to D:\NavigationApp_Deploy\
**Usage:**
```batch
build_and_deploy.bat [Release|Debug] [clean]
```
**Steps performed:**
1. Clean (if requested)
2. CMake configuration
3. Build project
4. Install locally
5. Deploy to D:\NavigationApp_Deploy\

**Output:** 
- `install\bin\` (temporary)
- `D:\NavigationApp_Deploy\` (final deployment)

**When to use:** When you want a complete standalone deployment

---

### 3. `quick_deploy.bat` - Fast Deployment
**Purpose:** Deploy already-built files without rebuilding
**Usage:**
```batch
quick_deploy.bat
```
**Prerequisite:** Must run `build.bat` or `build_and_deploy.bat` first

**When to use:** 
- After making only config/resource changes
- Quick re-deployment without rebuilding

---

### 4. `create_package.bat` - Create ZIP Package
**Purpose:** Create distributable ZIP file from deployment
**Usage:**
```batch
create_package.bat
```
**Output:** `D:\AutomotiveNavigation_v1.0.0_YYYYMMDD.zip`

**When to use:** Creating a release package for distribution

---

### 5. `deploy_package.ps1` - PowerShell Deployment (Legacy)
**Purpose:** Original PowerShell deployment script
**Usage:**
```powershell
.\deploy_package.ps1
```
**Note:** Use `build_and_deploy.bat` instead for better integration

---

## Typical Workflow

### Development Build
```batch
# Quick build for testing
build.bat Release

# Run from install directory
cd install\bin
nav_hmi_gui.exe
```

### Release Build
```batch
# Full build and deploy
build_and_deploy.bat Release clean

# Run from deployment directory
cd D:\NavigationApp_Deploy
run_navigation.bat
```

### Distribution Package
```batch
# 1. Build and deploy
build_and_deploy.bat Release clean

# 2. Create ZIP package
create_package.bat

# 3. Share the ZIP file
# Location: D:\AutomotiveNavigation_v1.0.0_YYYYMMDD.zip
```

---

## Deployment Directory Structure

```
D:\NavigationApp_Deploy\
├── bin\
│   ├── nav_hmi_gui.exe          # Main application
│   ├── Qt6Core.dll               # Qt dependencies
│   ├── Qt6Gui.dll
│   ├── Qt6Widgets.dll
│   └── Qt6Network.dll
├── platforms\
│   └── qwindows.dll              # Qt platform plugin
├── config\
│   └── navigation.conf           # Configuration file
├── maps\
│   ├── hanoi_satellite.jpg
│   ├── hanoi_street.jpg
│   └── vietnam_overview.jpg
├── icons\
│   └── *.png                     # Application icons
├── run_navigation.bat            # Quick launcher
└── README.txt                    # Runtime instructions
```

---

## Configuration

### Deployment Path
Configured in: `deploy.config`
```
DEPLOY_DIR=D:/NavigationApp_Deploy
```

### Qt Path
Auto-detected from CMake. Override in `CMakeLists.txt` if needed:
```cmake
set(CMAKE_PREFIX_PATH "C:/Qt/6.6.1/msvc2019_64" CACHE PATH "Qt installation path")
```

---

## Troubleshooting

### Build fails
- Check Qt installation: `C:\Qt\6.6.1\msvc2019_64`
- Verify Visual Studio 2019 is installed
- Run with `clean` option: `build_and_deploy.bat Release clean`

### Deployment fails
- Ensure `build.bat` or `build_and_deploy.bat` completed successfully
- Check `install\bin\nav_hmi_gui.exe` exists
- Verify write permissions to `D:\`

### Application won't run
- Check Qt DLLs in `D:\NavigationApp_Deploy\bin\`
- Verify `platforms\qwindows.dll` exists
- Run from command line to see error messages

---

## Quick Reference

| Task | Command |
|------|---------|
| First build | `build_and_deploy.bat Release clean` |
| Rebuild after code changes | `build_and_deploy.bat Release` |
| Deploy without rebuild | `quick_deploy.bat` |
| Create release package | `create_package.bat` |
| Run application | `cd D:\NavigationApp_Deploy && run_navigation.bat` |

---

## Notes

- All scripts ensure deployment to **D:\NavigationApp_Deploy\**
- The deployment directory is cleaned on each deployment
- Qt DLLs are automatically detected and copied
- Maps and icons are included in deployment
- A launch script is created automatically

---

Last updated: 2025-10-17
Version: 1.0.0
