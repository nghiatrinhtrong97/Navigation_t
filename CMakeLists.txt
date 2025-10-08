cmake_minimum_required(VERSION 3.16)
project(AutomotiveNavigation VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# QNX specific settings
if(QNX)
    set(CMAKE_SYSTEM_NAME QNX)
    add_definitions(-D__QNX__)
    # Add QNX specific libraries
    find_library(QNX_C_LIB c)
    find_library(QNX_SOCKET_LIB socket)
endif()

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2")
if(QNX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions")
endif()

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/common/include)

# Common library
add_subdirectory(common)

# Services
add_subdirectory(services/positioning)
add_subdirectory(services/map)
add_subdirectory(services/routing)
add_subdirectory(services/guidance)

# HMI Application
add_subdirectory(hmi)

# Tests (if enabled)
option(BUILD_TESTS "Build test programs" OFF)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Installation
install(TARGETS positioning_service map_service routing_service guidance_service
        RUNTIME DESTINATION bin)

# Create package
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
include(CPack)