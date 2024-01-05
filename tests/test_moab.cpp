// stl includes
#include <memory>

// testing includes
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

// xdg includes
#include "xdg/error.h"
#include "xdg/mesh_manager_interface.h"
#include "xdg/moab/mesh_manager.h"
#include "xdg/ray_tracing_interface.h"

using namespace xdg;

TEST_CASE("Test MOAB Initialization")
{
  std::unique_ptr<MeshManager> mesh_manager = std::make_unique<MOABMeshManager>();

  mesh_manager->load_file("pwr_pincell.h5m");
  mesh_manager->init();

  REQUIRE(mesh_manager->num_volumes() == 3);
  REQUIRE(mesh_manager->num_surfaces() == 12);

  // create the implicit complement volume
  mesh_manager->create_implicit_complement();
  REQUIRE(mesh_manager->num_volumes() == 4);

  // parse metadata
  mesh_manager->parse_metadata();

  std::map<MeshID, std::string> material_exp_results =
    {
      {1, "UO2 (2.4%)"},
      {2, "Zircaloy"},
      {3, "Hot borated water"},
      {4, "void"}
    };

  for (auto volume : mesh_manager->volumes()) {
    auto prop = mesh_manager->get_volume_property(volume, PropertyType::MATERIAL);
    REQUIRE(prop.type == PropertyType::MATERIAL);
    REQUIRE(material_exp_results[volume] == prop.value);
  }

  std::vector reflecting_surface_ids {2, 3, 14, 15, 17, 18};
  for (auto surface : reflecting_surface_ids) {
    auto prop = mesh_manager->get_surface_property(surface, PropertyType::BOUNDARY_CONDITION);
    REQUIRE(prop.value == "reflecting");
  }
}

TEST_CASE("Test BVH Build")
{
  std::shared_ptr<MeshManager> mesh_manager = std::make_shared<MOABMeshManager>();

  mesh_manager->load_file("cube.h5m");
  mesh_manager->init();

  REQUIRE(mesh_manager->num_volumes() == 1);
  REQUIRE(mesh_manager->num_surfaces() == 6);

  std::unique_ptr<RayTracer> ray_tracing_interface = std::make_unique<RayTracer>();

  for (auto volume : mesh_manager->volumes()) {
    ray_tracing_interface->register_volume(mesh_manager, volume);
  }

  REQUIRE(ray_tracing_interface->num_registered_volumes() == 1);
}

TEST_CASE("Test Ray Fire MOAB")
{
  std::shared_ptr<MeshManager> mesh_manager = std::make_shared<MOABMeshManager>();

  mesh_manager->load_file("cube.h5m");
  mesh_manager->init();
  REQUIRE(mesh_manager->mesh_library() == MeshLibrary::MOAB);

  std::shared_ptr<RayTracer> rti = std::make_shared<RayTracer>();

  rti->register_all_volumes(mesh_manager);

  MeshID volume = mesh_manager->volumes()[0];

  Position origin {0.0, 0.0, 0.0};
  Direction direction {1.0, 0.0, 0.0};
  double intersection_distance {0.0};

  rti->ray_fire(volume, origin, direction, intersection_distance);

  // this cube is 10 cm on a side, so the ray should hit the surface at 5 cm
  REQUIRE_THAT(intersection_distance, Catch::Matchers::WithinAbs(5.0, 1e-6));

  origin = {3.0, 0.0, 0.0};
  rti->ray_fire(volume, origin, direction, intersection_distance);
  REQUIRE_THAT(intersection_distance, Catch::Matchers::WithinAbs(2.0, 1e-6));

  origin = {-10.0, 0.0, 0.0};
  rti->ray_fire(volume, origin, direction, intersection_distance);
  REQUIRE_THAT(intersection_distance, Catch::Matchers::WithinAbs(15.0, 1e-6));
}