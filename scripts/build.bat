@echo off
REM Cross-platform Windows build script for Automotive Navigation System

echo === Building Automotive Navigation System for Windows ===

set BUILD_TYPE=%1
if "%BUILD_TYPE%"=="" set BUILD_TYPE=Release

REM Get the parent directory (outside repo)
for %%I in ("%~dp0.") do set "REPO_DIR=%%~fI"
for %%I in ("%REPO_DIR%\..") do set "PARENT_DIR=%%~fI"

REM Set build and install directories OUTSIDE the repo
set BUILD_DIR=%PARENT_DIR%\Automotive_build
set INSTALL_DIR=%PARENT_DIR%\Automotive_install

echo Build type: %BUILD_TYPE%
echo Repository: %REPO_DIR%
echo Build directory: %BUILD_DIR%
echo Install directory: %INSTALL_DIR%

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
cmake "%REPO_DIR%" -G "Visual Studio 16 2019" -A x64 ^
    -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
    -DCMAKE_INSTALL_PREFIX=%INSTALL_DIR% ^
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

REM Auto-detect Qt installation and copy dependencies
REM Try to find Qt from CMakeCache.txt
if exist CMakeCache.txt (
    for /f "tokens=2 delims==" %%i in ('findstr /C:"CMAKE_PREFIX_PATH:PATH=" CMakeCache.txt') do set QT_DIR=%%i
)

REM Copy GUI executable if it was built
if exist hmi\%BUILD_TYPE%\nav_hmi_gui.exe (
    echo Copying GUI executable...
    copy "hmi\%BUILD_TYPE%\nav_hmi_gui.exe" "%INSTALL_DIR%\bin\"
    
    REM Copy Qt DLLs for GUI - try auto-detected Qt path first
    if defined QT_DIR (
        echo Found Qt at: %QT_DIR%
        echo Copying Qt dependencies from detected path...
        copy "%QT_DIR%\bin\Qt6Core.dll" "%INSTALL_DIR%\bin\" 2>nul
        copy "%QT_DIR%\bin\Qt6Gui.dll" "%INSTALL_DIR%\bin\" 2>nul
        copy "%QT_DIR%\bin\Qt6Widgets.dll" "%INSTALL_DIR%\bin\" 2>nul
        copy "%QT_DIR%\bin\Qt6Network.dll" "%INSTALL_DIR%\bin\" 2>nul
        
        REM Copy platform plugins
        if not exist "%INSTALL_DIR%\bin\platforms" mkdir "%INSTALL_DIR%\bin\platforms"
        copy "%QT_DIR%\plugins\platforms\qwindows.dll" "%INSTALL_DIR%\bin\platforms\" 2>nul
        
        REM Try Qt5 if Qt6 not found
        if not exist "%INSTALL_DIR%\bin\Qt6Core.dll" (
            echo Trying Qt5 libraries...
            copy "%QT_DIR%\bin\Qt5Core.dll" "%INSTALL_DIR%\bin\" 2>nul
            copy "%QT_DIR%\bin\Qt5Gui.dll" "%INSTALL_DIR%\bin\" 2>nul
            copy "%QT_DIR%\bin\Qt5Widgets.dll" "%INSTALL_DIR%\bin\" 2>nul
            copy "%QT_DIR%\bin\Qt5Network.dll" "%INSTALL_DIR%\bin\" 2>nul
        )
        echo Qt GUI dependencies copied successfully.
    ) else (
        echo Warning: Could not auto-detect Qt path from CMake cache
        echo Please manually copy Qt DLLs to %INSTALL_DIR%\bin\
    )
)

cd "%REPO_DIR%"

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
    echo.
    echo To run GUI:
    echo   %INSTALL_DIR%\bin\nav_hmi_gui.exe
) else (
    echo   - GUI not built ^(Qt not found or incomplete^)
)
echo.
echo Build directory: %BUILD_DIR%
echo Install directory: %INSTALL_DIR%

pause