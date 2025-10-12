@echo off
REM Cross-platform Windows build script for Automotive Navigation System

echo === Building Automotive Navigation System for Windows ===

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

REM Configure with CMake - auto-detect Qt and Visual Studio
echo Configuring with CMake...
cmake .. -G "Visual Studio 16 2019" -A x64 ^
    -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
    -DCMAKE_INSTALL_PREFIX=../%INSTALL_DIR% ^
    -DBUILD_TESTS=OFF

if %ERRORLEVEL% neq 0 (
    echo CMake configuration failed! Please check Qt installation.
    cd ..
    pause
    exit /b 1
)

REM Build
echo Building...
cmake --build . --config %BUILD_TYPE% --parallel

if %ERRORLEVEL% neq 0 (
    echo Build failed!
    cd ..
    pause
    exit /b 1
)

REM Install
echo Installing...
cmake --build . --target install --config %BUILD_TYPE%

REM Copy GUI executable if it was built
if exist hmi\%BUILD_TYPE%\nav_hmi_gui.exe (
    echo Copying GUI executable...
    copy "hmi\%BUILD_TYPE%\nav_hmi_gui.exe" "..\%INSTALL_DIR%\bin\"
    
    REM Copy Qt DLLs for GUI
    echo Copying Qt dependencies...
    copy "C:\Qt\6.6.1\msvc2019_64\bin\Qt6Core.dll" "..\%INSTALL_DIR%\bin\" 2>nul
    copy "C:\Qt\6.6.1\msvc2019_64\bin\Qt6Gui.dll" "..\%INSTALL_DIR%\bin\" 2>nul
    copy "C:\Qt\6.6.1\msvc2019_64\bin\Qt6Widgets.dll" "..\%INSTALL_DIR%\bin\" 2>nul
    copy "C:\Qt\6.6.1\msvc2019_64\bin\Qt6Network.dll" "..\%INSTALL_DIR%\bin\" 2>nul
    
    REM Copy platform plugins
    if not exist "..\%INSTALL_DIR%\bin\platforms" mkdir "..\%INSTALL_DIR%\bin\platforms"
    copy "C:\Qt\6.6.1\msvc2019_64\plugins\platforms\qwindows.dll" "..\%INSTALL_DIR%\bin\platforms\" 2>nul
    
    echo Qt GUI dependencies copied successfully.
)

cd ..

echo === Build Complete ===
echo Binaries installed in: %INSTALL_DIR%\bin
echo.
echo Services built:
echo   - positioning_service.exe
echo   - map_service.exe  
echo   - routing_service.exe
echo   - guidance_service.exe
if exist %INSTALL_DIR%\bin\nav_hmi_gui.exe (
    echo   - nav_hmi_gui.exe ^(Qt GUI^)
    echo.
    echo Qt GUI application built successfully!
) else (
    echo   - GUI not built ^(Qt not found or incomplete^)
)
echo.
echo To run the system:
echo   1. cd %INSTALL_DIR%\bin
echo   2. Start services in background
echo   3. Run nav_hmi_gui.exe

pause