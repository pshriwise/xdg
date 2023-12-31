#ifndef _XDG_RAY_TRACING_INTERFACE_H
#define _XDG_RAY_TRACING_INTERFACE_H

#include "xdg/constants.h"
#include "xdg/embree_interface.h"

namespace xdg
{

class RayTracingInterface {
// Constructors
public:
  RayTracingInterface();

// Methods


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