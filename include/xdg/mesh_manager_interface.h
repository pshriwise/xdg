
#ifndef _XDG_MESH_MANAGER_INTERFACE
#define _XDG_MESH_MANAGER_INTERFACE

#include <string>
#include <vector>

#include "xdg/bbox.h"
#include "xdg/constants.h"
#include "xdg/vec3da.h"

namespace xdg {

class MeshManager {
public:

  // Abstract Methods

  // Setup
  virtual void load_file(const std::string& filepath) = 0;

  virtual void init() = 0;

  // Geometry
  virtual int num_volumes() const = 0;

  virtual int num_surfaces() const = 0;

  virtual int num_ents_of_dimension(int dim) const = 0;

  virtual std::vector<Vertex> get_vertices(MeshID element) const = 0;

  virtual std::vector<MeshID> get_surface_elements(MeshID surface) const = 0;

  // Mesh
  // TODO: can we accomplish this without allocating memory?
  virtual std::vector<std::array<double, 3>> element_vertices(MeshID element) const = 0;

  virtual BoundingBox element_bounding_box(MeshID element) const = 0;

  // Topology
  // Returns parent with forward sense, then reverse
  virtual std::pair<MeshID, MeshID> get_parent_volumes(MeshID surface) const = 0;

  virtual MeshID create_volume() = 0;

  virtual void add_surface_to_volume(MeshID volume, MeshID surface, Sense sense, bool overwrite=false) = 0;

  virtual std::pair<MeshID, MeshID> surface_senses(MeshID surface) const = 0;

  // Methods
  MeshID next_volume_id() const;

  MeshID next_surface_id() const;

  void create_implicit_complement();

  // Metadata methods
  virtual void parse_metadata() = 0;

  virtual Property get_volume_property(MeshID volume, PropertyType type) const = 0;
  virtual Property get_surface_property(MeshID surface, PropertyType type) const = 0;

  // Accessors
  const std::vector<MeshID>& volumes() const { return volumes_; }
  std::vector<MeshID>& volumes() { return volumes_; }
  const std::vector<MeshID>& surfaces() const { return surfaces_; }
  std::vector<MeshID>& surfaces() { return surfaces_; }

protected:
  // metadata
  std::map<std::pair<MeshID, PropertyType>, Property> volume_metadata_;
  std::map<std::pair<MeshID, PropertyType>, Property> surface_metadata_;

  std::vector<MeshID> volumes_;
  std::vector<MeshID> surfaces_;
};

} // namespace xdg

#endif