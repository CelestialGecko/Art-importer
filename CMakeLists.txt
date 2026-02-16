cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if ("${CMAKE_SYSTEM_NAME}" STREQUAL "iOS" OR IOS)
    set(CMAKE_OSX_ARCHITECTURES "arm64")
else()
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
endif()

set(CMAKE_CXX_VISIBILITY_PRESET hidden)

project(Artimporter VERSION 1.0.0)

add_library(${PROJECT_NAME} SHARED
    src/main.cpp
    src/importer/CreateArt.cpp
    src/importer/MainMenu.cpp
)

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK! Please define GEODE_SDK environment variable to point to Geode")
else()
    message(STATUS "Found Geode: $ENV{GEODE_SDK}")
endif()

set_source_files_properties(src/importer/CreateArt.cpp PROPERTIES SKIP_PRECOMPILE_HEADERS ON)

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

target_include_directories(${PROJECT_NAME} PRIVATE include)

setup_geode_mod(${PROJECT_NAME})
