@echo off
REM Complete navigation system startup script for Windows

echo === Automotive Navigation System Startup ===

REM Create build directory
if not exist build mkdir build
cd build

REM Build the entire system
echo Building navigation system...
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release

if %ERRORLEVEL% neq 0 (
    echo Build failed!
    pause
    exit /b 1
)

echo Build completed successfully

REM Start backend services in background
echo.
echo === Starting Backend Services ===

echo Starting Positioning Service...
start "Positioning Service" services\positioning\Release\nav_positioning.exe
timeout /t 1 /nobreak >nul

echo Starting Map Service...
start "Map Service" services\map\Release\nav_map.exe
timeout /t 1 /nobreak >nul

echo Starting Routing Service...
start "Routing Service" services\routing\Release\nav_routing.exe
timeout /t 1 /nobreak >nul

echo Starting Guidance Service...
start "Guidance Service" services\guidance\Release\nav_guidance.exe
timeout /t 1 /nobreak >nul

REM Wait for services to initialize
echo.
echo Waiting for services to initialize...
timeout /t 3 /nobreak >nul

REM Start GUI application
echo.
echo === Starting GUI Application ===

if exist "hmi\Release\nav_hmi_gui.exe" (
    echo Starting GUI Navigation Application...
    hmi\Release\nav_hmi_gui.exe
) else if exist "hmi\nav_hmi_gui.exe" (
    echo Starting GUI Navigation Application...
    hmi\nav_hmi_gui.exe
) else if exist "hmi\Release\nav_hmi.exe" (
    echo GUI not available, starting console application...
    hmi\Release\nav_hmi.exe
) else if exist "hmi\nav_hmi.exe" (
    echo GUI not available, starting console application...
    hmi\nav_hmi.exe
) else (
    echo No HMI application found!
    pause
    exit /b 1
)

echo.
echo Navigation system shutdown
pause