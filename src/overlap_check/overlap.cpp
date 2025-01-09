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
  auto allSurfs = mm->surfaces();
	std::vector<Vertex> allVerts;
  std::vector<Direction> allEdgeDirections; // all edge directions forwards and backwards
  std::vector<ElementVertices> elementVerts; 
  std::vector<MeshID> allElements;

  // auto VolumesToCheck = std::vector<MeshID>(allVols.begin(), allVols.end() - 1); // volumes excluding implcit complement
  // std::cout << "Number of volumes checked = " << VolumesToCheck.size() << std::endl;

  for (const auto& vol:allVols){  
    for (const auto& surf:mm->get_volume_surfaces(vol)){
      auto allElements = mm->get_surface_elements(surf);
      for (const auto& tri:allElements){
        auto parentVols = mm->get_parent_volumes(surf);
        auto triVert = mm->triangle_vertices(tri);
        elementVerts.push_back(triVert);
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
  std::ofstream rayDirectionsOut("ray_fire_lines.txt");
  std::ofstream rayPathOut("ray-path.txt");
  std::ofstream rayCollisionOut("ray-collision.txt");

  rayCollisionOut << "x, y, z \n";
  rayPathOut << "x, y, z \n";
  rayDirectionsOut << "x, y, z, Vx, Vy, Vz\n";

  std::cout << "Overlapping edges detected at:" << std::endl;

#pragma omp parallel shared(overlap_map, numChecked)
  {
  // now check along triangle edges
  // (curve edges are likely in here too,
  //  but it isn't hurting anything to check more locations)

  #pragma omp for schedule(auto)
    for (const auto& surf:allSurfs)
    {
      auto parentVols = mm->get_parent_volumes(surf);
      std::vector<MeshID> volsToCheck;
      std::copy_if(allVols.begin(), allVols.end(), std::back_inserter(volsToCheck), [&parentVols](MeshID vol)
      {
        return vol != parentVols.first && vol != parentVols.second;
      });
      auto elementsOnSurf = mm->get_surface_elements(surf);
      for (const auto& element:elementsOnSurf) {
        auto tri = mm->triangle_vertices(element);
        auto rayQueries = return_ray_queries(tri, &rayDirectionsOut);
        for (const auto& query:rayQueries)
        {
          Position origin = query.origin;
          Direction direction = query.direction;
          double distanceMax = query.edgeLength;
          check_along_edges(xdg, mm, query, volsToCheck, edgeOverlapLocs, &rayPathOut);
        }
      }
    }

  // Report edge overlaps (could move into a seperate function)
  // Probably want to standardise the outputs between this and the vertex overlaps
  for (auto& loc:edgeOverlapLocs)
  {
    std::cout << loc << std::endl;
    rayCollisionOut << loc.x << ", " << loc.y << ", " << loc.z << std::endl;
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


/* Return rayQueries along element edges. Currently limited to Triangles as it expects a std::array<xdg::vertex, 3> 
   but the rest of the function body could easily work with a container of any size so could readily be generalised 
   to work with quads. */
std::vector<EdgeRayQuery> return_ray_queries(const ElementVertices &element, 
                                             std::ofstream* rayDirectionsOut = nullptr)
{
  std::vector<EdgeRayQuery> rayQueries;
  // Loop through each edge of the element (triangle or quad)
  for (size_t vertex = 0; vertex < element.size(); ++vertex) {
    // Wrap around to the first vertex when at the final
    size_t nextVertex = (vertex + 1) % element.size();
    const auto& v1 = element[vertex];
    const auto& v2 = element[nextVertex];

    // Calculate direction and length of edge
    Direction dir = calculate_direction(v1, v2);
    double distMax = calculate_distance(v1, v2);

    // Add the edge ray query
    rayQueries.push_back({v1, dir, distMax});

    // Optionally write ray directions to the output file
    if (rayDirectionsOut) {
        const Position& origin = rayQueries.back().origin;
        const Direction& direction = rayQueries.back().direction;
        const double& distance = rayQueries.back().edgeLength;

        // Write out origin, direction, and distance for debugging
        *rayDirectionsOut << origin.x << ", " << origin.y << ", " << origin.z << ", "
                          << direction.x << ", " << direction.y << ", " << direction.z << "\n";
    }
  }

  return rayQueries;
}

// Return the normalised direction between two xdg::Position types
Direction calculate_direction(const Position& from, const Position& to) {
  Direction dir = {to.x - from.x, to.y - from.y, to.z - from.z};
  dir.normalize();
  return dir;
}

// Return the distance between two xdg::Position types
double calculate_distance(const Position& from, const Position& to) {
  double dx = to.x - from.x;
  double dy = to.y - from.y;
  double dz = to.z - from.z;
  return std::sqrt(dx * dx + dy * dy + dz * dz);
}

// Loop through all triangle edges firing rays along them against all volumes (except for triangle parent volumes)
void check_along_edges(std::shared_ptr<XDG> xdg, 
                       std::shared_ptr<MeshManager> mm, 
                       const EdgeRayQuery& rayquery, 
                       const std::vector<MeshID>& volsToCheck, 
                       std::vector<Position>& edgeOverlapLocs,
                       std::ofstream* rayPathOut = nullptr) // optional write out ray path to text file
{
  auto origin = rayquery.origin;
  auto direction = rayquery.direction;
  auto distanceMax = rayquery.edgeLength;
  for (const auto& testVol:volsToCheck)
  {
    auto ray = xdg->ray_fire(testVol, origin, direction, distanceMax); 
    double rayDistance = ray.first;
    MeshID surfHit = ray.second;  
    if (surfHit != -1) // if surface hit (Valid MeshID returned)
    {
      auto volHit = mm->get_parent_volumes(surfHit);
      if (rayPathOut)
      {
        double ds = rayDistance/1000;
        for (int i = 0; i < 1000; ++i)
        {
          *rayPathOut << origin.x + direction.x * ds * i << ", "
                   << origin.y + direction.y * ds * i << ", "
                   << origin.z + direction.z * ds * i << std::endl;
        }
      }

      Position collisionPoint = {origin.x + rayDistance*direction.x, origin.y + rayDistance*direction.y, origin.z + rayDistance*direction.z}; 
      // Check if overlap location already added to list from another ray
      if (std::find(edgeOverlapLocs.begin(), edgeOverlapLocs.end(), collisionPoint) == edgeOverlapLocs.end()) {
        edgeOverlapLocs.push_back(collisionPoint);
      }
    }
  }
}

