#include <memory>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "mocked_mesh.h"
#include "util.h"
#include "xdg/xdg.h"

using namespace xdg;
using namespace xdg::test;

TEST_CASE("Quad Ray Fire on MockedMesh (Embree)")
{
  check_ray_tracer_supported(RTLibrary::EMBREE);
  std::shared_ptr<MeshManager> mm = std::make_shared<MockedMesh>();
  mm->init();

  std::shared_ptr<XDG> xdg = std::make_shared<XDG>(mm, RTLibrary::EMBREE);
  xdg->prepare_raytracer();

  MeshID volume = mm->volumes()[0];

  Position origin {0.6, 0.6, 1.0};
  Direction dir {0.0, 0.0, -1.0};

  auto hit_any = xdg->ray_fire(volume, origin, dir, INFTY, HitOrientation::ENTERING);
  REQUIRE_THAT(hit_any.first, Catch::Matchers::WithinAbs(1.0, 1e-6));
  REQUIRE(hit_any.second == 0);

  auto hit_enter = xdg->ray_fire(volume, origin, dir, INFTY, HitOrientation::ENTERING);
  REQUIRE(hit_enter.second == 0);

  auto hit_exit = xdg->ray_fire(volume, origin, dir, INFTY, HitOrientation::EXITING);
  REQUIRE(hit_exit.second == ID_NONE);
}
