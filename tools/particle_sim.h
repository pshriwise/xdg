#include <cstdint>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

#include "xdg/error.h"
#include "xdg/mesh_manager_interface.h"
#include "xdg/vec3/vec3.h"
#include "xdg/xdg.h"

using namespace xdg;

struct SimulationData {
  std::shared_ptr<XDG> xdg_;
  double mfp_ {1.0};
  uint32_t n_particles_ {100};
  uint32_t start_particle_id_ {0};
  uint32_t max_events_ {1000};
  uint64_t initial_seed_ {42};
  bool verbose_particles_ {false};
  bool implicit_complement_is_graveyard_ {false};
  std::unordered_map<MeshID, double> cell_tracks;
  std::vector<std::pair<Position, MeshID>> valid_states;
  int n_lost {0};
  int n_stuck {0};
};

struct Particle {

Particle(std::shared_ptr<XDG> xdg, uint32_t id, uint32_t max_events,
         uint64_t initial_seed,
         const std::vector<std::pair<Position, MeshID>>& valid_states,
         bool verbose=true, bool ipc_graveyard=false)
  : verbose_(verbose), xdg_(xdg), id_(id), max_events_(max_events),
    initial_seed_(initial_seed), valid_states_(valid_states),
    ipc_graveyard_(ipc_graveyard) {
  set_rng_state(initial_seed_);
}

template<typename... Params>
void log (const std::string& msg, const Params&... fmt_args) {
  if (!verbose_) return;
  write_message(msg, fmt_args...);
}

void initialize() {
  // TODO: replace with sampling
  r_ = {0.0, 0.0, 0.0};
  u_ = rand_dir();
  volume_ = xdg_->find_volume(r_, u_);
  log("Particle {} initialized in volume {}", id_, volume_);
}

void surf_dist() {
  surface_intersection_ = xdg_->ray_fire(volume_, r_, u_, INFTY, HitOrientation::EXITING, &history_);
  // if (surface_intersection_.first == 0.0) {
  //   auto [dist, surf] = xdg_->ray_fire(volume_, r_, u_, INFTY, HitOrientation::EXITING, &history_);
  //   auto [dist_back, surf_back] = xdg_->ray_fire(volume_, r_, {-u_.x, -u_.y, -u_.z}, INFTY, HitOrientation::ANY, nullptr);
  //   write_message(fmt::format(
  //     "STUCK: particle {} at ({}, {}, {}) in volume {} | dir ({}, {}, {}) | "
  //     "fwd surf dist {} | back surf dist {} | events {} events_since_coll {}",
  //     id_, r_.x, r_.y, r_.z, volume_, u_.x, u_.y, u_.z,
  //     dist, dist_back,
  //     total_events_, n_events_));
  //   n_stuck++;
  //   alive_ = false;
  //   return;
  // }
  if (surface_intersection_.second == ID_NONE) {
    // Find nearest surface for diagnostics
    std::cout << "LOST" << std::endl;
    auto [dist, surf] = xdg_->ray_fire(volume_, r_, u_, INFTY, HitOrientation::EXITING, &history_);
    auto [dist_back, surf_back] = xdg_->ray_fire(volume_, r_, {-u_.x, -u_.y, -u_.z}, INFTY, HitOrientation::ANY, nullptr);

    write_message(fmt::format(
      "LOST: particle {} at ({}, {}, {}) in volume {} | dir ({}, {}, {}) | "
      "fwd surf dist {} | back surf dist {} | events {} events_since_coll {}",
      id_, r_.x, r_.y, r_.z, volume_, u_.x, u_.y, u_.z,
      dist, dist_back,
      total_events_, n_events_));
    n_lost++;
    alive_ = false;
    return;
  }
  log("Intersected surface {} at distance {} ", surface_intersection_.second, surface_intersection_.first);
}

void sample_collision_distance(double mfp) {
  collision_distance_ = -std::log(1.0 - random()) * mfp;
}

void collide() {
  n_events_++;
  total_events_++;
  log("Event {} for particle {}", n_events_, id_);
  u_ = rand_dir();
  log("Particle {} collides with material at position ({}, {}, {}), new direction is ({}, {}, {})", id_, r_.x, r_.y, r_.z, u_.z, u_.y, u_.z);
  history_.clear();
}

void advance(std::unordered_map<MeshID, double>& cell_tracks)
{
  log("Comparing surface intersection distance {} to collision distance {}", surface_intersection_.first, collision_distance_);
  if (collision_distance_ < surface_intersection_.first) {
    r_ += collision_distance_ * u_;
    cell_tracks[volume_] += collision_distance_;
    log("Particle {} collides with material at position ({}, {}, {}) ", id_, r_.x, r_.y, r_.z);
  } else {
    r_ += surface_intersection_.first * u_;
    cell_tracks[volume_] += surface_intersection_.first;
    log("Particle {} advances to surface {} at position ({}, {}, {}) ", id_, surface_intersection_.second, r_.x, r_.y, r_.z);
  }
}

void cross_surface()
{
  n_events_++;
  log("Event {} for particle {}", n_events_, id_);
  auto boundary_condition = xdg_->mesh_manager()->get_surface_property(surface_intersection_.second, PropertyType::BOUNDARY_CONDITION);
  // check for the surface boundary condition
  if (boundary_condition.value == "reflecting" || boundary_condition.value == "reflective") {
    log("Particle {} reflects off surface {}", id_, surface_intersection_.second);
    log("Direction before reflection: ({}, {}, {})", u_.x, u_.y, u_.z);

    Direction normal = xdg_->surface_normal(surface_intersection_.second, r_, &history_);
    log("Normal to surface: ({}, {}, {})", normal.x, normal.y, normal.z);

    Scalar proj = dot(normal, u_);
    Scalar mag = normal.length();
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
    log("Particle {} encounters vacuum boundary at surface {}", id_, surface_intersection_.second);
    alive_ = false;
  } else {
    volume_ = xdg_->mesh_manager()->next_volume(volume_, surface_intersection_.second);
    log("Particle {} enters volume {}", id_, volume_);
    if (ipc_graveyard_ && volume_ == xdg_->mesh_manager()->implicit_complement()) volume_ = ID_NONE;
    if (volume_ == ID_NONE) {
      alive_ = false;
      return;
    }
  }
}

static uint64_t base_rng_state(uint64_t seed) {
  return ((seed & 0xffffffffULL) << 16) | 0x330eULL;
}

static uint64_t advance_rng_state(uint64_t state, uint64_t n_steps) {
  constexpr uint64_t multiplier = 0x5deece66dULL;
  constexpr uint64_t increment = 0xbULL;
  constexpr uint64_t mask = (1ULL << 48) - 1ULL;

  uint64_t acc_mult = 1ULL;
  uint64_t acc_plus = 0ULL;
  uint64_t cur_mult = multiplier;
  uint64_t cur_plus = increment;

  while (n_steps > 0) {
    if ((n_steps & 1ULL) != 0ULL) {
      acc_plus = (cur_mult * acc_plus + cur_plus) & mask;
      acc_mult = (cur_mult * acc_mult) & mask;
    }
    cur_plus = (cur_plus * (cur_mult + 1ULL)) & mask;
    cur_mult = (cur_mult * cur_mult) & mask;
    n_steps >>= 1ULL;
  }

  return (acc_mult * state + acc_plus) & mask;
}

static uint64_t particle_initial_seed(uint64_t base_seed, uint32_t id, uint32_t max_events) {
  constexpr uint64_t rng_draws_per_initialization = 2ULL;
  constexpr uint64_t rng_draws_per_event = 3ULL;
  const uint64_t particle_stride =
      rng_draws_per_initialization + rng_draws_per_event * static_cast<uint64_t>(max_events);
  return advance_rng_state(base_rng_state(base_seed), particle_stride * static_cast<uint64_t>(id));
}

void set_rng_state(uint64_t seed) {
  constexpr uint64_t mask = (1ULL << 48) - 1ULL;
  const uint64_t state = seed & mask;
  rng_state_[0] = static_cast<unsigned short>(state & 0xffffULL);
  rng_state_[1] = static_cast<unsigned short>((state >> 16) & 0xffffULL);
  rng_state_[2] = static_cast<unsigned short>((state >> 32) & 0xffffULL);
}

double random() {
  return erand48(rng_state_);
}

Direction rand_dir() {
    using std::sin; using std::cos; using std::acos;

    Scalar theta = static_cast<Scalar>(random()) * 2.0 * M_PI;
    Scalar u     = 2.0 * static_cast<Scalar>(random()) - 1.0;
    Scalar phi   = acos(u);

    return Direction(sin(phi) * cos(theta), sin(phi) * sin(theta), cos(phi)).normalize();
}

// Data Members
const std::vector<std::pair<Position, MeshID>>& valid_states_;
bool verbose_ {true};
std::shared_ptr<XDG> xdg_;
uint32_t id_ {0};
int32_t max_events_ {1000};
uint64_t initial_seed_ {42};
bool ipc_graveyard_ {false};
unsigned short rng_state_[3] {0x330e, 0x0000, 0x0000};

Position r_;
Direction u_;
MeshID volume_ {ID_NONE};
std::vector<MeshID> history_ {};
std::pair<Scalar, MeshID> surface_intersection_ {INFTY, ID_NONE};
double collision_distance_ {INFTY};
int32_t n_events_ {0};
bool alive_ {true};
int n_lost {0};
int n_stuck {0};
int32_t total_events_ {0};
};

void transport_particles(SimulationData& sim_data) {
  for (uint32_t i = 0; i < sim_data.n_particles_; i++) {
    const uint32_t particle_id = sim_data.start_particle_id_ + i;
    const uint64_t initial_seed = Particle::particle_initial_seed(
        sim_data.initial_seed_, particle_id, sim_data.max_events_);
    Particle p {sim_data.xdg_, particle_id, sim_data.max_events_,
            initial_seed,
            sim_data.valid_states,
            sim_data.verbose_particles_,
            sim_data.implicit_complement_is_graveyard_};
    p.initialize();
    while (p.alive_ && p.n_events_ < p.max_events_) {
      p.surf_dist();
      if (!p.alive_) break;

      p.sample_collision_distance(sim_data.mfp_);
      p.advance(sim_data.cell_tracks);

      if (p.collision_distance_ < p.surface_intersection_.first) {
        p.collide();
      } else {
        p.cross_surface();
      }
    }
    sim_data.n_stuck += p.n_stuck;
    sim_data.n_lost += p.n_lost;
  }
}
