#ifndef _XDG_LIBMESH_MESH_MANAGER
#define _XDG_LIBMESH_MESH_MANAGER

#include <memory>

#include "xdg/constants.h"
#include "xdg/mesh_manager_interface.h"

#include "libmesh/libmesh.h"
#include "libmesh/mesh.h"
namespace xdg {

std::unique_ptr<libMesh::LibMeshInit> libmesh_init {nullptr};

class LibMeshMeshManager : public MeshManager {

public:
  LibMeshMeshManager(void* ptr);

  LibMeshMeshManager();

  MeshLibrary mesh_library() const override { return MeshLibrary::LIBMESH; }

  void load_file(const std::string& filepath) override;

  void init() override;

  void parse_metadata() override;

  int num_volumes() const override { return volumes_.size(); }

  int num_surfaces() const override { return surfaces_.size(); }

  int num_ents_of_dimension(int dim) const override {
    switch (dim) {
      case 3: return num_volumes();
      case 2: return num_surfaces();
      default: return 0;
    }
  }

  int num_volume_elements(MeshID volume) const override {
    return mesh()->n_elem();
  }

  int num_surface_elements(MeshID surface) const override {
    return mesh()->n_elem();
  }

  std::vector<MeshID> get_volume_elements(MeshID volume) const override;

  std::vector<MeshID> get_surface_elements(MeshID surface) const override;

  std::vector<Vertex> element_vertices(MeshID element) const override;

  std::array<Vertex, 3> triangle_vertices(MeshID triangle) const override;

  std::vector<MeshID> get_volume_surfaces(MeshID volume) const override;

  MeshID create_volume() override { throw std::runtime_error("Create volume not implemented for libMesh"); }

  void add_surface_to_volume(MeshID volume, MeshID surface, Sense sense, bool overwrite=false) override {
    throw std::runtime_error("Add surface to volume not implemented for libMesh");
  }

  std::pair<MeshID, MeshID> surface_senses(MeshID surface) const override;

  Sense surface_sense(MeshID surface, MeshID volume) const override;

  // Accessors
  const libMesh::Mesh* mesh() const { return mesh_.get(); }

  private:
    std::unique_ptr<libMesh::Mesh> mesh_;
};

} // namespace xdg

#endif // include guard