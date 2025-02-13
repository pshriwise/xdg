#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "xdg/error.h"
#include "xdg/mesh_manager_interface.h"
#include "xdg/moab/mesh_manager.h"
#include "xdg/vec3da.h"
#include "xdg/xdg.h"

#include "argparse/argparse.hpp"

using namespace xdg;

static double MFP {1.0};

struct Particle {

Particle(std::shared_ptr<XDG> xdg, uint32_t id, bool verbose=true) : verbose_(verbose), xdg_(xdg), id_(id) {}

template<typename... Params>
void log (const std::string& msg, const Params&... fmt_args) {
  if (!verbose_) return;
  write_message(msg, fmt_args...);
}

void initialize() {
  // TODO: replace position with sampling in bbox
  r_ = {0.0, 0.0, 0.0};
  u_ = rand_dir();
  history_.clear();
  volume_ = xdg_->find_volume(r_, u_);
}

void surf_dist() {
  surface_intersection_ = xdg_->ray_fire(volume_, r_, u_, INFTY, HitOrientation::EXITING, &history_);
  if (surface_intersection_.first == 0.0) {
    fatal_error("Particle {} stuck at position ({}, {}, {}) on surfacce {}", id_, r_.x, r_.y, r_.z, surface_intersection_.second);
    alive_ = false;
    return;
  }
  if (surface_intersection_.second == ID_NONE) {
    fatal_error("Particle {} lost in volume {}", id_, volume_);
    alive_ = false;
    return;
  }
  log("Intersected surface {} at distance {} ", surface_intersection_.second, surface_intersection_.first);
}

void sample_collision_distance() {
  collision_distance_ = -std::log(1.0 - drand48()) * MFP;
}

void collide() {
  n_events_++;
  log("Event {} for particle {}", n_events_, id_);
  u_ = rand_dir();
  log("Particle {} collides with material at position ({}, {}, {}), new direction is ({}, {}, {})", id_, r_.x, r_.y, r_.z, u_.x, u_.y, u_.z);
  history_.clear();
}

void advance()
{
  log("Comparing surface intersection distance {} to collision distance {}", surface_intersection_.first, collision_distance_);
  if (collision_distance_ < surface_intersection_.first) {
    r_ += collision_distance_ * u_;
    log("Particle {} collides with material at position ({}, {}, {}) ", id_, r_.x, r_.y, r_.z);

  } else {
    r_ += surface_intersection_.first * u_;
    log("Particle {} advances to surface {} at position ({}, {}, {}) ", id_, surface_intersection_.second, r_.x, r_.y, r_.z);
  }
}

std::string material() {
  auto prop = xdg_->mesh_manager()->get_volume_property(volume_, PropertyType::MATERIAL);
  return prop.value;
}

void cross_surface()
{
  n_events_++;
  log("Event {} for particle {}", n_events_, id_);
  auto boundary_condition = xdg_->mesh_manager()->get_surface_property(surface_intersection_.second, PropertyType::BOUNDARY_CONDITION);
  log("Particle crossing surface {}. Boundary condition: {}", surface_intersection_.second, boundary_condition.value);
  // check for the surface boundary condition
  if (boundary_condition.value == "reflecting") {
    log("Particle {} reflects off surface {}", id_, surface_intersection_.second);
    log("Direction before reflection: ({}, {}, {})", u_.x, u_.y, u_.z);

    Direction normal = xdg_->surface_normal(surface_intersection_.second, r_, &history_);
    log("Normal to surface: ({}, {}, {})", normal.x, normal.y, normal.z);

    double proj = dot(normal, u_);
    double mag = normal.length();
    normal = normal * (2.0 * proj/mag);
    u_ = u_ - normal;
    u_ = u_.normalize();
    log("Direction after reflection: ({}, {}, {})", u_.x, u_.y, u_.z);
    // reset to last intersection
    if (history_.size() > 0) {
      log("Resetting particle history to last intersection");
      history_ = {history_.back()};
    }
  } else if (boundary_condition.value == "vacuum") {
    log("Particle crossing vacuum boundary. Terminating...");
    alive_ = false;
  } else {
    volume_ = xdg_->mesh_manager()->next_volume(volume_, surface_intersection_.second);
    log("Particle {} enters volume {} with material assignment {}", id_, volume_, material());
    if (volume_ == ID_NONE) {
      log("Particle has reached the problem boundary. Terminating...");
      alive_ = false;
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

// argument parsing
argparse::ArgumentParser args("XDG Particle Pseudo-Simulation", "1.0", argparse::default_arguments::help);

args.add_argument("filename")
    .help("Path to the input file");

args.add_argument("-v", "--verbose")
    .default_value(false)
    .implicit_value(true)
    .help("Enable verbose output of particle events");

args.add_argument("-m", "--mfp")
    .default_value(MFP)
    .help("Mean free path of the particles").scan<'g', double>();

  try {
    args.parse_args(argc, argv);
  }
  catch (const std::runtime_error& err) {
    std::cout << err.what() << std::endl;
    std::cout << args;
    exit(0);
  }

// Problem Setup
srand48(42);

// create a mesh manager
std::shared_ptr<XDG> xdg = XDG::create(MeshLibrary::MOAB);
const auto& mm = xdg->mesh_manager();

mm->load_file(args.get<std::string>("filename"));
mm->init();
mm->parse_metadata();
xdg->prepare_raytracer();

// update the mean free path
MFP = args.get<double>("--mfp");

const int n_particles {100};

const int max_events {1000};

bool verbose_particles = args.get<bool>("--verbose");

for (int i = 0; i < n_particles; i++) {
  int particle_id = i+1;
  write_message("Starting particle {}", particle_id);
  Particle p(xdg, particle_id, verbose_particles);
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
