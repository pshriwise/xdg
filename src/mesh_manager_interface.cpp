#include <fstream>

#include "xdg/mesh_manager_interface.h"

#include <algorithm>

#include "xdg/error.h"

namespace xdg {

MeshID
MeshManager::create_implicit_complement()
{
  // create a new volume
  MeshID ipc_volume = this->create_volume();

  for (auto surface : this->surfaces()) {
    auto parent_vols = this->get_parent_volumes(surface);

  if (parent_vols.first == ID_NONE)
      this->add_surface_to_volume(ipc_volume, surface, Sense::FORWARD);

    if (parent_vols.second == ID_NONE)
      this->add_surface_to_volume(ipc_volume, surface, Sense::REVERSE);
  }

  // insert the ipc volume into volume sets if it isn't present already
  if (std::find(volumes().begin(), volumes().end(), ipc_volume) == volumes().end())
    volumes().push_back(ipc_volume);

  // TODO: allow for alternate material assignment in IPC
  volume_metadata_[{ipc_volume, PropertyType::MATERIAL}] = VOID_MATERIAL;

  implicit_complement_ = ipc_volume;

  return ipc_volume;
}

MeshID MeshManager::next_volume_id() const
{
    return *std::max_element(volumes().begin(), volumes().end()) + 1;
}

MeshID MeshManager::next_surface_id() const
{
    return *std::max_element(surfaces().begin(), surfaces().end()) + 1;
}

bool
MeshManager::volume_has_property(MeshID volume, PropertyType type) const
{
  return volume_metadata_.count({volume, type}) > 0;
}

bool
MeshManager::surface_has_property(MeshID surface, PropertyType type) const
{
  return surface_metadata_.count({surface, type}) > 0;
}

Property
MeshManager::get_volume_property(MeshID volume, PropertyType type) const
{
  return volume_metadata_.at({volume, type});
}

Property
MeshManager::get_surface_property(MeshID surface, PropertyType type) const
{
  if (surface_metadata_.count({surface, type}) == 0)
    return {PropertyType::BOUNDARY_CONDITION, "transmission"};
  return surface_metadata_.at({surface, type});
}

MeshID MeshManager::next_volume(MeshID current_volume, MeshID surface) const
{
  auto parent_vols = this->get_parent_volumes(surface);

  if (parent_vols.first == current_volume)
    return parent_vols.second;
  else if (parent_vols.second == current_volume)
    return parent_vols.first;
  else
    fatal_error("Volume {} is not a parent of surface {}", current_volume, surface);

  return ID_NONE;
}

Direction MeshManager::triangle_normal(MeshID element) const
{
  auto vertices = this->triangle_vertices(element);
  return (vertices[1] - vertices[0]).cross(vertices[2] - vertices[0]).normalize();
}

BoundingBox
MeshManager::element_bounding_box(MeshID element) const
{
  auto vertices = this->element_vertices(element);
  return BoundingBox::from_points(vertices);
}

BoundingBox
MeshManager::triangle_bounding_box(MeshID element) const
{
  auto vertices = this->triangle_vertices(element);
  return BoundingBox::from_points(vertices);
}

BoundingBox
MeshManager::volume_bounding_box(MeshID volume) const
{
  BoundingBox bb;
  auto surfaces = this->get_volume_surfaces(volume);
  for (auto surface : surfaces) {
    bb.update(this->surface_bounding_box(surface));
  }
  return bb;
}

BoundingBox
MeshManager::surface_bounding_box(MeshID surface) const
{
  auto elements = this->get_surface_elements(surface);
  BoundingBox bb;
  for (const auto& element : elements) {
    bb.update(this->element_bounding_box(element));
  }
  return bb;
}

std::pair<MeshID, MeshID>
MeshManager::get_parent_volumes(MeshID surface) const
{
  return this->surface_senses(surface);
}

void
MeshManager::display_model_topology() const
{
  // First section: total counts
  size_t total_volumes = this->volumes().size();
  size_t total_surfaces = this->surfaces().size();
  size_t total_triangles = 0;

  for (auto surface : this->surfaces()) {
    total_triangles += this->get_surface_elements(surface).size();
  }

  std::cout << "Model Topology Information" << std::endl;
  std::cout << "==========================" << std::endl;
  std::cout << "Total Volumes: " << total_volumes << std::endl;
  std::cout << "Total Surfaces: " << total_surfaces << std::endl;
  std::cout << "Total Triangles: " << total_triangles << std::endl;
  std::cout << std::endl;

  // Second section: volume information
  std::cout << "Volume Information" << std::endl;
  std::cout << "==================" << std::endl;
  for (auto volume : this->volumes()) {
    auto surfaces = this->get_volume_surfaces(volume);
    size_t num_triangles = 0;
    for (auto surface : surfaces) {
      num_triangles += this->get_surface_elements(surface).size();
    }

    std::cout << "Volume ID: " << volume << std::endl;
    std::cout << "  Number of Triangles: " << num_triangles << std::endl;
    std::cout << "  Number of Surfaces: " << surfaces.size() << std::endl;
    std::cout << "  Surface IDs: ";
    for (auto surface : surfaces) {
      std::cout << surface << " ";
    }
    std::cout << std::endl;

    // Volume metadata
    std::cout << "  Metadata: " << std::endl;
    for (const auto& [key, property] : volume_metadata_) {
      if (key.first == volume) {
        std::cout << "    " << PROP_TYPE_TO_STR.at(key.second) << ": " << property.value << std::endl;
      }
    }
  }
  std::cout << std::endl;

  // Third section: surface information
  std::cout << "Surface Information" << std::endl;
  std::cout << "===================" << std::endl;
  for (auto surface : this->surfaces()) {
    auto parent_volumes = this->get_parent_volumes(surface);
    size_t num_triangles = this->get_surface_elements(surface).size();

    std::cout << "Surface ID: " << surface << std::endl;
    std::cout << "  Parent Volumes: ";
    std::cout << parent_volumes.first << " ";
    std::cout << parent_volumes.second << std::endl;
    std::cout << "  Number of Triangles: " << num_triangles << std::endl;

    // Surface metadata
    std::cout << "  Metadata: " << std::endl;
    for (const auto& [key, property] : surface_metadata_) {
      if (key.first == surface) {
        std::cout << "    " << PROP_TYPE_TO_STR.at(key.second) << ": " << property.value << std::endl;
      }
    }
    std::cout << std::endl;
  }
  std::cout << std::endl;
}

void MeshManager::write_topology_to_xml(const std::string filename) const
{
  std::ofstream file(filename);
  if (!file.is_open()) {
    throw std::runtime_error("Unable to open file: " + filename);
  }
  file << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
  file << "<graphml xmlns=\"http://graphml.graphdrawing.org/xmlns\">\n";

  // Write key definitions
  file << "    <key id=\"type\" for=\"node\" attr.name=\"type\" attr.type=\"string\"/>\n";
  file << "    <key id=\"id\" for=\"node\" attr.name=\"id\" attr.type=\"string\"/>\n";
  file << "    <key id=\"num_triangles\" for=\"node\" attr.name=\"num_triangles\" attr.type=\"int\"/>\n";
  for (const auto& [key, _] : volume_metadata_) {
    file << "    <key id=\"" << PROP_TYPE_TO_STR.at(key.second) << "\" for=\"node\" attr.name=\"" << PROP_TYPE_TO_STR.at(key.second) << "\" attr.type=\"string\"/>\n";
    break;
  }
  for (const auto& [key, _] : surface_metadata_) {
    file << "    <key id=\"" << PROP_TYPE_TO_STR.at(key.second) << "\" for=\"node\" attr.name=\"" << PROP_TYPE_TO_STR.at(key.second) << "\" attr.type=\"string\"/>\n";
    break;
  }

  // Start graph
  file << "  <graph edgedefault=\"undirected\">\n";

  // Write volume nodes
  for (auto volume : this->volumes()) {
    auto surfaces = this->get_volume_surfaces(volume);
    size_t num_triangles = 0;
    for (auto surface : surfaces) {
      num_triangles += this->get_surface_elements(surface).size();
    }

    file << "    <node id=\"Volume " << volume << "\">\n";
    file << "      <data key=\"type\">volume</data>\n";
    file << "      <data key=\"id\">" << volume << "</data>\n";
    file << "      <data key=\"num_triangles\">" << num_triangles << "</data>\n";
    for (const auto& [key, property] : volume_metadata_) {
      if (key.first == volume) {
        file << "      <data key=\"" << PROP_TYPE_TO_STR.at(key.second) << "\">" << property.value << "</data>\n";
      }
    }
    file << "    </node>\n";
  }

  // Write surface nodes
  for (auto surface : this->surfaces()) {
    auto parent_volumes = this->get_parent_volumes(surface);
    size_t num_triangles = this->get_surface_elements(surface).size();

    file << "    <node id=\"Surface " << surface << "\">\n";
    file << "      <data key=\"type\">surface</data>\n";
    file << "      <data key=\"id\">" << surface << "</data>\n";
    file << "      <data key=\"num_triangles\">" << num_triangles << "</data>\n";
    for (const auto& [key, property] : surface_metadata_) {
      if (key.first == surface) {
        file << "      <data key=\"" << PROP_TYPE_TO_STR.at(key.second) << "\">" << property.value << "</data>\n";
      }
    }
    file << "    </node>\n";
  }

  // Write edges between volumes and surfaces
  for (auto surface : this->surfaces()) {
    auto parent_volumes = this->get_parent_volumes(surface);
    if (parent_volumes.first != ID_NONE) {
      file << "    <edge source=\"Volume " << parent_volumes.first << "\" target=\"Surface " << surface << "\"/>\n";
    }
    if (parent_volumes.second != ID_NONE) {
      file << "    <edge source=\"Volume " << parent_volumes.second << "\" target=\"Surface " << surface << "\"/>\n";
    }
  }

  file << "  </graph>\n";
  file << "</graphml>\n";

  file.close();
}

} // namespace xdg