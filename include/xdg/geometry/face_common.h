#ifndef XDG_GEOMETRY_TRIANGLE_INTERSECT_H
#define XDG_GEOMETRY_TRIANGLE_INTERSECT_H

#include <array>
#include "xdg/vec3da.h"

namespace xdg {

inline Direction triangle_normal(const std::array<Vertex, 3>& vertices) {
  const Position v1 = vertices[1] - vertices[0];
  const Position v2 = vertices[2] - vertices[0];
  return (v1.cross(v2)).normalize();
}

template<typename T>
bool quad_diagonal_selection(const T& verts) {
    auto pair_less = [](const Vertex& a0, const Vertex& a1,
                        const Vertex& b0, const Vertex& b1) {
    const Vertex& a_min = lower(a0, a1) ? a0 : a1;
    const Vertex& a_max = lower(a0, a1) ? a1 : a0;
    const Vertex& b_min = lower(b0, b1) ? b0 : b1;
    const Vertex& b_max = lower(b0, b1) ? b1 : b0;

    if (lower(a_min, b_min)) return true;
    if (lower(b_min, a_min)) return false;
    return lower(a_max, b_max);
  };

  return pair_less(verts[0], verts[2], verts[1], verts[3]);
}

} // namespace xdg

#endif // XDG_GEOMETRY_TRIANGLE_INTERSECT_H