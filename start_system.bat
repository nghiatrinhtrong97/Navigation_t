@echo off
REM System startup script for Automotive Navigation System (Windows)

echo === Starting Automotive Navigation System ===

set INSTALL_DIR=install\bin
set LOG_DIR=logs
set GPS_DEVICE=COM1
set CAN_DEVICE=can0
set MAP_DATA_PATH=data\map.data

REM Create log directory
if not exist %LOG_DIR% mkdir %LOG_DIR%

REM Check if binaries exist
if not exist "%INSTALL_DIR%" (
    echo Error: Install directory not found. Please run build.bat first.
    pause
    exit /b 1
)

echo Starting services...

REM Start positioning service
echo Starting positioning service...
start "Positioning Service" cmd /c "%INSTALL_DIR%\positioning_service.exe --gps %GPS_DEVICE% --can %CAN_DEVICE% > %LOG_DIR%\positioning.log 2>&1"
timeout /t 2 /nobreak > nul

REM Start map service
echo Starting map service...
start "Map Service" cmd /c "%INSTALL_DIR%\map_service.exe --data %MAP_DATA_PATH% > %LOG_DIR%\map.log 2>&1"
timeout /t 2 /nobreak > nul

REM Start routing service
echo Starting routing service...
start "Routing Service" cmd /c "%INSTALL_DIR%\routing_service.exe > %LOG_DIR%\routing.log 2>&1"
timeout /t 2 /nobreak > nul

REM Start guidance service
echo Starting guidance service...
start "Guidance Service" cmd /c "%INSTALL_DIR%\guidance_service.exe > %LOG_DIR%\guidance.log 2>&1"
timeout /t 2 /nobreak > nul

echo All services started successfully!
echo.
echo Logs are written to: %LOG_DIR%\
echo.
echo Starting HMI application...

REM Start HMI application
%INSTALL_DIR%\nav_hmi.exe

echo.
echo HMI application terminated.
echo Services are still running in background windows.
echo Close the service windows manually to stop the system.

pause