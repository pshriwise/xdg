#ifndef XDG_TESTING_MOCKED_MESH_H
#define XDG_TESTING_MOCKED_MESH_H

#include <algorithm>
#include <unordered_map>
#include <vector>

#include "xdg/bbox.h"
#include "xdg/constants.h"
#include "xdg/error.h"
#include "xdg/mesh_manager_interface.h"
#include "xdg/vec3da.h"

namespace xdg {

// A lightweight mesh for testing face representation (tri + quad).
class MockedMesh : public MeshManager {
public:
  MockedMesh() {
    volumes_ = {0};
    surfaces_ = {0, 1};
    volume_surfaces_map_[0] = surfaces_;
    surface_sense_map_[0] = {0, ID_NONE};
    surface_sense_map_[1] = {0, ID_NONE};
  }

  void load_file(const std::string& /*file_name*/) override {}
  void init() override {}

  int num_volumes() const override { return 1; }
  int num_surfaces() const override { return static_cast<int>(surfaces_.size()); }
  int num_ents_of_dimension(int dim) const override {
    if (dim == 2) return num_surfaces();
    if (dim == 3) return num_volumes();
    return 0;
  }

  int num_volume_elements(MeshID /*volume*/) const override { return 0; }
  int num_volume_faces(MeshID volume) const override {
    int count = 0;
    for (auto surf : get_volume_surfaces(volume)) {
      count += num_surface_faces(surf);
    }
    return count;
  }
  int num_surface_faces(MeshID surface) const override {
    return static_cast<int>(surface_faces_.at(surface).size());
  }
  int num_vertices() const override { return static_cast<int>(vertices_.size()); }

  std::vector<MeshID> get_volume_elements(MeshID /*volume*/) const override { return {}; }
  std::vector<MeshID> get_volume_connectivity(MeshID /*volume*/) const override { return {}; }
  std::vector<Vertex> get_volume_vertices(MeshID /*volume*/) const override { return {}; }

  std::vector<MeshID> get_surface_faces(MeshID surface) const override {
    return surface_faces_.at(surface);
  }

  std::vector<Vertex> element_vertices(MeshID /*element*/) const override { return {}; }

  std::vector<MeshID> face_vertices(MeshID face) const override {
    return face_connectivity_.at(face);
  }

  std::vector<Vertex> get_surface_vertices(MeshID surface) const override {
    auto faces = get_surface_faces(surface);
    std::unordered_map<MeshID, int> handle_to_index;
    std::vector<Vertex> vertices;
    int local_index = 0;

    for (const auto& face : faces) {
      const auto& conn = face_connectivity_.at(face);
      for (const auto& global_index : conn) {
        if (handle_to_index.find(global_index) == handle_to_index.end()) {
          handle_to_index[global_index] = local_index++;
          vertices.push_back(vertex_coordinates(global_index));
        }
      }
    }

    return vertices;
  }

  std::vector<int> get_surface_connectivity(MeshID surface) const override {
    auto faces = get_surface_faces(surface);

    std::unordered_map<MeshID, int> handle_to_index;
    int local_index = 0;

    for (const auto& face : faces) {
      const auto& conn = face_connectivity_.at(face);
      for (const auto& global_index : conn) {
        if (handle_to_index.find(global_index) == handle_to_index.end()) {
          handle_to_index[global_index] = local_index++;
        }
      }
    }

    std::vector<int> connectivity;
    for (const auto& face : faces) {
      const auto& conn = face_connectivity_.at(face);
      for (const auto& global_index : conn) {
        connectivity.push_back(handle_to_index[global_index]);
      }
    }

    return connectivity;
  }

  SurfaceElementType get_surface_element_type(MeshID surface) const override {
    auto faces = get_surface_faces(surface);
    for (const auto& face : faces) {
      if (face_connectivity_.at(face).size() == 4) return SurfaceElementType::QUAD;
    }
    return SurfaceElementType::TRI;
  }

  MeshID adjacent_element(MeshID /*element*/, int /*face*/) const override {
    return ID_NONE;
  }

  double element_volume(MeshID /*element*/) const override { return 0.0; }

  Vertex vertex_coordinates(MeshID vertex_id) const override {
    return vertices_.at(vertex_id);
  }

  std::vector<MeshID> element_connectivity(MeshID /*element*/) const override { return {}; }

  std::vector<MeshID> get_volume_surfaces(MeshID volume) const override {
    auto it = volume_surfaces_map_.find(volume);
    if (it == volume_surfaces_map_.end()) return {};
    return it->second;
  }

  std::pair<MeshID, MeshID> surface_senses(MeshID surface) const override {
    return surface_sense_map_.at(surface);
  }

  Sense surface_sense(MeshID surface, MeshID volume) const override {
    auto senses = surface_senses(surface);
    if (volume == senses.first) return Sense::FORWARD;
    if (volume == senses.second) return Sense::REVERSE;
    fatal_error("Volume {} is not a parent of surface {}", volume, surface);
    return Sense::UNSET;
  }

  MeshID create_volume() override { return next_volume_id(); }

  void add_surface_to_volume(MeshID volume, MeshID surface, Sense sense, bool overwrite=false) override {
    auto& surfaces = volume_surfaces_map_[volume];
    if (!overwrite && std::find(surfaces.begin(), surfaces.end(), surface) != surfaces.end()) {
      fatal_error("Surface {} already exists in volume {}", surface, volume);
    }
    surfaces.push_back(surface);
    auto& senses = surface_sense_map_[surface];
    if (sense == Sense::FORWARD) senses.first = volume;
    else if (sense == Sense::REVERSE) senses.second = volume;
  }

  void parse_metadata() override {}

  MeshLibrary mesh_library() const override { return MeshLibrary::MOCK; }

private:
  std::unordered_map<MeshID, std::pair<MeshID, MeshID>> surface_sense_map_;
  std::unordered_map<MeshID, std::vector<MeshID>> volume_surfaces_map_;

  const std::vector<Vertex> vertices_ {
    {0.0, 0.0, 0.0},
    {1.0, 0.0, 0.0},
    {1.0, 1.0, 0.0},
    {0.0, 1.0, 0.0},
    {0.0, 0.0, 1.0},
    {1.0, 0.0, 1.0},
    {0.0, 1.0, 1.0},
  };

  // Connectivity is CCW when viewed from +Z, yielding +Z normals (outward for a volume on -Z side).
  const std::unordered_map<MeshID, std::vector<MeshID>> face_connectivity_ {
    {0, {0, 1, 2, 3}}, // quad (+Z normal)
    {1, {4, 5, 6}},    // tri (+Z normal)
  };

  const std::unordered_map<MeshID, std::vector<MeshID>> surface_faces_ {
    {0, {0}},
    {1, {1}},
  };
};

} // namespace xdg

#endif // XDG_TESTING_MOCKED_MESH_H
