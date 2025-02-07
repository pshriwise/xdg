#include "xdg/ray_tracing_interface.h"

// So far, most of the methods in RayTracer require quite specific implementations to their RT backend.
// register_volume() is an exception to this. 
// I imagine the find_volume algorithm may have some common logic which can sit in here once its implemented.

namespace xdg {

// This function defines the common logic which is needed to register a volume with a particular RT library.
// This incldudes storage of various maps, which begs the question - where should that data be stored (base or derived?). 
// This function will not be virtual. 
// This function will call a virtual function which is overridden for each RayTracer class.
// Below I've copied over the parts which seem like they could be abstracted out of the derived class. 

/* RayTracer::register_volume(const std::shared_ptr<MeshManager> mesh_manager,MeshID volume_id)
{

  auto volume_scene = this->create_scene();  
  // volume_scene returns a TreeID. To get the underlying RTCScene we can do volume_scene.embree()
  // Once GPRT is implemented we could also use volume_scene.gprt()

  // allocate storage for this volume
  auto volume_elements = mesh_manager->get_volume_elements(volume_id);
  this->primitive_ref_storage_[volume_scene.embree()].resize(volume_elements.size());
  auto& triangle_storage = this->primitive_ref_storage_[volume_scene.embree()];

  auto volume_surfaces = mesh_manager->get_volume_surfaces(volume_id);
  int storage_offset {0};
  for (auto& surface_id : volume_surfaces) {
    // get the sense of this surface with respect to the volume
    Sense triangle_sense {Sense::UNSET};
    auto surf_to_vol_senses = mesh_manager->get_parent_volumes(surface_id);
    if (volume_id == surf_to_vol_senses.first) triangle_sense = Sense::FORWARD;
    else if (volume_id == surf_to_vol_senses.second) triangle_sense = Sense::REVERSE;
    else fatal_error("Volume {} is not a parent of surface {}", volume_id, surface_id);

    auto surface_elements = mesh_manager->get_surface_elements(surface_id);
    for (int i = 0; i < surface_elements.size(); ++i) {
      auto& primitive_ref = triangle_storage[i + storage_offset];
      primitive_ref.primitive_id = surface_elements[i];
      primitive_ref.sense = triangle_sense;
    }
    storage_offset += surface_elements.size();
 }

  PrimitiveRef* tri_ref_ptr = triangle_storage.data();

  // compute the bounding box of the volume
  auto volume_bounding_box = mesh_manager->volume_bounding_box(volume_id);

  // determine the bump distance for this volume based on the maximum distance a ray will travel
  // to an intersection
  double dx = volume_bounding_box.max_x - volume_bounding_box.min_x;
  double dy = volume_bounding_box.max_y - volume_bounding_box.min_y;
  double dz = volume_bounding_box.max_z - volume_bounding_box.min_z;

  double max_distance = std::sqrt(dx*dx + dy*dy + dz*dz);
  double bump = max_distance * std::pow(10, -std::numeric_limits<float>::digits10);
  bump = std::max(bump, 1e-03);
  bump = 1.0;

  register_volume_RT_backend(user_data_map?, mesh_manager?, volume_scene)

  return volume_scene

*/
}