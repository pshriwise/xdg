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
               MeshID& triangle) = 0;

  virtual void closest(TreeID scene,
               const Position& origin,
               double& dist) = 0;

  virtual bool occluded(TreeID scene,
                const Position& origin,
                const Direction& direction,
                double& dist) const = 0;

// Accessors
  virtual int num_registered_scenes() const = 0;

  virtual const std::shared_ptr<GeometryUserData>& geometry_data(MeshID surface) const = 0;


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