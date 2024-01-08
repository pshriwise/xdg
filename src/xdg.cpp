#include <vector>

#include "xdg/xdg.h"

#include "xdg/constants.h"
#include "xdg/geometry/area.h"


namespace xdg {

double XDG::measure_volume(MeshID volume) const
{
  double volume_total {0.0};

  auto surfaces = mesh_manager()->get_volume_surfaces(volume);

  std::vector<Sense> surface_senses;
  for (auto surface : surfaces) {
    surface_senses.push_back(mesh_manager()->surface_sense(surface, volume));
  }

  for (int i = 0; i < surfaces.size(); ++i) {
    MeshID& surface = surfaces[i];
    double surface_contribution {0.0};
    auto triangles = mesh_manager()->get_surface_elements(surface);
    for (auto triangle : triangles) {
      surface_contribution += triangle_volume_contribution(mesh_manager()->triangle_vertices(triangle));
    }
    if (surface_senses[i] == Sense::REVERSE) surface_contribution *= -1.0;
    volume_total += surface_contribution;
  }

  return volume_total / 6.0;
}

double XDG::measure_surface_area(MeshID surface) const
{
  double area {0.0};
  for (auto triangle : mesh_manager()->get_surface_elements(surface)) {
    area += triangle_area(mesh_manager()->triangle_vertices(triangle));
  }
  return area;
}

double XDG::measure_volume_area(MeshID volume) const
{
  double area {0.0};
  for (auto surface : mesh_manager()->get_volume_surfaces(volume)) {
    area += measure_surface_area(surface);
  }
  return area;
}

} // namespace xdg