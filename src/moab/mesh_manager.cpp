
#include "xdg/moab/mesh_manager.h"

// Constructors
MOABMeshManager::MOABMeshManager() {
  // create a new shared pointer
  this->shared_moab_instance_ = std::make_shared<moab::Core>();
  this->moab_raw_ptr_ = this->shared_moab_instance_.get();
}

MOABMeshManager::MOABMeshManager(moab::Interface* mbi) : moab_raw_ptr_(mbi) {};

// Methods
void MOABMeshManager::load_file(const std::string& filepath)
{
  this->moab_mesh()->load_file(filepath.c_str());
}