#ifndef _XDG_RAY_TRACING_INTERFACE_H
#define _XDG_RAY_TRACING_INTERFACE_H

#include <vector>

#include "xdg/constants.h"
#include "xdg/embree_interface.h"
#include "xdg/mesh_manager_interface.h"
#include "xdg/triangle_ref.h"

namespace xdg
{

class RayTracingInterface {
// Constructors
public:
  RayTracingInterface();
  ~RayTracingInterface();
// Methods
  void init();

  void register_volume(const MeshID mesh_id, const std::vector<TriangleRef>& triangles);

// Data members
private:
  // Embree members
  RTCDevice device_;

  // Mesh-to-Scene maps
  std::map<MeshID, RTCScene> volume_map_;
  std::map<MeshID, RTCScene> surface_map_;
  RTCScene gloabal_scene_;

  // Internal parameters
  double numerical_precision_ {1e-3};
};

} // namespace xdg


#endif // include guard