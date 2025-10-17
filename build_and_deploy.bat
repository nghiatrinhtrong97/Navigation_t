@echo off
REM Complete build and deployment script for Automotive Navigation System
REM This script builds the project and deploys to D:/NavigationApp_Deploy/

echo ========================================
echo Automotive Navigation - Build and Deploy
echo ========================================
echo.

set BUILD_TYPE=%1
if "%BUILD_TYPE%"=="" set BUILD_TYPE=Release

set BUILD_DIR=build
set INSTALL_DIR=install
set DEPLOY_DIR=D:\NavigationApp_Deploy

echo Configuration:
echo   Build Type: %BUILD_TYPE%
echo   Build Directory: %BUILD_DIR%
echo   Install Directory: %INSTALL_DIR%
echo   Deploy Target: %DEPLOY_DIR%
echo.

REM Step 1: Clean if requested
if "%2"=="clean" (
    echo [1/5] Cleaning previous build...
    rmdir /s /q %BUILD_DIR% 2>nul
    rmdir /s /q %INSTALL_DIR% 2>nul
    rmdir /s /q %DEPLOY_DIR% 2>nul
    echo Clean complete.
    echo.
) else (
    echo [1/5] Using existing build (use 'clean' to rebuild from scratch)
    echo.
)

REM Step 2: Configure with CMake
echo [2/5] Configuring with CMake...
if not exist %BUILD_DIR% mkdir %BUILD_DIR%
cd %BUILD_DIR%

cmake .. -G "Visual Studio 16 2019" -A x64 ^
    -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
    -DCMAKE_INSTALL_PREFIX=../%INSTALL_DIR% ^
    -DBUILD_TESTS=OFF

if %ERRORLEVEL% neq 0 (
    echo ERROR: CMake configuration failed!
    cd ..
    pause
    exit /b 1
)
echo Configuration complete.
echo.

REM Step 3: Build
echo [3/5] Building project...
cmake --build . --config %BUILD_TYPE% --parallel

if %ERRORLEVEL% neq 0 (
    echo ERROR: Build failed!
    cd ..
    pause
    exit /b 1
)
echo Build complete.
echo.

REM Step 4: Install locally
echo [4/5] Installing to local directory...
cmake --build . --target install --config %BUILD_TYPE%

REM Auto-detect Qt path
if exist CMakeCache.txt (
    for /f "tokens=2 delims==" %%i in ('findstr /C:"CMAKE_PREFIX_PATH:PATH=" CMakeCache.txt') do set QT_DIR=%%i
)

REM Copy GUI executable and Qt dependencies
if exist hmi\%BUILD_TYPE%\nav_hmi_gui.exe (
    echo Copying GUI executable...
    copy "hmi\%BUILD_TYPE%\nav_hmi_gui.exe" "..\%INSTALL_DIR%\bin\" >nul
    
    if defined QT_DIR (
        echo Running windeployqt to gather all Qt dependencies...
        cd "..\%INSTALL_DIR%\bin"
        "%QT_DIR%\bin\windeployqt.exe" --release --no-translations nav_hmi_gui.exe
        cd "..\..\%BUILD_DIR%"
        echo Qt dependencies deployed successfully.
    )
)
cd ..
echo Local installation complete.
echo.

REM Step 5: Deploy to D:/NavigationApp_Deploy/
echo [5/5] Deploying to %DEPLOY_DIR%...

REM Clean deployment directory
if exist %DEPLOY_DIR% (
    echo Cleaning existing deployment...
    rmdir /s /q %DEPLOY_DIR% 2>nul
)

REM Create deployment structure
mkdir %DEPLOY_DIR% 2>nul
mkdir %DEPLOY_DIR%\bin 2>nul
mkdir %DEPLOY_DIR%\config 2>nul
mkdir %DEPLOY_DIR%\maps 2>nul
mkdir %DEPLOY_DIR%\icons 2>nul
mkdir %DEPLOY_DIR%\platforms 2>nul

REM Copy executables
echo Copying executables...
copy %INSTALL_DIR%\bin\nav_hmi_gui.exe %DEPLOY_DIR%\bin\ >nul

REM Copy Qt DLLs and all plugins
echo Copying Qt dependencies...
copy %INSTALL_DIR%\bin\*.dll %DEPLOY_DIR%\bin\ 2>nul

REM Copy all Qt plugin directories
echo Copying Qt plugins...
if exist %INSTALL_DIR%\bin\platforms (
    xcopy /E /I /Y %INSTALL_DIR%\bin\platforms %DEPLOY_DIR%\bin\platforms >nul
)
if exist %INSTALL_DIR%\bin\imageformats (
    xcopy /E /I /Y %INSTALL_DIR%\bin\imageformats %DEPLOY_DIR%\bin\imageformats >nul
)
if exist %INSTALL_DIR%\bin\iconengines (
    xcopy /E /I /Y %INSTALL_DIR%\bin\iconengines %DEPLOY_DIR%\bin\iconengines >nul
)
if exist %INSTALL_DIR%\bin\styles (
    xcopy /E /I /Y %INSTALL_DIR%\bin\styles %DEPLOY_DIR%\bin\styles >nul
)
if exist %INSTALL_DIR%\bin\tls (
    xcopy /E /I /Y %INSTALL_DIR%\bin\tls %DEPLOY_DIR%\bin\tls >nul
)
if exist %INSTALL_DIR%\bin\networkinformation (
    xcopy /E /I /Y %INSTALL_DIR%\bin\networkinformation %DEPLOY_DIR%\bin\networkinformation >nul
)
if exist %INSTALL_DIR%\bin\generic (
    xcopy /E /I /Y %INSTALL_DIR%\bin\generic %DEPLOY_DIR%\bin\generic >nul
)

REM Copy configuration files
if exist config\navigation.conf (
    copy config\navigation.conf %DEPLOY_DIR%\config\ >nul
)
if exist %INSTALL_DIR%\etc\navigation.conf (
    copy %INSTALL_DIR%\etc\navigation.conf %DEPLOY_DIR%\config\ >nul
)

REM Copy maps
if exist hmi\maps\*.jpg (
    copy hmi\maps\*.jpg %DEPLOY_DIR%\maps\ >nul
)

REM Copy icons
if exist hmi\icons\*.png (
    copy hmi\icons\*.png %DEPLOY_DIR%\icons\ >nul
)

REM Create launch script in deployment directory
echo @echo off > %DEPLOY_DIR%\run_navigation.bat
echo REM Automotive Navigation System Launcher >> %DEPLOY_DIR%\run_navigation.bat
echo cd /d "%%~dp0\bin" >> %DEPLOY_DIR%\run_navigation.bat
echo start nav_hmi_gui.exe >> %DEPLOY_DIR%\run_navigation.bat

REM Create README
echo Automotive Navigation System > %DEPLOY_DIR%\README.txt
echo ============================= >> %DEPLOY_DIR%\README.txt
echo. >> %DEPLOY_DIR%\README.txt
echo To run the application: >> %DEPLOY_DIR%\README.txt
echo   1. Double-click run_navigation.bat >> %DEPLOY_DIR%\README.txt
echo   OR >> %DEPLOY_DIR%\README.txt
echo   2. Navigate to bin\ folder and run nav_hmi_gui.exe >> %DEPLOY_DIR%\README.txt
echo. >> %DEPLOY_DIR%\README.txt
echo Build Date: %DATE% %TIME% >> %DEPLOY_DIR%\README.txt
echo Build Type: %BUILD_TYPE% >> %DEPLOY_DIR%\README.txt

echo Deployment complete!
echo.

REM Summary
echo ========================================
echo BUILD AND DEPLOYMENT SUMMARY
echo ========================================
echo Build Type: %BUILD_TYPE%
echo Deployed to: %DEPLOY_DIR%
echo.
echo Files deployed:
dir /b %DEPLOY_DIR%\bin\*.exe 2>nul
echo.
echo Qt DLLs:
dir /b %DEPLOY_DIR%\bin\*.dll 2>nul | find /c ".dll"
echo DLL files copied.
echo.
echo ========================================
echo To run the application:
echo   cd %DEPLOY_DIR%
echo   run_navigation.bat
echo ========================================
echo.

pause
