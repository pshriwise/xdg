#ifndef _XDG_EMBREE_RAY_TRACING_INTERFACE_H
#define _XDG_EMBREE_RAY_TRACING_INTERFACE_H


#include <memory>
#include <vector>
#include <unordered_map>

#include "xdg/generic_types.h"
#include "xdg/constants.h"
#include "xdg/embree_interface.h"
#include "xdg/mesh_manager_interface.h"
#include "xdg/primitive_ref.h"
#include "xdg/geometry_data.h"
#include "xdg/ray_tracing_interface.h"
#include "xdg/ray.h"
#include "xdg/error.h"



namespace xdg {

class EmbreeRayTracer : public RayTracer {
  // constructors
public:
  EmbreeRayTracer();
  ~EmbreeRayTracer();

  void init() override;
  TreeID create_scene() override;
  TreeID register_volume(const std::shared_ptr<MeshManager> mesh_manager, MeshID volume) override;

  // Query Methods

  bool point_in_volume(TreeID scene,
                      const Position& point,
                      const Direction* direction = nullptr,
                      const std::vector<MeshID>* exclude_primitives = nullptr) const override;


  std::pair<double, MeshID> ray_fire(TreeID scene,
                                     const Position& origin,
                                     const Direction& direction,
                                     const double dist_limit = INFTY,
                                     std::vector<MeshID>* const exclude_primitives = nullptr) override;

  void closest(TreeID scene,
               const Position& origin,
               double& dist,
               MeshID& triangle) override;

  void closest(TreeID scene,
               const Position& origin,
               double& dist) override;

  bool occluded(TreeID scene,
                const Position& origin,
                const Direction& direction,
                double& dist) const override;

  // Accessors 
  int num_registered_scenes() const override { return scenes_.size(); };
  const std::shared_ptr<GeometryUserData>& geometry_data(MeshID surface) const override { return user_data_map_.at(surface_to_geometry_map_.at(surface)); };


  // Embree members
  RTCGeometry device_;
  std::vector<TreeID> scenes_; //<! All scenes created by this ray tracer
  std::vector<RTCGeometry> geometries_; //<! All geometries created by this ray tracer

  // Mesh-to-Scene maps
  std::map<MeshID, TreeID> surface_to_scene_map_; //<! Map from mesh surface to embree scnee
  std::map<MeshID, RTCGeometry> surface_to_geometry_map_; //<! Map from mesh surface to embree geometry
  TreeID gloabal_scene_;

  // Internal Embree Mappings
  std::unordered_map<RTCGeometry, std::shared_ptr<GeometryUserData>> user_data_map_;

  // Internal parameters
  double numerical_precision_ {1e-3};

  // storage
  std::unordered_map<RTCScene, std::vector<PrimitiveRef>> primitive_ref_storage_;
// Data members

private:

};

}


#endif // include guard