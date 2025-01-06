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

  auto VolumesToCheck = std::vector<MeshID>(allVols.begin(), allVols.end() - 1); // volumes excluding implcit complement
  std::cout << "Number of volumes checked = " << VolumesToCheck.size() << std::endl;

  for (const auto& vol:VolumesToCheck){  
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
  std::cout << "Number of vertices checked = " << allVerts.size() << std::endl;
  // number of locations we'll be checking
  int numLocations = allVerts.size(); // + pnts_per_edge * all_edges.size();
  int numChecked = 1;

  Direction dir = {0.1, 0.1, 0.1};
  dir = dir.normalize();

  ProgressBar progBar;

  std::cout << "Checking for overlapped regions at element vertices..." << std::endl;
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

  std::cout << "Checking for overlapped regions along element edges..." << std::endl; 
  int edgesChecked = 0;
  int raysMissed = 0;

  std::vector<Position> edgeOverlapLocs;

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

    std::cout << "Overlapping edges detected at:" << std::endl;

  #pragma omp for schedule(auto)
    for (const auto& tri:allTris) {
      auto rayQueries = return_ray_queries(tri, lineOut);
      for (const auto& query:rayQueries)
      {
        Position origin = query.origin;
        Direction direction = query.direction;
        double distanceMax = query.edgeLength;
        for (const auto& testVol:VolumesToCheck)
        {
          edgesChecked++;
          if (testVol == query.parentVol) { continue; } // skip if firing against parent vol
          auto ray = xdg->ray_fire(testVol, origin, direction, distanceMax); 
          double rayDistance = ray.first;
          MeshID surfHit = ray.second;  
          if (surfHit != -1) // if surface hit (Valid MeshID returned)
          {
            auto volHit = mm->get_parent_volumes(surfHit);
            std::cout << "Parent Volume = " << query.parentVol << " | Volume Hit = " << volHit.first << " | Distance = " << rayDistance << std::endl;
            double ds = rayDistance/1000;
            for (int i=0; i<1000; ++i)
            {
              rayOut << origin.x + direction.x*ds*i << ", " << origin.y + direction.y*ds*i << ", " << origin.z + direction.z*ds*i << std::endl;
            }

            Position collisionPoint = {origin.x + rayDistance*direction.x, origin.y + rayDistance*direction.y, origin.z + rayDistance*direction.z}; 
            // Check if overlap location already added
            if (std::find(edgeOverlapLocs.begin(), edgeOverlapLocs.end(), collisionPoint) == edgeOverlapLocs.end()) {
              edgeOverlapLocs.push_back(collisionPoint);
              rayCollision << collisionPoint.x << ", " << collisionPoint.y << ", " << collisionPoint.z << std::endl;
            }
          else 
          {
            raysMissed++;
          }
        }
      }
    }
  }
  // std::cout << "Number of element edges checked = " << edgesChecked << std::endl;
  // std::cout << "Number of Rays which miss any surfaces = " << raysMissed << std::endl;
  for (auto& loc:edgeOverlapLocs)
  {
    std::cout << loc << std::endl;
  }
  return;
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

std::vector<TriangleEdgeRayQuery> return_ray_queries(const TriVolPairs &tri, std::ofstream& lineOut)
{
  // Recover triangle vertices and parent volume
  auto v1 = tri.first[0];
  auto v2 = tri.first[1];
  auto v3 = tri.first[2];
  auto parentVolume = tri.second;

  // Maybe a better approach can be used instead of this struct?
  std::vector<TriangleEdgeRayQuery> rayQuery;

  // Not entirely sure how I feel about this block, maybe these helper functions could be a part of xdg::Position/vec3da??
  rayQuery.push_back({parentVolume, v1, calculate_direction(v1, v2), calculate_distance(v1, v2)}); // v1 to v2
  rayQuery.push_back({parentVolume, v2, calculate_direction(v2, v1), calculate_distance(v2, v1)}); // v2 to v1
  rayQuery.push_back({parentVolume, v2, calculate_direction(v2, v3), calculate_distance(v2, v3)}); // v2 to v3
  rayQuery.push_back({parentVolume, v3, calculate_direction(v3, v2), calculate_distance(v3, v2)}); // v3 to v2
  rayQuery.push_back({parentVolume, v1, calculate_direction(v1, v3), calculate_distance(v1, v3)}); // v1 to v3
  rayQuery.push_back({parentVolume, v3, calculate_direction(v3, v1), calculate_distance(v3, v1)}); // v3 to v1

  // Write origins and directions using triple
  // for (const auto& entry : triple) {
  //     const Position& origin = entry.origin;
  //     const Direction& direction = entry.direction;
  //     const double& distance = entry.distanceLimit;

  //     // Write out midpoints and directions for debugging
  //     lineOut << origin.x << ", " << origin.y << ", " << origin.z << ", "
  //             << direction.x << ", " << direction.y << ", " << direction.z << "\n";
  // }

  return rayQuery;
}

// Return the normalised direction between two xdg::Position types
Direction calculate_direction(const Position& from, const Position& to) {
  Direction dir = {to.x - from.x, to.y - from.y, to.z - from.z};
  dir.normalize();
  return dir;
}

double calculate_distance(const Position& from, const Position& to) {
  double dx = to.x - from.x;
  double dy = to.y - from.y;
  double dz = to.z - from.z;
  return std::sqrt(dx * dx + dy * dy + dz * dz);
}