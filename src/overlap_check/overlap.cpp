#include "xdg/overlap.h"
#include "xdg/progressBar.h"
#include <fstream>
#include <map>


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
                                 OverlapMap& overlap_map,
                                 bool checkEdges) {
  auto mm = xdg->mesh_manager();
	auto allVols = mm->volumes();
	std::vector<Vertex> allVerts;
  std::vector<Direction> allEdgeDirections; // all edge directions forwards and backwards
  std::vector<std::pair<std::array<xdg::Vertex, 3UL>, MeshID>> allTris;
  std::vector<MeshID> allElements;

  std::cout << "allVols.size() = " << allVols.size() << std::endl;
  for (const auto& vol:mm->volumes()){  
    for (const auto& surf:mm->get_volume_surfaces(vol)){
      auto allElements = mm->get_surface_elements(surf);
      
      for (const auto& tri:allElements){
        auto triVert = mm->triangle_vertices(tri);
        allTris.push_back(std::make_pair(triVert, vol));
        // Push vertices in triangle to end of array
        allVerts.push_back(triVert[0]); 
        allVerts.push_back(triVert[1]); 
        allVerts.push_back(triVert[2]);
      }
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
 
  // if we aren't checking along edges, return early
  if (!checkEdges) {
    return;
  }

#pragma omp parallel shared(overlap_map, numChecked)
  {
// now check along triangle edges
// (curve edges are likely in here too,
//  but it isn't hurting anything to check more locations)

  std::ofstream lineOut("ray_fire_lines.txt");
  std::ofstream rayOut("ray-path.txt");
  std::ofstream rayOutNoColl("ray-path-no-coll.txt");
  std::ofstream rayCollision("ray-collision.txt");

  rayCollision << "x, y, z \n";
  rayOut << "x, y, z \n";
  rayOutNoColl << "x, y, z \n";
  lineOut << "x, y, z, Vx, Vy, Vz\n"; 

  std::cout << "Number of triangles = " << allTris.size() << std::endl;
  
#pragma omp for schedule(auto)
  for (const auto& tri:allTris) {
    std::array<xdg::Vertex, 3UL> triVerts = tri.first;
    MeshID parentVol = tri.second;
    auto origDirList = return_edges_midpoint_dirs(triVerts, lineOut);
    for (const auto& element:origDirList)
    {
      Position origin = element.first;
      Direction direction = element.second;
      // loop over all the volumes in the problem. Skip parent vol of triangle 

      for (const auto& testVol:allVols)
      {
        if (testVol == parentVol) { continue; } // skip if firing against parent vol

        auto rayQuery = xdg->ray_fire(testVol, origin, direction); 
        double rayDistance = rayQuery.first;
        MeshID surfHit = rayQuery.second;  
        if (surfHit != -1) // if surface hit (Valid MeshID returned)
        {
          auto volHit = mm->get_parent_volumes(surfHit);
          std::cout << "Parent Volume = " << parentVol << " | Volume Hit = " << volHit.first << " | Distance = " << rayDistance << std::endl;
          double ds = rayDistance/1000;
          for (int i=0; i<1000; ++i)
          {
            rayOut << origin.x + direction.x*ds*i << ", " << origin.y + direction.y*ds*i << ", " << origin.z + direction.z*ds*i << std::endl;
          }
          if (volHit.first != parentVol) {
            // Must be overlapped by another volume?
            rayCollision << origin.x + rayDistance*direction.x << ", " << origin.y + rayDistance*direction.y << ", " << origin.z + rayDistance*direction.z << std::endl;
            std::cout << "Overlapped region detected" << std::endl; 
          }
        }
        else 
        {
          std::cout << "No Surface Hit!" << std::endl;
        }
      }
    }
  }
}
  return;

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

std::vector<std::pair<Position, Direction>> return_edges_midpoint_dirs(const std::array<xdg::Vertex, 3> &tri, std::ofstream& lineOut)
{
  auto v1 = tri[0];
  auto v2 = tri[1];
  auto v3 = tri[2];

  // Maybe a better approach can be used instead of this vector of pairs?
  std::vector<std::pair<Position, Direction>> origDirMap;

  // Not entirely sure how I feel about this block, maybe these helper functions could be a part of xdg::Position/vec3da??
  origDirMap.push_back({v1, calculate_direction(v1, v2)}); // v1 to v2
  origDirMap.push_back({v2, calculate_direction(v2, v1)}); // v2 to v1
  origDirMap.push_back({v2, calculate_direction(v2, v3)}); // v2 to v3
  origDirMap.push_back({v3, calculate_direction(v3, v2)}); // v3 to v2
  origDirMap.push_back({v1, calculate_direction(v1, v3)}); // v1 to v3
  origDirMap.push_back({v3, calculate_direction(v3, v1)}); // v3 to v1

  // Write origins and directions using origDirMap
  for (const auto& entry : origDirMap) {
      const Position& origin = entry.first;
      const Direction& direction = entry.second;

      // Write out midpoints and directions for debugging
      lineOut << origin.x << ", " << origin.y << ", " << origin.z << ", "
              << direction.x << ", " << direction.y << ", " << direction.z << "\n";
  }

  return origDirMap;
}

// Return the normalised direction between two xdg::Position types
Direction calculate_direction(const Position& from, const Position& to) {
    Direction dir = {to.x - from.x, to.y - from.y, to.z - from.z};
    dir.normalize();
    return dir;
}