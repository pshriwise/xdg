#ifndef _XDG_PLUCKER_H
#define _XDG_PLUCKER_H

#include <cmath>
#include <limits>

#include "dp_math.h"
#include "triangle_distance.h"

namespace xdg {

/*
 * Triangle vertex ordering convention:
 *
 *      v2
 *     /  \
 *    /    \
 *   /      \
 *  /        \
 * v0--------v1
 *
 * The vertices are ordered counter-clockwise when viewed from the front face
 * (normal pointing out of the plane). This ordering is based on the reference:
 * https://doi.org/10.1002/cnm.1237
 */

struct PluckerIntersectionResult {
  bool hit = false;    // Whether an intersection occurred
  Scalar t = 0.0;      // Distance along the ray to the intersection point
};

static constexpr PluckerIntersectionResult EXIT_EARLY = {false, 0.0};

/* Function to return the vertex with the lowest coordinates. To force the same
  ray-edge computation, the Plücker test needs to use consistent edge
  representation. This would be more simple with MOAB handles instead of
  coordinates...
*/
inline bool first(dp::vec3 a, dp::vec3 b) {
  if (a[0] < b[0]) return true;
  if (a[0] > b[0]) return false;

  if (a[1] < b[1]) return true;
  if (a[1] > b[1]) return false;

  return a[2] < b[2];
}

inline Scalar plucker_edge_test(dp::vec3 vertexa, dp::vec3 vertexb,
                                dp::vec3 ray, dp::vec3 ray_normal)
{
  Scalar pip;
  if (first(vertexa, vertexb)) {
    const dp::vec3 edge = vertexb - vertexa;
    const dp::vec3 edge_normal = dp::cross(edge, vertexa);
    pip = dp::dot(ray, edge_normal) + dp::dot(ray_normal, edge);
  } else {
    const dp::vec3 edge = vertexa - vertexb;
    const dp::vec3 edge_normal = dp::cross(edge, vertexb);
    pip = dp::dot(ray, edge_normal) + dp::dot(ray_normal, edge);
    pip = -pip;
  }
  // if (dp::DBL_ZERO_TOL > dp::abs(pip))  // <-- absd
  //   pip = 0.0;
  return pip;
}

namespace detail {

// Evaluate dot(direction, cross(b - a, a - origin)), retaining the
// subtraction, multiplication, and addition residuals in Scalar pairs.
inline Scalar compensated_plucker_edge(dp::vec3 a, dp::vec3 b,
                                       dp::vec3 origin, dp::vec3 direction)
{
  // A shared edge follows the same arithmetic in both adjacent triangles.
  const bool reverse = !first(a, b);
  if (reverse) {
    const dp::vec3 temporary = a;
    a = b;
    b = temporary;
  }

  CompensatedScalar edge[3], offset[3], normal[3];
  for (int i = 0; i < 3; ++i) {
    edge[i] = compensated_difference(b[i], a[i]);
    offset[i] = compensated_difference(a[i], origin[i]);
  }
  for (int i = 0; i < 3; ++i) {
    const int j = (i + 1) % 3;
    const int k = (i + 2) % 3;
    normal[i] = compensated_add(
        compensated_multiply(edge[j], offset[k]),
        compensated_negate(compensated_multiply(edge[k], offset[j])));
  }

  CompensatedScalar product = {0, 0};
  for (int i = 0; i < 3; ++i) {
    product = compensated_add(product,
        compensated_multiply(normal[i], {direction[i], 0}));
  }
  const Scalar result = product.value + product.error;
  return reverse ? -result : result;
}

} // namespace detail

inline PluckerIntersectionResult plucker_ray_tri_intersect(dp::vec3 verts[3],
                                dp::vec3 origin,
                                dp::vec3 direction,
                                Scalar tMax,
                                Scalar tMin,
                                Scalar useOrientation,
                                int orientation)
{
  Scalar dist_out = dp::INFTY;

  // Determine the value of the first Plucker coordinate from edge 0
  Scalar plucker_coord0 =
    detail::compensated_plucker_edge(verts[0], verts[1], origin, direction);

  // If orientation is set, confirm that sign of plucker_coordinate indicate
  // correct orientation of intersection
  if (useOrientation && orientation * plucker_coord0 > 0) {
    return EXIT_EARLY;
  }

  // Determine the value of the second Plucker coordinate from edge 1
  Scalar plucker_coord1 =
    detail::compensated_plucker_edge(verts[1], verts[2], origin, direction);

  // If orientation is set, confirm that sign of plucker_coordinate indicate
  // correct orientation of intersection
  if (useOrientation) {
    if (orientation * plucker_coord1 > 0) {
      return EXIT_EARLY;
    }
    // If the orientation is not specified, all plucker_coords must be the same
    // sign or zero.
  } else if ((0.0 < plucker_coord0 && 0.0 > plucker_coord1) ||
             (0.0 > plucker_coord0 && 0.0 < plucker_coord1)) {
    return EXIT_EARLY;
  }

  // Determine the value of the third Plucker coordinate from edge 2
  Scalar plucker_coord2 =
    detail::compensated_plucker_edge(verts[2], verts[0], origin, direction);

  // If orientation is set, confirm that sign of plucker_coordinate indicate
  // correct orientation of intersection
  if (useOrientation) {
    if (orientation * plucker_coord2 > 0) {
      return EXIT_EARLY;
    }
    // If the orientation is not specified, all plucker_coords must be the same
    // sign or zero.
  } else if ((0.0 < plucker_coord1 && 0.0 > plucker_coord2) ||
             (0.0 > plucker_coord1 && 0.0 < plucker_coord2) ||
             (0.0 < plucker_coord0 && 0.0 > plucker_coord2) ||
             (0.0 > plucker_coord0 && 0.0 < plucker_coord2)) {
    return EXIT_EARLY;
  }

  // check for coplanar case to avoid dividing by zero
  if (0.0 == plucker_coord0 && 0.0 == plucker_coord1 && 0.0 == plucker_coord2) {
    return EXIT_EARLY;
  }

  // Retain the Plucker weights for the existing barycentric checks. Evaluate t
  // separately so errors in their magnitudes do not distort the hit distance.
  const Scalar inverse_sum =
    1.0 / (plucker_coord0 + plucker_coord1 + plucker_coord2);

  dist_out = detail::compensated_triangle_distance(verts, origin, direction);

  if (std::abs(dist_out) <= PLUCKER_ZERO_TOL) {
      dist_out = Scalar(0);
  }

  // Barycentric coords check
  Scalar u = plucker_coord2 * inverse_sum;
  Scalar v = plucker_coord0 * inverse_sum;

  // // Barycentric coords check
  // if (u < 0.0 || v < 0.0 || (u + v) > 1.0) {
  //     dist_out = -1.0;
  // }

  // Require an ordered distance in range; also reject NaN from a singular plane.
  if (!(dist_out >= tMin && dist_out <= tMax)) return EXIT_EARLY;

  return {true, dist_out};
}

} // namespace xdg

#endif // include guard
