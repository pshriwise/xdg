// stl includes
#include <memory>

// testing includes
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

// xdg includes
#include "xdg/error.h"
#include "xdg/overlap.h"

using namespace xdg;

TEST_CASE("Overlap-Check Test 1")
{
  // Create a mesh manager
	std::shared_ptr<XDG> xdg = XDG::create(MeshLibrary::MOAB);
	const auto& mm = xdg->mesh_manager();

	mm->load_file("no_overlap.h5m");
	mm->init();
  xdg->prepare_raytracer();
  OverlapMap overlap_map;
  check_instance_for_overlaps(xdg, overlap_map);

  // Expected 0 overlaps 
  REQUIRE(overlap_map.size() == 0); 
}

TEST_CASE("Overlap-Check Test 2")
{
  // Create a mesh manager
	std::shared_ptr<XDG> xdg = XDG::create(MeshLibrary::MOAB);
	const auto& mm = xdg->mesh_manager();

	mm->load_file("overlap.h5m");
	mm->init();
  xdg->prepare_raytracer();
  OverlapMap overlap_map;
  check_instance_for_overlaps(xdg, overlap_map);

  // Expected 1 overlap
  REQUIRE(overlap_map.size() == 1); 
  std::set<int> expected_overlaps = {1, 2};

  // Expected overlaps between volumes [1,2]
  REQUIRE(expected_overlaps == overlap_map.begin()->first); 
}

TEST_CASE("Overlap-Check Test 3")
{
  // Create a mesh manager
	std::shared_ptr<XDG> xdg = XDG::create(MeshLibrary::MOAB);
	const auto& mm = xdg->mesh_manager();

	mm->load_file("small_overlap.h5m");
	mm->init();
  xdg->prepare_raytracer();
  OverlapMap overlap_map;
  check_instance_for_overlaps(xdg, overlap_map);

  // Expected 1 overlap
  REQUIRE(overlap_map.size() == 1); 
  std::set<int> expected_overlaps = {1, 2};

  // Expected overlaps between volumes [1,2]
  REQUIRE(expected_overlaps == overlap_map.begin()->first); 
}