
#include<memory>

#include "xdg/moab/mesh_manager.h"

// Constructors
MOABMeshManager::MOABMeshManager()
{
  // create a new shared pointer
  this->shared_moab_instance_ = std::make_shared<moab::Core>();
  this->moab_raw_ptr_ = this->shared_moab_instance_.get();

  mdam_ = std::make_shared<MBDirectAccess>(this->moab_mesh());
}

MOABMeshManager::MOABMeshManager(moab::Interface* mbi) : moab_raw_ptr_(mbi)
{
  mdam_ = std::make_shared<MBDirectAccess>(mbi);
};

void MOABMeshManager::init() {
  this->mb_direct()->setup();
}

// Methods
void MOABMeshManager::load_file(const std::string& filepath)
{
  this->moab_mesh()->load_file(filepath.c_str());
}

int MOABMeshManager::num_volumes() const { return 1; }

int MOABMeshManager::num_surfaces() const { return 1; }

int MOABMeshManager::num_ents_of_dimension(int dim) const { return 1; }

MeshID MOABMeshManager::volume(int idx) const { return 1; };

MeshID MOABMeshManager::surface(int idx) const { return 1; };
