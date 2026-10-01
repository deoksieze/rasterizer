cmake_minimum_required(VERSION 3.20)

project(rasterizer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()

add_executable(rasterizer main.cpp)

target_include_directories(rasterizer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_options(rasterizer PRIVATE -Wall -Wextra -Wpedantic)
