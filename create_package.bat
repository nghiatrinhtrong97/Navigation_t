@echo off
REM Create a distributable ZIP package from deployment directory

echo ========================================
echo Creating Distribution Package
echo ========================================

set DEPLOY_DIR=D:\NavigationApp_Deploy
set PACKAGE_NAME=AutomotiveNavigation_v1.0.0_%DATE:~-4%%DATE:~3,2%%DATE:~0,2%
set OUTPUT_DIR=D:\

if not exist %DEPLOY_DIR%\bin\nav_hmi_gui.exe (
    echo ERROR: Deployment directory not found!
    echo Please run build_and_deploy.bat first!
    pause
    exit /b 1
)

echo Creating package: %PACKAGE_NAME%.zip
echo.

REM Use PowerShell to create ZIP
powershell -Command "Compress-Archive -Path '%DEPLOY_DIR%\*' -DestinationPath '%OUTPUT_DIR%%PACKAGE_NAME%.zip' -Force"

if %ERRORLEVEL% equ 0 (
    echo.
    echo ========================================
    echo Package Created Successfully!
    echo ========================================
    echo Location: %OUTPUT_DIR%%PACKAGE_NAME%.zip
    echo.
    
    REM Get file size
    for %%A in ("%OUTPUT_DIR%%PACKAGE_NAME%.zip") do echo Size: %%~zA bytes
    echo.
) else (
    echo ERROR: Failed to create package!
)

pause
