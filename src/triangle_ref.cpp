#include "xdg/triangle_ref.h"

namespace xdg
{

void TriangleBoundsFunc(const struct RTCBoundsFunctionArguments* args)
{
    const GeometryUserData* user_data = (const GeometryUserData*)args->geometryUserPtr;
    const TriangleRef* tri_ref = user_data->tri_ref;
    const MeshManager* mesh_manager = user_data->mesh_manager;

    // const Mesh* mesh = mesh_manager->get_mesh(tri_ref->mesh_id);
    // const Triangle* tri = mesh->get_triangle(tri_ref->triangle_id);
}

} // namespace xdg


