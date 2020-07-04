# Prevent CMake from generating in-tree builds
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
  message(FATAL_ERROR "Prevented in-tree build. Please specify another directory outside of the source root.")
endif()

cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
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

# Set compile flags
if(MSVC)
  add_compile_options(/W4 /WX)
else()
  add_compile_options(-Wall -Wpedantic -Werror)
endif()

configure_file(${PROJECT_SOURCE_DIR}/Include/Config.hpp.in Config.hpp @ONLY)

set(SOURCE_FILES Source/Main.cpp Source/Application.cpp)
set(INCLUDE_FILES Include/Application.hpp)

add_executable(Vulkan-Engine ${SOURCE_FILES} ${INCLUDE_FILES})

target_link_libraries(Vulkan-Engine Vulkan::Vulkan)
target_link_libraries(Vulkan-Engine SDL2::SDL2-static)

target_include_directories(Vulkan-Engine PRIVATE ${PROJECT_BINARY_DIR})
target_include_directories(Vulkan-Engine PRIVATE ${CMAKE_SOURCE_DIR}/Include)
target_include_directories(Vulkan-Engine PRIVATE ${Vulkan_INCLUDE_DIRS})
