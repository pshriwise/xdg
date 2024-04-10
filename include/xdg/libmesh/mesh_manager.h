#ifndef _XDG_LIBMESH_MESH_MANAGER
#define _XDG_LIBMESH_MESH_MANAGER

#include "xdg/mesh_manager_interface.h"

namespace xdg {

class LibMeshMeshManager : public MeshManager {

public:
  LibMeshMeshManager(void* ptr);

  LibMeshMeshManager();
};

} // namespace xdg

#endif // include guard