cmake_minimum_required(VERSION 3.16)
project(mp2722_tests CXX)

set(CMAKE_CXX_STANDARD 17)

# Fetch a test framework
include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.5.2
)
FetchContent_MakeAvailable(Catch2)

add_executable(mp2722_tests
    test_mp2722.cpp
    mock_platform.cpp
    ${CMAKE_SOURCE_DIR}/../src/MP2722.cpp
)

target_include_directories(mp2722_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/../src
    ${CMAKE_SOURCE_DIR}/../include
)

target_link_libraries(mp2722_tests PRIVATE Catch2::Catch2WithMain)

include(CTest)
include(Catch)
catch_discover_tests(mp2722_tests)
