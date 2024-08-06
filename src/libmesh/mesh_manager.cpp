#include "xdg/libmesh/mesh_manager.h"


#include "xdg/error.h"

#include "libmesh/elem.h"
#include "libmesh/mesh_base.h"
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

void
LibMeshMeshManager::parse_metadata() {
  // surface metadata
  auto boundary_info = mesh()->get_boundary_info();
  auto sideset_name_map = boundary_info.get_sideset_name_map();
  for (auto surface : surfaces_) {
    if (sideset_name_map.find(surface) != sideset_name_map.end()) {
      surface_metadata_[{surface, PropertyType::BOUNDARY_CONDITION}] = {PropertyType::BOUNDARY_CONDITION, sideset_name_map[surface]};
    }
  }

  // volume metadata
  for (auto volume : volumes_) {
    std::string subdomain_name = mesh()->subdomain_name(volume);
    volume_metadata_[{volume, PropertyType::MATERIAL}] = {PropertyType::MATERIAL, subdomain_name};
  }
}

std::vector<MeshID>
LibMeshMeshManager::get_volume_elements(MeshID volume) const {
  std::vector<MeshID> elements;
  libMesh::MeshBase::const_element_iterator it = mesh()->active_subdomain_elements_begin(volume);
  libMesh::MeshBase::const_element_iterator it_end = mesh()->active_subdomain_elements_end(volume);
  for (; it != it_end; ++it) {
    elements.push_back((*it)->id());
  }
  return elements;
}

std::vector<MeshID>
LibMeshMeshManager::get_surface_elements(MeshID surface) const {
  std::vector<MeshID> faces;
  for (auto& [elem, side_info] : mesh()->get_boundary_info().get_sideset_map()) {
    if (side_info.second == surface) {
      faces.push_back(elem->side_ptr(side_info.first)->id());
    }
  }
  return faces;
}

std::vector<Vertex>
LibMeshMeshManager::element_vertices(MeshID element) const {
  std::vector<Vertex> vertices;
  auto elem = mesh()->elem_ptr(element);
  for (unsigned int i = 0; i < elem->n_nodes(); ++i) {
    auto node = elem->node_ref(i);
    vertices.push_back({node(0), node(1), node(2)});
  }
  return vertices;
}

std::array<Vertex, 3>
LibMeshMeshManager::triangle_vertices(MeshID element) const {
  auto elem = mesh()->elem_ptr(element);
  std::array<Vertex, 3> vertices;
  for (unsigned int i = 0; i < 3; ++i) {
    auto node = elem->node_ref(i);
    vertices[i] = {node(0), node(1), node(2)};
  }
  return vertices;
}

std::vector<MeshID>
LibMeshMeshManager::get_volume_surfaces(MeshID volume) const {
  std::vector<MeshID> surfaces;
  for (auto& [elem, side_info] : mesh()->get_boundary_info().get_sideset_map()) {
    if (side_info.first == volume) {
      surfaces.push_back(side_info.second);
    }
  }
  return surfaces;
}

std::pair<MeshID, MeshID>
LibMeshMeshManager::surface_senses(MeshID surface) const {
  return {ID_NONE, ID_NONE};
}

Sense
LibMeshMeshManager::surface_sense(MeshID surface, MeshID volume) const {
  return Sense::FORWARD;
}
