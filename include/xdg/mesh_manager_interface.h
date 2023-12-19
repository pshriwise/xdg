
#ifndef _XDG_MESH_MANAGER_INTERFACE
#define _XDG_MESH_MANAGER_INTERFACE

#include <string>
#include <vector>

#include "xdg/constants.h"
#include "xdg/vec3da.h"

namespace xdg {

class MeshManager {
public:

  // setup
  virtual void load_file(const std::string& filepath) = 0;

  virtual void init() = 0;

  // geometry
  virtual int num_volumes() const = 0;

  virtual int num_surfaces() const = 0;

  virtual int num_ents_of_dimension(int dim) const = 0;

  virtual std::vector<Vertex> get_vertices(MeshID element) const = 0;

  virtual std::vector<MeshID> get_surface_elements(MeshID surface) const = 0;

  // Returns parent with forward sense, then reverse
  virtual std::pair<MeshID, MeshID> get_parent_volumes(MeshID surface) const = 0;

  virtual MeshID create_volume() = 0;

  virtual void add_surface_to_volume(MeshID volume, MeshID surface, Sense sense, bool overwrite=false) = 0;

  virtual std::pair<MeshID, MeshID> surface_senses(MeshID surface) const = 0;

  void create_implicit_complement();

  // metadata
  virtual Property get_volume_property(MeshID volume, PropertyType type) const = 0;

  // Accessors
  const std::vector<MeshID>& volumes() const { return volumes_; }
  std::vector<MeshID>& volumes() { return volumes_; }
  const std::vector<MeshID>& surfaces() const { return surfaces_; }
  std::vector<MeshID>& surfaces() { return surfaces_; }

private:
  std::vector<MeshID> volumes_;
  std::vector<MeshID> surfaces_;
};

} // namespace xdg

#endif