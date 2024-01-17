#include <memory>

// for testing
#include <catch2/catch_test_macros.hpp>

// xdg includes
#include "xdg/xdg.h"

using namespace xdg;

TEST_CASE("XDG Interface")
{
  std::shared_ptr<AcceleratedDiscretizedGeometry> xdg = std::make_shared<AcceleratedDiscretizedGeometry>();
  REQUIRE(xdg->ray_tracing_interface() != nullptr);
  REQUIRE(xdg->mesh_manager() == nullptr);
}