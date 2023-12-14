#ifndef _XDG_TOPOLOGY_INTERFACE
#define _XDG_TOPOLOGY_INTERFACE

#include <memory>
#include <unordered_map>
#include <vector>

#include "xdg/constants.h"
#include "xdg/mesh_manager_interface.h"

namespace xdg {

class TopologyInterface
{
public:

  // Methods
  void find_geometry_sets();

  void setup_implicit_complement();

  void load_sense_data();

  void generate_implicit_complement();

  MeshID get_next_vol(MeshID surface, MeshID current_volume) const;

  std::vector<MeshID> get_volume_surfaces(MeshID volume) const;

  // Accessors
  const std::vector<MeshID>& surfaces() const { return surfaces_; }
  std::vector<MeshID>& surfaces() { return surfaces_; }

  const std::vector<MeshID>& volumes() const { return volumes_; }
  std::vector<MeshID>& volumes() { return volumes_; }

  MeshID implicit_complement() const { return ipc_; }

  const std::shared_ptr<MeshManager>& mesh_manager() const { return mesh_manager_; }

  std::pair<MeshID, MeshID> sense_data(MeshID volume) const { return sense_data_.at(volume); }

private:
  std::vector<MeshID> surfaces_;
  std::vector<MeshID> volumes_;
  MeshID ipc_;

  std::unordered_map<MeshID, std::pair<MeshID, MeshID>> sense_data_;

  std::shared_ptr<MeshManager> mesh_manager_;
};

}
#endif // include guard