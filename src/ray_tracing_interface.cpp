#include <algorithm>
#include "xdg/ray_tracing_interface.h"

// Any methods which are identical for all RT backends should be defined here

namespace xdg {

RayTracer::~RayTracer() {}

SurfaceTreeID RayTracer::next_surface_tree_id()
{
  return ++next_surface_tree_id_;
}

ElementTreeID RayTracer::next_element_tree_id()
{
  return ++next_element_tree_id_;
}

const double RayTracer::bounding_box_bump(const std::shared_ptr<MeshManager> mesh_manager, MeshID volume_id)
{
  auto volume_bounding_box = mesh_manager->volume_bounding_box(volume_id);
  return std::max(volume_bounding_box.dilation(), numerical_precision_);
}

} // namespace xdg