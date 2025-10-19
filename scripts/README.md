# Build Scripts Directory

This directory contains all build, deployment, and utility scripts for the Automotive Navigation project.

## 📁 Directory Structure

```
scripts/
├── build.bat                   # Main build script for Windows (creates build outside repo)
├── build.sh                    # Build script for Linux/Unix
├── rebuild_and_run.bat         # Quick rebuild and run HMI
├── run_integrated_nav.bat      # Run the navigation application
├── build_and_deploy.bat        # Build + Deploy in one step
├── deploy.bat                  # Deploy built application
├── quick_deploy.bat            # Fast deployment
├── create_package.bat          # Create distribution package
└── deploy.config               # Deployment configuration
```

## 🚀 Quick Start

### Development Workflow

```powershell
# 1. Build the project (first time or after major changes)
.\scripts\build.bat

# 2. Quick rebuild and run (for development)
.\scripts\rebuild_and_run.bat

# 3. Run without rebuilding
.\scripts\run_integrated_nav.bat
```

### Deployment Workflow

```powershell
# Option 1: Build and deploy together
.\scripts\build_and_deploy.bat

# Option 2: Deploy only (after building)
.\scripts\deploy.bat

# Option 3: Quick deployment
.\scripts\quick_deploy.bat

# Create distribution package
.\scripts\create_package.bat
```

## 📋 Script Descriptions

### build.bat
**Purpose:** Main CMake build script  
**Output:** Creates `../build_Automotive` and `../install_Automotive` folders outside the repository  
**Usage:**
```powershell
cd scripts
.\build.bat
```

**Features:**
- Uses Qt 6.6.1 (msvc2019_64)
- Creates Release build
- Installs to `../install_Automotive`
- Keeps repository clean (no build artifacts)

### rebuild_and_run.bat
**Purpose:** Quick development cycle - rebuild and run  
**Usage:**
```powershell
cd scripts
.\rebuild_and_run.bat
```

**Features:**
- Incremental build (faster than full rebuild)
- Automatically runs nav_hmi_gui.exe
- Includes all Qt dependencies

### run_integrated_nav.bat
**Purpose:** Run the navigation application without building  
**Usage:**
```powershell
cd scripts
.\run_integrated_nav.bat
```

**Requirements:**
- Application must be built first
- Looks for executable in `../install_Automotive/bin/`

### build_and_deploy.bat
**Purpose:** Complete build and deployment in one step  
**Usage:**
```powershell
cd scripts
.\build_and_deploy.bat
```

**Process:**
1. Builds the project
2. Deploys to configured location
3. Copies configuration files
4. Sets up runtime environment

### deploy.bat
**Purpose:** Deploy built application to target location  
**Configuration:** Edit `deploy.config` to set target directory  
**Usage:**
```powershell
cd scripts
.\deploy.bat
```

### quick_deploy.bat
**Purpose:** Fast deployment for testing  
**Usage:**
```powershell
cd scripts
.\quick_deploy.bat
```

### create_package.bat
**Purpose:** Create distribution package (ZIP or installer)  
**Output:** Package in `packages/` directory  
**Usage:**
```powershell
cd scripts
.\create_package.bat
```

## ⚙️ Configuration

### deploy.config
Edit this file to configure deployment settings:

```ini
DEPLOY_DIR=C:\AutomotiveNav\Deploy
INCLUDE_DEBUG_SYMBOLS=false
CREATE_SHORTCUTS=true
```

### Environment Requirements

**Required:**
- Qt 6.6.1 (msvc2019_64)
- CMake 3.16+
- Visual Studio 2019+ or Build Tools
- Windows PowerShell

**Qt Path:**
Default: `C:\Qt\6.6.1\msvc2019_64`  
Set `QT_DIR` environment variable to override

## 🔧 Troubleshooting

### "Qt not found"
```powershell
# Set Qt path
$env:QT_DIR = "C:\Qt\6.6.1\msvc2019_64"
.\scripts\build.bat
```

### "Build directory not found"
```powershell
# Run full build first
.\scripts\build.bat
```

### "Application fails to start"
- Check Qt DLLs are in install directory
- Verify config files in `../install_Automotive/etc/`
- Check logs in `logs/` directory

## 📚 Related Documentation

- [BUILD_GUIDE.md](../docs/BUILD_GUIDE.md) - Detailed build instructions
- [BUILD_SCRIPTS_GUIDE.md](../docs/BUILD_SCRIPTS_GUIDE.md) - Script development guide
- [BUILD_STRUCTURE.md](../docs/BUILD_STRUCTURE.md) - Project structure
- [MIGRATION_GUIDE.md](../docs/MIGRATION_GUIDE.md) - Migration from old structure

## 💡 Tips

1. **Development:** Use `rebuild_and_run.bat` for fastest iteration
2. **Testing:** Use `quick_deploy.bat` for rapid testing on target machine
3. **Release:** Use `build_and_deploy.bat` + `create_package.bat` for distribution
4. **Clean Build:** Delete `../build_Automotive` folder and run `build.bat`

## 🔄 Version History

- **v1.0** - Initial script organization
- **v1.1** - Added out-of-source build support
- **v1.2** - Added quick deployment scripts
