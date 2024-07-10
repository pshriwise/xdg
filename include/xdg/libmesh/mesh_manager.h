#ifndef _XDG_LIBMESH_MESH_MANAGER
#define _XDG_LIBMESH_MESH_MANAGER

#include <memory>

#include "xdg/mesh_manager_interface.h"
#include "libmesh/libmesh.h"
#include "libmesh/mesh.h"
namespace xdg {

std::unique_ptr<libMesh::LibMeshInit> libmesh_init {nullptr};

class LibMeshMeshManager : public MeshManager {

public:
  LibMeshMeshManager(void* ptr);

  LibMeshMeshManager();

  void load_file(const std::string& filepath) override;

  void init() override;

  // Accessors
  const libMesh::Mesh* mesh() const { return mesh_.get(); }

  private:
    std::unique_ptr<libMesh::Mesh> mesh_;

};

} // namespace xdg

#endif // include guard