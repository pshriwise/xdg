#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "xdg/error.h"
#include "xdg/mesh_manager_interface.h"
#include "xdg/moab/mesh_manager.h"
#include "xdg/vec3da.h"
#include "xdg/xdg.h"

using namespace xdg;

static double mfp {1.0};

struct Particle {

Particle(std::shared_ptr<XDG> xdg, uint32_t id, bool verbose=true) : verbose_(verbose), xdg_(xdg), id_(id) {}

template<typename... Params>
void log (const std::string& msg, const Params&... fmt_args) {
  if (!verbose_) return;
  write_message(msg, fmt_args...);
}

void initialize() {
  // TODO: replace with sampling
  r_ = {0.0, 0.0, 0.0};
  u_ = {1.0, 0.0, 0.0};

  volume_ = xdg_->find_volume(r_, u_);
}

void surf_dist() {
  surface_intersection_ = xdg_->ray_fire(volume_, r_, u_, &history_);
  if (surface_intersection_.first == 0.0) {
    fatal_error("Particle {} stuck at position ({}, {}, {}) on surfacce {}", id_, r_.x, r_.y, r_.z, surface_intersection_.second);
    alive_ = false;
    return;
  }
  if (surface_intersection_.second == ID_NONE) {
    std::cerr << "Particle " << id_ << " lost in volume " << volume_ << std::endl;
    alive_ = false;
    return;
  }
  log("Intersected surface {} at distance {} ", surface_intersection_.second, surface_intersection_.first);
}

void sample_collision_distance() {
  collision_distance_ = -std::log(1.0 - drand48()) / mfp;
}

void collide() {
  n_events_++;
  u_ = rand_dir();
  log("Particle {} collides with material at position ({}, {}, {}), new direction is ({}, {}, {})", id_, r_.x, r_.y, r_.z, u_.z, u_.y, u_.z);
  history_.clear();
}

void advance()
{
  if (collision_distance_ < surface_intersection_.first) {
    r_ += collision_distance_ * u_;
    log("Particle {} collides with material at position ({}, {}, {}) ", id_, r_.x, r_.y, r_.z);

  } else {
    r_ += surface_intersection_.first * u_;
    log("Particle {} advances to surface {} at position ({}, {}, {}) ", id_, surface_intersection_.second, r_.x, r_.y, r_.z);
  }
}

void cross_surface()
{
  n_events_++;
  // check for the surface boundary condition
  if (xdg_->mesh_manager()->get_surface_property(surface_intersection_.second, PropertyType::BOUNDARY_CONDITION).value == "reflecting") {
    log("Particle {} reflects off surface {}", id_, surface_intersection_.second);

    Direction normal = xdg_->surface_normal(surface_intersection_.second, r_, &history_);
    u_ = u_ - 2.0 * dot(u_, normal) * normal;
    u_ = u_.normalize();
    if (history_.size() > 0) history_ = {history_.back()}; // reset to last intersection
  } else {
    volume_ = xdg_->mesh_manager()->next_volume(volume_, surface_intersection_.second);
    log("Particle {} enters volume {}", id_, volume_);
    if (volume_ == ID_NONE) {
      alive_ = false;
      return;
    }
  }
}

// Data Members
bool verbose_ {true};
std::shared_ptr<XDG> xdg_;
uint32_t id_ {0};
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

srand48(42);

// create a mesh manager
std::shared_ptr<XDG> xdg = XDG::create(MeshLibrary::MOAB);
const auto& mm = xdg->mesh_manager();

std::string filename {argv[1]};

mm->load_file(filename);
mm->init();
mm->parse_metadata();
xdg->prepare_raytracer();

// create a new particle

const int n_particles {100};

const int max_events {1000};

bool verbose = false;

 for (int i = 0; i < n_particles; i++) {
 write_message("Starting particle {}", i);
 Particle p(xdg, i, verbose);
 p.initialize();
 while (true) {
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

   if (p.n_events_ > max_events) {
     write_message("Maximum number of events ({}) reached", max_events);
     break;
   }
 }
 }

 return 0;
}
