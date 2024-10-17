#include "xdg/libmesh/mesh_manager.h"

#include "xdg/error.h"

#include "libmesh/boundary_info.h"
#include "libmesh/elem.h"
#include "libmesh/mesh_base.h"
#include "libmesh/mesh_tools.h"

namespace xdg {

// Constructors
LibMeshMeshManager::LibMeshMeshManager(void *ptr) {

  if (libmesh_init == nullptr) {
    initialize_libmesh();
  }
}

LibMeshMeshManager::LibMeshMeshManager() : MeshManager() {
  if (libmesh_init == nullptr) {
    initialize_libmesh();
  }
}

void LibMeshMeshManager::load_file(const std::string &filepath) {
  mesh_ = std::make_unique<libMesh::Mesh>(libmesh_init->comm(), 3);
  mesh_->read(filepath);
}

LibMeshMeshManager::~LibMeshMeshManager() {
  mesh_->clear();
  libmesh_init.reset();
}

// libMesh mesh manager
void LibMeshMeshManager::initialize_libmesh() {
  // libmesh requires the program name, so at least one argument is needed
  int argc = 1;
  const std::string argv{"XDG"};
  const char *argv_cstr = argv.c_str();
  libmesh_init =
      std::move(std::make_unique<libMesh::LibMeshInit>(argc, &argv_cstr, 0));
}

void LibMeshMeshManager::init() {
  // mesh_->prepare_for_use();

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

  std::set<MeshID> boundary_ids;

  auto boundary_info = mesh()->get_boundary_info();
  for (auto entry : boundary_info.get_sideset_name_map()) {
    boundary_ids.insert(entry.first);
  }

  MeshID next_boundary_id =
      *std::max_element(boundary_ids.begin(), boundary_ids.end()) + 1;

  // invert the boundary info sideset map
  for (auto entry : boundary_info.get_sideset_map()) {
    sideset_element_map_[entry.second.second].push_back(
        {entry.first, entry.second.first});
  }

  discover_surface_elements();

  mesh()->print_info(std::cout, 2);

  surfaces_ = std::vector<MeshID>(boundary_ids.begin(), boundary_ids.end());
}

MeshID LibMeshMeshManager::create_volume() {
  std::unique_ptr<libMesh::Mesh> submesh_ =
      std::make_unique<libMesh::Mesh>(mesh_->comm(), 3);
  // start by makind a copy of the current mesh
  // for (auto elem : mesh()->element_ptr_range()) {
  //   std::cout << elem->subdomain_id() << std::endl;
  // }
  return -1;
  //  throw std::runtime_error("Not implemented");
}

void LibMeshMeshManager::parse_metadata() {
  // surface metadata
  auto boundary_info = mesh()->get_boundary_info();
  auto sideset_name_map = boundary_info.get_sideset_name_map();
  for (auto surface : surfaces_) {
    if (sideset_name_map.find(surface) != sideset_name_map.end()) {
      surface_metadata_[{surface, PropertyType::BOUNDARY_CONDITION}] = {
          PropertyType::BOUNDARY_CONDITION, sideset_name_map[surface]};
    }
  }

  // volume metadata
  for (auto volume : volumes_) {
    std::string subdomain_name = mesh()->subdomain_name(volume);
    volume_metadata_[{volume, PropertyType::MATERIAL}] = {
        PropertyType::MATERIAL, subdomain_name};
  }
}

void LibMeshMeshManager::discover_surface_elements() {
  for (const auto *elem : mesh()->active_local_element_ptr_range()) {
    auto subdomain_id = elem->subdomain_id();
    for (int i = 0; i < elem->n_sides(); i++) {
      auto neighbor = elem->neighbor_ptr(i);
      MeshID neighbor_id = ID_NONE;
      if (neighbor)
        neighbor_id = neighbor->subdomain_id();
      // if these IDs are different, then this is a surface element
      if (neighbor_id != subdomain_id) {
        subdomain_interface_map_[{subdomain_id, neighbor_id}].push_back(
            {elem, i});
      }
    }
  }

  MeshID surface_id = 1000;

  std::set<std::pair<MeshID, MeshID>> visited_interfaces;

  for (auto &[pair, elements] : subdomain_interface_map_) {
    // if we've already visited this interface, but going the other direction,
    // skip it
    if (visited_interfaces.count({pair.second, pair.first}) > 0)
      continue;
    visited_interfaces.insert(pair);

    surface_senses_[surface_id] = pair;
    for (const auto &elem : elements) {
      surface_map_[surface_id].push_back(elem);
      surfaces().push_back(surface_id);
    }
    surface_id++;
  }

  auto& boundary_info = mesh_->get_boundary_info();
  // put all boundary elements in a special sideset
  for (auto &[id, elem_side] : subdomain_interface_map_) {
    if (id.first == ID_NONE || id.second == ID_NONE) {
      for (const auto &elem : elem_side) {
        boundary_info.add_side(elem.first, elem.second, 1);
      }
    }
  }
  boundary_info.sideset_name(1) = "boundary";

  mesh_->prepare_for_use();
  mesh_->write("test.e");
}

std::vector<MeshID>
LibMeshMeshManager::get_volume_elements(MeshID volume) const {
  std::vector<MeshID> elements;
  libMesh::MeshBase::const_element_iterator it =
      mesh()->active_subdomain_elements_begin(volume);
  libMesh::MeshBase::const_element_iterator it_end =
      mesh()->active_subdomain_elements_end(volume);
  for (; it != it_end; ++it) {
    elements.push_back((*it)->id());
  }
  return elements;
}

std::vector<MeshID>
LibMeshMeshManager::get_surface_elements(MeshID surface) const {
  std::vector<MeshID> faces;
  for (auto &[elem, side_info] :
       mesh()->get_boundary_info().get_sideset_map()) {
    if (side_info.second == surface) {
      faces.push_back(elem->side_ptr(side_info.first)->id());
    }
  }
  return faces;
}

std::vector<Vertex> LibMeshMeshManager::element_vertices(MeshID element) const {
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
  for (auto &[elem, side_info] :
       mesh()->get_boundary_info().get_sideset_map()) {
    if (side_info.first == volume) {
      surfaces.push_back(side_info.second);
    }
  }
  return surfaces;
}

std::pair<MeshID, MeshID>
LibMeshMeshManager::surface_senses(MeshID surface) const {

  return surface_senses_.at(surface);
}
//   std::pair<MeshID, MeshID> senses {ID_NONE, ID_NONE};

//   // get one of the sides for this surface
//   auto map_entry = sideset_element_map_.at(surface).at(0);
//   auto side = map_entry.first->side_ptr(map_entry.second);

//   // if (!side->is_face()) {
//   //   fatal_error("Surface element is not a face");
//   // }

//   // get the parent of this side elmenet
//   auto parent = side->parent();
//   // if (parent) {
//     // determine what subdomain this parent element is in
//     auto subdomain_id = map_entry.first->subdomain_id();
//     senses.first = subdomain_id;
//   // }

//   // get the subdomain on the other side of this face
//   if (!side->on_boundary()) {
//     auto side_number = side->which_side_am_i(parent);
//     auto other_element = parent->neighbor_ptr(side_number);
//     senses.second = other_element->subdomain_id();
//   }

//   return senses;
// }

Sense LibMeshMeshManager::surface_sense(MeshID surface, MeshID volume) const {
  auto senses = surface_senses(surface);
  return volume == senses.first ? Sense::FORWARD : Sense::REVERSE;
}

} // namespace xdg
