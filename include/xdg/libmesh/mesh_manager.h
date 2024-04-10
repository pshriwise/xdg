#ifndef _XDG_LIBMESH_MESH_MANAGER
#define _XDG_LIBMESH_MESH_MANAGER

#include <memory>

#include "xdg/mesh_manager_interface.h"

#include "libmesh/libmesh.h"
namespace xdg {

std::unique_ptr<libMesh::LibMeshInit> libmesh_init {nullptr};

class LibMeshMeshManager : public MeshManager {

public:
  LibMeshMeshManager(void* ptr);

  LibMeshMeshManager();
};

} // namespace xdg

#endif // include guard