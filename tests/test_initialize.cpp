// stl includes
#include <memory>

// testing includes
#include <catch2/catch_test_macros.hpp>

// xdg includes
#include "xdg/mesh_manager_interface.h"
#include "xdg/moab/mesh_manager.h"

using namespace xdg;

TEST_CASE("Test initialization")
{

  std::unique_ptr<MeshManager> mesh_manager = std::make_unique<MOABMeshManager>();

  mesh_manager->load_file("test_files/pwr_pincell.h5m");
  mesh_manager->init();

  REQUIRE(mesh_manager->num_volumes() == 3);
  REQUIRE(mesh_manager->num_surfaces() == 12);
}