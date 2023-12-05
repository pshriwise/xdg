
#include<memory>

#include "xdg/moab/mesh_manager.h"
#include "xdg/moab/tag_conventions.h"

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
  this->moab_interface()->tag_get_handle(GEOM_DIMENSION_TAG_NAME, geometry_dimension_tag_);
  this->moab_interface()->tag_get_handle(GLOBAL_ID_TAG_NAME, global_id_tag_);
  this->moab_interface()->tag_get_handle(CATEGORY_TAG_NAME, category_tag_);
  this->moab_interface()->tag_get_handle(NAME_TAG_NAME, name_tag_);
  this->moab_interface()->tag_get_handle(GEOM_SENSE_2_TAG_NAME, surf_to_volume_sense_tag_);
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
  this->_ents_of_dim(dim);
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

MeshID MOABMeshManager::volume(int idx) const { return 1; };

MeshID MOABMeshManager::surface(int idx) const { return 1; };
