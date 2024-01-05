cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(openmc C CXX)

option(XDG_ENABLE_MOAB "Enable support for the MOAB mesh library" ON)
option(XDG_ENABLE_MFEM "Enable support for the MFEM mesh library" OFF)
option(XDG_ENABLE_LIBMESH "Enable support for the libMesh mesh library" OFF)
option(XDG_BUILD_TESTS "Enable C++ unit testing" ON)

if(NOT CMAKE_BUILD_TYPE)
 set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose build type" FORCE)
endif()

# Compiler options (things in this section may not be platform-portable)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# require MOAB for now
if (XDG_ENABLE_MOAB)
find_package(MOAB REQUIRED HINTS ${MOAB_DIR})
endif()

# use Embree for CPU ray tracing
find_package(embree 3.6.1 REQUIRED)
if (NOT ${EMBREE_VERSION} VERSION_GREATER 3.6.0)
  message(FATAL_ERROR "XDG requires Embree v3.6.1 or higher.")
endif()

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
src/closest.cpp
src/error.cpp
src/mesh_manager_interface.cpp
src/plucker.cpp
src/ray_tracing_interface.cpp
src/triangle_ref.cpp
src/util/str_utils.cpp
)

if (XDG_ENABLE_MOAB)
list(APPEND xdg_sources
# MOAB
src/moab/mesh_manager.cpp
src/moab/direct_access.cpp
src/moab/metadata.cpp
)
endif()

add_library(xdg SHARED ${xdg_sources})

add_library(xdg::libxdg ALIAS xdg)

# pass precompile definition depending on embree version
if (${EMBREE_VERSION} VERSION_GREATER_EQUAL 4.0.0)
  target_compile_definitions(xdg PUBLIC XDG_EMBREE4)
else()
  target_compile_definitions(xdg PUBLIC XDG_EMBREE3)
endif()

if (XDG_BUILD_TESTS)
  add_subdirectory("tests")
endif()

target_include_directories(xdg
  PUBLIC
  $<INSTALL_INTERFACE:include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(xdg embree fmt::fmt)

if (XDG_ENABLE_MOAB)
  target_link_libraries(xdg MOAB)
endif()
