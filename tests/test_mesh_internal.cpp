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
  REQUIRE(mm->num_volume_faces(1) == 12);
}

TEST_CASE("Mesh Mock Get Surface Mesh")
{
  std::shared_ptr<MeshManager> mm = std::make_shared<MeshMock>();
  mm->init(); // this should do nothing

  float fpTol = 1e-5;

  size_t surface_index = 0;
  for (const auto surface : mesh_manager->surfaces()) {
    auto surfaceMesh = mesh_manager->get_surface_mesh(surface);
    auto vertices = surfaceMesh.first;
    auto connectivity = surfaceMesh.second;

    // Test connectivity
    REQUIRE(connectivity.size() == 2);
    for (size_t i = 0; i < connectivity.size(); ++i) {
      REQUIRE(connectivity[i] == expected_connectivity[surface_index][i]);
    }

    // Test vertices
    REQUIRE(vertices.size() == 4);
    for (size_t i = 0; i < vertices.size(); ++i) {
      REQUIRE_THAT(vertices[i].x, Catch::Matchers::WithinAbs(expected_vertices[surface_index][i].x, fpTol));
      REQUIRE_THAT(vertices[i].y, Catch::Matchers::WithinAbs(expected_vertices[surface_index][i].y, fpTol));
      REQUIRE_THAT(vertices[i].z, Catch::Matchers::WithinAbs(expected_vertices[surface_index][i].z, fpTol));
    }

    ++surface_index;
  }
}