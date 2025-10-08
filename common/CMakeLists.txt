cmake_minimum_required(VERSION 3.16)

# Common library containing shared types and utilities
add_library(nav_common STATIC
    src/nav_utils.cpp
    src/nmea_parser.cpp
    src/can_interface.cpp
)

target_include_directories(nav_common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Link QNX libraries if building for QNX
if(QNX)
    target_link_libraries(nav_common ${QNX_C_LIB})
endif()

target_compile_features(nav_common PUBLIC cxx_std_17)