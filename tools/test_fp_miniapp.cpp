#include <chrono>

#include "particle_sim.h"

int main(int argc, char** argv) {
    SimulationData sim;
    sim.xdg_ = XDG::create(MeshLibrary::MOAB, RTLibrary::EMBREE);
    sim.xdg_->mesh_manager()->load_file(argv[1]);
    sim.xdg_->mesh_manager()->init();
    sim.xdg_->prepare_raytracer();
    
    auto surfaces = sim.xdg_->mesh_manager()->get_volume_surfaces(1);
    int max_states = 1000;
    int count = 0;
    for (auto surf : surfaces) {
        if (count >= max_states) break;
        auto faces = sim.xdg_->mesh_manager()->get_surface_faces(surf);
        for (auto face : faces) {
            if (count >= max_states) break;
            auto verts = sim.xdg_->mesh_manager()->face_vertices(face);
            Position centroid = (verts[0] + verts[1] + verts[2]) * (1.0f/3.0f);
            MeshID vol = sim.xdg_->find_volume(centroid, {1.0, 0.0, 0.0});
            if (vol != ID_NONE) {
                sim.valid_states.push_back({centroid, vol});
                count++;
            }
        }
    }

    sim.n_particles_ = 1000;
    sim.implicit_complement_is_graveyard_ = true;

    auto start = std::chrono::high_resolution_clock::now();
    transport_particles(sim);
    auto end = std::chrono::high_resolution_clock::now();

    double elapsed = std::chrono::duration<double>(end - start).count();
    fmt::print("Stuck: {} | Lost: {} | Time: {:.2f}s | Rate: {:.0f} p/s\n",
               sim.n_stuck, sim.n_lost, elapsed,
               sim.n_particles_ / elapsed);

    return 0;
}