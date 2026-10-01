cmake_minimum_required(VERSION 3.20)

project(rasterizer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()

add_library(rasterizer_lib STATIC
  src/core/Color.cpp
  src/core/Matrix.cpp
  src/core/Texture.cpp
  src/core/Vec.cpp
  src/io/ObjLoader.cpp
  src/io/PpmLoader.cpp
  src/io/PpmWriter.cpp
  src/raster/Clipper.cpp
  src/raster/Framebuffer.cpp
  src/raster/Projector.cpp
  src/raster/Rasterizer.cpp
)

target_include_directories(rasterizer_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_compile_options(rasterizer_lib PRIVATE -Wall -Wextra -Wpedantic)

add_executable(rasterizer src/main.cpp)

target_link_libraries(rasterizer PRIVATE rasterizer_lib)

target_compile_options(rasterizer PRIVATE -Wall -Wextra -Wpedantic)
