#ifndef OVERLAP_H
#define OVERLAP_H

#include <map>
#include <memory>
#include <set>
#include <algorithm>

#include "xdg/mesh_manager_interface.h"
#include "xdg/vec3da.h"
#include "xdg/xdg.h"

using namespace xdg;

using OverlapMap = std::map<std::set<int>, Position>;
using ElementVertices = std::array<xdg::Vertex, 3>; // This could be swapped out for something more general later on down the line

struct EdgeRayQuery {
    Position origin; // ray_fire() launch origin
    Direction direction; // ray_fire() launch direction
    double edgeLength; // length of edge used as max distance in ray_fire()
};

// check mesh manager instance for overlaps
void check_instance_for_overlaps(std::shared_ptr<XDG> xdg,
                                 OverlapMap& overlap_map,
                                 bool checkEdges);

void report_overlaps(const OverlapMap& overlap_map);

// Helper Geometry functions
Direction calculate_direction(const Position& from, const Position& to); 
double calculate_distance(const Position& from, const Position& to);


std::vector<EdgeRayQuery> return_ray_queries(const ElementVertices &tri, 
                                                     std::ofstream* rayDirectionsOut);

void check_along_edge(std::shared_ptr<XDG> xdg, 
                       std::shared_ptr<MeshManager> mm, 
                       const EdgeRayQuery& rayquery, 
                       const std::vector<MeshID>& volsToCheck, 
                       std::vector<Position>& edgeOverlapLocs, 
                       std::ofstream* rayPathOut);
#endif