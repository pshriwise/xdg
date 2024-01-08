#include "xdg/mesh_manager_interface.h"

#include <algorithm>

#include "xdg/error.h"

namespace xdg {

void
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
}

MeshID MeshManager::next_volume_id() const
{
    return *std::max_element(volumes().begin(), volumes().end()) + 1;
}

MeshID MeshManager::next_surface_id() const
{
    return *std::max_element(surfaces().begin(), surfaces().end()) + 1;
}

MeshID MeshManager::next_volume(MeshID surface, MeshID current_volume) const
{
  auto parent_vols = this->get_parent_volumes(surface);

  if (parent_vols.first == current_volume)
    return parent_vols.second;
  else if (parent_vols.second == current_volume)
    return parent_vols.first;
  else
    fatal_error("Volume {} is not a parent of surface {}", current_volume, surface);
}

} // namespace xdg