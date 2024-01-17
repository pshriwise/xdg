#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "xdg/mesh_manager_interface.h"
#include "xdg/moab/mesh_manager.h"
#include "xdg/triangle_ref.h"
#include "xdg/vec3da.h"
#include "xdg/xdg.h"

using namespace xdg;

struct Particle {
  Position r;
  Direction u;
  MeshID volume;
  std::vector<TriangleRef> history;
};

int main(int argc, char** argv) {

// create a mesh manager
std::shared_ptr<MeshManager> mm = std::make_shared<MOABMeshManager>();

std::shared_ptr<AcceleratedDiscretizedGeometry> xdg =
  std::make_shared<AcceleratedDiscretizedGeometry>();
xdg->set_mesh_manager_interface(mm);

std::string filename {argv[1]};

mm->load_file(filename);
mm->init();

xdg->prepare_raytracer();

// create a new particle
Particle p{ {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {ID_NONE}, {}};

// determine what volume this particle is in
p.volume = xdg->find_volume(p.r, p.u);

std::cout << "Particle starts in volume " << p.volume << std::endl;

return 0;

}