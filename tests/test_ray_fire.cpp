// for testing
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_template_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <catch2/generators/catch_generators.hpp>


// xdg includes
#include "xdg/constants.h"
#include "xdg/mesh_manager_interface.h"
#include "mesh_mock.h"
#include "util.h"

using namespace xdg;
using namespace xdg::test;

// ------- single test, multiple sections (one per built backend) --------------

TEMPLATE_TEST_CASE("Ray Fire on MeshMock (per-backend sections)", "[rayfire][mock]",
                   Embree_Raytracer,
                   GPRT_Raytracer)
{
  // Generate one test run per enabled backend
  constexpr auto rt_backend = TestType::value;
  check_ray_tracer_supported(rt_backend); // skip if backend not enabled at configuration time

  DYNAMIC_SECTION(fmt::format("Backend = {}", rt_backend))
  {
    auto rti = create_raytracer(rt_backend);
    REQUIRE(rti);

    auto mm = std::make_shared<MeshMock>(false);
    mm->init();
    REQUIRE(mm->mesh_library() == MeshLibrary::MOCK);

    auto [volume_tree, element_tree] = rti->register_volume(mm, mm->volumes()[0]);
    REQUIRE(volume_tree != ID_NONE);
    REQUIRE(element_tree == ID_NONE);

    rti->init(); // Ensure ray tracer is initialized (e.g. build SBT for GPRT)
    Position origin {0.0, 0.0, 0.0};
    Direction direction {1.0, 0.0, 0.0};
    std::pair<double, MeshID> intersection;

    intersection = rti->ray_fire(volume_tree, origin, direction);
    REQUIRE_THAT(intersection.first, Catch::Matchers::WithinAbs(5.0, 1e-6));

    direction *= -1;
    intersection = rti->ray_fire(volume_tree, origin, direction);
    REQUIRE_THAT(intersection.first, Catch::Matchers::WithinAbs(2.0, 1e-6));

    direction = {0.0, 1.0, 0.0};
    intersection = rti->ray_fire(volume_tree, origin, direction);
    REQUIRE_THAT(intersection.first, Catch::Matchers::WithinAbs(6.0, 1e-6));
    direction *= -1;
    intersection = rti->ray_fire(volume_tree, origin, direction);
    REQUIRE_THAT(intersection.first, Catch::Matchers::WithinAbs(3.0, 1e-6));

    direction = {0.0, 0.0, 1.0};
    intersection = rti->ray_fire(volume_tree, origin, direction);
    REQUIRE_THAT(intersection.first, Catch::Matchers::WithinAbs(7.0, 1e-6));
    direction *= -1;
    intersection = rti->ray_fire(volume_tree, origin, direction);
    REQUIRE_THAT(intersection.first, Catch::Matchers::WithinAbs(4.0, 1e-6));

    // fire from the outside of the cube toward each face, ensuring that the intersection distances are correct
    // rays should skip entering intersections and intersect with the far side of the cube
    origin = {-10.0, 0.0, 0.0};
    direction = {1.0, 0.0, 0.0};
    intersection = rti->ray_fire(volume_tree, origin, direction);
    REQUIRE_THAT(intersection.first, Catch::Matchers::WithinAbs(15.0, 1e-6));

    origin = {10.0, 0.0, 0.0};
    direction = {-1.0, 0.0, 0.0};
    intersection = rti->ray_fire(volume_tree, origin, direction);
    REQUIRE_THAT(intersection.first, Catch::Matchers::WithinAbs(12.0, 1e-6));

    // fire from the outside of the cube toward each face, ensuring that the intersection distances are correct
    // in this case rays are fired with a HitOrientation::ENTERING. Rays should hit the first surface intersected
    origin = {-10.0, 0.0, 0.0};
    direction = {1.0, 0.0, 0.0};
    intersection = rti->ray_fire(volume_tree, origin, direction, INFTY, HitOrientation::ENTERING);
    REQUIRE_THAT(intersection.first, Catch::Matchers::WithinAbs(8.0, 1e-6));

    origin = {10.0, 0.0, 0.0};
    direction = {-1.0, 0.0, 0.0};
    intersection = rti->ray_fire(volume_tree, origin, direction, INFTY, HitOrientation::ENTERING);
    REQUIRE_THAT(intersection.first, Catch::Matchers::WithinAbs(5.0, 1e-6));

    // Keep the launch origin outside the cube and advance only the minimum
    // parameter. The nearer entering face must not hide the later exiting one.
    origin = {-10.0, 0.0, 0.0};
    direction = {1.0, 0.0, 0.0};
    intersection = rti->ray_fire(volume_tree, origin, direction, INFTY,
                                  HitOrientation::ANY, nullptr, 9.0);
    REQUIRE(intersection.second != ID_NONE);
    REQUIRE_THAT(intersection.first, Catch::Matchers::WithinAbs(15.0, 1e-6));
    const Scalar far_parameter = static_cast<Scalar>(intersection.first);

    // Both endpoints are inclusive; returned parameters remain relative to origin.
    intersection = rti->ray_fire(volume_tree, origin, direction, far_parameter,
                                  HitOrientation::ANY, nullptr, far_parameter);
    REQUIRE(intersection.second != ID_NONE);
    REQUIRE_THAT(intersection.first, Catch::Matchers::WithinAbs(15.0, 1e-6));
    intersection = rti->ray_fire(volume_tree, origin, direction, INFTY,
                                  HitOrientation::ANY, nullptr, 15.1);
    REQUIRE(intersection.second == ID_NONE);
    intersection = rti->ray_fire(volume_tree, origin, direction, 14.0,
                                  HitOrientation::ANY, nullptr, 9.0);
    REQUIRE(intersection.second == ID_NONE);

    // limit distance of the ray, shouldn't get a hit
    origin = {0.0, 0.0, 0.0};
    direction = {1.0, 0.0, 0.0};
    intersection = rti->ray_fire(volume_tree, origin, direction, 4.5);
    REQUIRE(intersection.second == ID_NONE);

    // if the distance is just enough, we should still get a hit
    intersection = rti->ray_fire(volume_tree, origin, direction, 5.1);
    REQUIRE(intersection.second != ID_NONE);
    REQUIRE_THAT(intersection.first, Catch::Matchers::WithinAbs(5.0, 1e-6));
    const Scalar hit_parameter = static_cast<Scalar>(intersection.first);

    // In a double build this minimum differs from the hit by less than a float ULP.
    // The exact primitive bound must still reject the earlier hit.
    intersection = rti->ray_fire(volume_tree, origin, direction, INFTY,
        HitOrientation::ANY, nullptr, std::nextafter(hit_parameter, INFTY));
    REQUIRE(intersection.second == ID_NONE);

    // Test excluding primitives, fire a ray from the origin and log the hit face
    // By providing the hit face as an excluded primitive in a subsequent ray fire,
    // there should be no intersection returned
    std::vector<MeshID> exclude_primitives;
    intersection = rti->ray_fire(volume_tree, origin, direction, INFTY, HitOrientation::EXITING, &exclude_primitives);
    REQUIRE_THAT(intersection.first, Catch::Matchers::WithinAbs(5.0, 1e-6));
    REQUIRE(exclude_primitives.size() == 1);

    intersection = rti->ray_fire(volume_tree, origin, direction, INFTY, HitOrientation::EXITING, &exclude_primitives);
    REQUIRE(intersection.second == ID_NONE);
  }
}

TEST_CASE("One fixed ray resolves both crossings of a narrow rim", "[rayfire][precision]")
{
  // The chord is smaller than one world-space y ULP, but the two hit parameters
  // are distinct floats when they are measured from the same flight origin.
  dp::vec3 cap[3] = {
    {1.1315739154815674f, -8.5323190689086914f, 0.0f},
    {1.2361010313034058f, -8.5171489715576172f, 0.0f},
    {-1.1851097345352173f, -8.5250473022460938f, 0.0f}
  };
  dp::vec3 wall[3] = {
    {1.1315739154815674f, -8.5323190689086914f, 0.0f},
    {1.2361010313034058f, -8.5171489715576172f, 0.0f},
    {1.1747461557388306f, -8.6378946304321289f, -0.26195883750915527f}
  };
  Position origin {1.074920654296875f, -8.225346565246582f, 0.032130628824234009f};
  Direction direction {0.25883090496063232f, -0.96054893732070923f, -0.10174677520990372f};
  auto entry = plucker_ray_tri_intersect(cap, origin, direction, INFTY, 0, false, 0);
  REQUIRE(entry.hit);
  auto exit = plucker_ray_tri_intersect(wall, origin, direction, INFTY, entry.t, false, 0);
  REQUIRE(exit.hit);
  REQUIRE(exit.t > entry.t);
  REQUIRE(exit.t - entry.t < Scalar(1e-6));
  REQUIRE_FALSE(plucker_ray_tri_intersect(wall, origin, direction, INFTY,
      std::nextafter(exit.t, INFTY), false, 0).hit);
}
