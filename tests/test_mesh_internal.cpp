#include <memory>

// for testing
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "mesh_mocks.h"

#include "xdg/ray_tracing_interface.h"

using namespace xdg;

TEST_CASE("Test Mesh Mock")
{
  std::shared_ptr<MeshManager> mm = std::make_shared<MockedTriTetMesh>();
  mm->init(); // this should do nothing

  REQUIRE(mm->num_volumes() == 1);
  REQUIRE(mm->num_surfaces() == 6);
  REQUIRE(mm->num_volume_faces(1) == 12);

  REQUIRE(mm->num_volume_elements() == 12);
  REQUIRE(mm->num_volume_elements(1) == 12);
}

TEST_CASE("MockedQuadHexMesh Face Representation")
{
  std::shared_ptr<MeshManager> mm = std::make_shared<MockedQuadHexMesh>();
  mm->init();

  auto quad_face = mm->get_surface_faces(0).at(0);
  auto tri_face = mm->get_surface_faces(1).at(0);

  REQUIRE(mm->face_vertices(quad_face).size() == 4);
  REQUIRE(mm->face_vertices(tri_face).size() == 3);

  auto quad_connectivity = mm->get_surface_connectivity(0);
  REQUIRE(quad_connectivity.size() == 4);
  REQUIRE(quad_connectivity[0] == 0);
  REQUIRE(quad_connectivity[1] == 1);
  REQUIRE(quad_connectivity[2] == 2);
  REQUIRE(quad_connectivity[3] == 3);

  Direction normal = mm->face_normal(quad_face);
  REQUIRE_THAT(normal.x, Catch::Matchers::WithinAbs(0.0, 1e-12));
  REQUIRE_THAT(normal.y, Catch::Matchers::WithinAbs(0.0, 1e-12));
  REQUIRE_THAT(normal.z, Catch::Matchers::WithinAbs(1.0, 1e-12));

  auto bbox = mm->face_bounding_box(quad_face);
  REQUIRE_THAT(bbox.min_x, Catch::Matchers::WithinAbs(0.0, 1e-12));
  REQUIRE_THAT(bbox.min_y, Catch::Matchers::WithinAbs(0.0, 1e-12));
  REQUIRE_THAT(bbox.min_z, Catch::Matchers::WithinAbs(0.0, 1e-12));
  REQUIRE_THAT(bbox.max_x, Catch::Matchers::WithinAbs(1.0, 1e-12));
  REQUIRE_THAT(bbox.max_y, Catch::Matchers::WithinAbs(1.0, 1e-12));
  REQUIRE_THAT(bbox.max_z, Catch::Matchers::WithinAbs(0.0, 1e-12));

  REQUIRE(mm->get_surface_face_type(0) == SurfaceFaceType::QUAD);
  REQUIRE(mm->get_surface_face_type(1) == SurfaceFaceType::TRI);
}

