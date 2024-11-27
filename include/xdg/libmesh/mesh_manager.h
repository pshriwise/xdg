#ifndef _XDG_LIBMESH_MESH_MANAGER
#define _XDG_LIBMESH_MESH_MANAGER

#include <memory>

#include "xdg/constants.h"
#include "xdg/mesh_manager_interface.h"
#include "xdg/error.h"

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

    SidePair() = default;

    SidePair(std::pair<const libMesh::Elem*, int> old_side) {
      side.first = old_side.first;
      side.second = old_side.first->neighbor_ptr(old_side.second);
      set_order();
      set_side_num();
    }

    SidePair(const libMesh::Elem* elem, int side_num) {
      side.first = elem;
      side.second = elem->neighbor_ptr(side_num);
      set_order();
      set_side_num();
    }

    SidePair(const libMesh::Elem* elem1, const libMesh::Elem* elem2) {
      side.first = elem1;
      side.second = elem2;
      set_order();
      set_side_num();
    }

    SidePair(std::pair<const libMesh::Elem*, const libMesh::Elem*> side_pair) : side(side_pair) {
      set_order();
      set_side_num();
    }

    void set_order() {
      if (side.first == nullptr && side.second == nullptr) {
        fatal_error("SidePair created with null elements");
      }

      // null pointers come second always
      if (side.first == nullptr) {
        std::swap(side.first, side.second);
        return;
      }

      if (side.second == nullptr) {
        return;
      }

      if (side.first->id() == side.second->id()) {
        fatal_error("SidePair created with same element on both sides");
      }

      if (side.first->id() > side.second->id()) {
        std::swap(side.first, side.second);
      }
    }

    void set_side_num() {
      for (int i = 0; i < side.first->n_sides(); i++) {
        if (side.first->neighbor_ptr(i) == side.second) {
          side_num = i;
          break;
        }
      }
      if (side_num == -1) {
        fatal_error("SidePair created with elements that are not neighbors");
      }
    }


    bool operator==(const SidePair& other) const
    {
      return side == other.side; // || side == std::make_pair(other.side.second, other.side.first);
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

    const std::unique_ptr<const libMesh::Elem> face_ptr() const {
      return first()->side_ptr(side_num);
    }

    MeshID second_to_first_side() const {
      for (int i = 0; i < second()->n_sides(); i++) {
        if (second()->neighbor_ptr(i) == first()) {
          return i;
        }
      }
    }

    std::pair<const libMesh::Elem*, const libMesh::Elem*> side {nullptr, nullptr};
    int32_t side_num {-1};
  }; // SidePair

  struct SidePairHash {

    std::size_t operator()(const SidePair& p) const
    {
      auto hash1 = std::hash<const libMesh::Elem*>{}(p.first());
      auto hash2 = std::hash<const libMesh::Elem*>{}(p.second());
      // Combine the two hashes into a single hash value
      // This can be done using a common technique that combines them with XOR and bit shifting
      return hash1 ^ (hash2 + 0x9e3779b9 + (hash1 << 6) + (hash1 >> 2));
    }
  };

  MeshID sidepair_id(const SidePair& sidepair) {
    if (sidepair_to_mesh_id_.count(sidepair) == 0) {
      MeshID id = next_sidepair_id();
      mesh_id_to_sidepair_[id] = sidepair;
      sidepair_to_mesh_id_[sidepair] = id;
      return id;
    } else {
      return sidepair_to_mesh_id_[sidepair];
    }
  }

  MeshID sidepair_id(const libMesh::Elem* elem1, const libMesh::Elem* elem2) {
    return sidepair_id(SidePair(elem1, elem2));
  }

  SidePair sidepair(MeshID sidepair) const {
    return mesh_id_to_sidepair_.at(sidepair);
  }

  MeshID next_sidepair_id() const {
    if (mesh_id_to_sidepair_.size() == 0) {
      return 1;
    }
    return std::max_element(mesh_id_to_sidepair_.begin(), mesh_id_to_sidepair_.end())->first + 1;
  }

    std::unique_ptr<libMesh::Mesh> mesh_ {nullptr};
    // TODO: make this global so it isn't owned by a single mesh manager
    std::unique_ptr<libMesh::LibMeshInit> libmesh_init {nullptr};

    // Ugh, double mapping
    std::unordered_map<MeshID, SidePair> mesh_id_to_sidepair_;
    std::unordered_map<SidePair, MeshID, SidePairHash> sidepair_to_mesh_id_;

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
  std::unordered_map<MeshID, std::vector<MeshID>> surface_map_;

  std::unordered_map<MeshID, std::pair<MeshID, MeshID>> surface_senses_;

};

} // namespace xdg

#endif // include guard