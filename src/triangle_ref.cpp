
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
    double dist;

    bool hit_tri = plucker_ray_tri_intersect(vertices, ray_origin, ray_direction, dist);



}

} // namespace xdg


