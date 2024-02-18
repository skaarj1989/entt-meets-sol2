cmake_minimum_required(VERSION 3.20)

if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE
    "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
    CACHE STRING "")
endif()

message(STATUS "TOOLCHAIN: ${CMAKE_TOOLCHAIN_FILE}")

project(entt-meets-sol2)

if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
  message(
    FATAL_ERROR
    "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there."
  )
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS YES)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG")

option(BUILD_SHARED_LIBS "Use shared libraries" OFF)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "$<1:${CMAKE_BINARY_DIR}/lib>") # .lib, .a
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "$<1:${CMAKE_BINARY_DIR}/lib>") # .dll, .so
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "$<1:${CMAKE_BINARY_DIR}/bin>") # .exe

if(WIN32)
  add_definitions(-DWIN32)
endif()

include(cmake/AddExample.cmake)
include(cmake/AddScripts.cmake)

find_package(EnTT CONFIG REQUIRED)

find_package(Lua REQUIRED)
add_library(Lua INTERFACE)
target_include_directories(Lua INTERFACE ${LUA_INCLUDE_DIR})
target_link_libraries(Lua INTERFACE ${LUA_LIBRARIES})
add_library(Lua::Lua ALIAS Lua)

find_package(sol2 CONFIG REQUIRED)
target_link_libraries(sol2 INTERFACE Lua::Lua)

add_scripts(TARGET Scripts DIR "${CMAKE_CURRENT_SOURCE_DIR}/scripts")
add_subdirectory(utility)
add_subdirectory(examples)
