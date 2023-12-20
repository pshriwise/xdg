#ifndef _XDG_MOAB_MESH_MANAGER
#define _XDG_MOAB_MESH_MANAGER

#include <vector>
#include <unordered_map>

#include "xdg/mesh_manager_interface.h"
#include "xdg/moab/direct_access.h"
#include "xdg/moab/metadata.h"

#include "moab/Core.hpp"
#include "moab/Types.hpp"

namespace xdg {

static std::string VOLUME_CATEGORY_VALUE {"Volume"};
static std::string SURFACE_CATEGORY_VALUE {"Surface"};
static std::string GROUP_CATEGORY_VALUE {"Group"};

class MOABMeshManager : public MeshManager {

public:
  MOABMeshManager(moab::Interface* mbi);

  MOABMeshManager();

  // Methods
  void load_file(const std::string& filepath);

  void init() override;

  // Virtual method implementations
  int num_volumes() const override;

  int num_surfaces() const override;

  int num_ents_of_dimension(int dim) const override;

  std::vector<Vertex> get_vertices(MeshID element) const override;

  std::vector<MeshID> get_surface_elements(MeshID surface) const override;

  MeshID create_volume() override;

  void add_surface_to_volume(MeshID volume, MeshID surface, Sense sense, bool overwrite=false) override;

  std::pair<MeshID, MeshID> surface_senses(MeshID surface) const override;

  std::pair<MeshID, MeshID> get_parent_volumes(MeshID surface) const override;

  // Geometry

  // Metadata
  void parse_metadata() const;

  Property get_volume_property(MeshID volume, PropertyType type) const override;

  std::string get_volume_property(const std::string& property, MeshID vol) const;

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

  // Maps from XDG identifiers to MOAB handles
  std::unordered_map<MeshID, moab::EntityHandle> volume_id_map_;
  std::unordered_map<MeshID, moab::EntityHandle> surface_id_map_;
  std::unordered_map<MeshID, moab::EntityHandle> element_id_map_;

  // tag handles
  moab::Tag geometry_dimension_tag_;
  moab::Tag global_id_tag_;
  moab::Tag category_tag_;
  moab::Tag name_tag_;
  moab::Tag surf_to_volume_sense_tag_;

  // metadata
  std::map<MeshID, std::vector<Property>> volume_metadata_;
  std::map<MeshID, std::vector<Property>> surface_metadata_;

  // TODO: Make this comprehensive or a parameter in the constructor
  inline static const std::string delimiters = "|";
};

} // namespace xdg

#endif

