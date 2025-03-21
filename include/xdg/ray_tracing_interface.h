#ifndef _XDG_RAY_TRACING_INTERFACE_H
#define _XDG_RAY_TRACING_INTERFACE_H

#include <memory>
#include <vector>
#include <unordered_map>

#include "xdg/constants.h"
#include "xdg/embree_interface.h"
#include "xdg/mesh_manager_interface.h"
#include "xdg/primitive_ref.h"
#include "xdg/geometry_data.h"

namespace xdg
{

class RayTracer {
public:
  // Constructors/Destructors
  virtual ~RayTracer();

  // Methods
  virtual void init() = 0;

  virtual TreeID register_volume(const std::shared_ptr<MeshManager> mesh_manager, MeshID volume) = 0;

  // Query Methods
  virtual bool point_in_volume(TreeID tree,
                       const Position& point,
                       const Direction* direction = nullptr,
                       const std::vector<MeshID>* exclude_primitives = nullptr) const = 0;

  virtual std::pair<double, MeshID> ray_fire(TreeID tree,
                                     const Position& origin,
                                     const Direction& direction,
                                     const double dist_limit = INFTY,
                                     HitOrientation orientation = HitOrientation::EXITING,
                                     std::vector<MeshID>* const exclude_primitives = nullptr) = 0;

  virtual void closest(TreeID tree,
               const Position& origin,
               double& dist,
               MeshID& triangle) = 0;

  virtual void closest(TreeID tree,
               const Position& origin,
               double& dist) = 0;

  virtual bool occluded(TreeID tree,
                const Position& origin,
                const Direction& direction,
                double& dist) const = 0;

  // Generic Accessors
  int num_registered_trees() const { return trees_.size(); };
  const std::vector<MeshID>& trees() const { return trees_; }
  virtual const std::shared_ptr<GeometryUserData>& geometry_data(MeshID surface) const = 0;

// TODO: Think about which variables will be shared between RayTracers independent of which library is used
// Right now I have moved pretty much everything into EmbreeRayTracer whilst this sits as an abstract interface
protected:
  // Common functions across RayTracers
  const double bounding_box_bump(const std::shared_ptr<MeshManager> mesh_manager, MeshID volume_id); // return a bump value based on the size of a bounding box (minimum 1e-3). Should this be a part of mesh_manager?

  TreeID next_tree_id() const; // get next treeid

  // Common member variables across RayTracers

  TreeID global_tree_; //<! TreeID for the global tree
  std::map<MeshID, TreeID> surface_to_tree_map_; //<! Map from mesh surface to embree scene
  std::vector<TreeID> trees_; //<! All trees created by this ray tracer
  // Internal parameters
  double numerical_precision_ {1e-3};
};

} // namespace xdg


#endif // include guard