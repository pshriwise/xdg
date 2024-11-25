#include "xdg/overlap.h"
#include "xdg/progressBar.h"

using namespace xdg;

void check_location_for_overlap(std::shared_ptr<XDG> xdg,
                                     const std::vector<MeshID>& allVols, Vertex loc,
                                     Direction dir, OverlapMap& overlap_map) {

  std::set<MeshID> vols_found;
  double bump = 1E-9;

  // move the point slightly off the vertex
  loc += dir * bump;

  for (const auto& vol : allVols) {
		bool pointInVol = false;
		pointInVol = xdg->point_in_volume(vol, loc, &dir, nullptr);

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

  for (const auto& vol : allVols) {
		bool pointInVol = false;
    pointInVol = xdg->point_in_volume(vol, loc, &dir, nullptr);

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
	auto allVols = mm->volumes();
	std::vector<Vertex> allVerts;
  std::vector<Direction> allEdgeDirections; // all edge directions forwards and backwards
  std::vector<std::array<xdg::Vertex, 3UL>> allTris;

  for (const auto& surf:mm->surfaces())
  {
		auto elements_on_surf = mm->get_surface_elements(surf);
		for (const auto& tri:elements_on_surf)
		{
			auto triVert = mm->triangle_vertices(tri);
    	allTris.push_back(triVert);
      // Push vertices in triangle to end of array
			allVerts.push_back(triVert[0]); 
    	allVerts.push_back(triVert[1]); 
    	allVerts.push_back(triVert[2]);
		}
	}



  // number of locations we'll be checking
  int numLocations = allVerts.size(); // + pnts_per_edge * all_edges.size();
  int numChecked = 1;

  Direction dir = xdg::rand_dir();

  ProgressBar progBar;

// first check all triangle vertex locations
#pragma omp parallel shared(overlap_map, numChecked)
  {
#pragma omp for schedule(auto)
    for (size_t i = 0; i < allVerts.size(); i++) {
      Vertex vert = allVerts[i];
      check_location_for_overlap(xdg, allVols, vert, dir, overlap_map);

#pragma omp critical
      progBar.set_value(100.0 * (double)numChecked++ / (double)numLocations);
    }
  }

  bool checkEdges = true;

  // if we aren't checking along edges, return
  if (!checkEdges) {
    return;
  }

#pragma omp parallel shared(overlap_map, numChecked)
  {
// now check along triangle edges
// (curve edges are likely in here too,
//  but it isn't hurting anything to check more locations)
#pragma omp for schedule(auto)
      for (const auto& tri:allTris) {
      //tri[0], tri[1], tri[2] --> recovers the three vertices of the triangle   
        Direction dir1 = direction_between_verts(tri[0], tri[1]);
        Direction dir2 = direction_between_verts(tri[1], tri[2]);
        Direction dir3 = direction_between_verts(tri[2], tri[0]);

        

       }
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
//         rval = check_location_for_overlap(GQT, allVols, loc, dir, overlap_map);
//         MB_CHK_SET_ERR_CONT(rval, "Failed to check point for overlap");
// #pragma omp critical
//         progBar.set_value(100.0 * (double)numChecked++ /
//                            (double)numLocations);
//       }
//     }
//   }
  }
  return
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

MeshID ray_fire_along_tri_edges(std::shared_ptr<XDG> xdg, const xdg::Vertex& vert1, const xdg::Vertex& vert2)
{
  Direction dir = {vert1.x-vert2.x, vert1.y-vert2.y, vert1.z-vert2.z}; 
  Position origin = {(vert1.x+vert2.x)/2, (vert1.y+vert2.y)/2, (vert1.z+vert1.z)/2};
  dir.normalize();

  MeshID launchVolume;
  xdg->find_volume(origin, );

  MeshID VolumeHit;
  return VolumeHit;
}
