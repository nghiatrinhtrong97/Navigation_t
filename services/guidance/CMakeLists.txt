cmake_minimum_required(VERSION 3.16)

add_executable(guidance_service
    src/guidance_service.cpp
    src/guidance_main.cpp
    src/turn_by_turn.cpp
    src/route_monitoring.cpp
)

target_include_directories(guidance_service PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(guidance_service
    nav_common
)

if(QNX)
    target_link_libraries(guidance_service ${QNX_C_LIB})
endif()

target_compile_features(guidance_service PRIVATE cxx_std_17)