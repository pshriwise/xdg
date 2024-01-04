
// for testing
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "mesh_mock.h"
// xdg includes
#include "xdg/mesh_manager_interface.h"
#include "xdg/moab/mesh_manager.h"
#include "xdg/ray_tracing_interface.h"

using namespace xdg;

TEST_CASE("Test Ray Fire Mesh Mock")
{
  std::shared_ptr<MeshManager> mm = std::make_shared<MeshMock>();
  mm->init(); // this should do nothing

  std::shared_ptr<RayTracingInterface> rti = std::make_shared<RayTracingInterface>();

  rti->register_all_volumes(mm);

  MeshID volume = mm->volumes()[0];

  Position origin {0.0, 0.0, 0.0};
  Direction direction {1.0, 0.0, 0.0};
  double intersection_distance {0.0};

  // fire from the origin toward each face, ensuring that the intersection distances are correct
  rti->ray_fire(volume, origin, direction, intersection_distance);
  REQUIRE_THAT(intersection_distance, Catch::Matchers::WithinAbs(5.0, 1e-6));

  direction *= -1;
  rti->ray_fire(volume, origin, direction, intersection_distance);
  REQUIRE_THAT(intersection_distance, Catch::Matchers::WithinAbs(2.0, 1e-6));

  direction = {0.0, 1.0, 0.0};
  rti->ray_fire(volume, origin, direction, intersection_distance);
  REQUIRE_THAT(intersection_distance, Catch::Matchers::WithinAbs(6.0, 1e-6));

  direction *= -1;
  rti->ray_fire(volume, origin, direction, intersection_distance);
  REQUIRE_THAT(intersection_distance, Catch::Matchers::WithinAbs(3.0, 1e-6));

  direction = {0.0, 0.0, 1.0};
  rti->ray_fire(volume, origin, direction, intersection_distance);
  REQUIRE_THAT(intersection_distance, Catch::Matchers::WithinAbs(7.0, 1e-6));

  direction *= -1;
  rti->ray_fire(volume, origin, direction, intersection_distance);
  REQUIRE_THAT(intersection_distance, Catch::Matchers::WithinAbs(4.0, 1e-6));

  // fire from the outside of the cube toward each face, ensuring that the intersection distances are correct
  // rays should skip entering intersections and intersect with the far side of the cube
  origin = {-10.0, 0.0, 0.0};
  direction = {1.0, 0.0, 0.0};
  rti->ray_fire(volume, origin, direction, intersection_distance);
  REQUIRE_THAT(intersection_distance, Catch::Matchers::WithinAbs(15.0, 1e-6));

  origin = {10.0, 0.0, 0.0};
  direction = {-1.0, 0.0, 0.0};
  rti->ray_fire(volume, origin, direction, intersection_distance);
  REQUIRE_THAT(intersection_distance, Catch::Matchers::WithinAbs(12.0, 1e-6));

}

TEST_CASE("Test Ray Fire MOAB")
{
  std::shared_ptr<MeshManager> mesh_manager = std::make_shared<MOABMeshManager>();

  mesh_manager->load_file("cube.h5m");
  mesh_manager->init();

  std::shared_ptr<RayTracingInterface> rti = std::make_shared<RayTracingInterface>();

  rti->register_all_volumes(mesh_manager);

  MeshID volume = mesh_manager->volumes()[0];

  Position origin {0.0, 0.0, 0.0};
  Direction direction {1.0, 0.0, 0.0};
  double intersection_distance {0.0};

  rti->ray_fire(volume, origin, direction, intersection_distance);

  // this cube is 10 cm on a side, so the ray should hit the surface at 5 cm
  REQUIRE_THAT(intersection_distance, Catch::Matchers::WithinAbs(5.0, 1e-6));

  origin = {3.0, 0.0, 0.0};
  rti->ray_fire(volume, origin, direction, intersection_distance);
  REQUIRE_THAT(intersection_distance, Catch::Matchers::WithinAbs(2.0, 1e-6));

  origin = {-10.0, 0.0, 0.0};
  rti->ray_fire(volume, origin, direction, intersection_distance);
  REQUIRE_THAT(intersection_distance, Catch::Matchers::WithinAbs(15.0, 1e-6));
}
