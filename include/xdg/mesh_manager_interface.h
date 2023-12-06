
#ifndef _XDG_MESH_MANAGER_INTERFACE
#define _XDG_MESH_MANAGER_INTERFACE

#include <string>
#include <vector>

#include "xdg/constants.h"
#include "xdg/vec3da.h"

namespace xdg {

class MeshManager {
public:
  virtual void load_file(const std::string& filepath) = 0;

  virtual int num_volumes() const = 0;

  virtual int num_surfaces() const = 0;

  virtual int num_ents_of_dimension(int dim) const = 0;

  virtual std::vector<Vertex> get_vertices(MeshID element) const = 0;

  virtual std::vector<MeshID> get_surface_elements(MeshID surface) const = 0;

  // Accessors
  std::vector<MeshID> volumes() const { return volumes_; }
  std::vector<MeshID> surfaces() const { return surfaces_; }

private:
  std::vector<MeshID> volumes_;
  std::vector<MeshID> surfaces_;
};

} // namespace xdg

#endif