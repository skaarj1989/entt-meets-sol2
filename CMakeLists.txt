cmake_minimum_required(VERSION 3.13.4...3.19)
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

option(BUILD_SHARED_LIBS "Use shared libraries" OFF)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib") # .lib
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib") # .dll
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin") # .exe

# --- Third-party ---

set(EXTERN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/extern")

add_library(entt::entt_header_only INTERFACE IMPORTED)
set_target_properties(entt::entt_header_only
	PROPERTIES
		INTERFACE_INCLUDE_DIRECTORIES "${EXTERN_DIR}/entt/single_include"
)

set(lua_DIR "${EXTERN_DIR}/lua")
add_library(Lua::Lua SHARED IMPORTED)
set_target_properties(Lua::Lua
	PROPERTIES
		INTERFACE_INCLUDE_DIRECTORIES "${lua_DIR}/include"
		IMPORTED_IMPLIB "${lua_DIR}/liblua54.a"
		IMPORTED_LOCATION "${lua_DIR}/lua54.dll"
)

#add_custom_command(TARGET Lua::Lua

foreach(build_type IN LISTS CMAKE_CONFIGURATION_TYPES)
	file(COPY "${lua_DIR}/lua54.dll"
		DESTINATION "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${build_type}"
	)
endforeach()

add_library(sol2::sol2 INTERFACE IMPORTED)
target_link_libraries(sol2::sol2 INTERFACE Lua::Lua)
target_compile_definitions(sol2::sol2
	INTERFACE
		SOL_ALL_SAFETIES_ON
)
set_target_properties(sol2::sol2
	PROPERTIES
		INTERFACE_INCLUDE_DIRECTORIES "${EXTERN_DIR}/sol2/include"
)

# ---

add_subdirectory(examples)