
#include <memory>
#include <string>

#include "xdg/moab/mesh_manager.h"

#include "xdg/error.h"
#include "xdg/moab/tag_conventions.h"
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
  }
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

  MeshID volume_id;
  this->moab_interface()->tag_get_data(global_id_tag_, &volume_set, 1, &volume_id);

  // set geometry dimension
  int dim = 3;
  this->moab_interface()->tag_set_data(geometry_dimension_tag_, &volume_set, 1, &dim);

  // set category tag
  this->moab_interface()->tag_set_data(category_tag_, &volume_set, 1, VOLUME_CATEGORY_VALUE.c_str());

  volume_id_map_[volume_id] = volume_set;

  return volume_id;
}

void MOABMeshManager::add_surface_to_volume(MeshID volume, MeshID surface)
{
  moab::EntityHandle vol_handle = volume_id_map_.at(volume);
  moab::EntityHandle surf_handle = surface_id_map_.at(surface);
  this->moab_interface()->add_parent_child(volume, surface);
}

std::pair<MeshID, MeshID>
MOABMeshManager::surface_senses(MeshID surface) const
{
  std::array<moab::EntityHandle, 2> sense_data;
  moab::EntityHandle surf_handle = surface_id_map_.at(surface);
  this->moab_interface()->tag_get_data(surf_to_volume_sense_tag_, &surf_handle, 1, sense_data.data());

  std::array<MeshID, 2> mesh_ids;
  this->moab_interface()->tag_get_data(global_id_tag_, sense_data.data(), 2, mesh_ids.data());

  return {mesh_ids[0], mesh_ids[1]};
}



std::vector<moab::EntityHandle>
MOABMeshManager::_ents_of_dim(int dim) const {
  std::array<moab::Tag, 1> tags = {geometry_dimension_tag_};
  std::array<void*, 1> values = {&dim};
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

std::vector<Vertex> MOABMeshManager::get_vertices(MeshID element) const {
  auto out = this->mb_direct()->get_mb_coords(element_id_map_.at(element));
  return std::vector<Vertex>(out.begin(), out.end());
}

std::vector<MeshID> MOABMeshManager::get_surface_elements(MeshID surface) const {

  moab::EntityHandle surf_handle = this->surface_id_map_.at(surface);

  std::vector<moab::EntityHandle> elements;
  this->moab_interface()->get_entities_by_type(surf_handle, moab::MBTRI, elements);

  std::vector<MeshID> element_ids(elements.size());
  this->moab_interface()->tag_get_data(global_id_tag_, elements.data(), elements.size(), element_ids.data());

  return element_ids;
}