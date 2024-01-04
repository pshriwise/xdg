#include <memory>

// for testing
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "mesh_mock.h"

#include "xdg/ray_tracing_interface.h"

using namespace xdg;

TEST_CASE("Test Mesh Mock")
{
  std::shared_ptr<MeshManager> mm = std::make_shared<MeshMock>();
  mm->init(); // this should do nothing

  REQUIRE(mm->num_volumes() == 1);
  REQUIRE(mm->num_surfaces() == 6);
  REQUIRE(mm->num_volume_elements(1) == 12);

  std::shared_ptr<RayTracingInterface> rti = std::make_shared<RayTracingInterface>();

  rti->register_all_volumes(mm);

  MeshID volume = mm->volumes()[0];

  Position origin {0.0, 0.0, 0.0};
  double nearest_distance {0.0};

  rti->closest(volume, origin, nearest_distance);
  REQUIRE_THAT(nearest_distance, Catch::Matchers::WithinAbs(2.0, 1e-6));

  // move the point closer to the positive x surface
  origin = {4.0, 0.0, 0.0};
  rti->closest(volume, origin, nearest_distance);
  REQUIRE_THAT(nearest_distance, Catch::Matchers::WithinAbs(1.0, 1e-6));



}

