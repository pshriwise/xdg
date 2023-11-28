cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(openmc C CXX)

option(ENABLE_MOAB_BACKEND "Enable MAOB mesh" ON)
option(ENABLE_MFEM_BACKEND "Enable MFEM mesh" OFF)
option(ENABLE_LIBMESH_BACKEND "Enable libMesh mesh" OFF)

find_package(MOAB REQUIRED HINTS ${MOAB_DIR})

# use Embree for CPU ray tracing
find_package(embree REQUIRED HINTS ${EMBREE_DIR})




