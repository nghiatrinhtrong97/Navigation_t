cmake_minimum_required(VERSION 3.16)

# GoogleTest setup
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        release-1.12.1
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

# Test for address components
add_executable(test_address_components
    test_address_components.cpp
    ../hmi/services/geocoding/src/address_components.cpp
)

target_include_directories(test_address_components PRIVATE
    ../hmi/services/geocoding/include
    ../common/include
)

target_link_libraries(test_address_components
    gtest_main
    nav_common
)

# Register test with CTest
include(GoogleTest)
gtest_discover_tests(test_address_components)

message(STATUS "Unit tests configured with GoogleTest")
