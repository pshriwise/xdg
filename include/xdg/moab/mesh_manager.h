#ifndef _XDG_MOAB_MESH_MANAGER
#define _XDG_MOAB_MESH_MANAGER

#include <memory>

#include "xdg/mesh_manager_interface.h"

#include "moab/Core.hpp"

using namespace xdg;

class MOABMeshManager : MeshManager {

  MOABMeshManager(moab::Interface* mbi);

  MOABMeshManager();

  // Methods
  void load_file(const std::string& filepath);

  // Accessors
  moab::Interface* moab_mesh() const { return moab_raw_ptr_; };

private:

  std::shared_ptr<moab::Interface> shared_moab_instance_;
  moab::Interface* moab_raw_ptr_;
};

#endif