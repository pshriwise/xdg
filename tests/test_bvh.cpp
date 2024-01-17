// for testing
#include <catch2/catch_test_macros.hpp>

// xdg includes
#include "xdg/constants.h"
#include "xdg/mesh_manager_interface.h"
#include "xdg/ray_tracing_interface.h"

#include "mesh_mock.h"

using namespace xdg;

TEST_CASE("Test Mesh BVH")
{
  std::shared_ptr<MeshManager> mm = std::make_shared<MeshMock>();
  mm->init(); // this should do nothing

  REQUIRE(mm->num_volumes() == 1);
  REQUIRE(mm->num_surfaces() == 6);
  REQUIRE(mm->num_volume_elements(1) == 12);

  std::shared_ptr<RayTracer> rti = std::make_shared<RayTracer>();

  std::unordered_map<MeshID, TreeID> volume_to_scene_map;
  for (auto volume: mm->volumes()) {
    volume_to_scene_map[volume]= rti->register_volume(mm, volume);
  }

  REQUIRE(rti->num_registered_volumes() == 1);
}