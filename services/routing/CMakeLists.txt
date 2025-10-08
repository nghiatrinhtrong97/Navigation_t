cmake_minimum_required(VERSION 3.16)

add_executable(routing_service
    src/routing_service.cpp
    src/routing_main.cpp
    src/astar_algorithm.cpp
)

target_include_directories(routing_service PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(routing_service
    nav_common
)

if(QNX)
    target_link_libraries(routing_service ${QNX_C_LIB})
endif()

target_compile_features(routing_service PRIVATE cxx_std_17)