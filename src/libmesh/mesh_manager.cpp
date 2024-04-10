#include "xdg/libmesh/mesh_manager.h"

using namespace xdg;

void initialize_libmesh() {
  int argc = 0;
  const char **argv = nullptr;
  libmesh_init = std::make_unique<libMesh::LibMeshInit>(argc, nullptr);
}

// Constructors
LibMeshMeshManager::LibMeshMeshManager(void* ptr) {

  if (libmesh_init == nullptr) { initialize_libmesh(); }

}

LibMeshMeshManager::LibMeshMeshManager() : MeshManager() {}