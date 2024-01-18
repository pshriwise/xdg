#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "xdg/mesh_manager_interface.h"
#include "xdg/moab/mesh_manager.h"
#include "xdg/vec3da.h"
#include "xdg/xdg.h"

using namespace xdg;

struct Particle {
  Position r;
  Direction u;
  MeshID volume;
  std::vector<MeshID> history;
};

int main(int argc, char** argv) {

// create a mesh manager
std::shared_ptr<XDG> xdg = XDG::create(MeshLibrary::MOAB);
const auto& mm = xdg->mesh_manager();

std::string filename {argv[1]};

mm->load_file(filename);
mm->init();
xdg->prepare_raytracer();

// create a new particle
Particle p{ {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {ID_NONE}, {}};

// determine what volume this particle is in
p.volume = xdg->find_volume(p.r, p.u);

std::cout << "Particle starts in volume " << p.volume << std::endl;

std::pair<double, MeshID> intersection;

intersection = xdg->ray_fire(p.volume, p.r, p.u, &p.history);

std::cout << "Intersected surface " << intersection.second << " at distance " << intersection.first << std::endl;

p.volume = xdg->mesh_manager()->next_volume(p.volume, intersection.second);
p.r += intersection.first * p.u;

std::cout << "Particle enters volume " << p.volume << " at position " << p.r << std::endl;

return 0;

}