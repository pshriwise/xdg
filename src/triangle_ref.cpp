#include <algorithm> // for find

#include "xdg/triangle_ref.h"
#include "xdg/geometry_data.h"
#include "xdg/plucker.h"
#include "xdg/ray.h"

namespace xdg
{

void TriangleBoundsFunc(RTCBoundsFunctionArguments* args)
{
  const GeometryUserData* user_data = (const GeometryUserData*)args->geometryUserPtr;
  const MeshManager* mesh_manager = user_data->mesh_manager;

  const TriangleRef& tri_ref = user_data->tri_ref_buffer[args->primID];
  BoundingBox bounds = mesh_manager->element_bounding_box(tri_ref.triangle_id);

  args->bounds_o->lower_x = bounds.min_x - user_data->box_bump;
  args->bounds_o->lower_y = bounds.min_y - user_data->box_bump;
  args->bounds_o->lower_z = bounds.min_z - user_data->box_bump;
  args->bounds_o->upper_x = bounds.max_x + user_data->box_bump;
  args->bounds_o->upper_y = bounds.max_y + user_data->box_bump;
  args->bounds_o->upper_z = bounds.max_z + user_data->box_bump;
}

void orientation_cull(RTCDRayHit* rayhit) {
  RTCDRay& ray = rayhit->ray;
  RTCDHit& hit = rayhit->hit;

  // if looking for exiting intersections, cull if the normal is not pointing out wrt ray direction
  if (ray.orientation == HitOrientation::EXITING) {
    if (rayhit->dot_prod() < 0.0) {
      rayhit->hit.geomID = RTC_INVALID_GEOMETRY_ID;
      rayhit->hit.primID = RTC_INVALID_GEOMETRY_ID;
    }
  }
  // if looking for entering intersections, cull if the normal is not pointing in wrt ray direction
  else if (ray.orientation == HitOrientation::ENTERING) {
    if (rayhit->dot_prod() >= 0.0) {
      rayhit->hit.geomID = RTC_INVALID_GEOMETRY_ID;
      rayhit->hit.primID = RTC_INVALID_GEOMETRY_ID;
    }
  }
}

void primitive_mask_cull(RTCDRayHit* rayhit) {
  RTCDRay& ray = rayhit->ray;
  RTCDHit& hit = rayhit->hit;

  // if the primitive mask is set, cull if the primitive is not in the mask
  if (std::find(ray.exclude_primitives->begin(), ray.exclude_primitives->end(), hit.primID) != ray.exclude_primitives->end()) {
    rayhit->hit.geomID = RTC_INVALID_GEOMETRY_ID;
    rayhit->hit.primID = RTC_INVALID_GEOMETRY_ID;
  }
}

void TriangleIntersectionFunc(const RTCIntersectFunctionNArguments* args) {
  const GeometryUserData* user_data = (const GeometryUserData*)args->geometryUserPtr;
  const MeshManager* mesh_manager = user_data->mesh_manager;

  const TriangleRef& tri_ref = user_data->tri_ref_buffer[args->primID];

  auto vertices = mesh_manager->triangle_vertices(tri_ref.triangle_id);

  RTCDRayHit* rayhit = (RTCDRayHit*)args->rayhit;
  RTCDRay& ray = rayhit->ray;
  RTCDHit& hit = rayhit->hit;

  Position ray_origin = {ray.dorg[0], ray.dorg[1], ray.dorg[2]};
  Direction ray_direction = {ray.ddir[0], ray.ddir[1], ray.ddir[2]};

  // local variable for distance to the triangle intersection
  double plucker_dist;
  bool hit_tri = plucker_ray_tri_intersect(vertices, ray_origin, ray_direction, plucker_dist);

  if (!hit_tri) {
    rayhit->hit.geomID = RTC_INVALID_GEOMETRY_ID;
    return;
  }

  if (plucker_dist > rayhit->ray.dtfar) {
    rayhit->hit.geomID = RTC_INVALID_GEOMETRY_ID;
    return;
  }

  rayhit->ray.set_tfar(plucker_dist);
  // zero-out barycentric coords
  rayhit->hit.u = 0.0;
  rayhit->hit.v = 0.0;
  // set the hit information
  rayhit->hit.geomID = args->geomID;
  rayhit->hit.primID = args->primID;
  rayhit->hit.tri_ref = &tri_ref;

  Direction normal = (vertices[1] - vertices[0]) - (vertices[2] - vertices[0]);
  normal.normalize();
  // if this is a normal ray fire, flip the normal as needed
  if (tri_ref.sense == Sense::REVERSE && rayhit->ray.rf_type != RayFireType::FIND_VOLUME) normal = -normal;

  rayhit->hit.dNg[0] = normal[0];
  rayhit->hit.dNg[1] = normal[1];
  rayhit->hit.dNg[2] = normal[2];

  if (rayhit->ray.rf_type == RayFireType::VOLUME) {
    orientation_cull(rayhit);
    primitive_mask_cull(rayhit);
  }
}

} // namespace xdg


