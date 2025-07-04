#include <algorithm>
#include <map>
#include <memory>
#include <string>

#include "xdg/moab/mesh_manager.h"

#include "xdg/error.h"
#include "xdg/moab/tag_conventions.h"
#include "xdg/util/str_utils.h"
#include "xdg/vec3da.h"

using namespace xdg;

// Constructors
MOABMeshManager::MOABMeshManager()
{
  // create a new shared pointer
  this->shared_moab_instance_ = std::make_shared<moab::Core>();
  this->moab_raw_ptr_ = this->shared_moab_instance_.get();

  mdam_ = std::make_shared<MBDirectAccess>(this->moab_interface());
}

MOABMeshManager::MOABMeshManager(moab::Interface* mbi) : moab_raw_ptr_(mbi)
{
  mdam_ = std::make_shared<MBDirectAccess>(mbi);
};

void MOABMeshManager::init() {
  // initialize the direct access manager
  this->mb_direct()->setup();

  // get all of the necessary tag handles
  if (this->moab_interface()->tag_get_handle(GEOM_DIMENSION_TAG_NAME, geometry_dimension_tag_) != moab::MB_SUCCESS)
    fatal_error("Failed to find the MOAB geometry dimension tag");
  if (this->moab_interface()->tag_get_handle(GLOBAL_ID_TAG_NAME, global_id_tag_) != moab::MB_SUCCESS)
      fatal_error("Failed to find the MOAB global ID tag");
  if(this->moab_interface()->tag_get_handle(CATEGORY_TAG_NAME, category_tag_) != moab::MB_SUCCESS)
    fatal_error("Failed to find the MOAB category tag");
  if(this->moab_interface()->tag_get_handle(NAME_TAG_NAME, name_tag_) != moab::MB_SUCCESS)
    fatal_error("Failed to find the MOAB name tag");
  if(this->moab_interface()->tag_get_handle(GEOM_SENSE_2_TAG_NAME, surf_to_volume_sense_tag_) != moab::MB_SUCCESS)
    fatal_error("Failed to find the MOAB surface sense tag");

  // populate volumes vector and ID map
  auto moab_volume_handles = this->_ents_of_dim(3);
  std::vector<int> moab_volume_ids(moab_volume_handles.size());
  this->moab_interface()->tag_get_data(global_id_tag_,
                                       moab_volume_handles.data(),
                                       moab_volume_handles.size(),
                                       moab_volume_ids.data());

  for (int i = 0; i < moab_volume_ids.size(); i++) {
    volume_id_map_[moab_volume_ids[i]] = moab_volume_handles[i];
    volumes_.push_back(moab_volume_ids[i]);
  }
  // populate volumes vector and ID map
  auto moab_surface_handles = this->_ents_of_dim(2);
  std::vector<int> moab_surface_ids(moab_surface_handles.size());
  this->moab_interface()->tag_get_data(global_id_tag_,
                                       moab_surface_handles.data(),
                                       moab_surface_handles.size(),
                                       moab_surface_ids.data());
  for (int i = 0; i < moab_surface_ids.size(); i++) {
    surface_id_map_[moab_surface_ids[i]] = moab_surface_handles[i];
    surfaces_.push_back(moab_surface_ids[i]);
  }

  MeshID ipc = create_implicit_complement();
}

// Methods
void MOABMeshManager::load_file(const std::string& filepath)
{
  this->moab_interface()->load_file(filepath.c_str());
}

int MOABMeshManager::num_volumes() const
{
  return this->num_ents_of_dimension(3);
}

int MOABMeshManager::num_surfaces() const
{
  return this->num_ents_of_dimension(2);
}

int MOABMeshManager::num_ents_of_dimension(int dim) const {
  return this->_ents_of_dim(dim).size();
}

MeshID MOABMeshManager::create_volume() {
  moab::EntityHandle volume_set;
  this->moab_interface()->create_meshset(moab::MESHSET_SET, volume_set);

  MeshID volume_id = next_volume_id();

  this->moab_interface()->tag_set_data(global_id_tag_, &volume_set, 1, &volume_id);

  // set geometry dimension
  int dim = 3;
  this->moab_interface()->tag_set_data(geometry_dimension_tag_, &volume_set, 1, &dim);

  // set category tag
  this->moab_interface()->tag_set_data(category_tag_, &volume_set, 1, VOLUME_CATEGORY_VALUE);

  volume_id_map_[volume_id] = volume_set;

  return volume_id;
}

void MOABMeshManager::add_surface_to_volume(MeshID volume, MeshID surface, Sense sense, bool overwrite)
{
  moab::EntityHandle vol_handle = volume_id_map_.at(volume);
  moab::EntityHandle surf_handle = surface_id_map_.at(surface);
  this->moab_interface()->add_parent_child(vol_handle, surf_handle);

  // insert new volume into sense data
  auto sense_data = this->surface_senses(surface);

  if (sense == Sense::FORWARD) {
    if (sense_data.first != ID_NONE && !overwrite) fatal_error("Surface to volume sense is already set");
    sense_data.first = volume;
  } else if (sense == Sense::REVERSE) {
    if (sense_data.second != ID_NONE && !overwrite) fatal_error("Surface to volume sense is already set");
    sense_data.second = volume;
  } else {
    fatal_error("Invalid sense provided");
  }

  std::array<moab::EntityHandle, 2> sense_handles;
  sense_handles[0] = sense_data.first == ID_NONE ? 0 : volume_id_map_[sense_data.first];
  sense_handles[1] = sense_data.second == ID_NONE ? 0 : volume_id_map_[sense_data.second];
  const moab::EntityHandle* surf_handle_ptr = &surf_handle; // this is lame
  this->moab_interface()->tag_set_data(surf_to_volume_sense_tag_, surf_handle_ptr, 1, sense_handles.data());
}

// Mesh Methods
moab::Range
MOABMeshManager::_surface_faces(MeshID surface) const
{
  moab::EntityHandle surf_handle = surface_id_map_.at(surface);
  moab::Range elements;
  this->moab_interface()->get_entities_by_type(surf_handle, moab::MBTRI, elements);
  return elements;
}

// Get the coordinates of vertices for a given set of vertex EntityHandles
std::vector<Vertex> MOABMeshManager::_get_coords(moab::Range& verts) const
{
  std::vector<double> coords(verts.size() * 3);
  this->moab_interface()->get_coords(verts, coords.data());
  std::vector<Vertex> vertices(verts.size());
  for (int i = 0; i < verts.size(); i++) {
    vertices[i] = Vertex(coords[3*i], coords[3*i+1], coords[3*i+2]);
  }
  return vertices;
}

int
MOABMeshManager::num_volume_elements(MeshID volume) const
{
  return this->get_volume_elements(volume).size();
}

int
MOABMeshManager::num_volume_faces(MeshID volume) const
{
  int out {0};
  for (auto surface : this->get_volume_surfaces(volume)) {
    out += this->num_surface_faces(surface);
  }
  return out;
}

int
MOABMeshManager::num_surface_faces(MeshID surface) const
{
  return this->_surface_faces(surface).size();
}

std::vector<MeshID>
MOABMeshManager::get_volume_elements(MeshID volume) const
{
  moab::EntityHandle vol_handle = volume_id_map_.at(volume);

  moab::Range elements;
  this->moab_interface()->get_entities_by_dimension(vol_handle, 3, elements);

  std::vector<MeshID> element_ids(elements.size());
  for (int i = 0; i < elements.size(); i++) {
    element_ids[i] = this->moab_interface()->id_from_handle(elements[i]);
  }
  return element_ids;
}

std::vector<MeshID>
MOABMeshManager::get_surface_faces(MeshID surface) const
{
  auto elements = this->_surface_faces(surface);

  std::vector<MeshID> element_ids(elements.size());
  for (int i = 0; i < elements.size(); i++) {
    element_ids[i] = this->moab_interface()->id_from_handle(elements[i]);
  }

  return element_ids;
}

std::vector<Vertex> MOABMeshManager::element_vertices(MeshID element) const
{
  moab::EntityHandle element_handle;
  this->moab_interface()->handle_from_id(moab::MBTET, element, element_handle);
  auto out = this->mb_direct()->get_element_coords(element_handle);
  return std::vector<Vertex>(out.begin(), out.end());
}

std::array<Vertex, 3> MOABMeshManager::face_vertices(MeshID element) const
{
  moab::EntityHandle element_handle;
  this->moab_interface()->handle_from_id(moab::MBTRI, element, element_handle);
  auto out = this->mb_direct()->get_mb_coords(element_handle);
  return out;
}

std::pair<MeshID, MeshID>
MOABMeshManager::surface_senses(MeshID surface) const
{
  std::array<moab::EntityHandle, 2> sense_data {0, 0};
  moab::EntityHandle surf_handle = surface_id_map_.at(surface);
  this->moab_interface()->tag_get_data(surf_to_volume_sense_tag_, &surf_handle, 1, sense_data.data());

  std::array<MeshID, 2> mesh_ids {ID_NONE, ID_NONE};
  // independent calls in case one of the handles is invalid
  if (sense_data[0] != 0)
  this->moab_interface()->tag_get_data(global_id_tag_, sense_data.data(), 1, mesh_ids.data());
  if (sense_data[1] != 0)
  this->moab_interface()->tag_get_data(global_id_tag_, sense_data.data()+1, 1, mesh_ids.data()+1);

  return {mesh_ids[0], mesh_ids[1]};
}

Sense
MOABMeshManager::surface_sense(MeshID surface, MeshID volume) const
{
  auto sense_data = this->get_parent_volumes(surface);
  if (sense_data.first == volume) return Sense::FORWARD;
  else if (sense_data.second == volume) return Sense::REVERSE;
  else fatal_error("Volume {} is not a parent of surface {}", volume, surface);
  return Sense::UNSET;
}

std::vector<moab::EntityHandle>
MOABMeshManager::_ents_of_dim(int dim) const {
  std::array<moab::Tag, 1> tags = {geometry_dimension_tag_};
  std::array<const void*, 1> values = {&dim};
  moab::Range entities;

  this->moab_interface()->get_entities_by_type_and_tag(
    this->root_set(),
    moab::MBENTITYSET,
    tags.data(),
    values.data(),
    1,
    entities
  );
  return std::vector<moab::EntityHandle>(entities.begin(), entities.end());
}

std::vector<MeshID>
MOABMeshManager::get_volume_surfaces(MeshID volume) const
{
  moab::EntityHandle vol_handle = this->volume_id_map_.at(volume);

  std::vector<moab::EntityHandle> surfaces;
  this->moab_interface()->get_child_meshsets(vol_handle, surfaces);

  std::vector<MeshID> surface_ids(surfaces.size());
  this->moab_interface()->tag_get_data(global_id_tag_, surfaces.data(), surfaces.size(), surface_ids.data());

  return surface_ids;
}

std::vector<Vertex>
MOABMeshManager::get_surface_vertices(MeshID surface) const
{
  moab::Range faces = _surface_faces(surface);
  moab::Range verts;
  this->moab_interface()->get_adjacencies(faces, 0, false, verts, moab::Interface::UNION);
  return _get_coords(verts);
}

std::pair<std::vector<Vertex>, std::vector<int>>
MOABMeshManager::get_surface_mesh(MeshID surface) const
{
  moab::Range faces = _surface_faces(surface);
  moab::Range verts;
  this->moab_interface()->get_adjacencies(faces, 0, false, verts, moab::Interface::UNION);

  // Create a mapping from global vertex handles to local surface indices
  std::unordered_map<moab::EntityHandle, int> handle_to_index;
  int local_index = 0;
  for (auto vert : verts) {
    handle_to_index[vert] = local_index++;
  }

  std::vector<int> connectivity;
  for (auto face : faces) {
    std::vector<moab::EntityHandle> conn;
    this->moab_interface()->get_connectivity(&face, 1, conn); // global indices

    // Remap global indices to local indices on the surface
    connectivity.push_back(handle_to_index[conn[0]]);
    connectivity.push_back(handle_to_index[conn[1]]);
    connectivity.push_back(handle_to_index[conn[2]]);
  }
  return {_get_coords(verts), connectivity};
}

SurfaceElementType
MOABMeshManager::get_surface_element_type(MeshID surface) const
{
  moab::EntityHandle surf_handle = this->surface_id_map_.at(surface);

  moab::EntityType type = moab::MBTRI; // TODO: hardcodeed to tri for now

  switch (type)
  {
  case moab::MBTRI:
    return SurfaceElementType::TRI;
  case moab::MBQUAD:
    return SurfaceElementType::QUAD;
  }

  fatal_error("Unsupported surface element type");
}

void
MOABMeshManager::parse_metadata()
{
  // loop over all groups
  moab::Range groups;

  std::array<moab::Tag, 1> tags = {category_tag_};
  std::array<const void*, 1> values = {GROUP_CATEGORY_VALUE};

  this->moab_interface()->get_entities_by_type_and_tag(
    this->root_set(),
    moab::MBENTITYSET,
    tags.data(),
    values.data(),
    1,
    groups
  );

  for (auto group : groups) {
    // get the value of the name tag for this group
    std::string group_name(' ', CATEGORY_TAG_SIZE);
    this->moab_interface()->tag_get_data(name_tag_, &group, 1, group_name.data());
    std::vector<std::string> tokens = tokenize(strtrim(group_name), metadata_delimiters);

    // this group is often present and is meaningless
    if (tokens.size() == 1 && tokens[0] == "picked")
      continue;

    // if no metadata keywords are present in the tokens, ignore this group
    bool has_keywords = false;
    for (const auto& token : tokens) {
      if (MOAB_PROPERTY_MAP.count(token) > 0) {
        has_keywords = true;
        break;
      }
    }

    if (!has_keywords) {
      write_message(fmt::format("Ignoring group: {}", group_name));
      continue;
    }

    // ensure we have an even number of tokens
    // TODO: are there any cases in which this shouldn't be true???
    if (tokens.size() % 2 != 0) {
      std:: string msg = fmt::format("Group name tokens ({}) are of incorrect size: {}\n", tokens.size());
      for (const auto& t : tokens) {
        msg += fmt::format("{}", t);
      }
      msg += "\n";
      fatal_error(msg);
    }

    std::vector<Property> group_properties;
    // iterate over tokens by 2 and setup property objects
    for (unsigned int i = 0; i < tokens.size(); i += 2) {
      const std::string& key = tokens[i];
      const std::string& value = tokens[i+1];
      if (MOAB_PROPERTY_MAP.count(key) == 0)
        fatal_error("Could not find property for key '{}'", key);
      group_properties.push_back({MOAB_PROPERTY_MAP.at(key), value});
    }

    // separate out implicit complement properties
    for (auto it = group_properties.begin(); it != group_properties.end();) {
      auto prop = *it;
      if (prop.type == PropertyType::MATERIAL && ends_with(prop.value, "_comp")) {
        remove_substring(prop.value, "_comp");
        if (implicit_complement() != ID_NONE)
          volume_metadata_[{implicit_complement(), PropertyType::MATERIAL}] = prop;
        else
           write_message(fmt::format("Implicit complement material property '{}' found but no implicit complement volume set", prop.value));
        it = group_properties.erase(it);
      } else {
        ++it;
      }
    }

    // now we have all of the properties. Get the geometric entities they apply to
    moab::Range entities;
    this->moab_interface()->get_entities_by_type(group, moab::MBENTITYSET, entities);

    // assign parsed properties to metadata maps based on geom dimension tag
    // TODO: verify that the property data is valid for an entity with that dimenison
    for (auto entity : entities) {
      // get the entity dimension and id
      int dim, global_id;
      this->moab_interface()->tag_get_data(global_id_tag_, &entity, 1, &global_id);
      this->moab_interface()->tag_get_data(geometry_dimension_tag_, &entity, 1, &dim);

      if (dim == 3) {
        for (const auto& p : group_properties) {
          volume_metadata_[{global_id, p.type}] = p;
        }
      } else if (dim == 2) {
        for (const auto& p : group_properties) {
          surface_metadata_[{global_id, p.type}] = p;
        }
      } else {
        fatal_error("Properties for entities with dimension {} are unsupported", dim);
      }
    }
  }

  graveyard_check();
}

void
MOABMeshManager::graveyard_check()
{
  for (auto volume : this->volumes()) {
    auto prop = MeshManager::get_volume_property(volume, PropertyType::MATERIAL);
    // set the boundary condition to vacuum for all surfaces on volumes using a graveyard material
    if (to_lower(prop.value) == "graveyard") {
      auto volume_surfaces = this->get_volume_surfaces(volume);
      for (auto surface : volume_surfaces) {
        surface_metadata_[{surface, PropertyType::BOUNDARY_CONDITION}] = {PropertyType::BOUNDARY_CONDITION, "vacuum"};
      }
    }
  }
}
