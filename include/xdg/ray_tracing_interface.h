#ifndef _XDG_RAY_TRACING_INTERFACE_H
#define _XDG_RAY_TRACING_INTERFACE_H

#include <memory>
#include <vector>
#include <unordered_map>

#include "xdg/constants.h"
#include "xdg/embree_interface.h"
#include "xdg/mesh_manager_interface.h"
#include "xdg/triangle_ref.h"
#include "xdg/geometry_data.h"

namespace xdg
{

class RayTracer {
// Constructors
public:
  RayTracer();
  ~RayTracer();

// Methods
  void init();

  void register_volume(const std::shared_ptr<MeshManager> mesh_manager, MeshID volume);

  void register_all_volumes(const std::shared_ptr<MeshManager> mesh_manager) {
    for (auto volume : mesh_manager->volumes()) {
      this->register_volume(mesh_manager, volume);
    }
  }


  // Query Methods

  bool point_in_volume(MeshID volume,
                       const Position& point,
                       const Direction* direction = nullptr,
                       const std::vector<MeshID>* exclude_primitives = nullptr);

  void ray_fire(MeshID volume,
                const Position& origin,
                const Direction& direction,
                double& distance,
                const std::vector<MeshID>* exclude_primitives = nullptr);

  void closest(MeshID volume,
               const Position& origin,
               double& dist,
               TriangleRef& triangle);

  void closest(MeshID volume,
               const Position& origin,
               double& dist);

  bool occluded(MeshID volume,
                const Position& origin,
                const Direction& direction,
                double& dist) const;

  Direction get_normal(MeshID surface,
                       Position point,
                       const std::vector<MeshID>* exclude_primitives = nullptr);

// Accessors
  int num_registered_volumes() const { return volume_to_scene_map_.size(); }

  const GeometryUserData& geometry_data(MeshID surface) const { return user_data_map_.at(surface_to_geometry_map_.at(surface)); }

// Data members
private:
  // Embree members
  RTCDevice device_;
  // Mesh-to-Scene maps
  std::map<MeshID, RTCScene> volume_to_scene_map_;  //<! Map from mesh volume to embree scene
  std::map<MeshID, RTCScene> surface_to_scene_map_; //<! Map from mesh surface to embree scnee
  std::map<MeshID, RTCGeometry> surface_to_geometry_map_; //<! Map from mesh surface to embree geometry
  RTCScene gloabal_scene_;

  // Internal Embree Mappings
  std::unordered_map<RTCGeometry, GeometryUserData> user_data_map_;

  // Internal parameters
  double numerical_precision_ {1e-3};

  // storage
  std::unordered_map<MeshID, std::vector<TriangleRef>> triangle_storage_map_;
};

} // namespace xdg


#endif // include guard