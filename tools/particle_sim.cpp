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

  Particle(std::shared_ptr<XDG> xdg) : xdg_(xdg) {}

  void initialize() {
    // TODO: replace with sampling
    r_ = {0.0, 0.0, 0.0};
    u_ = {1.0, 0.0, 0.0};

    volume_ = xdg_->find_volume(r_, u_);
  }

  void surf_dist() {
    surface_intersection_ = xdg_->ray_fire(volume_, r_, u_, &history_);
    if (surface_intersection_.second == ID_NONE) {
      std::cerr << "Particle lost in volume " << volume_ << std::endl;
      alive_ = false;
      return;
    }
    std::cout << "Intersected surface " << surface_intersection_.second << " at distance " << surface_intersection_.first << std::endl;
  }

  void sample_collision_distance() { collision_distance_ = INFTY; }

  void collide() {
    n_events_++;
    std::cout << "Particle collides with material at position " << r_ << std::endl;
  }

  void advance() {
    if (collision_distance_ < surface_intersection_.first) {
      r_ += collision_distance_ * u_;
      std::cout << "Particle collides with material at position " << r_ << std::endl;
    } else {
      std::cout << "Particle advances to surface " << surface_intersection_.second << " at position " << r_ << std::endl;
      r_ += surface_intersection_.first * u_;
    }
  }

  void cross_surface() {
    n_events_++;

    volume_ = xdg_->mesh_manager()->next_volume(volume_, surface_intersection_.second);
    std::cout << "Particle enters volume " << volume_ << std::endl;
    if (volume_ == ID_NONE) {
      alive_ = false;
      return;
    }
  }

  // Data Members
  std::shared_ptr<XDG> xdg_;

  Position r_;
  Direction u_;
  MeshID volume_ {ID_NONE};
  std::vector<MeshID> history_{};

  std::pair<double, MeshID> surface_intersection_ {INFTY, ID_NONE};
  double collision_distance_ {INFTY};

  int32_t n_events_ {0};
  bool alive_ {true};
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
Particle p(xdg);

const int max_events {10};
p.initialize();
while (p.n_events_ < max_events) {
  p.surf_dist();
  // terminate for leakage
  if (!p.alive_) break;
  p.sample_collision_distance();
  p.advance();
  if (p.surface_intersection_.first < p.collision_distance_)
    p.cross_surface();
  else
    p.collide();
  if (!p.alive_) break;
}

return 0;
}