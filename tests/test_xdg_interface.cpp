#include <memory>
#include <catch2/catch_test_macros.hpp>

// xdg includes
#include "xdg/xdg.h"
#include "xdg/embree/ray_tracer.h"
#include "xdg/moab/mesh_manager.h"
#include "xdg/libmesh/mesh_manager.h"

using namespace xdg;

TEST_CASE("XDG Interface") {
  std::shared_ptr<XDG> xdg = std::make_shared<XDG>();
  REQUIRE(xdg->ray_tracing_interface() == nullptr);
  REQUIRE(xdg->mesh_manager() == nullptr);
}

TEST_CASE("XDG Factory Creation") {
  // Create xdg instances using the factory method
  std::shared_ptr<XDG> default_xdg = XDG::create();
  std::shared_ptr<XDG> moab_embree_xdg = XDG::create(MeshLibrary::MOAB, RTLibrary::EMBREE);

  // Check that the factory method creates pointers to ray_tracing_interface and mesh_manager
  REQUIRE(default_xdg->ray_tracing_interface() != nullptr);
  REQUIRE(default_xdg->mesh_manager() != nullptr);
  REQUIRE(moab_embree_xdg->ray_tracing_interface() != nullptr);
  REQUIRE(moab_embree_xdg->mesh_manager() != nullptr);


  // Check that the factory method creates interface pointers of the right types
  REQUIRE(std::dynamic_pointer_cast<EmbreeRayTracer>(default_xdg->ray_tracing_interface()) != nullptr);
  REQUIRE(std::dynamic_pointer_cast<MOABMeshManager>(default_xdg->mesh_manager()) != nullptr);
  REQUIRE(std::dynamic_pointer_cast<EmbreeRayTracer>(moab_embree_xdg->ray_tracing_interface()) != nullptr);
  REQUIRE(std::dynamic_pointer_cast<MOABMeshManager>(moab_embree_xdg->mesh_manager()) != nullptr);

}

TEST_CASE("XDG Constructor") {
  // Create meshmanager instances
  std::shared_ptr<MeshManager> moab_mm = std::make_shared<MOABMeshManager>();

  // Create xdg instances
  std::shared_ptr<XDG> moab_embree_xdg = std::make_shared<XDG>(moab_mm, RTLibrary::EMBREE); 

  // Check that the constructor creates interface pointers of the right types
  REQUIRE(std::dynamic_pointer_cast<EmbreeRayTracer>(moab_embree_xdg->ray_tracing_interface()) != nullptr);
  REQUIRE(std::dynamic_pointer_cast<MOABMeshManager>(moab_embree_xdg->mesh_manager()) != nullptr);
}
