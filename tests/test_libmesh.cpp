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

  REQUIRE(mesh_manager->num_volumes() == 2);
  REQUIRE(mesh_manager->num_surfaces() == 12);
  mesh_manager.reset();


  // create the implicit complement volume
  // mesh_manager->create_implicit_complement();
  // REQUIRE(mesh_manager->num_volumes() == 3);

  // // parse metadata
  // mesh_manager->parse_metadata();

  // std::map<MeshID, std::string> material_exp_results =
  //   {
  //     {1, "UO2 (2.4%)"},
  //     {2, "Zircaloy"},
  //     {3, "Hot borated water"},
  //     {4, "void"}
  //   };

  // for (auto volume : mesh_manager->volumes()) {
  //   auto prop = mesh_manager->get_volume_property(volume, PropertyType::MATERIAL);
  //   REQUIRE(prop.type == PropertyType::MATERIAL);
  //   REQUIRE(material_exp_results[volume] == prop.value);
  // }

  // std::vector reflecting_surface_ids {2, 3, 14, 15, 17, 18};
  // for (auto surface : reflecting_surface_ids) {
  //   auto prop = mesh_manager->get_surface_property(surface, PropertyType::BOUNDARY_CONDITION);
  //   REQUIRE(prop.value == "reflecting");
  // }
}
