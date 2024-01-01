#include "xdg/triangle_ref.h"

namespace xdg
{

void TriangleBoundsFunc(RTCBoundsFunctionArguments* args)
{
    const GeometryUserData* user_data = (const GeometryUserData*)args->geometryUserPtr;
    const TriangleRef* tri_ref = user_data->tri_ref;
    const MeshManager* mesh_manager = user_data->mesh_manager;

    BoundingBox bounds = mesh_manager->element_bounding_box(tri_ref->triangle_id);

    args->bounds_o->lower_x = bounds.min_x - user_data->box_bump;
    args->bounds_o->lower_y = bounds.min_y - user_data->box_bump;
    args->bounds_o->lower_z = bounds.min_z - user_data->box_bump;
    args->bounds_o->upper_x = bounds.max_x + user_data->box_bump;
    args->bounds_o->upper_y = bounds.max_y + user_data->box_bump;
    args->bounds_o->upper_z = bounds.max_z + user_data->box_bump;
}

} // namespace xdg


