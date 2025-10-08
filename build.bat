@echo off
REM Build script for Automotive Navigation System (Windows)

echo === Building Automotive Navigation System ===

set BUILD_TYPE=%1
if "%BUILD_TYPE%"=="" set BUILD_TYPE=Release

set BUILD_DIR=build
set INSTALL_DIR=install

echo Build type: %BUILD_TYPE%

REM Clean previous build if requested
if "%2"=="clean" (
    echo Cleaning previous build...
    rmdir /s /q %BUILD_DIR% 2>nul
    rmdir /s /q %INSTALL_DIR% 2>nul
)

REM Create build directory
if not exist %BUILD_DIR% mkdir %BUILD_DIR%
cd %BUILD_DIR%

REM Configure with CMake
echo Configuring with CMake...
cmake .. -DCMAKE_BUILD_TYPE=%BUILD_TYPE% -DCMAKE_INSTALL_PREFIX=../%INSTALL_DIR%

REM Build
echo Building...
cmake --build . --config %BUILD_TYPE% --parallel

REM Install
echo Installing...
cmake --build . --target install --config %BUILD_TYPE%

cd ..

echo === Build Complete ===
echo Binaries installed in: %INSTALL_DIR%\bin
echo.
echo Services built:
echo   - positioning_service.exe
echo   - map_service.exe
echo   - routing_service.exe
echo   - guidance_service.exe
echo   - nav_hmi.exe
echo.
echo To run the system:
echo   1. Start services in order: positioning, map, routing, guidance
echo   2. Run HMI application
echo   3. Or use the start_system.bat script

pause