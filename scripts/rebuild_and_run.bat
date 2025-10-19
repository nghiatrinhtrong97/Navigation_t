@echo off
REM Quick rebuild and run script
REM This is the fastest way to rebuild and test

cls
echo ========================================
echo   AUTOMOTIVE NAVIGATION - QUICK BUILD
echo ========================================
echo.
echo This will:
echo   1. Build the project (Release)
echo   2. Deploy to D:\NavigationApp_Deploy\
echo   3. Launch the application
echo.
echo Press Ctrl+C to cancel, or
pause

REM Build and deploy
call build_and_deploy.bat Release

REM Launch application
if exist D:\NavigationApp_Deploy\bin\nav_hmi_gui.exe (
    echo.
    echo ========================================
    echo Launching application...
    echo ========================================
    echo.
    cd D:\NavigationApp_Deploy\bin
    start nav_hmi_gui.exe
    echo Application launched!
) else (
    echo ERROR: Build or deployment failed!
    pause
)
