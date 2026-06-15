#include <array>
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

  REQUIRE(mm->num_surfaces() == 6);
  REQUIRE(mm->num_volume_faces(0) == 6);

  const std::array<Direction, 6> expected_normals {
    Direction {0.0, 0.0, 1.0},
    Direction {0.0, 0.0, -1.0},
    Direction {0.0, -1.0, 0.0},
    Direction {1.0, 0.0, 0.0},
    Direction {0.0, 1.0, 0.0},
    Direction {-1.0, 0.0, 0.0},
  };

  for (MeshID surface = 0; surface < 6; ++surface) {
    const auto face = mm->get_surface_faces(surface).at(0);
    REQUIRE(mm->face_vertices(face).size() == 4);
    REQUIRE(mm->get_surface_connectivity(surface).size() == 4);
    REQUIRE(mm->get_surface_face_type(surface) == SurfaceFaceType::QUAD);

    const auto normal = mm->face_normal(face);
    REQUIRE_THAT(normal.x, Catch::Matchers::WithinAbs(expected_normals[surface].x, 1e-12));
    REQUIRE_THAT(normal.y, Catch::Matchers::WithinAbs(expected_normals[surface].y, 1e-12));
    REQUIRE_THAT(normal.z, Catch::Matchers::WithinAbs(expected_normals[surface].z, 1e-12));
  }

  auto bbox = mm->face_bounding_box(mm->get_surface_faces(1).at(0));
  REQUIRE_THAT(bbox.min_x, Catch::Matchers::WithinAbs(0.0, 1e-12));
  REQUIRE_THAT(bbox.min_y, Catch::Matchers::WithinAbs(0.0, 1e-12));
  REQUIRE_THAT(bbox.min_z, Catch::Matchers::WithinAbs(0.0, 1e-12));
  REQUIRE_THAT(bbox.max_x, Catch::Matchers::WithinAbs(1.0, 1e-12));
  REQUIRE_THAT(bbox.max_y, Catch::Matchers::WithinAbs(1.0, 1e-12));
  REQUIRE_THAT(bbox.max_z, Catch::Matchers::WithinAbs(0.0, 1e-12));
}
