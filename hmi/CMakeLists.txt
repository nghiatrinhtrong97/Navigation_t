cmake_minimum_required(VERSION 3.16)

# HMI Application (Qt-based)
find_package(Qt5 COMPONENTS Core Widgets Network QUIET)

# Set up Qt5
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(Qt5_FOUND)
    # GUI application with full window interface
    add_executable(nav_hmi_gui
        src/main.cpp
        src/navigation_main_window.cpp
        src/navigation_controller_v2.cpp
        src/map_renderer.cpp
        src/service_clients_qt.cpp
    )
    
    target_include_directories(nav_hmi_gui PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../common/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../services/positioning/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../services/routing/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../services/guidance/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../services/map/include
    )
    
    target_link_libraries(nav_hmi_gui
        nav_common
        Qt5::Core
        Qt5::Widgets
        Qt5::Network
    )
    
    if(QNX)
        target_link_libraries(nav_hmi_gui ${QNX_C_LIB})
    endif()
    
    target_compile_features(nav_hmi_gui PRIVATE cxx_std_17)
    
    # Console application (fallback)
    add_executable(nav_hmi
        src/console_main.cpp
    )
    
    target_include_directories(nav_hmi PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../common/include
    )
    
    target_link_libraries(nav_hmi
        nav_common
    )
    
    if(QNX)
        target_link_libraries(nav_hmi ${QNX_C_LIB})
    endif()
    
    target_compile_features(nav_hmi PRIVATE cxx_std_17)
    
    message(STATUS "Qt5 found - Both GUI and console HMI applications will be built")
else()
    # Fallback console application only
    add_executable(nav_hmi
        src/console_main.cpp
    )
    
    target_include_directories(nav_hmi PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../common/include
    )
    
    target_link_libraries(nav_hmi
        nav_common
    )
    
    if(QNX)
        target_link_libraries(nav_hmi ${QNX_C_LIB})
    endif()
    
    target_compile_features(nav_hmi PRIVATE cxx_std_17)
    
    message(STATUS "Qt5 not found - Building console HMI application only")
endif()