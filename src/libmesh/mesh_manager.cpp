#include "xdg/libmesh/mesh_manager.h"


#include "xdg/error.h"

#include "libmesh/mesh_tools.h"
#include "libmesh/boundary_info.h"

using namespace xdg;

// libMesh mesh manager
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

void
LibMeshMeshManager::load_file(const std::string& filepath) {
  mesh_ = std::make_unique<libMesh::Mesh>(libmesh_init->comm());
  mesh_->read(filepath);
}

void
LibMeshMeshManager::init() {
  mesh_->prepare_for_use();

  // ensure that the mesh is 3-dimensional
  if (mesh_->mesh_dimension() != 3) {
    fatal_error("Mesh must be 3-dimensional");
  }

  auto libmesh_bounding_box = libMesh::MeshTools::create_bounding_box(*mesh());

  std::set<libMesh::subdomain_id_type> subdomain_ids;
  mesh()->subdomain_ids(subdomain_ids);
  for (auto id : subdomain_ids) {
    volumes_.push_back(id);
  }

  auto boundary_info = mesh()->get_boundary_info();
  for (auto entry : boundary_info.get_sideset_name_map()) {
    surfaces_.push_back(entry.first);
  }
}