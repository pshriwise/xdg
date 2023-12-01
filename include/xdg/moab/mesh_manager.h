#ifndef _XDG_MOAB_MESH_MANAGER
#define _XDG_MOAB_MESH_MANAGER

#include "xdg/mesh_manager_interface.h"
#include "xdg/moab/moab_direct_access.h"

#include "moab/Core.hpp"

using namespace xdg;

class MOABMeshManager : MeshManager {

  MOABMeshManager(moab::Interface* mbi);

  MOABMeshManager();

  // Methods
  void load_file(const std::string& filepath);

  void init();

  // Accessors
  moab::Interface* moab_mesh() const { return moab_raw_ptr_; };
  const auto& mb_direct() const { return mdam_; }

private:

  std::shared_ptr<moab::Interface> shared_moab_instance_;
  moab::Interface* moab_raw_ptr_;
  std::shared_ptr<MBDirectAccess> mdam_;
};

#endif