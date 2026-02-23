// stl includes
#include <algorithm>
#include <cmath>
#include <memory>
#include <numeric>

// testing includes
#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

// xdg includes
#include "xdg/error.h"
#include "xdg/mesh_manager_interface.h"
#include "xdg/moab/mesh_manager.h"
#include "xdg/xdg.h"
#include "util.h"

#include "particle_sim.h"

using namespace xdg;
using namespace xdg::test;

TEST_CASE("Test MOAB Transport on Tris")
{
  std::shared_ptr<XDG> xdg = XDG::create(MeshLibrary::MOAB);
  REQUIRE(xdg->mesh_manager()->mesh_library() == MeshLibrary::MOAB);
  const auto& mesh_manager = xdg->mesh_manager();
  mesh_manager->load_file("cyl-brick.h5m");
  mesh_manager->init();
  mesh_manager->parse_metadata();
  xdg->prepare_raytracer();

  SimulationData sim_data;
  sim_data.xdg_ = xdg;
  sim_data.verbose_particles_ = false;
  sim_data.implicit_complement_is_graveyard_ = true;
  transport_particles(sim_data);
}

TEST_CASE("Test MOAB Transport on Quads")
{
  std::shared_ptr<XDG> xdg = XDG::create(MeshLibrary::MOAB);
  REQUIRE(xdg->mesh_manager()->mesh_library() == MeshLibrary::MOAB);
  const auto& mesh_manager = xdg->mesh_manager();
  mesh_manager->load_file("cyl-brick-quads.h5m");
  mesh_manager->init();
  mesh_manager->parse_metadata();
  xdg->prepare_raytracer();

  SimulationData sim_data;
  sim_data.xdg_ = xdg;
  sim_data.verbose_particles_ = false;
  sim_data.implicit_complement_is_graveyard_ = true;
  transport_particles(sim_data);
}
