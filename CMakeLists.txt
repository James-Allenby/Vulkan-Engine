# Prevent CMake from generating in-tree builds
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
  message(FATAL_ERROR "Prevented in-tree build. Please specify another directory outside of the source root.")
endif()

cmake_minimum_required(VERSION 3.5)
project(Vulkan-Engine VERSION 0.1.0 LANGUAGES CXX)

message(NOTICE "----------------------")
message(NOTICE "Building Vulkan-Engine")
message(NOTICE "----------------------")
message(NOTICE "Source Dir: ${PROJECT_SOURCE_DIR}")
message(NOTICE "Binary Dir: ${PROJECT_BINARY_DIR}")

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMake)

find_package(Vulkan REQUIRED)
find_package(SDL2 REQUIRED)

# Require a C++17 compatible compiler
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

configure_file(${PROJECT_SOURCE_DIR}/Include/Config.hpp.in Config.hpp @ONLY)

add_executable(vulkan-engine
  Source/Main.cpp
  Source/Application.cpp)

target_link_libraries(vulkan-engine Vulkan::Vulkan)
target_link_libraries(vulkan-engine SDL2::SDL2-static)

target_include_directories(vulkan-engine PRIVATE ${PROJECT_BINARY_DIR})
target_include_directories(vulkan-engine PRIVATE ${CMAKE_SOURCE_DIR}/Include)
target_include_directories(vulkan-engine PRIVATE ${Vulkan_INCLUDE_DIRS})
