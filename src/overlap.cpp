#include "overlap.h"


using namespace moab;
using namespace xdg;

void check_location_for_overlap(std::shared_ptr<xdg::RayTracer>& rti,
                                     const std::vector<MeshID>& all_vols, Vertex loc,
                                     Direction dir, OverlapMap& overlap_map) {

  std::set<MeshID> vols_found;
  double bump = 1E-9;

  // move the point slightly off the vertex
  loc += dir * bump;

  for (const auto& vol : all_vols) {
		bool pointInVol = false;
		pointInVol = rti->point_in_volue(vol, loc, dir, nullptr);

    // rval = GQT->point_in_volume(vol, loc.array(), result, dir.array());
    // MB_CHK_SET_ERR(rval, "Failed point in volume for Vol with id "
    //                          << GTT->global_id(vol) << " at location " << loc);
		if (pointinVol) {
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
    pointInVol = rti->point_in_volue(vol, loc, dir, nullptr);

    // MB_CHK_SET_ERR(rval, "Failed point in volume for Vol with id "
    //                          << GTT->global_id(vol) << " at location " << loc);

    if (pointInVol) {
      vols_found.insert(GTT->global_id(vol));
    }
  }

  if (vols_found.size() > 1) {
#pragma omp critical
    overlap_map[vols_found] = loc;
  }
}

void check_instance_for_overlaps(std::shared_ptr<xdg::MeshManager> mm,
                                 std::shared_ptr<xdg::RayTracer> rti,
                                 OverlapMap& overlap_map,
                                int pnts_per_edge=0) {
  
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
      check_location_for_overlap(rti, all_vols, vert, dir, overlap_map);

// #pragma omp critical
//       prog_bar.set_value(100.0 * (double)num_checked++ / (double)num_locations);
//     }
  }
  // if we aren't checking along edges, return
  if (pnts_per_edge == 0) {
  }

// #pragma omp parallel shared(overlap_map, num_checked)
//   {
// // now check along triangle edges
// // (curve edges are likely in here too,
// //  but it isn't hurting anything to check more locations)
// #pragma omp for schedule(auto)
//     for (size_t i = 0; i < all_edges.size(); i++) {
//       EntityHandle edge = all_edges[i];
//       Range edge_verts;
//       rval = MBI->get_connectivity(&edge, 1, edge_verts);
//       MB_CHK_SET_ERR_CONT(rval, "Failed to get triangle vertices");

//       CartVect edge_coords[2];
//       rval = MBI->get_coords(edge_verts, edge_coords[0].array());
//       MB_CHK_SET_ERR_CONT(rval, "Failed to get triangle coordinates");

//       std::vector<CartVect> locations;

//       CartVect edge_vec = edge_coords[1] - edge_coords[0];
//       CartVect& start = edge_coords[0];

//       // create locations along the edge to check
//       for (int j = 1; j <= pnts_per_edge; j++) {
//         double t = (double)j / (double)pnts_per_edge;
//         locations.push_back(start + t * edge_vec);
//       }

//       // check edge locations
//       for (auto& loc : locations) {
//         rval = check_location_for_overlap(GQT, all_vols, loc, dir, overlap_map);
//         MB_CHK_SET_ERR_CONT(rval, "Failed to check point for overlap");
// #pragma omp critical
//         prog_bar.set_value(100.0 * (double)num_checked++ /
//                            (double)num_locations);
//       }
//     }
//   }
//   return MB_SUCCESS;
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

