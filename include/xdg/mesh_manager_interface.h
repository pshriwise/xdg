
#ifndef _XDG_MESH_MANAGER_INTERFACE
#define _XDG_MESH_MANAGER_INTERFACE

#include <string>

#include "xdg/constants.h"

namespace xdg {

class MeshManager {
public:
  virtual void load_file(const std::string& filepath) = 0;

  virtual int num_volumes() const = 0;

  virtual int num_surfaces() const = 0;

  virtual int num_ents_of_dimension(int dim) const = 0;

  virtual MeshID volume(int idx) const = 0;

  virtual MeshID surface(int idx) const = 0;
};

} // namespace xdg

#endif