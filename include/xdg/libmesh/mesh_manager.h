#ifndef _XDG_LIBMESH_MESH_MANAGER
#define _XDG_LIBMESH_MESH_MANAGER

#include <memory>

#include "xdg/constants.h"
#include "xdg/mesh_manager_interface.h"

#include "libmesh/libmesh.h"
#include "libmesh/elem.h"
#include "libmesh/mesh.h"
namespace xdg {


class LibMeshMeshManager : public MeshManager {

public:
  LibMeshMeshManager(void* ptr);

  LibMeshMeshManager();

  ~LibMeshMeshManager();

  MeshLibrary mesh_library() const override { return MeshLibrary::LIBMESH; }

  void load_file(const std::string& filepath) override;

  void initialize_libmesh();

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

  void discover_surface_elements();

  std::vector<MeshID> get_volume_elements(MeshID volume) const override;

  std::vector<MeshID> get_surface_elements(MeshID surface) const override;

  std::vector<Vertex> element_vertices(MeshID element) const override;

  std::array<Vertex, 3> triangle_vertices(MeshID triangle) const override;

  std::vector<MeshID> get_volume_surfaces(MeshID volume) const override;

  MeshID create_volume() override;

  void add_surface_to_volume(MeshID volume, MeshID surface, Sense sense, bool overwrite=false) override;

  std::pair<MeshID, MeshID> surface_senses(MeshID surface) const override;

  Sense surface_sense(MeshID surface, MeshID volume) const override;

  // Accessors
  const libMesh::Mesh* mesh() const { return mesh_.get(); }
  libMesh::Mesh* mesh() { return mesh_.get(); }

  private:

  struct SidePair {

    SidePair(std::pair<const libMesh::Elem*, int> old_side) {
      side.first = old_side.first;
      side.second = old_side.first->neighbor_ptr(old_side.second);
    }

    SidePair(const libMesh::Elem* elem, int side_num) {
      side.first = elem;
      side.second = elem->neighbor_ptr(side_num);
    }

    SidePair(const libMesh::Elem* elem1, const libMesh::Elem* elem2) {
      side.first = elem1;
      side.second = elem2;
    }

    SidePair(std::pair<const libMesh::Elem*, const libMesh::Elem*> side_pair) : side(side_pair) {}

    std::pair<const libMesh::Elem*, const libMesh::Elem*> side;

    bool operator==(const SidePair& other) const
    {
      return side == other.side || side == std::make_pair(other.side.second, other.side.first);
    }

    bool operator<(const SidePair& other) const
    {
      return side < other.side;
    }

    const libMesh::Elem* first() const { return side.first; }
    const libMesh::Elem* second() const { return side.second; }

    MeshID first_to_second_side() const {
      for (int i = 0; i < first()->n_sides(); i++) {
        if (first()->neighbor_ptr(i) == second()) {
          return i;
        }
      }
    }

    MeshID second_to_first_side() const {
      for (int i = 0; i < second()->n_sides(); i++) {
        if (second()->neighbor_ptr(i) == first()) {
          return i;
        }
      }
    }
  };

    std::unique_ptr<libMesh::Mesh> mesh_ {nullptr};
    // TODO: make this global so it isn't owned by a single mesh manager
    std::unique_ptr<libMesh::LibMeshInit> libmesh_init {nullptr};

    // sideset face mapping, stores the element and the side number
    // for each face in the mesh that lies on a boundary
    std::unordered_map<MeshID, std::vector<SidePair>> sideset_element_map_;

  struct MeshIDPairHash {
    std::size_t operator()(const std::pair<MeshID, MeshID>& p) const
    {
      return 4096 * p.first + p.second;
    }
  };

  std::unordered_map<std::pair<MeshID, MeshID>, std::vector<SidePair>, MeshIDPairHash>
  subdomain_interface_map_;

  // TODO: store proper data types here
  std::unordered_map<MeshID, std::vector<SidePair>> surface_map_;

  std::unordered_map<MeshID, std::pair<MeshID, MeshID>> surface_senses_;

};

} // namespace xdg

#endif // include guard