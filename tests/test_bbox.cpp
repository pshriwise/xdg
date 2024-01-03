

// testing includes
#include <catch2/catch_test_macros.hpp>

// xdg includes
#include "xdg/bbox.h"
#include "xdg/vec3da.h"

using namespace xdg;

TEST_CASE("Test BoundingBox")
{

  BoundingBox a = {0.0, 0.0, 0.0, 1.0, 1.0, 1.0};

  BoundingBox b = {-1.0, -1.0, -1.0, 0.0, 0.0, 0.0};

  BoundingBox expanded_box = {-1.0, -1.0, -1.0, 1.0, 1.0, 1.0};

  BoundingBox c = a;
  REQUIRE(c == a);
  c.update(b);
  REQUIRE(c == expanded_box);
  c = b;
  REQUIRE(c == b);
  c.update(a);
  REQUIRE(c == expanded_box);

  Position p1 {-2.0, -3.0, -4.0};
  c.update(p1);
  REQUIRE(c.min_x == -2.0);
  REQUIRE(c.min_y == -3.0);
  REQUIRE(c.min_z == -4.0);

  Position p2 {5.0, 6.0, 7.0};
  c.update(p2);
  REQUIRE(c.max_x == 5.0);
  REQUIRE(c.max_y == 6.0);
  REQUIRE(c.max_z == 7.0);
}