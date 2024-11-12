#include "xdg/overlap.h"

using namespace xdg;

void check_location_for_overlap(std::shared_ptr<XDG> xdg,
                                     const std::vector<MeshID>& all_vols, Vertex loc,
                                     Direction dir, OverlapMap& overlap_map) {

  std::set<MeshID> vols_found;
  double bump = 1E-9;

  // move the point slightly off the vertex
  loc += dir * bump;

  for (const auto& vol : all_vols) {
		bool pointInVol = false;
		pointInVol = xdg->point_in_volume(vol, loc, &dir, nullptr);

    // rval = GQT->point_in_volume(vol, loc.array(), result, dir.array());
    // MB_CHK_SET_ERR(rval, "Failed point in volume for Vol with id "
    //                          << GTT->global_id(vol) << " at location " << loc);
		if (pointInVol) {
			vols_found.insert(vol);
		}
  }

  if (vols_found.size() > 1) {
#pragma omp critical
    overlap_map[vols_found] = loc;
  }

  // move the point slightly off the vertex
  dir *= -1;
  loc += dir * 2.0 * bump;
  vols_found.clear();

  for (const auto& vol : all_vols) {
		bool pointInVol = false;
    pointInVol = xdg->point_in_volume(vol, loc, &dir, nullptr);

    // MB_CHK_SET_ERR(rval, "Failed point in volume for Vol with id "
    //                          << GTT->global_id(vol) << " at location " << loc);

    if (pointInVol) {
      vols_found.insert(vol);
    }
  }

  if (vols_found.size() > 1) {
#pragma omp critical
    overlap_map[vols_found] = loc;
  }
}

void check_instance_for_overlaps(std::shared_ptr<XDG> xdg,
                                 OverlapMap& overlap_map) {
  auto mm = xdg->mesh_manager();
	auto all_vols = mm->volumes();
	std::vector<Vertex> all_verts;

  for (const auto& surf:mm->surfaces())
  {
		auto elements_on_surf = mm->get_surface_elements(surf);
		for (const auto& tri:elements_on_surf)
		{
			auto triVerts = mm->triangle_vertices(tri);
    	// Push vertices in triangle to end of array
			all_verts.push_back(triVerts[0]); 
    	all_verts.push_back(triVerts[1]); 
    	all_verts.push_back(triVerts[2]); 
		}
	}

  // number of locations we'll be checking
  int num_locations = all_verts.size(); // + pnts_per_edge * all_edges.size();
  int num_checked = 1;

  Direction dir = xdg::rand_dir();

  // ProgressBar prog_bar;

// first check all triangle vertex locations
#pragma omp parallel shared(overlap_map, num_checked)
  {
#pragma omp for schedule(auto)
    for (size_t i = 0; i < all_verts.size(); i++) {
      Vertex vert = all_verts[i];
      check_location_for_overlap(xdg, all_vols, vert, dir, overlap_map);

// #pragma omp critical
//       prog_bar.set_value(100.0 * (double)num_checked++ / (double)num_locations);
//     }

    }
  }
}

void report_overlaps(const OverlapMap& overlap_map) {
  std::cout << "Overlap locations found: " << overlap_map.size() << std::endl;

  for (const auto& entry : overlap_map) {
    std::set<MeshID> overlap_vols = entry.first;
    Position loc = entry.second;

    std::cout << "Overlap Location: " << loc[0] << " " << loc[1] << " "
              << loc[2] << std::endl;
    std::cout << "Overlapping volumes: ";
    for (const auto& i : overlap_vols) {
      std::cout << i << " ";
    }
    std::cout << std::endl;
  }
}

