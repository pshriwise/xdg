#ifndef _XDG_RAY_TRACING_INTERFACE_H
#define _XDG_RAY_TRACING_INTERFACE_H

#include <memory>
#include <vector>
#include <unordered_map>

#include "xdg/generic_types.h"
#include "xdg/constants.h"
#include "xdg/embree_interface.h"
#include "xdg/mesh_manager_interface.h"
#include "xdg/primitive_ref.h"
#include "xdg/geometry_data.h"



namespace xdg
{


class RayTracer {
// Constructors
public:
  // RayTracer();
  virtual ~RayTracer();

// Methods
  virtual void init() = 0;

  virtual TreeID create_scene() = 0;

  virtual TreeID register_volume(const std::shared_ptr<MeshManager> mesh_manager, MeshID volume) = 0;

  // Query Methods
  virtual bool point_in_volume(TreeID scene,
                       const Position& point,
                       const Direction* direction = nullptr,
                       const std::vector<MeshID>* exclude_primitives = nullptr) const = 0;

  virtual std::pair<double, MeshID> ray_fire(TreeID scene,
                                     const Position& origin,
                                     const Direction& direction,
                                     const double dist_limit = INFTY,
                                     HitOrientation orientation = HitOrientation::EXITING,
                                     std::vector<MeshID>* const exclude_primitives = nullptr) = 0;

  virtual void closest(TreeID scene,
               const Position& origin,
               double& dist,
               MeshID& triangle) = 0;

  virtual void closest(TreeID scene,
               const Position& origin,
               double& dist) = 0;

  virtual bool occluded(TreeID scene,
                const Position& origin,
                const Direction& direction,
                double& dist) const = 0;

// Generic Accessors
  int num_registered_scenes() const { return scenes_.size(); };

  virtual const std::shared_ptr<GeometryUserData>& geometry_data(MeshID surface) const = 0;


// TODO: Think about which variables will be shared between RayTracers independent of which library is used
// Right now I have moved pretty much everything into EmbreeRayTracer whilst this sits as an abstract interface
protected:
// Common functions across RayTracers
  const double bounding_box_bump(const std::shared_ptr<MeshManager> mesh_manager, MeshID volume_id); // return a bump value based on the size of a bounding box (minimum 1e-3). Should this be a part of mesh_manager?


// Common member variables across RayTracers
  TreeID global_scene_;
  std::map<MeshID, TreeID> surface_to_scene_map_; //<! Map from mesh surface to embree scene
  std::vector<TreeID> scenes_; //<! All scenes created by this ray tracer
  // Internal parameters
  double numerical_precision_ {1e-3};

  private:
};

} // namespace xdg


#endif // include guard