#include <algorithm> // for find

#include "xdg/geometry/closest.h"
#include "xdg/primitive_ref.h"
#include "xdg/quadrilateral_intersection.h"
#include "xdg/error.h"
#include "xdg/constants.h"
#include "xdg/geometry_data.h"
#include "xdg/geometry/face_common.h"
#include "xdg/geometry/plucker.h"
#include "xdg/ray.h"

namespace xdg
{

namespace {

bool orientation_cull(const Direction& ray_dir, const Direction& normal, HitOrientation orientation)
{
  if (orientation == HitOrientation::ANY) return false;

  double dot_prod = ray_dir.dot(normal);
  if (orientation == HitOrientation::EXITING && dot_prod < 0.0) {
    return true;
  }
  else if (orientation == HitOrientation::ENTERING && dot_prod >= 0.0) {
    return true;
  }
  return false;
}

bool primitive_mask_cull(RTCDualRayHit* rayhit, int primID) {
  if (!rayhit->ray.exclude_primitives) return false;

  RTCSurfaceDualRay& ray = rayhit->ray;

  // if the primitive mask is set, cull if the primitive is not in the mask
  return std::find(ray.exclude_primitives->begin(), ray.exclude_primitives->end(), primID) != ray.exclude_primitives->end();
}

} // namespace

void QuadBoundsFunction(RTCBoundsFunctionArguments* args)
{
  const SurfaceUserData* user_data = (const SurfaceUserData*)args->geometryUserPtr;
  const MeshManager* mesh_manager = user_data->mesh_manager;

  const PrimitiveRef& primitive_ref = user_data->prim_ref_buffer[args->primID];
  BoundingBox bounds = mesh_manager->face_bounding_box(primitive_ref.primitive_id);

  args->bounds_o->lower_x = bounds.min_x - user_data->box_bump;
  args->bounds_o->lower_y = bounds.min_y - user_data->box_bump;
  args->bounds_o->lower_z = bounds.min_z - user_data->box_bump;
  args->bounds_o->upper_x = bounds.max_x + user_data->box_bump;
  args->bounds_o->upper_y = bounds.max_y + user_data->box_bump;
  args->bounds_o->upper_z = bounds.max_z + user_data->box_bump;
}

void QuadIntersectionFunction(RTCIntersectFunctionNArguments* args) {
  const SurfaceUserData* user_data = (const SurfaceUserData*)args->geometryUserPtr;
  const MeshManager* mesh_manager = user_data->mesh_manager;

  const PrimitiveRef& primitive_ref = user_data->prim_ref_buffer[args->primID];
  auto face_vertices = mesh_manager->face_vertex_coordinates(primitive_ref.primitive_id);

  if (face_vertices.size() != 4) {
    fatal_error("Face {} has unsupported vertex count {}", primitive_ref.primitive_id, face_vertices.size());
  }

  RTCDualRayHit* rayhit = (RTCDualRayHit*)args->rayhit;
  RTCSurfaceDualRay& ray = rayhit->ray;

  Position ray_origin = {ray.dorg[0], ray.dorg[1], ray.dorg[2]};
  Direction ray_direction = {ray.ddir[0], ray.ddir[1], ray.ddir[2]};

  std::array<Position, 3> tri0 {face_vertices[0], face_vertices[1], face_vertices[2]};
  std::array<Position, 3> tri1 {face_vertices[0], face_vertices[2], face_vertices[3]};

  double dist0 = INFTY;
  double dist1 = INFTY;
  bool hit0 = plucker_ray_tri_intersect(tri0, ray_origin, ray_direction, dist0);
  bool hit1 = plucker_ray_tri_intersect(tri1, ray_origin, ray_direction, dist1);

  if (!hit0 && !hit1) return;

  std::array<Position, 3> hit_tri_vertices;
  double plucker_dist = INFTY;
  if (hit0 && dist0 < plucker_dist) {
    plucker_dist = dist0;
    hit_tri_vertices = tri0;
  }
  if (hit1 && dist1 < plucker_dist) {
    plucker_dist = dist1;
    hit_tri_vertices = tri1;
  }

  if (plucker_dist > rayhit->ray.dtfar) return;

  Direction normal = triangle_normal(hit_tri_vertices);

  // Check if ray is entering or exiting the volume it was fired against
  // if this is a normal ray fire, flip the normal as needed
  if (ray.volume_tree == user_data->reverse_vol && rayhit->ray.rf_type != RayFireType::FIND_VOLUME)
  {
    normal = -normal;
  }

  if (rayhit->ray.rf_type == RayFireType::VOLUME) {
    if (orientation_cull(rayhit->ray.ddir, normal, rayhit->ray.orientation)) return;
    if (primitive_mask_cull(rayhit, primitive_ref.primitive_id)) return;
  }

  // if we've gotten through all of the filters, set the ray information
  rayhit->ray.set_tfar(plucker_dist);
  // zero-out barycentric coords
  rayhit->hit.u = 0.0;
  rayhit->hit.v = 0.0;
  rayhit->hit.Ng_x = 0.0;
  rayhit->hit.Ng_y = 0.0;
  rayhit->hit.Ng_z = 0.0;
  // set the hit information
  rayhit->hit.geomID = args->geomID;
  rayhit->hit.primID = args->primID;
  rayhit->hit.primitive_ref = &primitive_ref;
  rayhit->hit.surface = user_data->surface_id;
  rayhit->hit.dNg = normal;
}

bool QuadClosestFunction(RTCPointQueryFunctionArguments* args) {
  RTCGeometry g = rtcGetGeometry(*(RTCScene*)args->userPtr, args->geomID);
  // get the array of DblTri's stored on the geometry
  const SurfaceUserData* user_data = (const SurfaceUserData*) rtcGetGeometryUserData(g);

  const MeshManager* mesh_manager = user_data->mesh_manager;

  const PrimitiveRef& primitive_ref = user_data->prim_ref_buffer[args->primID];
  auto face_vertices = mesh_manager->face_vertex_coordinates(primitive_ref.primitive_id);

  if (face_vertices.size() != 4) {
    fatal_error("Face {} has unsupported vertex count {}", primitive_ref.primitive_id, face_vertices.size());
  }

  RTCDPointQuery* query = (RTCDPointQuery*) args->query;
  Position p {query->dblx, query->dbly, query->dblz};

  std::array<Vertex, 3> tri0 {face_vertices[0], face_vertices[1], face_vertices[2]};
  std::array<Vertex, 3> tri1 {face_vertices[0], face_vertices[2], face_vertices[3]};
  Position result0 = closest_location_on_triangle(tri0, p);
  Position result1 = closest_location_on_triangle(tri1, p);
  double dist0 = (result0 - p).length();
  double dist1 = (result1 - p).length();

  Position result = dist0 <= dist1 ? result0 : result1;
  double dist = dist0 <= dist1 ? dist0 : dist1;

  if ( dist < query->dradius) {
    query->radius = dist;
    query->dradius = dist;
    query->primitive_ref = &primitive_ref;
    query->primID = args->primID;
    query->geomID = args->geomID;
    return true;
  } else {
    return false;
  }
}

void QuadOcclusionFunction(RTCOccludedFunctionNArguments* args) {
  const SurfaceUserData* user_data = (const SurfaceUserData*) args->geometryUserPtr;
  const MeshManager* mesh_manager = user_data->mesh_manager;
  const PrimitiveRef& primitive_ref = user_data->prim_ref_buffer[args->primID];

  auto face_vertices = mesh_manager->face_vertex_coordinates(primitive_ref.primitive_id);

  if (face_vertices.size() != 4) {
    fatal_error("Face {} has unsupported vertex count {}", primitive_ref.primitive_id, face_vertices.size());
  }

  // get the double precision ray from the args
  RTCSurfaceDualRay* ray = (RTCSurfaceDualRay*) args->ray;

  double plucker_dist = INFTY;
  std::array<Position, 3> tri0 {face_vertices[0], face_vertices[1], face_vertices[2]};
  std::array<Position, 3> tri1 {face_vertices[0], face_vertices[2], face_vertices[3]};
  bool hit = plucker_ray_tri_intersect(tri0, ray->dorg, ray->ddir, plucker_dist) ||
             plucker_ray_tri_intersect(tri1, ray->dorg, ray->ddir, plucker_dist);

  if (hit) ray->set_tfar(-INFTY);
}

bool SurfaceClosestFunc(RTCPointQueryFunctionArguments* args) {
  RTCGeometry g = rtcGetGeometry(*(RTCScene*)args->userPtr, args->geomID);
  const SurfaceUserData* user_data = (const SurfaceUserData*) rtcGetGeometryUserData(g);

  if (user_data->element_type == SurfaceElementType::QUAD) {
    return QuadClosestFunction(args);
  }

  return TriangleClosestFunc(args);
}

} // namespace xdg
