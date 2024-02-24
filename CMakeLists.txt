cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(openmc C CXX)

option(XDG_ENABLE_MOAB    "Enable support for the MOAB mesh library"    ON)
option(XDG_ENABLE_MFEM    "Enable support for the MFEM mesh library"    OFF)
option(XDG_ENABLE_LIBMESH "Enable support for the libMesh mesh library" OFF)
option(XDG_BUILD_TESTS    "Enable C++ unit testing"                     ON)
option(XDG_BUILD_TOOLS    "Enable tools and miniapps"                   ON)

# Set version numbers
set(XDG_VERSION_MAJOR 0)
set(XDG_VERSION_MINOR 14)
set(XDG_VERSION_RELEASE 1)
set(XDG_VERSION ${OPENMC_VERSION_MAJOR}.${OPENMC_VERSION_MINOR}.${OPENMC_VERSION_RELEASE})

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
find_package(embree 3.0.0...4.0.0 REQUIRED)
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
src/geometry/measure.cpp
src/geometry/plucker.cpp
src/geometry/closest.cpp
src/error.cpp
src/mesh_manager_interface.cpp
src/ray_tracing_interface.cpp
src/triangle_intersect.cpp
src/util/str_utils.cpp
src/xdg.cpp
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

# pass precompile definition depending on embree version
if (${EMBREE_VERSION} VERSION_GREATER_EQUAL 4.0.0)
  target_compile_definitions(xdg PUBLIC XDG_EMBREE4)
else()
  target_compile_definitions(xdg PUBLIC XDG_EMBREE3)
endif()

if (${CMAKE_BUILD_TYPE} MATCHES "Debug")
  target_compile_definitions(xdg PUBLIC XDG_DEBUG)
endif()

if (XDG_BUILD_TESTS)
  add_subdirectory("tests")
endif()

if (XDG_BUILD_TOOLS)
  add_subdirectory("tools")
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

#===============================================================================
# RPATH information (from OpenMC)
#===============================================================================

# Provide install directory variables as defined by GNU coding standards
include(GNUInstallDirs)

# This block of code ensures that dynamic libraries can be found via the RPATH
# whether the executable is the original one from the build directory or the
# installed one in CMAKE_INSTALL_PREFIX. Ref:
# https://gitlab.kitware.com/cmake/community/wikis/doc/cmake/RPATH-handling

# use, i.e. don't skip the full RPATH for the build tree
set(CMAKE_SKIP_BUILD_RPATH  FALSE)

# when building, don't use the install RPATH already
# (but later on when installing)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)

# add the automatically determined parts of the RPATH
# which point to directories outside the build tree to the install RPATH
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# the RPATH to be used when installing, but only if it's not a system directory
list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_FULL_LIBDIR}" isSystemDir)
if("${isSystemDir}" STREQUAL "-1")
  set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_FULL_LIBDIR}")
endif()

#=================================================================
# Installation & Packaging
#=================================================================
configure_file(cmake/XDGConfig.cmake.in "${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/XDGConfig.cmake" @ONLY)
configure_file(cmake/XDGConfigVersion.cmake.in "${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/XDGConfigVersion.cmake" @ONLY)

install(TARGETS xdg
  EXPORT xdg-targets
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  INCLUDES DESTINATION include
)

install(EXPORT xdg-targets
  FILE XDGTargets.cmake
  NAMESPACE xdg::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/xdg
)

install(FILES
  "${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/XDGConfig.cmake"
  "${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/XDGConfigVersion.cmake"
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/xdg
)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
