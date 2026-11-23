#include <cstdint>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

#include "xdg/error.h"
#include "xdg/mesh_manager_interface.h"
#include "xdg/vec3/vec3.h"
#include "xdg/xdg.h"

#include "position_uncertainty.h"

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

enum class Event { COLLISION, SURFACE };

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
  start_flight();
  volume_ = xdg_->find_volume(r_, u_);
  log("Particle {} initialized in volume {}", id_, volume_);
}

void surf_dist() {
  auto hit = xdg_->ray_fire(volume_, flight_origin_, u_, INFTY,
                           HitOrientation::EXITING, &history_, flight_parameter_);
  surface_parameter_ = hit.first;
  // Ray parameters are relative to the fixed flight origin; transport and
  // track-length tallies need the distance from the current parameter instead.
  surface_intersection_ = {hit.second == ID_NONE ? INFTY : hit.first - flight_parameter_,
                           hit.second};
  if (surface_intersection_.second == ID_NONE) {
    // Find nearest surface for diagnostics
    auto [dist_back, surf_back] = xdg_->ray_fire(volume_, r_, {-u_.x, -u_.y, -u_.z}, INFTY, HitOrientation::ANY, nullptr);

    // Keep each lost-particle diagnostic together when several workers report.
#ifdef _OPENMP
    #pragma omp critical(xdg_particle_diagnostics)
#endif
    {
      std::cout << "LOST" << std::endl;
      write_message(fmt::format(
        "LOST: particle {} at ({}, {}, {}) in volume {} | dir ({}, {}, {}) | "
        "fwd surf dist {} | back surf dist {} | events {} events_since_coll {}",
        id_, r_.x, r_.y, r_.z, volume_, u_.x, u_.y, u_.z,
        surface_intersection_.first, dist_back,
        total_events_, n_events_));
    }
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
  u_.normalize();
  log("Particle {} collides with material at position ({}, {}, {}), new direction is ({}, {}, {})", id_, r_.x, r_.y, r_.z, u_.x, u_.y, u_.z);
  history_.clear();
  start_flight();
}

// A straight flight keeps one ray across all transmitting boundaries. Rebase
// only when a collision or reflection changes its direction.
void start_flight() {
  flight_origin_ = r_;
  flight_parameter_ = Scalar(0);
}

void score_track(std::unordered_map<MeshID, double>& cell_tracks,
                 double distance, bool shared_tallies)
{
  if (shared_tallies) {
    // transport_particles creates every entry before its parallel region.
    // Lookup cannot insert or rehash the map while other workers use it.
    double& total = cell_tracks.at(volume_);
#ifdef _OPENMP
    #pragma omp atomic update
#endif
    total += distance;
  } else {
    cell_tracks[volume_] += distance;
  }
}

bool collision_is_safely_inside() const
{
  if (!(collision_distance_ < surface_intersection_.first)) return false;

  const auto& mesh = xdg_->mesh_manager();
  const auto vertices = mesh->face_vertices(history_.back());
  const bool reverse = mesh->surface_sense(surface_intersection_.second, volume_) ==
      Sense::REVERSE;
  const auto position = particle_sim_detail::collision_position_bounds(
      flight_origin_, u_, flight_parameter_, static_cast<Scalar>(collision_distance_));
  const auto clearance = particle_sim_detail::interior_clearance(vertices, position, reverse);
  // Certify the lower bound against the selected outgoing face. Ambiguous
  // collisions become surface events; nearby/entry faces are not covered here.
  return clearance.lower > Scalar(0);
}

Event advance(std::unordered_map<MeshID, double>& cell_tracks,
              bool shared_tallies = false)
{
  log("Comparing surface intersection distance {} to collision distance {}", surface_intersection_.first, collision_distance_);
  const bool collision = collision_is_safely_inside();
  if (collision) {
    flight_parameter_ += static_cast<Scalar>(collision_distance_);
    r_ = flight_origin_ + flight_parameter_ * u_;
    score_track(cell_tracks, collision_distance_, shared_tallies);
    log("Particle {} collides with material at position ({}, {}, {}) ", id_, r_.x, r_.y, r_.z);
  } else {
    if (collision_distance_ < surface_intersection_.first) {
      log("Particle {} takes surface event: collision-position uncertainty reaches surface {}",
          id_, surface_intersection_.second);
    }
    flight_parameter_ = surface_parameter_;
    r_ = flight_origin_ + flight_parameter_ * u_;
    score_track(cell_tracks, surface_intersection_.first, shared_tallies);
    log("Particle {} advances to surface {} at position ({}, {}, {}) with direction ({}, {}, {})", id_, surface_intersection_.second, r_.x, r_.y, r_.z, u_.x, u_.y, u_.z);
  }
  return collision ? Event::COLLISION : Event::SURFACE;
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
    start_flight();
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
  rng_state_ = seed & mask;
}

double random() {
  // The same 48-bit LCG and output as erand48, with no shared libc RNG state.
  constexpr uint64_t mask = (1ULL << 48) - 1ULL;
  rng_state_ = (0x5deece66dULL * rng_state_ + 0xbULL) & mask;
  return static_cast<double>(rng_state_) * 0x1p-48;
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
uint64_t rng_state_ {0x330e};

Position r_;
Direction u_;
Position flight_origin_;
Scalar flight_parameter_ {0};
Scalar surface_parameter_ {INFTY};
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
  if (sim_data.n_particles_ == 0) return;

  // Only values change during transport; map insertion/rehashing stays serial.
  for (MeshID volume : sim_data.xdg_->mesh_manager()->volumes()) {
    sim_data.cell_tracks.try_emplace(volume, 0.0);
  }

  int lost = 0;
  int stuck = 0;
#ifdef _OPENMP
  // Verbose histories remain ordered. GPRT uses shared GPU query buffers.
  const bool parallel = !sim_data.verbose_particles_ && sim_data.n_particles_ > 1 &&
      sim_data.xdg_->ray_tracing_interface()->library() == RTLibrary::EMBREE;
  #pragma omp parallel for if(parallel) schedule(static) reduction(+:lost,stuck)
#endif
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
      const auto event = p.advance(sim_data.cell_tracks, true);
      if (event == Particle::Event::COLLISION) {
        p.collide();
      } else {
        p.cross_surface();
      }
    }
    stuck += p.n_stuck;
    lost += p.n_lost;
  }
  sim_data.n_stuck += stuck;
  sim_data.n_lost += lost;
}
