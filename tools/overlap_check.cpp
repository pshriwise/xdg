#include <memory>
#include <set>

#include "xdg/error.h"
#include "xdg/overlap.h"

#include "argparse/argparse.hpp"

#ifdef _OPENMP
#include <omp.h>
#endif

using namespace xdg;

int main(int argc, char* argv[]) {
  
	// Argument Parsing
  argparse::ArgumentParser args("XDG Overlap Checker Tool", "1.0", argparse::default_arguments::help);
	args.add_argument("filename")
	  .help("Path to the faceted .h5m file to check");	
	args.add_argument("-e","--disable-edge-checking")
	    .default_value(false)
    	.implicit_value(true)
		.help("Disable checking along elements edges");
	args.add_argument("-v","--verbose")
	    .default_value(false)
    	.implicit_value(true)
		.help("Enable more verbose outputs (all overlap locations)");

	try {
		args.parse_args(argc, argv);
	}
	catch(const std::runtime_error& err)
	{
		std::cerr << err.what() << std::endl;
		std::cout << args;
		exit(0);
	}

	bool checkEdges = true;
	bool verboseOuput = false;

	if (args.get<bool>("--disable-edge-checking")) {
		checkEdges = false;
	}	

	if (args.get<bool>("--verbose")) {
		verboseOuput = true;
	}
	
  // Create a mesh manager
  std::shared_ptr<XDG> xdg = XDG::create(MeshLibrary::MOAB);
  const auto& mm = xdg->mesh_manager();
  mm->load_file(args.get<std::string>("filename"));
  mm->init();	
  xdg->prepare_raytracer();

  std::cout << "Running overlap check..." << std::endl;

  // check for overlaps
  OverlapMap overlap_map;
  Direction dir = xdg::rand_dir();
  check_instance_for_overlaps(xdg, overlap_map, checkEdges, verboseOuput);
  
  std::cout << std::endl;
  // if any overlaps are found, report them
  if (overlap_map.size() > 0) {
    report_overlaps(overlap_map);
  } else {
    std::cout << "No overlaps were found." << std::endl;
  }
	
  return 0;
}
