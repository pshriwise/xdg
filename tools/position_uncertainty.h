#ifndef XDG_PARTICLE_POSITION_UNCERTAINTY_H
#define XDG_PARTICLE_POSITION_UNCERTAINTY_H

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

#include "xdg/vec3/vec3.h"

namespace xdg {
namespace particle_sim_detail {

// All endpoints and arithmetic use Scalar, including in single precision.
// Round each operation outward to enclose both the exact result and the
// rounded position that transport will store. Requires ordinary IEEE arithmetic
// without reassociation, and finite, nonnegative flight distances.
struct Interval {
  Scalar lower;
  Scalar upper;
};

inline Scalar round_down(Scalar value)
{
  return std::nextafter(value, -std::numeric_limits<Scalar>::infinity());
}

inline Scalar round_up(Scalar value)
{
  return std::nextafter(value, std::numeric_limits<Scalar>::infinity());
}

inline Interval entire_interval()
{
  return {-std::numeric_limits<Scalar>::infinity(),
           std::numeric_limits<Scalar>::infinity()};
}

inline Interval exact(Scalar value) { return {value, value}; }

inline bool is_zero(Interval value)
{
  return value.lower == Scalar(0) && value.upper == Scalar(0);
}

inline Interval add(Interval a, Interval b)
{
  if (is_zero(a)) return b;
  if (is_zero(b)) return a;
  const Scalar lower = a.lower + b.lower;
  const Scalar upper = a.upper + b.upper;
  if (std::isnan(lower) || std::isnan(upper)) return entire_interval();
  return {round_down(lower), round_up(upper)};
}

inline Interval negate(Interval value)
{
  return {-value.upper, -value.lower};
}

inline Interval subtract(Interval a, Interval b)
{
  if (a.lower == a.upper && b.lower == b.upper && a.lower == b.lower &&
      std::isfinite(a.lower)) return exact(Scalar(0));
  return add(a, negate(b));
}

inline Interval multiply(Interval a, Interval b)
{
  if (is_zero(a) || is_zero(b)) return exact(Scalar(0));
  const std::array<Scalar, 4> products {
    a.lower * b.lower, a.lower * b.upper,
    a.upper * b.lower, a.upper * b.upper
  };
  for (Scalar product : products) {
    if (std::isnan(product)) return entire_interval();
  }
  const auto limits = std::minmax_element(products.begin(), products.end());
  return {round_down(*limits.first), round_up(*limits.second)};
}

using VectorBounds = std::array<Interval, 3>;

inline VectorBounds collision_position_bounds(Position origin, Direction direction,
                                              Scalar current, Scalar step)
{
  // Neighboring Scalar values also enclose the original sampled distance
  // before its conversion to Scalar. A collision advances a nonnegative step.
  const Interval distance {std::max(Scalar(0), round_down(step)), round_up(step)};
  Interval parameter = add(exact(current), distance);
  parameter.lower = std::max(current, parameter.lower);

  VectorBounds position;
  for (int i = 0; i < 3; ++i) {
    position[i] = add(exact(origin[i]), multiply(parameter, exact(direction[i])));
  }
  return position;
}

inline VectorBounds triangle_normal_bounds(const std::array<Vertex, 3>& vertices)
{
  VectorBounds a, b, normal;
  for (int i = 0; i < 3; ++i) {
    a[i] = subtract(exact(vertices[1][i]), exact(vertices[0][i]));
    b[i] = subtract(exact(vertices[2][i]), exact(vertices[0][i]));
  }
  for (int i = 0; i < 3; ++i) {
    const int j = (i + 1) % 3;
    const int k = (i + 2) % 3;
    normal[i] = subtract(multiply(a[j], b[k]), multiply(a[k], b[j]));
  }
  return normal;
}

inline Interval interior_clearance(const std::array<Vertex, 3>& vertices,
                                   const VectorBounds& position, bool reverse)
{
  auto normal = triangle_normal_bounds(vertices);
  Interval clearance = exact(Scalar(0));
  for (int i = 0; i < 3; ++i) {
    if (reverse) normal[i] = negate(normal[i]);
    clearance = add(clearance, multiply(normal[i],
        subtract(exact(vertices[0][i]), position[i])));
  }
  // Positive is the interior side of the outward-oriented face. Avoid normal
  // normalization and division by a nearly tangent ray's normal dot direction.
  return clearance;
}

} // namespace particle_sim_detail
} // namespace xdg

#endif // XDG_PARTICLE_POSITION_UNCERTAINTY_H
