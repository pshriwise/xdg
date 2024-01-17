#ifndef _XDG_INTERFACE_H
#define _XDG_INTERFACE_H

#include <memory>

#include "xdg/mesh_manager_interface.h"
#include "xdg/ray_tracing_interface.h"

namespace xdg {


class XDG {

public:
// Constructors
  XDG() = default;

  XDG(std::shared_ptr<MeshManager> mesh_manager) :
    mesh_manager_(mesh_manager) {}

// Methods
  void prepare_raytracer() {
    ray_tracing_interface_->register_all_volumes(mesh_manager_);
  }

// Geometric Queries
MeshID find_volume(const Position& point,
                   const Direction& direction) const;

  // Geometric Measurements
  double measure_volume(MeshID volume) const;
  double measure_surface_area(MeshID surface) const;
  double measure_volume_area(MeshID surface) const;

// Mutators
  void set_mesh_manager_interface(std::shared_ptr<MeshManager> mesh_manager) {
    mesh_manager_ = mesh_manager;
  }

// Accessors
  const std::shared_ptr<RayTracer> ray_tracing_interface() const {
    return ray_tracing_interface_;
  }

  const MeshManager* mesh_manager() const {
    return mesh_manager_.get();
  }

// Private methods
private:
  double _triangle_volume_contribution(const PrimitiveRef& triangle) const;
  double _triangle_area_contribution(const PrimitiveRef& triangle) const;

private:
  const std::shared_ptr<RayTracer> ray_tracing_interface_ {std::make_shared<RayTracer>()};
  std::shared_ptr<MeshManager> mesh_manager_ {nullptr};
};

}


#endif