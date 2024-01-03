

// for testing
#include <catch2/catch_test_macros.hpp>

// xdg includes
#include "xdg/constants.h"
#include "xdg/mesh_manager_interface.h"
#include "xdg/moab/mesh_manager.h"
#include "xdg/ray_tracing_interface.h"

using namespace xdg;

TEST_CASE("Test BVH Build")
{
  std::shared_ptr<MeshManager> mesh_manager = std::make_shared<MOABMeshManager>();

  mesh_manager->load_file("cube.h5m");
  mesh_manager->init();

  REQUIRE(mesh_manager->num_volumes() == 1);
  REQUIRE(mesh_manager->num_surfaces() == 6);

  std::unique_ptr<RayTracingInterface> ray_tracing_interface = std::make_unique<RayTracingInterface>();

  for (auto volume : mesh_manager->volumes()) {
    ray_tracing_interface->register_volume(mesh_manager, volume);
  }

  REQUIRE(ray_tracing_interface->num_registered_volumes() == 1);
}