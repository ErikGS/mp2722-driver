cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

idf_component_register(SRCS "src/MP2722.cpp" "src/MP2722_platform.cpp"
                       INCLUDE_DIRS "src"
                       REQUIRES esp_driver_i2c
                       )

idf_build_set_property(MINIMAL_BUILD ON)

project(mp2722-driver)
