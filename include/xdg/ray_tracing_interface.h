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
  virtual ~RayTracer() {};

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
                                     std::vector<MeshID>* const exclude_primitives = nullptr) = 0;

  virtual void closest(TreeID scene,
               const Position& origin,
               double& dist,
               MeshID& triangle);

  virtual void closest(TreeID scene,
               const Position& origin,
               double& dist);

  virtual bool occluded(TreeID scene,
                const Position& origin,
                const Direction& direction,
                double& dist) const ;

// Accessors
  int num_registered_scenes() const { return scenes_.size(); }

  const std::shared_ptr<GeometryUserData>& geometry_data(MeshID surface) const { return user_data_map_.at(surface_to_geometry_map_.at(surface)); }

  // Embree members
  XdgDevice device_;
  std::vector<TreeID> scenes_; //<! All scenes created by this ray tracer
  std::vector<XdgGeometry> geometries_; //<! All geometries created by this ray tracer

  // Mesh-to-Scene maps
  std::map<MeshID, TreeID> surface_to_scene_map_; //<! Map from mesh surface to embree scnee
  std::map<MeshID, XdgGeometry> surface_to_geometry_map_; //<! Map from mesh surface to embree geometry
  TreeID gloabal_scene_;

  // Internal Embree Mappings
  std::unordered_map<XdgGeometry, std::shared_ptr<GeometryUserData>> user_data_map_;

  // Internal parameters
  double numerical_precision_ {1e-3};

  // storage
  std::unordered_map<TreeID, std::vector<PrimitiveRef>> primitive_ref_storage_;
// Data members
private:

  /*
  Right now, all of the generic types used in the ray tracing interface are just aliases
  to Embree types. In the future we will need to have a think about which data will be common
  across RTIs. Thoughts so far:
  - TreeID needs to correspond to the specific ray tracers internal type to reference BVH trees
  - XdgDevice corresponds to the internal type which references the device (what is this?)
  - XdgGeometry corresponds to the internal type referencing the geometry native to that ray tracer
  */

};
} // namespace xdg


#endif // include guard