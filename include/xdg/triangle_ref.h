#ifndef _XDG_TRIANGLE_REF_H
#define _XDG_TRIANGLE_REF_H

#include "xdg/embree_interface.h"
#include "xdg/mesh_manager_interface.h"

namespace xdg {

struct TriangleRef; // Forward declaration

struct GeometryUserData {
  MeshManager* mesh_manager; //! Pointer to the mesh manager for this geometry
  TriangleRef* tri_ref; //! Pointer to the triangles in the geometry
  double box_bump; //! Bump distance for the bounding boxes in this geometry
};

// TODO: could be a more generic primitive ref?
struct TriangleRef {
  uint32_t mesh_id;
  uint32_t triangle_id;
  uint32_t sense;
};

void TriangleBoundsFunc(const struct RTCBoundsFunctionArguments* args);

} // namespace xdg

#endif // include guard