cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(openmc C CXX)

option(ENABLE_MOAB_BACKEND "Enable MAOB mesh" ON)
option(ENABLE_MFEM_BACKEND "Enable MFEM mesh" OFF)
option(ENABLE_LIBMESH_BACKEND "Enable libMesh mesh" OFF)

if(NOT CMAKE_BUILD_TYPE)
 set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Choose build type" FORCE)
endif()

find_package(MOAB REQUIRED HINTS ${MOAB_DIR})

# use Embree for CPU ray tracing
find_package(embree REQUIRED HINTS ${EMBREE_DIR})

# Catch2 testing module
find_package(Catch2 QUIET NO_SYSTEM_ENVIRONMENT_PATH)
if (NOT Catch2_FOUND)
  add_subdirectory("vendor/catch2")
endif()
include(CTest)

list(APPEND xdg_sources
src/moab/mesh_manager.cpp
src/moab/direct_access.cpp
src/moab/metadata.cpp
src/error.cpp
)

add_library(libxdg SHARED ${xdg_sources})

add_library(xdg::libxdg ALIAS libxdg)

add_subdirectory("tests")

target_include_directories(libxdg
  PUBLIC
  $<INSTALL_INTERFACE:include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

if (ENABLE_MOAB_BACKEND)
  target_link_libraries(libxdg MOAB)
endif()