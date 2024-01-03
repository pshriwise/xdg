#ifndef _XDG_TRIANGLE_REF_H
#define _XDG_TRIANGLE_REF_H

#include "xdg/constants.h"
#include "xdg/embree_interface.h"
#include "xdg/mesh_manager_interface.h"

namespace xdg {

struct TriangleRef; // Forward declaration

// TODO: could be a more generic primitive ref?
struct TriangleRef {
  MeshID triangle_id {ID_NONE};
  MeshID surface_id {ID_NONE};
  Sense sense {Sense::UNSET};
};

void TriangleBoundsFunc(RTCBoundsFunctionArguments* args);

} // namespace xdg

#endif // include guard