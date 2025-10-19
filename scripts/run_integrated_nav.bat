@echo off
REM Get the parent directory (outside repo)
for %%I in ("%~dp0.") do set "REPO_DIR=%%~fI"
for %%I in ("%REPO_DIR%\..") do set "PARENT_DIR=%%~fI"

set INSTALL_DIR=%PARENT_DIR%\Automotive_install

if not exist "%INSTALL_DIR%\bin\nav_hmi_gui.exe" (
    echo ERROR: nav_hmi_gui.exe not found!
    echo Please run build.bat first to build the project.
    echo Expected location: %INSTALL_DIR%\bin\nav_hmi_gui.exe
    pause
    exit /b 1
)

cd /d "%INSTALL_DIR%\bin"
echo Starting Integrated Navigation Application from: %CD%
echo.
nav_hmi_gui.exe
pause