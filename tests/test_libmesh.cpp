// stl includes
#include <iostream>
#include <memory>

// testing includes
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

// xdg includes
#include "xdg/error.h"
#include "xdg/mesh_manager_interface.h"
#include "xdg/libmesh/mesh_manager.h"
#include "xdg/xdg.h"

using namespace xdg;

TEST_CASE("Test libMesh Initialization")
{
  std::unique_ptr<MeshManager> mesh_manager  {std::make_unique<LibMeshMeshManager>()};

  mesh_manager->load_file("cyl-block.exo");

  mesh_manager->init();
  // mesh_manager->create_volume();

  REQUIRE(mesh_manager->num_volumes() == 2);

  // This correct based on geometry topology (provided sidesets)
  REQUIRE(mesh_manager->num_surfaces() == 13);
  // This is correct based on mesh topology
  // REQUIRE(mesh_manager->num_surfaces() == 3);

  // create the implicit complement volume
  mesh_manager->create_implicit_complement();
  REQUIRE(mesh_manager->num_volumes() == 3);

  // parse metadata
  mesh_manager->parse_metadata();

  std::map<MeshID, std::string> material_exp_results =
    {
      {1, "mat:steel"},
      {2, "mat:iron"},
      {3, "void"}
    };
    //   {1, "UO2 (2.4%)"},
    //   {2, "Zircaloy"},
    //   {3, "Hot borated water"},
    //   {4, "void"}
    // };

  for (auto volume : mesh_manager->volumes()) {
    auto prop = mesh_manager->get_volume_property(volume, PropertyType::MATERIAL);
    REQUIRE(prop.type == PropertyType::MATERIAL);
    REQUIRE(material_exp_results[volume] == prop.value);
  }

  std::vector reflecting_surface_ids {1};
  for (auto surface : reflecting_surface_ids) {
    auto prop = mesh_manager->get_surface_property(surface, PropertyType::BOUNDARY_CONDITION);
    REQUIRE(prop.value == "boundary:reflective");
  }
}


TEST_CASE("Test BVH Build LibMesh")
{
  std::shared_ptr<MeshManager> mesh_manager = std::make_shared<LibMeshMeshManager>();

  mesh_manager->load_file("cyl-block.exo");
  mesh_manager->init();

  REQUIRE(mesh_manager->num_volumes() == 2);
  REQUIRE(mesh_manager->num_surfaces() == 13);

  std::unique_ptr<RayTracer> ray_tracing_interface = std::make_unique<RayTracer>();

  for (auto volume : mesh_manager->volumes()) {
    ray_tracing_interface->register_volume(mesh_manager, volume);
  }

  REQUIRE(ray_tracing_interface->num_registered_scenes() == 2);
}

// TEST_CASE("Test Ray Fire LibMesh")
// {
//   std::shared_ptr<XDG> xdg = XDG::create(MeshLibrary::LIBMESH);
//   xdg->mesh_manager()->mesh_library();
//   REQUIRE(xdg->mesh_manager()->mesh_library() == MeshLibrary::LIBMESH);
//   const auto& mesh_manager = xdg->mesh_manager();
//   mesh_manager->load_file("cyl-block.exo");
//   mesh_manager->init();
//   xdg->prepare_raytracer();

//   MeshID volume = 2;

//   Position origin {0.0, 0.0, 10.0};
//   Direction direction {0.0, 0.0, 1.0};
//   std::pair<double, MeshID> intersection;

//   intersection = xdg->ray_fire(volume, origin, direction);

//   // this cube is 10 cm on a side, so the ray should hit the surface at 5 cm
//   REQUIRE_THAT(intersection.first, Catch::Matchers::WithinAbs(5.0, 1e-6));

//   origin = {0.0, 0.0, 10.0};
//   REQUIRE(xdg->point_in_volume(volume, origin));
// }