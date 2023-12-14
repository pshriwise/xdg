
#include "xdg/topology_interface.h"

void TopologyInterface::load_sense_data() {
    for (auto surface : this->mesh_manager()->surfaces()) {
        sense_data_[surface] = this->mesh_manager()->surface_senses(surface);
    }
}


void TopologyInterface::generate_implicit_complement()
{
  MeshID ipc = this->mesh_manager()->create_volume();
}