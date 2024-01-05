#ifndef _XDG_INTERFACE_H
#define _XDG_INTERFACE_H

#include <memory>

#include "xdg/mesh_manager_interface.h"
#include "xdg/ray_tracing_interface.h"

namespace xdg {

class XDG {

public:

// Methods
  void prepare_raytracer() {
    ray_tracing_interface_->register_all_volumes(mesh_manager_interface_);
  }

// Mutators
  void set_mesh_manager_interface(std::shared_ptr<MeshManager> mesh_manager_interface) {
    mesh_manager_interface_ = mesh_manager_interface;
  }

// Accessors
  const RayTracer* ray_tracing_interface() const {
    return ray_tracing_interface_.get();
  }

  const MeshManager* mesh_manager_interface() const {
    return mesh_manager_interface_.get();
  }

private:
  const std::shared_ptr<RayTracer> ray_tracing_interface_ {std::make_shared<RayTracer>()};
  std::shared_ptr<MeshManager> mesh_manager_interface_ {nullptr};
};

}


#endif