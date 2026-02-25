#include <sstream>

// MOAB
#include "moab/Range.hpp"

#include "xdg/moab/direct_access.h"

namespace xdg {

MBDirectAccess::MBDirectAccess(Interface* mbi)
: mbi(mbi)
{
  face_data_.emplace(SurfaceFaceType::TRI, ConnectivityData{});
  face_data_.at(SurfaceFaceType::TRI).entity_type = MBTRI;

  element_data_.emplace(VolumeElementType::TET, ConnectivityData{});
  element_data_.at(VolumeElementType::TET).entity_type = MBTET;

  element_adjacency_data_.emplace(VolumeElementType::TET, AdjacencyData{});
  element_adjacency_data_.at(VolumeElementType::TET).entity_type = MBTET;
  setup();
}

void
MBDirectAccess::setup() {
  for (auto& [type, data] : face_data_) {
    data.setup(mbi);
  }
  for (auto& [type, data] : element_data_) {
    data.setup(mbi);
  }
  vertex_data_.setup(mbi);
<<<<<<< HEAD
  element_adjacency_data_.setup(mbi);
  boundary_face_adjacency_data_.setup(mbi, face_data_);
=======
  for (auto& [type, data] : element_adjacency_data_) {
    data.setup(mbi);
  }
>>>>>>> 91afbd5 (Extending data layout of the MOAB direct access class to support multiple types)
}

void
MBDirectAccess::clear()
{
  for (auto& [type, data] : face_data_) {
    data.clear();
  }
  for (auto& [type, data] : element_data_) {
    data.clear();
  }
  vertex_data_.clear();
<<<<<<< HEAD
  element_adjacency_data_.clear();
  boundary_face_adjacency_data_.clear();
=======
  for (auto& [type, data] : element_adjacency_data_) {
    data.clear();
  }
>>>>>>> 91afbd5 (Extending data layout of the MOAB direct access class to support multiple types)
}

void
MBDirectAccess::update() {
  clear();
  setup();
}

} // namespace xdg
