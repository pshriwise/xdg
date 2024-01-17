
// for testing
#include <catch2/catch_test_macros.hpp>

// xdg includes
#include "xdg/mesh_manager_interface.h"
#include "xdg/xdg.h"

#include "mesh_mock.h"

using namespace xdg;

TEST_CASE("Test Occluded")
{
  std::shared_ptr<MeshManager> mm = std::make_shared<MeshMock>();
  mm->init(); // this should do nothing, just good practice to call it
  std::shared_ptr<XDG> xdg = std::make_shared<XDG>(mm);
  xdg->prepare_raytracer();
  std::shared_ptr<RayTracer> rti = std::make_shared<RayTracer>();
  TreeID volume_tree = rti->register_volume(mm, mm->volumes()[0]);

  // setup ray to fire that won't hit the mock model
  Position r {-100.0, 0.0, 0.0};
  Direction u {1.0, 0.0, 0.0};
  double dist {0.0};

  bool result = rti->occluded(volume_tree, r, u, dist);
  REQUIRE(result == true);

  u = {-1.0, 0.0, 0.0};
  result = rti->occluded(volume_tree, r, u, dist);
  REQUIRE(result == false);
}