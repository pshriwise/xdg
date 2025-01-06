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
using TriVolPairs = std::pair<std::array<xdg::Vertex, 3>, xdg::MeshID>;

struct TriangleEdgeRayQuery {
    MeshID parentVol; // parent volume of triangle
    Position origin; // ray_fire() launch origin
    Direction direction; // ray_fire() launch direction
    double edgeLength; // length of edge used as max distance in ray_fire()
};

// check mesh manager instance for overlaps
void check_instance_for_overlaps(std::shared_ptr<XDG> xdg,
                                 OverlapMap& overlap_map,
                                 bool checkEdges);

std::vector<TriangleEdgeRayQuery> return_ray_queries(const TriVolPairs &tri, std::ofstream& lineOut);
void report_overlaps(const OverlapMap& overlap_map);
Direction calculate_direction(const Position& from, const Position& to); 
double calculate_distance(const Position& from, const Position& to);

#endif