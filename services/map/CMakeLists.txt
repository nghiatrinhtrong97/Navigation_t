cmake_minimum_required(VERSION 3.16)

add_executable(map_service
    src/map_service.cpp
    src/map_main.cpp
    src/map_data_loader.cpp
    src/map_tile_manager.cpp
)

target_include_directories(map_service PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(map_service
    nav_common
)

if(QNX)
    target_link_libraries(map_service ${QNX_C_LIB})
endif()

target_compile_features(map_service PRIVATE cxx_std_17)