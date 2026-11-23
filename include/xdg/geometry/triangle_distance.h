#ifndef XDG_TRIANGLE_DISTANCE_H
#define XDG_TRIANGLE_DISTANCE_H

#include "dp_math.h"

namespace xdg {
namespace detail {

// Keep rounding residuals in the configured scalar type. In a single-precision
// build both fields and every arithmetic operation remain float.
struct CompensatedScalar {
  dp::scalar value;
  dp::scalar error;
};

inline dp::scalar sum_error(dp::scalar a, dp::scalar b, dp::scalar sum)
{
  const dp::scalar b_virtual = sum - a;
  return (a - (sum - b_virtual)) + (b - b_virtual);
}

inline CompensatedScalar compensated_difference(dp::scalar a, dp::scalar b)
{
  const dp::scalar value = a - b;
  return {value, sum_error(a, -b, value)};
}

inline CompensatedScalar compensated_add(CompensatedScalar a,
                                         CompensatedScalar b)
{
  const dp::scalar sum = a.value + b.value;
  const dp::scalar error = sum_error(a.value, b.value, sum) + (a.error + b.error);
  const dp::scalar value = sum + error;
  return {value, (sum - value) + error};
}

inline CompensatedScalar compensated_negate(CompensatedScalar a)
{
  return {-a.value, -a.error};
}

inline CompensatedScalar compensated_multiply(CompensatedScalar a,
                                              CompensatedScalar b)
{
  const dp::scalar product = a.value * b.value;
  dp::scalar error = dp::fma(a.value, b.value, -product);
  error = dp::fma(a.value, b.error, error);
  error = dp::fma(a.error, b.value, error);
  const dp::scalar value = product + error;
  return {value, (product - value) + error};
}

inline dp::scalar compensated_quotient(CompensatedScalar numerator,
                                      CompensatedScalar denominator)
{
  const dp::scalar t = numerator.value / denominator.value;
  dp::scalar residual = dp::fma(-t, denominator.value, numerator.value);
  residual += numerator.error - t * denominator.error;
  return t + residual / denominator.value;
}

// Evaluate dot(cross(v1-v0, v2-v0), v0-origin) /
//          dot(cross(v1-v0, v2-v0), direction).
// Preserve residuals through the differences, cross/dot products, and quotient;
// rounding the normal to one scalar per component loses accuracy for short hits.
// These error-free transforms require ordinary IEEE arithmetic without algebraic
// reassociation or implicit multiply-add contraction; FMA is used explicitly.
inline dp::scalar compensated_triangle_distance(dp::vec3 vertices[3],
                                                dp::vec3 origin,
                                                dp::vec3 direction)
{
  CompensatedScalar edges[2][3], offset[3], normal[3];
  for (int i = 0; i < 3; ++i) {
    edges[0][i] = compensated_difference(vertices[1][i], vertices[0][i]);
    edges[1][i] = compensated_difference(vertices[2][i], vertices[0][i]);
    offset[i] = compensated_difference(vertices[0][i], origin[i]);
  }

  for (int i = 0; i < 3; ++i) {
    const int j = (i + 1) % 3;
    const int k = (i + 2) % 3;
    normal[i] = compensated_add(compensated_multiply(edges[0][j], edges[1][k]),
        compensated_negate(compensated_multiply(edges[0][k], edges[1][j])));
  }

  CompensatedScalar numerator = {0, 0};
  CompensatedScalar denominator = {0, 0};
  for (int i = 0; i < 3; ++i) {
    numerator = compensated_add(numerator, compensated_multiply(normal[i], offset[i]));
    denominator = compensated_add(denominator,
        compensated_multiply(normal[i], {direction[i], 0}));
  }
  return compensated_quotient(numerator, denominator);
}

} // namespace detail
} // namespace xdg

#endif // XDG_TRIANGLE_DISTANCE_H
