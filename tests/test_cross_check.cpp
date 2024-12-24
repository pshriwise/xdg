// stl includes
#include <iostream>
#include <memory>

// testing includes
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

// xdg includes
#include "xdg/error.h"
#include "xdg/mesh_manager_interface.h"
#include "xdg/xdg.h"

#include "particle_sim.h"

using namespace xdg;

TEST_CASE("Test MOAB-libMesh Cross-Check")
{
  std::shared_ptr<XDG> libmesh_xdg {XDG::create(MeshLibrary::LIBMESH)};

  libmesh_xdg->mesh_manager()->load_file("cyl-brick.exo");
  libmesh_xdg->mesh_manager()->init();
  libmesh_xdg->mesh_manager()->parse_metadata();
  libmesh_xdg->prepare_raytracer();

  SimulationData libmesh_sim_data;

  libmesh_sim_data.xdg_ = libmesh_xdg;
  libmesh_sim_data.verbose_particles_ = false;

  transport_particles(libmesh_sim_data);

  std::shared_ptr<XDG> moab_xdg {XDG::create(MeshLibrary::MOAB)};
  moab_xdg->mesh_manager()->load_file("cyl-brick.h5m");
  moab_xdg->mesh_manager()->init();
  moab_xdg->mesh_manager()->parse_metadata();
  moab_xdg->prepare_raytracer();

  SimulationData moab_sim_data;

  moab_sim_data.xdg_ = moab_xdg;
  moab_sim_data.verbose_particles_ = false;

  transport_particles(moab_sim_data);

  // these two problems should be using the same boundary triangles and conditions.
  // as a result we epxect the two different backends to return the same result
  for (const auto& [volume, distance] : libmesh_sim_data.cell_tracks) {
    REQUIRE_THAT(moab_sim_data.cell_tracks[volume], Catch::Matchers::WithinAbs(libmesh_sim_data.cell_tracks[volume], 1e-10));
  }

}