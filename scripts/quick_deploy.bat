@echo off
REM Quick deployment script - only deploys already built files
REM Use build_and_deploy.bat for full build + deploy

echo ========================================
echo Quick Deploy to D:/NavigationApp_Deploy
echo ========================================

set DEPLOY_DIR=D:\NavigationApp_Deploy
set SOURCE_DIR=install\bin

if not exist %SOURCE_DIR%\nav_hmi_gui.exe (
    echo ERROR: No built files found in %SOURCE_DIR%
    echo Please run build_and_deploy.bat first!
    pause
    exit /b 1
)

echo Cleaning deployment directory...
if exist %DEPLOY_DIR% rmdir /s /q %DEPLOY_DIR%

echo Creating deployment structure...
mkdir %DEPLOY_DIR%\bin
mkdir %DEPLOY_DIR%\config
mkdir %DEPLOY_DIR%\maps
mkdir %DEPLOY_DIR%\icons
mkdir %DEPLOY_DIR%\platforms

echo Copying files...
copy %SOURCE_DIR%\nav_hmi_gui.exe %DEPLOY_DIR%\bin\ >nul
copy %SOURCE_DIR%\*.dll %DEPLOY_DIR%\bin\ 2>nul

REM Copy all Qt plugin directories
if exist %SOURCE_DIR%\platforms (
    xcopy /E /I /Y %SOURCE_DIR%\platforms %DEPLOY_DIR%\bin\platforms >nul
)
if exist %SOURCE_DIR%\imageformats (
    xcopy /E /I /Y %SOURCE_DIR%\imageformats %DEPLOY_DIR%\bin\imageformats >nul
)
if exist %SOURCE_DIR%\iconengines (
    xcopy /E /I /Y %SOURCE_DIR%\iconengines %DEPLOY_DIR%\bin\iconengines >nul
)
if exist %SOURCE_DIR%\styles (
    xcopy /E /I /Y %SOURCE_DIR%\styles %DEPLOY_DIR%\bin\styles >nul
)
if exist %SOURCE_DIR%\tls (
    xcopy /E /I /Y %SOURCE_DIR%\tls %DEPLOY_DIR%\bin\tls >nul
)
if exist %SOURCE_DIR%\networkinformation (
    xcopy /E /I /Y %SOURCE_DIR%\networkinformation %DEPLOY_DIR%\bin\networkinformation >nul
)
if exist %SOURCE_DIR%\generic (
    xcopy /E /I /Y %SOURCE_DIR%\generic %DEPLOY_DIR%\bin\generic >nul
)

if exist config\navigation.conf copy config\navigation.conf %DEPLOY_DIR%\config\ >nul
if exist hmi\maps\*.jpg copy hmi\maps\*.jpg %DEPLOY_DIR%\maps\ >nul
if exist hmi\icons\*.png copy hmi\icons\*.png %DEPLOY_DIR%\icons\ >nul

echo Creating launcher...
echo @echo off > %DEPLOY_DIR%\run_navigation.bat
echo cd /d "%%~dp0\bin" >> %DEPLOY_DIR%\run_navigation.bat
echo start nav_hmi_gui.exe >> %DEPLOY_DIR%\run_navigation.bat

echo ========================================
echo Deployment Complete!
echo Location: %DEPLOY_DIR%
echo ========================================
echo.
echo To run: cd %DEPLOY_DIR% ^&^& run_navigation.bat
echo.

pause
