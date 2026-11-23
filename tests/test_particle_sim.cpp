#include <array>
#include <atomic>
#include <memory>
#include <unordered_map>
#include <vector>

#ifdef _OPENMP
#include <omp.h>
#endif

#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "../tools/particle_sim.h"
#include "mesh_mock.h"
#include "util.h"

using namespace xdg::test;

namespace {

// Two boxes sharing x=5: the original mock spans [-2,5], and the second
// spans [5,12]. This exercises transmissions without any model-file dependency.
class AdjacentBoxes : public MeshMock {
public:
  AdjacentBoxes() : MeshMock(false) {
    volumes_ = {0, 1};
    surfaces_ = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  }

  int num_volumes() const override { return 2; }
  int num_surfaces() const override { return 11; }
  int num_vertices() const override { return 2 * MeshMock::num_vertices(); }

  std::vector<MeshID> get_volume_surfaces(MeshID volume) const override {
    if (volume == 0) return {0, 1, 2, 3, 4, 5};
    return {2, 6, 7, 8, 9, 10};
  }

  std::pair<MeshID, MeshID> surface_senses(MeshID surface) const override {
    if (surface == 2) return {0, 1};
    return {surface < 6 ? 0 : 1, ID_NONE};
  }

  Sense surface_sense(MeshID surface, MeshID volume) const override {
    return surface_senses(surface).first == volume ? Sense::FORWARD : Sense::REVERSE;
  }

  std::vector<MeshID> get_surface_faces(MeshID surface) const override {
    return {2 * surface, 2 * surface + 1};
  }

  std::array<Vertex, 3> face_vertices(MeshID face) const override {
    if (face < 12) return MeshMock::face_vertices(face);
    auto vertices = MeshMock::face_vertices(original_face(face));
    for (auto& vertex : vertices) vertex.x += Scalar(7);
    return vertices;
  }

  std::vector<MeshID> face_connectivity(MeshID face) const override {
    if (face < 12) return MeshMock::face_connectivity(face);
    auto vertices = MeshMock::face_connectivity(original_face(face));
    for (auto& vertex : vertices) vertex += MeshMock::num_vertices();
    return vertices;
  }

  Vertex vertex_coordinates(MeshID vertex) const override {
    if (vertex < MeshMock::num_vertices()) return MeshMock::vertex_coordinates(vertex);
    auto position = MeshMock::vertex_coordinates(vertex - MeshMock::num_vertices());
    position.x += Scalar(7);
    return position;
  }

  void reflect_at(MeshID surface) {
    surface_metadata_[{surface, PropertyType::BOUNDARY_CONDITION}] =
        {PropertyType::BOUNDARY_CONDITION, "reflecting"};
  }

private:
  static MeshID original_face(MeshID face) {
    constexpr std::array<int, 5> surfaces {0, 1, 2, 4, 5};
    return 2 * surfaces[(face - 12) / 2] + face % 2;
  }
};

} // namespace

TEMPLATE_TEST_CASE("Particle collision uncertainty respects both surface senses",
                   "[particle][uncertainty]", Embree_Raytracer, GPRT_Raytracer)
{
  constexpr auto backend = TestType::value;
  check_ray_tracer_supported(backend);
  auto geometry = std::make_shared<XDG>(std::make_shared<AdjacentBoxes>(), backend);
  geometry->prepare_raytracer();
  const std::vector<std::pair<Position, MeshID>> valid_states;

  const bool reverse = GENERATE(false, true);
  CAPTURE(reverse);
  Particle particle(geometry, 0, 1000, Particle::base_rng_state(1), valid_states, false);
  particle.r_ = reverse ? Position{7, 0, 0} : Position{0, 0, 0};
  particle.u_ = reverse ? Direction{-1, 0, 0} : Direction{1, 0, 0};
  particle.volume_ = reverse ? 1 : 0;
  particle.start_flight();
  particle.surf_dist();
  REQUIRE(particle.alive_);
  REQUIRE(particle.surface_intersection_.second == 2);
  const auto original_volume = particle.volume_;
  std::unordered_map<MeshID, double> tracks;

  SECTION("A collision well inside the face retains its sampled distance") {
    particle.collision_distance_ = 1.0;
    REQUIRE(particle.advance(tracks) == Particle::Event::COLLISION);
    REQUIRE(particle.r_.x == (reverse ? Scalar(6) : Scalar(1)));
    REQUIRE(particle.flight_parameter_ == Scalar(1));
    REQUIRE(particle.collision_distance_ == 1.0);
    REQUIRE(tracks.at(original_volume) == 1.0);
  }

  SECTION("Uncertain collisions advance and score the full surface segment") {
    const Scalar surface_distance = particle.surface_intersection_.first;
    const Scalar surface_parameter = particle.surface_parameter_;
    const double sampled_distance = std::nextafter(surface_distance, Scalar(0));
    particle.collision_distance_ = sampled_distance;
    REQUIRE(sampled_distance < surface_distance);
    REQUIRE(particle.advance(tracks) == Particle::Event::SURFACE);
    REQUIRE(particle.flight_parameter_ == surface_parameter);
    REQUIRE(particle.r_.x == Scalar(5));
    REQUIRE(particle.collision_distance_ == sampled_distance);
    REQUIRE(tracks.at(original_volume) == surface_distance);
    particle.cross_surface();
    REQUIRE(particle.volume_ == (reverse ? 0 : 1));
    REQUIRE(particle.total_events_ == 0); // No material collision was dispatched.
  }
}

#ifdef XDG_SINGLE_PRECISION
TEST_CASE("Oblique collision reconstruction selects the surface before rounding outside",
          "[particle][uncertainty]")
{
  // Frozen event 140 of teapot particle 9410439. The nominal collision is
  // strictly before the surface, but its reconstructed float point is outside.
  class CollisionFace : public MeshMock {
  public:
    std::array<Vertex, 3> face_vertices(MeshID) const override {
      return {{
        {-0.3796940743923187f, -9.40762996673584f, -2.2369942665100098f},
        {-0.14327971637248993f, -9.414198875427246f, -2.2369942665100098f},
        {-0.1438383162021637f, -9.450901985168457f, -2.3725318908691406f}
      }};
    }
  };
  auto geometry = std::make_shared<XDG>();
  geometry->set_mesh_manager_interface(std::make_shared<CollisionFace>());
  const std::vector<std::pair<Position, MeshID>> valid_states;
  Particle particle(geometry, 9410439, 1000, Particle::base_rng_state(1), valid_states, false);
  particle.r_ = {-0.11484712362289429f, -9.35221004486084f, -2.413515090942383f};
  particle.u_ = {-0.7653573751449585f, -0.25083887577056885f, 0.59271240234375f};
  particle.volume_ = 0;
  particle.start_flight();
  particle.surface_parameter_ = 0.25561854243278503f;
  particle.surface_intersection_ = {particle.surface_parameter_, 0};
  particle.history_ = {0};
  const double sampled_distance = 0.255618422126808187;
  particle.collision_distance_ = sampled_distance;
  std::unordered_map<MeshID, double> tracks;

  REQUIRE(sampled_distance < particle.surface_intersection_.first);
  REQUIRE(particle.advance(tracks) == Particle::Event::SURFACE);
  REQUIRE(particle.flight_parameter_ == particle.surface_parameter_);
  REQUIRE(tracks.at(0) == particle.surface_intersection_.first);
  REQUIRE(particle.collision_distance_ == sampled_distance);
}
#endif

TEMPLATE_TEST_CASE("Particle keeps one ray across transmitting boundaries", "[particle][flight]",
                   Embree_Raytracer, GPRT_Raytracer)
{
  constexpr auto backend = TestType::value;
  check_ray_tracer_supported(backend);
  auto mesh = std::make_shared<AdjacentBoxes>();
  auto geometry = std::make_shared<XDG>(mesh, backend);
  geometry->prepare_raytracer();
  const std::vector<std::pair<Position, MeshID>> valid_states;
  Particle particle(geometry, 0, 1000, Particle::base_rng_state(1), valid_states, false);
  particle.r_ = {0, 0, 0};
  particle.u_ = {1, 0, 0};
  particle.volume_ = 0;
  particle.start_flight();
  std::unordered_map<MeshID, double> tracks;

  particle.surf_dist();
  REQUIRE(particle.alive_);
  REQUIRE(particle.surface_intersection_.second == 2);
  REQUIRE(particle.surface_intersection_.first == Scalar(5));
  particle.collision_distance_ = INFTY;
  particle.advance(tracks);
  particle.cross_surface();
  REQUIRE(particle.volume_ == 1);
  REQUIRE(particle.flight_origin_.x == Scalar(0));
  REQUIRE(particle.flight_parameter_ == Scalar(5));
  REQUIRE(tracks[0] == 5.0);

  particle.surf_dist();
  REQUIRE(particle.alive_);
  REQUIRE(particle.surface_intersection_.second == 8);
  REQUIRE(particle.surface_parameter_ == Scalar(12));
  REQUIRE(particle.surface_intersection_.first == Scalar(7));

  SECTION("Transmission tallies only the current cell segment") {
    particle.collision_distance_ = INFTY;
    particle.advance(tracks);
    REQUIRE(particle.r_.x == Scalar(12));
    REQUIRE(particle.flight_origin_.x == Scalar(0));
    REQUIRE(particle.flight_parameter_ == Scalar(12));
    REQUIRE(tracks[1] == 7.0);
  }

  SECTION("Material collision starts a new flight and clears history") {
    particle.collision_distance_ = 2.0;
    particle.advance(tracks);
    REQUIRE(particle.r_.x == Scalar(7));
    REQUIRE(tracks[1] == 2.0);
    particle.collide();
    REQUIRE(particle.flight_origin_ == particle.r_);
    REQUIRE(particle.flight_parameter_ == Scalar(0));
    REQUIRE(particle.history_.empty());
    REQUIRE(particle.volume_ == 1);
  }

  SECTION("Reflection starts a new flight and retains its last face") {
    mesh->reflect_at(8);
    particle.collision_distance_ = INFTY;
    particle.advance(tracks);
    const auto reflected_face = particle.history_.back();
    particle.cross_surface();
    REQUIRE(particle.volume_ == 1);
    REQUIRE(particle.u_.x == Scalar(-1));
    REQUIRE(particle.flight_origin_.x == Scalar(12));
    REQUIRE(particle.flight_parameter_ == Scalar(0));
    REQUIRE(particle.history_ == std::vector<MeshID>{reflected_face});
    particle.surf_dist();
    REQUIRE(particle.alive_);
    REQUIRE(particle.surface_intersection_.second == 2);
    REQUIRE(particle.surface_intersection_.first == Scalar(7));
  }
}

TEMPLATE_TEST_CASE("A zero-distance transmission does not move the flight origin", "[particle][flight]",
                   Embree_Raytracer, GPRT_Raytracer)
{
  constexpr auto backend = TestType::value;
  check_ray_tracer_supported(backend);
  auto geometry = std::make_shared<XDG>(std::make_shared<AdjacentBoxes>(), backend);
  geometry->prepare_raytracer();
  const std::vector<std::pair<Position, MeshID>> valid_states;
  Particle particle(geometry, 0, 1000, Particle::base_rng_state(1), valid_states, false);
  particle.r_ = {5, 0, 0};
  particle.u_ = {1, 0, 0};
  particle.volume_ = 0;
  particle.start_flight();
  std::unordered_map<MeshID, double> tracks;

  particle.surf_dist();
  REQUIRE(particle.alive_);
  REQUIRE(particle.surface_intersection_.first == Scalar(0));
  particle.collision_distance_ = INFTY;
  particle.advance(tracks);
  particle.cross_surface();
  particle.surf_dist();
  REQUIRE(particle.alive_);
  REQUIRE(particle.volume_ == 1);
  REQUIRE(particle.flight_origin_.x == Scalar(5));
  REQUIRE(particle.flight_parameter_ == Scalar(0));
  REQUIRE(particle.surface_intersection_.first == Scalar(7));
  REQUIRE(tracks[0] == 0.0);
}

TEST_CASE("Particle RNG preserves the erand48 sequence", "[particle][rng]")
{
  const std::vector<std::pair<Position, MeshID>> valid_states;
  for (uint32_t id : {0u, 1u, 8449u, 9999999u}) {
    CAPTURE(id);
    const auto seed = Particle::particle_initial_seed(1, id, 1000);
    unsigned short reference_state[3] = {
      static_cast<unsigned short>(seed & 0xffffULL),
      static_cast<unsigned short>((seed >> 16) & 0xffffULL),
      static_cast<unsigned short>((seed >> 32) & 0xffffULL)
    };
    Particle particle(nullptr, id, 1000, seed, valid_states, false);
    for (int i = 0; i < 32; ++i) {
      REQUIRE(particle.random() == erand48(reference_state));
    }
  }
}

#ifdef _OPENMP
namespace {

class ObservedBoxes : public AdjacentBoxes {
public:
  std::array<Vertex, 3> face_vertices(MeshID face) const override {
    threads_seen.fetch_or(1u << omp_get_thread_num(), std::memory_order_relaxed);
    return AdjacentBoxes::face_vertices(face);
  }

  mutable std::atomic<unsigned> threads_seen {0};
};

struct OpenMPSettings {
  int threads = omp_get_max_threads();
  int dynamic = omp_get_dynamic();
  ~OpenMPSettings() {
    omp_set_num_threads(threads);
    omp_set_dynamic(dynamic);
  }
};

} // namespace

TEST_CASE("Parallel particle transport preserves histories and atomic tallies",
          "[particle][openmp]")
{
  check_ray_tracer_supported(RTLibrary::EMBREE);
  if (omp_get_thread_limit() < 2) SKIP("The runtime limits OpenMP to one thread");
  OpenMPSettings restore;
  omp_set_dynamic(0);
  auto mesh = std::make_shared<ObservedBoxes>();
  auto geometry = std::make_shared<XDG>(mesh, RTLibrary::EMBREE);
  geometry->prepare_raytracer();

  SimulationData initial;
  initial.xdg_ = geometry;
  initial.start_particle_id_ = 8449;
  initial.n_particles_ = 257; // Nonzero start and a partial final worker range.
  initial.max_events_ = 25;
  initial.initial_seed_ = 1234;
  initial.mfp_ = 2.0;
  initial.cell_tracks = {{0, 1.25}, {1, 2.5}, {12345, 3.75}};
  initial.n_lost = 2;
  initial.n_stuck = 3;

  omp_set_num_threads(1);
  auto serial = initial;
  transport_particles(serial);

  omp_set_num_threads(4);
  mesh->threads_seen.store(0);
  auto parallel = initial;
  transport_particles(parallel);
  const auto workers = mesh->threads_seen.load();
  REQUIRE((workers & (workers - 1)) != 0); // Geometry was queried by multiple workers.
  REQUIRE(parallel.n_lost == serial.n_lost);
  REQUIRE(parallel.n_stuck == serial.n_stuck);
  REQUIRE(parallel.cell_tracks.size() == serial.cell_tracks.size());
  for (const auto& [volume, distance] : serial.cell_tracks) {
    CAPTURE(volume);
    REQUIRE_THAT(parallel.cell_tracks.at(volume),
                 Catch::Matchers::WithinRel(distance, 1e-12));
  }

  // Empty calls preserve existing state and do not require a geometry object.
  SimulationData empty;
  empty.n_particles_ = 0;
  empty.cell_tracks = initial.cell_tracks;
  transport_particles(empty);
  REQUIRE((empty.cell_tracks == initial.cell_tracks));
}
#endif
