#ifndef _XDG_INTERFACE_H
#define _XDG_INTERFACE_H

#include <memory>
#include <unordered_map>

#include "xdg/mesh_manager_interface.h"
#include "xdg/ray_tracing_interface.h"

namespace xdg {


class XDG {

public:
// Constructors
  XDG() = default;

  XDG(std::shared_ptr<MeshManager> mesh_manager) :
    mesh_manager_(mesh_manager) {}

  // factor method that allows for specification of a backend mesh library
  static std::shared_ptr<XDG> create(MeshLibrary library);

// Methods
  void prepare_raytracer();

// Geometric Queries
MeshID find_volume(const Position& point,
                   const Direction& direction) const;

bool point_in_volume(MeshID volume,
                          const Position point,
                          const Direction* direction = nullptr,
                          const std::vector<MeshID>* exclude_primitives = nullptr) const;

std::pair<double, MeshID> ray_fire(MeshID volume,
                                   const Position& origin,
                                   const Direction& direction,
                                   const double dist_limit = INFTY,
                                   std::vector<MeshID>* const exclude_primitives = nullptr) const;

void closest(MeshID volume,
              const Position& origin,
              double& dist,
              MeshID& triangle) const;

void closest(MeshID volume,
              const Position& origin,
              double& dist) const;

bool occluded(MeshID volume,
              const Position& origin,
              const Direction& direction,
              double& dist) const;

Direction surface_normal(MeshID surface,
                         Position point,
                         const std::vector<MeshID>* exclude_primitives = nullptr) const;


  // Geometric Measurements
  double measure_volume(MeshID volume) const;
  double measure_surface_area(MeshID surface) const;
  double measure_volume_area(MeshID surface) const;

// Mutators
  void set_mesh_manager_interface(std::shared_ptr<MeshManager> mesh_manager) {
    mesh_manager_ = mesh_manager;
  }

// Accessors
  const std::shared_ptr<RayTracer>& ray_tracing_interface() const {
    return ray_tracing_interface_;
  }

  const std::shared_ptr<MeshManager>& mesh_manager() const {
    return mesh_manager_;
  }
// Private methods
private:
  double _triangle_volume_contribution(const PrimitiveRef& triangle) const;
  double _triangle_area_contribution(const PrimitiveRef& triangle) const;

// Data members
  const std::shared_ptr<RayTracer> ray_tracing_interface_ {std::make_shared<RayTracer>()};
  std::shared_ptr<MeshManager> mesh_manager_ {nullptr};

  std::unordered_map<MeshID, TreeID> volume_to_scene_map_;  //<! Map from mesh volume to embree scene
  std::unordered_map<MeshID, TreeID> surface_to_scene_map_; //<! Map from mesh surface to embree scnee
  std::unordered_map<MeshID, RTCGeometry> surface_to_geometry_map_; //<! Map from mesh surface to embree geometry
  TreeID gloabal_scene_;
};

}


#endif