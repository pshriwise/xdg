
// for testing
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

// xdg includes
#include "xdg/mesh_manager_interface.h"
#include "xdg/moab/mesh_manager.h"
#include "xdg/ray_tracing_interface.h"

using namespace xdg;

TEST_CASE("Test Ray Fire")
{
  std::shared_ptr<MeshManager> mesh_manager = std::make_shared<MOABMeshManager>();

  mesh_manager->load_file("cube.h5m");
  mesh_manager->init();

  std::shared_ptr<RayTracingInterface> rti = std::make_shared<RayTracingInterface>();

  rti->register_all_volumes(mesh_manager);

  Position origin {0.0, 0.0, 0.0};
  Direction direction {1.0, 0.0, 0.0};
  double intersection_distance {0.0};

  rti->ray_fire(mesh_manager->volumes()[0], origin, direction, intersection_distance);

  // this cube is 10 cm on a side, so the ray should hit the surface at 5 cm
  REQUIRE_THAT(intersection_distance, Catch::Matchers::WithinAbs(5.0, 1e-6));
}