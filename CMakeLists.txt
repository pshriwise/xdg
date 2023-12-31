cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(openmc C CXX)

option(XDG_MOAB_BACKEND "Enable MAOB mesh" ON)
option(XDG_MFEM_BACKEND "Enable MFEM mesh" OFF)
option(XDG_LIBMESH_BACKEND "Enable libMesh mesh" OFF)
option(XDG_BUILD_TESTS "Enable C++ unit testing" ON)

if(NOT CMAKE_BUILD_TYPE)
 set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose build type" FORCE)
endif()

# Compiler options (things in this section may not be platform-portable)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# require MOAB for now
find_package(MOAB REQUIRED HINTS ${MOAB_DIR})

# use Embree for CPU ray tracing
find_package(embree 3.0.0 REQUIRED)

# Catch2 testing module
if (XDG_BUILD_TESTS)
  find_package(Catch2 QUIET NO_SYSTEM_ENVIRONMENT_PATH)
  if (NOT Catch2_FOUND)
    add_subdirectory("vendor/catch2")
  endif()
  include(CTest)
endif()

# fmt
find_package(fmt QUIET NO_SYSTEM_ENVIRONMENT_PATH)
if (NOT fmt_FOUND)
  set(FMT_INSTALL ON CACHE BOOL "Generate the fmt install target")
  add_subdirectory(vendor/fmt)
endif()

list(APPEND xdg_sources
src/mesh_manager_interface.cpp
src/error.cpp
src/util/str_utils.cpp

# MOAB
src/moab/mesh_manager.cpp
src/moab/direct_access.cpp
src/moab/metadata.cpp
)

add_library(libxdg SHARED ${xdg_sources})

add_library(xdg::libxdg ALIAS libxdg)

if (XDG_BUILD_TESTS)
  add_subdirectory("tests")
endif()

target_include_directories(libxdg
  PUBLIC
  $<INSTALL_INTERFACE:include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(libxdg fmt::fmt)

if (XDG_MOAB_BACKEND)
  target_link_libraries(libxdg MOAB)
endif()
