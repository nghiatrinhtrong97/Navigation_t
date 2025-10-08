@echo off
REM Build and run Qt GUI navigation application for Windows

echo Building Qt GUI Navigation Application...

REM Create build directory
if not exist build mkdir build
cd build

REM Configure with CMake
cmake .. -DCMAKE_BUILD_TYPE=Release

REM Build the project
cmake --build . --config Release

REM Check if GUI application was built
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
    echo Build failed - no executable found
    pause
    exit /b 1
)

pause