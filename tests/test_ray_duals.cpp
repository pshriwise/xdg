#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "xdg/ray.h"
#include "xdg/vec3/vec3.h"

using namespace xdg;

TEST_CASE("Test RTCDualRay properties")
{
  RTCDualRay ray;

  SECTION("Default constructor initializes values correctly")
  {
    REQUIRE(ray.tnear == 0.0);
    REQUIRE(ray.tfar == INFTYF);
    REQUIRE(ray.mask == -1);
  }

  SECTION("set_org with array")
  {
    double org[3] = {1.0, 2.0, 3.0};
    ray.set_org(org);
    REQUIRE(ray.org_x == 1.0f);
    REQUIRE(ray.org_y == 2.0f);
    REQUIRE(ray.org_z == 3.0f);
    REQUIRE(ray.dorg_x == 1.0);
    REQUIRE(ray.dorg_y == 2.0);
    REQUIRE(ray.dorg_z == 3.0);
  }

  SECTION("set_org with Position")
  {
    Position org(1.0, 2.0, 3.0);
    ray.set_org(org);
    REQUIRE(ray.org_x == 1.0f);
    REQUIRE(ray.org_y == 2.0f);
    REQUIRE(ray.org_z == 3.0f);
    REQUIRE(ray.dorg_x == 1.0);
    REQUIRE(ray.dorg_y == 2.0);
    REQUIRE(ray.dorg_z == 3.0);
  }

  SECTION("set_dir with array")
  {
    double dir[3] = {1.0, 2.0, 3.0};
    ray.set_dir(dir);
    REQUIRE(ray.dir_x == 1.0f);
    REQUIRE(ray.dir_y == 2.0f);
    REQUIRE(ray.dir_z == 3.0f);
    REQUIRE(ray.ddir_x == 1.0);
    REQUIRE(ray.ddir_y == 2.0);
    REQUIRE(ray.ddir_z == 3.0);
  }

  SECTION("set_dir with Direction")
  {
    Direction dir(1.0, 2.0, 3.0);
    ray.set_dir(dir);
    REQUIRE(ray.dir_x == 1.0f);
    REQUIRE(ray.dir_y == 2.0f);
    REQUIRE(ray.dir_z == 3.0f);
    REQUIRE(ray.ddir_x == 1.0);
    REQUIRE(ray.ddir_y == 2.0);
    REQUIRE(ray.ddir_z == 3.0);
  }

  SECTION("set_tfar")
  {
    ray.set_tfar(10.0);
    REQUIRE(ray.tfar == 10.0f);
    REQUIRE(ray.dtfar == 10.0);

    // Test clamping to INFTYF
    ray.set_tfar(INFTY);
    REQUIRE(ray.tfar == INFTYF);
    REQUIRE(ray.dtfar == INFTY);
  }

  SECTION("set_tnear")
  {
    ray.set_tnear(5.0);
    REQUIRE(ray.tnear == 5.0);
  }
}

TEST_CASE("Test RTCSurfaceDualRay properties")
{
  RTCSurfaceDualRay ray;

  SECTION("Default constructor inherits RTCDualRay properties")
  {
    REQUIRE(ray.tnear == 0.0);
    REQUIRE(ray.tfar == INFTYF);
    REQUIRE(ray.mask == -1);
  }

  SECTION("Member variables initialization")
  {
    REQUIRE(ray.rf_type == RayFireType::VOLUME);
    REQUIRE(ray.orientation == HitOrientation::EXITING);
    REQUIRE(ray.exclude_primitives == nullptr);
  }

  SECTION("Setting ray fire type and orientation")
  {
    ray.rf_type = RayFireType::FIND_VOLUME;
    ray.orientation = HitOrientation::ANY;
    REQUIRE(ray.rf_type == RayFireType::FIND_VOLUME);
    REQUIRE(ray.orientation == HitOrientation::ANY);
  }
}

TEST_CASE("Test RTCElementDualRay properties")
{
  RTCElementDualRay ray;

  SECTION("Default constructor inherits RTCDualRay properties")
  {
    REQUIRE(ray.tnear == 0.0);
    REQUIRE(ray.tfar == INFTYF);
    REQUIRE(ray.mask == -1);
  }

  SECTION("Element ID initialization")
  {
    REQUIRE(ray.element == ID_NONE);
  }

  SECTION("Setting element ID")
  {
    ray.element = 42;
    REQUIRE(ray.element == 42);
  }
}

TEST_CASE("Test RTCDualHit properties")
{
  RTCDualHit hit;

  SECTION("Default constructor initializes values correctly")
  {
    REQUIRE(hit.geomID == RTC_INVALID_GEOMETRY_ID);
    REQUIRE(hit.primID == RTC_INVALID_GEOMETRY_ID);
    REQUIRE(hit.Ng_x == 0.0f);
    REQUIRE(hit.Ng_y == 0.0f);
    REQUIRE(hit.Ng_z == 0.0f);
    REQUIRE(hit.primitive_ref == nullptr);
    REQUIRE(hit.surface == ID_NONE);
  }

  SECTION("Setting hit properties")
  {
    hit.geomID = 1;
    hit.primID = 2;
    hit.surface = 42;

    Direction n(1.0, 2.0, 3.0);
    hit.set_Ng(n);

    REQUIRE(hit.geomID == 1);
    REQUIRE(hit.primID == 2);
    REQUIRE(hit.Ng_x == 1.0f);
    REQUIRE(hit.Ng_y == 2.0f);
    REQUIRE(hit.Ng_z == 3.0f);
    REQUIRE(hit.dNg_x == 1.0);
    REQUIRE(hit.dNg_y == 2.0);
    REQUIRE(hit.dNg_z == 3.0);
    REQUIRE(hit.surface == 42);
  }
}

TEST_CASE("Test RTCDualRayHit properties")
{
  RTCDualRayHit rayhit;

  SECTION("Default constructor initializes ray and hit correctly")
  {
    REQUIRE(rayhit.ray.tnear == 0.0);
    REQUIRE(rayhit.ray.tfar == INFTYF);
    REQUIRE(rayhit.ray.mask == -1);
    REQUIRE(rayhit.hit.geomID == RTC_INVALID_GEOMETRY_ID);
    REQUIRE(rayhit.hit.primID == RTC_INVALID_GEOMETRY_ID); // Corrected scope mismatch
  }

  SECTION("dot_prod method")
  {
    rayhit.ray.set_dir(Direction(1.0, 0.0, 0.0));
    
    Direction n1(1.0, 0.0, 0.0);
    rayhit.hit.set_Ng(n1);
    REQUIRE(rayhit.dot_prod() == 1.0);

    Direction n2(-1.0, 0.0, 0.0);
    rayhit.hit.set_Ng(n2);
    REQUIRE(rayhit.dot_prod() == -1.0);

    Direction n3(0.0, 1.0, 0.0);
    rayhit.hit.set_Ng(n3);
    REQUIRE(rayhit.dot_prod() == 0.0);
  }
}

TEST_CASE("Test RTCDPointQuery properties")
{
  RTCDPointQuery query;

  SECTION("Default constructor initializes values correctly")
  {
    REQUIRE(query.radius == INFTYF);
    REQUIRE(query.dradius == INFTY);
    REQUIRE(query.time == 0.0);
    REQUIRE(query.primID == RTC_INVALID_GEOMETRY_ID);
    REQUIRE(query.geomID == RTC_INVALID_GEOMETRY_ID);
    REQUIRE(query.primitive_ref == nullptr);
  }

  SECTION("set_radius")
  {
    query.set_radius(10.0);
    REQUIRE(query.radius == 10.0f);
    REQUIRE(query.dradius == 10.0);

    query.set_radius(INFTY);
    REQUIRE(query.radius == INFTYF);
    REQUIRE(query.dradius == INFTY);
  }

  SECTION("set_point with array")
  {
    double point[3] = {1.0, 2.0, 3.0};
    query.set_point(point);
    REQUIRE(query.x == 1.0f);
    REQUIRE(query.y == 2.0f);
    REQUIRE(query.z == 3.0f);
    REQUIRE(query.dbl_x == 1.0);
    REQUIRE(query.dbl_y == 2.0);
    REQUIRE(query.dbl_z == 3.0);
  }

  SECTION("set_point with Position")
  {
    Position point(1.0, 2.0, 3.0);
    query.set_point(point);
    REQUIRE(query.x == 1.0f);
    REQUIRE(query.y == 2.0f);
    REQUIRE(query.z == 3.0f);
    REQUIRE(query.dbl_x == 1.0);
    REQUIRE(query.dbl_y == 2.0);
    REQUIRE(query.dbl_z == 3.0);
  }
}
