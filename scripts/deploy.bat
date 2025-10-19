@echo off
REM Deployment script for Automotive Navigation System
REM Creates a standalone package with all dependencies

echo === Deploying Automotive Navigation System ===

set DEPLOY_DIR=%1
if "%DEPLOY_DIR%"=="" set DEPLOY_DIR=D:\NavigationApp_Deploy

echo Deployment directory: %DEPLOY_DIR%

REM Clean and create deployment directory
if exist "%DEPLOY_DIR%" (
    echo Cleaning existing deployment directory...
    rmdir /s /q "%DEPLOY_DIR%"
)

mkdir "%DEPLOY_DIR%"
mkdir "%DEPLOY_DIR%\bin"
mkdir "%DEPLOY_DIR%\config"
mkdir "%DEPLOY_DIR%\resources"
mkdir "%DEPLOY_DIR%\maps"
mkdir "%DEPLOY_DIR%\icons"

echo.
echo === Copying application files ===

REM Copy main executable
if exist "install\bin\nav_hmi_gui.exe" (
    echo Copying nav_hmi_gui.exe...
    copy "install\bin\nav_hmi_gui.exe" "%DEPLOY_DIR%\bin\" >nul
) else (
    echo ERROR: nav_hmi_gui.exe not found! Please build the project first.
    pause
    exit /b 1
)

REM Copy configuration files
echo Copying configuration files...
if exist "install\etc\navigation.conf" (
    copy "install\etc\navigation.conf" "%DEPLOY_DIR%\config\" >nul
)
if exist "config\navigation.conf" (
    copy "config\navigation.conf" "%DEPLOY_DIR%\config\" >nul
)

REM Copy map resources
echo Copying map resources...
if exist "hmi\maps\" (
    xcopy "hmi\maps\*.*" "%DEPLOY_DIR%\maps\" /E /I /Y >nul
)

REM Copy icons
echo Copying icons...
if exist "hmi\icons\" (
    xcopy "hmi\icons\*.*" "%DEPLOY_DIR%\icons\" /E /I /Y >nul
)

echo.
echo === Detecting and copying Qt dependencies ===

REM Auto-detect Qt path from CMakeCache.txt
set QT_DIR=
if exist "build\CMakeCache.txt" (
    for /f "tokens=2 delims==" %%i in ('findstr /C:"CMAKE_PREFIX_PATH:PATH=" build\CMakeCache.txt') do set QT_DIR=%%i
)

if "%QT_DIR%"=="" (
    echo WARNING: Could not auto-detect Qt path from CMake cache
    echo Trying common Qt installation paths...
    
    if exist "C:\Qt\6.6.1\msvc2019_64" (
        set QT_DIR=C:\Qt\6.6.1\msvc2019_64
    ) else if exist "C:\Qt\6.9.0\msvc2022_64" (
        set QT_DIR=C:\Qt\6.9.0\msvc2022_64
    ) else if exist "C:\Qt\6.9.0\msvc2019_64" (
        set QT_DIR=C:\Qt\6.9.0\msvc2019_64
    ) else (
        echo ERROR: Qt not found! Please specify Qt path manually.
        pause
        exit /b 1
    )
)

echo Found Qt at: %QT_DIR%

REM Copy Qt6 DLLs
echo Copying Qt6 core libraries...
copy "%QT_DIR%\bin\Qt6Core.dll" "%DEPLOY_DIR%\bin\" 2>nul
copy "%QT_DIR%\bin\Qt6Gui.dll" "%DEPLOY_DIR%\bin\" 2>nul
copy "%QT_DIR%\bin\Qt6Widgets.dll" "%DEPLOY_DIR%\bin\" 2>nul
copy "%QT_DIR%\bin\Qt6Network.dll" "%DEPLOY_DIR%\bin\" 2>nul

REM Copy Qt platform plugins
echo Copying Qt platform plugins...
mkdir "%DEPLOY_DIR%\bin\platforms" 2>nul
copy "%QT_DIR%\plugins\platforms\qwindows.dll" "%DEPLOY_DIR%\bin\platforms\" 2>nul

REM Copy Qt styles (optional but recommended)
echo Copying Qt styles...
mkdir "%DEPLOY_DIR%\bin\styles" 2>nul
copy "%QT_DIR%\plugins\styles\qwindowsvistastyle.dll" "%DEPLOY_DIR%\bin\styles\" 2>nul

REM Copy Qt image formats
echo Copying Qt image format plugins...
mkdir "%DEPLOY_DIR%\bin\imageformats" 2>nul
copy "%QT_DIR%\plugins\imageformats\qjpeg.dll" "%DEPLOY_DIR%\bin\imageformats\" 2>nul
copy "%QT_DIR%\plugins\imageformats\qgif.dll" "%DEPLOY_DIR%\bin\imageformats\" 2>nul
copy "%QT_DIR%\plugins\imageformats\qico.dll" "%DEPLOY_DIR%\bin\imageformats\" 2>nul

REM Try Qt5 if Qt6 DLLs not found
if not exist "%DEPLOY_DIR%\bin\Qt6Core.dll" (
    echo Qt6 not found, trying Qt5...
    copy "%QT_DIR%\bin\Qt5Core.dll" "%DEPLOY_DIR%\bin\" 2>nul
    copy "%QT_DIR%\bin\Qt5Gui.dll" "%DEPLOY_DIR%\bin\" 2>nul
    copy "%QT_DIR%\bin\Qt5Widgets.dll" "%DEPLOY_DIR%\bin\" 2>nul
    copy "%QT_DIR%\bin\Qt5Network.dll" "%DEPLOY_DIR%\bin\" 2>nul
)

echo.
echo === Copying Visual C++ Runtime ===

REM Copy MSVC runtime DLLs (from Qt directory)
if exist "%QT_DIR%\bin\vcruntime140.dll" (
    copy "%QT_DIR%\bin\vcruntime140.dll" "%DEPLOY_DIR%\bin\" 2>nul
)
if exist "%QT_DIR%\bin\vcruntime140_1.dll" (
    copy "%QT_DIR%\bin\vcruntime140_1.dll" "%DEPLOY_DIR%\bin\" 2>nul
)
if exist "%QT_DIR%\bin\msvcp140.dll" (
    copy "%QT_DIR%\bin\msvcp140.dll" "%DEPLOY_DIR%\bin\" 2>nul
)

echo.
echo === Creating launcher script ===

REM Create a launcher script
echo @echo off > "%DEPLOY_DIR%\run_navigation.bat"
echo echo Starting Automotive Navigation System... >> "%DEPLOY_DIR%\run_navigation.bat"
echo cd /d "%%~dp0bin" >> "%DEPLOY_DIR%\run_navigation.bat"
echo start nav_hmi_gui.exe >> "%DEPLOY_DIR%\run_navigation.bat"
echo echo Navigation GUI started! >> "%DEPLOY_DIR%\run_navigation.bat"

REM Create README
echo Creating README...
echo AUTOMOTIVE NAVIGATION SYSTEM > "%DEPLOY_DIR%\README.txt"
echo ============================= >> "%DEPLOY_DIR%\README.txt"
echo. >> "%DEPLOY_DIR%\README.txt"
echo INSTALLATION: >> "%DEPLOY_DIR%\README.txt"
echo 1. Extract or copy this entire folder to your desired location >> "%DEPLOY_DIR%\README.txt"
echo 2. Make sure all files in 'bin' folder are present >> "%DEPLOY_DIR%\README.txt"
echo. >> "%DEPLOY_DIR%\README.txt"
echo TO RUN: >> "%DEPLOY_DIR%\README.txt"
echo - Double-click 'run_navigation.bat' >> "%DEPLOY_DIR%\README.txt"
echo - OR navigate to 'bin' folder and run 'nav_hmi_gui.exe' >> "%DEPLOY_DIR%\README.txt"
echo. >> "%DEPLOY_DIR%\README.txt"
echo REQUIREMENTS: >> "%DEPLOY_DIR%\README.txt"
echo - Windows 10/11 64-bit >> "%DEPLOY_DIR%\README.txt"
echo - Visual C++ Redistributable 2019 or later >> "%DEPLOY_DIR%\README.txt"
echo. >> "%DEPLOY_DIR%\README.txt"
echo FOLDER STRUCTURE: >> "%DEPLOY_DIR%\README.txt"
echo - bin/          : Application executable and DLLs >> "%DEPLOY_DIR%\README.txt"
echo - config/       : Configuration files >> "%DEPLOY_DIR%\README.txt"
echo - maps/         : Map image resources >> "%DEPLOY_DIR%\README.txt"
echo - icons/        : UI icons >> "%DEPLOY_DIR%\README.txt"
echo. >> "%DEPLOY_DIR%\README.txt"
echo Build Date: %DATE% %TIME% >> "%DEPLOY_DIR%\README.txt"

echo.
echo === Deployment Summary ===
echo.
echo Deployment directory: %DEPLOY_DIR%
echo.
echo Files deployed:
dir /b "%DEPLOY_DIR%\bin\*.dll" 2>nul | find /c /v "" > temp.txt
set /p DLL_COUNT=<temp.txt
del temp.txt
echo   - DLL files: %DLL_COUNT%
echo   - Executable: nav_hmi_gui.exe
echo   - Platform plugins: platforms\qwindows.dll
echo   - Configuration files in: config\
echo   - Map resources in: maps\
echo   - Icons in: icons\
echo.

if exist "%DEPLOY_DIR%\bin\Qt6Core.dll" (
    echo ✓ Qt6 dependencies found
) else if exist "%DEPLOY_DIR%\bin\Qt5Core.dll" (
    echo ✓ Qt5 dependencies found
) else (
    echo ✗ WARNING: Qt DLLs not found! Application may not run.
)

echo.
echo === Deployment Complete! ===
echo.
echo To run the application:
echo   1. Navigate to: %DEPLOY_DIR%
echo   2. Run: run_navigation.bat
echo.
echo Or copy the entire folder to another computer.
echo.

explorer "%DEPLOY_DIR%"

pause
