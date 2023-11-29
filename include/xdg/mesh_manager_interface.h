
#ifndef _XDG_MESH_MANAGER_INTERFACE
#define _XDG_MESH_MANAGER_INTERFACE

#include <string>

namespace xdg {
class MeshManager {

    virtual void load_file(const std::string& filepath) = 0;

};

} // namespace xdg

#endif