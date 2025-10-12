cmake_minimum_required(VERSION 3.16)

# Common library containing shared types and utilities (non-Qt)
set(COMMON_HEADERS
    include/nav_types.h
    include/nav_messages.h  
    include/nav_utils.h
)

set(COMMON_SOURCES
    src/nav_utils.cpp
    src/nmea_parser.cpp
    src/can_interface.cpp
)

add_library(nav_common STATIC
    ${COMMON_SOURCES}
    ${COMMON_HEADERS}
)

target_include_directories(nav_common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Link QNX libraries if building for QNX
if(QNX)
    target_link_libraries(nav_common ${QNX_C_LIB})
endif()

target_compile_features(nav_common PUBLIC cxx_std_17)

# Separate Qt-based library for ServiceBase
if(BUILD_GUI AND (Qt5_FOUND OR Qt6_FOUND))
    # Enable MOC for ServiceBase
    set(CMAKE_AUTOMOC ON)
    
    add_library(nav_service_base STATIC
        src/service_base.cpp
        include/service_base.h
    )
    
    target_include_directories(nav_service_base PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    
    # Qt linking (version-agnostic)
    if(QT_VERSION_MAJOR EQUAL 6)
        target_link_libraries(nav_service_base PUBLIC
            Qt6::Core
            Qt6::Network
        )
    else()
        target_link_libraries(nav_service_base PUBLIC
            Qt5::Core
            Qt5::Network
        )
    endif()
    
    target_compile_features(nav_service_base PUBLIC cxx_std_17)
    
    message(STATUS "Building nav_service_base with Qt support")
endif()