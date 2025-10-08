cmake_minimum_required(VERSION 3.16)

# Find Qt5 for service implementation
find_package(Qt5 COMPONENTS Core Network QUIET)

set(CMAKE_AUTOMOC ON)

if(Qt5_FOUND)
    # Qt-based service
    add_executable(nav_positioning_qt
        src/positioning_main_qt.cpp
        src/positioning_service.cpp
        src/dead_reckoning.cpp
        src/map_matching.cpp
    )
    
    target_include_directories(nav_positioning_qt PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../common/include
    )
    
    target_link_libraries(nav_positioning_qt
        nav_common
        Qt5::Core
        Qt5::Network
    )
    
    if(QNX)
        target_link_libraries(nav_positioning_qt ${QNX_C_LIB})
    endif()
    
    target_compile_features(nav_positioning_qt PRIVATE cxx_std_17)
    
    message(STATUS "Qt5 found - Building Qt positioning service")
    
    # Also create alias for original name
    add_executable(positioning_service
        src/positioning_service.cpp
        src/positioning_main.cpp
        src/dead_reckoning.cpp
        src/map_matching.cpp
    )
    
    target_include_directories(positioning_service PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../common/include
    )
    
    target_link_libraries(positioning_service
        nav_common
    )
    
    if(QNX)
        target_link_libraries(positioning_service ${QNX_C_LIB})
    endif()
    
    target_compile_features(positioning_service PRIVATE cxx_std_17)
    
else()
    # Fallback to original implementation
    add_executable(positioning_service
        src/positioning_service.cpp
        src/positioning_main.cpp
        src/dead_reckoning.cpp
        src/map_matching.cpp
    )
    
    target_include_directories(positioning_service PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../common/include
    )
    
    target_link_libraries(positioning_service
        nav_common
    )
    
    if(QNX)
        target_link_libraries(positioning_service ${QNX_C_LIB})
    endif()
    
    target_compile_features(positioning_service PRIVATE cxx_std_17)
    
    message(STATUS "Qt5 not found - Building original positioning service")
endif()