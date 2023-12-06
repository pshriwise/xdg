#ifndef _XDG_MOAB_MESH_MANAGER
#define _XDG_MOAB_MESH_MANAGER

#include <vector>
#include <unordered_map>

#include "xdg/mesh_manager_interface.h"
#include "xdg/moab/direct_access.h"
#include "xdg/moab/metadata.h"

#include "moab/Core.hpp"

using namespace xdg;

class MOABMeshManager : MeshManager {

  MOABMeshManager(moab::Interface* mbi);

  MOABMeshManager();

  // Methods
  void load_file(const std::string& filepath);

  void init();

  // Virtual method implementations
  int num_volumes() const override;

  int num_surfaces() const override;

  int num_ents_of_dimension(int dim) const override;

  std::vector<Vertex> get_vertices(MeshID element) const override;

private:
  // Internal MOAB methods
  std::vector<moab::EntityHandle> _ents_of_dim(int dim) const;

public:
  // Accessors
  moab::Interface* moab_interface() const { return moab_raw_ptr_; };
  const auto& mb_direct() const { return mdam_; }
  moab::EntityHandle root_set() const { return 0; }

private:
  std::shared_ptr<moab::Interface> shared_moab_instance_;
  moab::Interface* moab_raw_ptr_;
  std::shared_ptr<MBDirectAccess> mdam_;

  // Map from XDG identifier to MOAB handle
  std::unordered_map<MeshID, moab::EntityHandle> volume_id_map_;
  std::unordered_map<MeshID, moab::EntityHandle> surface_id_map_;
  std::unordered_map<MeshID, moab::EntityHandle> element_id_map_;

  // tag handles
  moab::Tag geometry_dimension_tag_;
  moab::Tag global_id_tag_;
  moab::Tag category_tag_;
  moab::Tag name_tag_;
  moab::Tag surf_to_volume_sense_tag_;
};

#endif