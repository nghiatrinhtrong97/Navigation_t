cmake_minimum_required(VERSION 3.16)
project(AutomotiveNavigation VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Cross-platform configuration
if(WIN32)
    message(STATUS "Building for Windows")
    set(CMAKE_PREFIX_PATH "C:/Qt/5.15.2/msvc2019_64" CACHE PATH "Qt installation path")
    add_definitions(-DWIN32_LEAN_AND_MEAN -DNOMINMAX)
elseif(UNIX AND NOT APPLE)
    message(STATUS "Building for Linux")
    # Common Linux Qt paths
    set(CMAKE_PREFIX_PATH "/usr/lib/x86_64-linux-gnu/qt5" "/usr/lib/qt5" "/opt/qt515" CACHE PATH "Qt installation path")
elseif(QNX)
    message(STATUS "Building for QNX")
    set(CMAKE_SYSTEM_NAME QNX)
    add_definitions(-D__QNX__)
    # Add QNX specific libraries
    find_library(QNX_C_LIB c)
    find_library(QNX_SOCKET_LIB socket)
else()
    message(STATUS "Building for generic Unix")
endif()

# Compiler flags - cross-platform
if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /O2")
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2")
    if(QNX)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions")
    endif()
endif()

# Enhanced Qt detection for cross-platform support (Qt5 and Qt6)
# Auto-detect Qt installation paths on Windows
if(WIN32 AND NOT CMAKE_PREFIX_PATH)
    # Common Qt installation paths on Windows
    set(QT_POSSIBLE_PATHS
        # Qt6 paths first (modern)
        "C:/Qt/6.9.0/msvc2019_64"
        "C:/Qt/6.6.1/msvc2019_64"
        "C:/Qt/6.6.0/msvc2019_64"
        "C:/Qt/6.5.3/msvc2019_64"
        # Qt5 paths (legacy)
        "C:/Qt/5.15.2/msvc2019_64"
        "C:/Qt/5.15.0/msvc2019_64"
        "C:/Qt/5.14.2/msvc2017_64"
        "C:/Qt/5.12.12/msvc2017_64"
        # Alternative drives
        "D:/Qt/6.9.0/msvc2019_64"
        "D:/Qt/6.6.1/msvc2019_64"
        "D:/Qt/5.15.2/msvc2019_64"
    )
    
    foreach(QT_PATH ${QT_POSSIBLE_PATHS})
        if(EXISTS "${QT_PATH}/lib/cmake/Qt6")
            set(CMAKE_PREFIX_PATH "${QT_PATH}")
            message(STATUS "Found Qt6 at: ${QT_PATH}")
            break()
        elseif(EXISTS "${QT_PATH}/lib/cmake/Qt5")
            set(CMAKE_PREFIX_PATH "${QT_PATH}")
            message(STATUS "Found Qt5 at: ${QT_PATH}")
            break()
        endif()
    endforeach()
endif()

# Try Qt6 first, then fall back to Qt5
find_package(Qt6 COMPONENTS Core Network QUIET)
if(Qt6_FOUND)
    find_package(Qt6 COMPONENTS Widgets UiTools QUIET)
    if(Qt6Widgets_FOUND)
        message(STATUS "Qt6 with Widgets found - Building GUI application")
        set(BUILD_GUI ON)
        set(QT_VERSION_MAJOR 6)
    else()
        message(STATUS "Qt6 Widgets not found - Building console services only")
        set(BUILD_GUI OFF)
    endif()
else()
    # Fall back to Qt5
    find_package(Qt5 COMPONENTS Core Network QUIET)
    if(Qt5_FOUND)
        find_package(Qt5 COMPONENTS Widgets UiTools QUIET)
        if(Qt5Widgets_FOUND)
            message(STATUS "Qt5 with Widgets found - Building GUI application")
            set(BUILD_GUI ON)
            set(QT_VERSION_MAJOR 5)
        else()
            message(STATUS "Qt5 Widgets not found - Building console services only")
            set(BUILD_GUI OFF)
        endif()
    else()
        message(STATUS "Qt not found - Building console services only")
        set(BUILD_GUI OFF)
    endif()
endif()

# Threading support
find_package(Threads REQUIRED)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/common/include)

# Common library
add_subdirectory(common)

# HMI Application with integrated services
add_subdirectory(hmi)

# Tests (if enabled)
option(BUILD_TESTS "Build test programs" OFF)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Installation - integrated application
if(BUILD_GUI)
    install(TARGETS nav_hmi_gui
            RUNTIME DESTINATION bin)
else()
    message(WARNING "GUI components not available - cannot build navigation application")
endif()

# Install configuration files
install(FILES config/navigation.conf DESTINATION etc)

# Platform-specific installation
if(WIN32)
    # Windows-specific deployment
    if(BUILD_GUI AND Qt5_FOUND)
        # Find Qt5 deployment tool
        get_target_property(QT_QMAKE_EXECUTABLE Qt5::qmake IMPORTED_LOCATION)
        get_filename_component(QT_WINDEPLOYQT_EXECUTABLE ${QT_QMAKE_EXECUTABLE} PATH)
        set(QT_WINDEPLOYQT_EXECUTABLE "${QT_WINDEPLOYQT_EXECUTABLE}/windeployqt.exe")
        
        # Deploy Qt libraries with the application
        install(CODE "execute_process(COMMAND ${QT_WINDEPLOYQT_EXECUTABLE} \${CMAKE_INSTALL_PREFIX}/bin/nav_hmi_gui.exe)")
    endif()
elseif(UNIX AND NOT APPLE)
    # Linux-specific installation
    install(FILES scripts/nav_system.service DESTINATION /etc/systemd/system OPTIONAL)
endif()

# Create package configuration
set(CPACK_PACKAGE_NAME "AutomotiveNavigation")
set(CPACK_PACKAGE_VENDOR "Navigation Systems Inc")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Cross-platform Automotive Navigation System")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})

if(WIN32)
    set(CPACK_GENERATOR "NSIS;ZIP")
    set(CPACK_NSIS_DISPLAY_NAME "Automotive Navigation System")
    set(CPACK_NSIS_PACKAGE_NAME "AutomotiveNavigation")
    set(CPACK_NSIS_MODIFY_PATH ON)
elseif(UNIX AND NOT APPLE)
    set(CPACK_GENERATOR "DEB;RPM;TGZ")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Navigation Team")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libqt5core5a, libqt5widgets5, libqt5network5")
    set(CPACK_RPM_PACKAGE_GROUP "Applications/Engineering")
    set(CPACK_RPM_PACKAGE_REQUIRES "qt5-qtbase, qt5-qtbase-gui")
endif()

include(CPack)