// testing includes
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

// xdg includes
#include "xdg/moab/mesh_manager.h"
#include "xdg/libmesh/mesh_manager.h"

using namespace xdg;

TEST_CASE("Test MOAB Mesh Without Geometry")
{
  std::unique_ptr<MeshManager> mesh_manager = std::make_unique<MOABMeshManager>();

  mesh_manager->load_file("cube-mesh-no-geom.h5m");
  mesh_manager->init();

  // there will be two volumes: one for the cube and one for the implicit complement
  REQUIRE(mesh_manager->num_volumes() == 2);
  // one surface (the boundary of the mesh) separates the two volumes
  REQUIRE(mesh_manager->num_surfaces() == 1);

  REQUIRE(mesh_manager->num_volume_elements() == 8814);
}

TEST_CASE("Test libMesh Mesh Without Geometry")
{
  std::unique_ptr<MeshManager> mesh_manager = std::make_unique<LibMeshManager>();

  mesh_manager->load_file("cube-mesh-no-geom.exo");
  mesh_manager->init();

  // there will be two volumes: one for the cube and one for the implicit complement
  REQUIRE(mesh_manager->num_volumes() == 2);
  // one surface (the boundary of the mesh) separates the two volumes
  REQUIRE(mesh_manager->num_surfaces() == 1);

  REQUIRE(mesh_manager->num_volume_elements() == 8814);
}