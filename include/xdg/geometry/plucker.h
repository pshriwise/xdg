#ifndef _XDG_PLUCKER_H
#define _XDG_PLUCKER_H

#include <cmath>
#include <limits>

#include "dp_math.h"

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
  if (dp::DBL_ZERO_TOL > dp::abs(pip))  // <-- absd
    pip = 0.0;
  return pip;
}

inline PluckerIntersectionResult plucker_ray_tri_intersect(dp::vec3 vertices[3],
                                dp::vec3 origin,
                                dp::vec3 direction,
                                Scalar tMax,
                                Scalar tMin,
                                Scalar useOrientation,
                                int orientation)
{
  Scalar dist_out = dp::INFTY;

  const dp::vec3 raya = direction;
  const dp::vec3 rayb = dp::cross(direction, origin);

  // Determine the value of the first Plucker coordinate from edge 0
  Scalar plucker_coord0 =
    plucker_edge_test(vertices[0], vertices[1], raya, rayb);

  // If orientation is set, confirm that sign of plucker_coordinate indicate
  // correct orientation of intersection
  if (useOrientation && orientation * plucker_coord0 > 0) {
    return EXIT_EARLY;
  }

  // Determine the value of the second Plucker coordinate from edge 1
  Scalar plucker_coord1 =
    plucker_edge_test(vertices[1], vertices[2], raya, rayb);

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
    plucker_edge_test(vertices[2], vertices[0], raya, rayb);

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

  // get the distance to intersection
  const Scalar inverse_sum =
    1.0 / (plucker_coord0 + plucker_coord1 + plucker_coord2);

  const dp::vec3 intersection = dp::vec3(plucker_coord0 * inverse_sum * vertices[2] +
                                         plucker_coord1 * inverse_sum * vertices[0] +
                                         plucker_coord2 * inverse_sum * vertices[1]);

  // std::cout << "Plucker weights: " << plucker_coord0 * inverse_sum << ", "
  //           << plucker_coord1 * inverse_sum << ", "
  //           << plucker_coord2 * inverse_sum << std::endl;

  // To minimize numerical error, get index of largest magnitude direction.
  int idx = 0;
  Scalar max_abs_dir = 0;
  for (uint i = 0; i < 3; ++i) {
    if (std::abs(direction[i]) > max_abs_dir) {
      idx = i;
      max_abs_dir = std::abs(direction[i]);
    }
  }

  dist_out = (intersection[idx] - origin[idx]) / direction[idx];



  // Determine the floating-point resolution at the coordinate scale involved.
  Scalar coord_scale = std::abs(origin[idx]);

  coord_scale = std::max(coord_scale, std::abs(intersection[idx]));
  coord_scale = std::max(coord_scale, std::abs(vertices[0][idx]));
  coord_scale = std::max(coord_scale, std::abs(vertices[1][idx]));
  coord_scale = std::max(coord_scale, std::abs(vertices[2][idx]));

  const Scalar next_coord =
      std::nextafter(coord_scale,
                    std::numeric_limits<Scalar>::infinity());

  const Scalar coordinate_ulp = next_coord - coord_scale;

  // Allow a small number of ULPs for the weighted-sum reconstruction.
  const Scalar coordinate_tol = Scalar(2) * coordinate_ulp;
  const Scalar t_zero_tol =
      coordinate_tol / std::abs(direction[idx]);

  if (std::abs(dist_out) <= t_zero_tol) {
      dist_out = Scalar(0);
  }

  // Barycentric coords check
  Scalar u = plucker_coord2 * inverse_sum;
  Scalar v = plucker_coord0 * inverse_sum;

  // Barycentric coords check
  if (u < 0.0 || v < 0.0 || (u + v) > 1.0) {
      dist_out = -1.0;
  }

  // is the intersection within distance limits?
  if (dist_out < tMin || dist_out > tMax) return EXIT_EARLY;

  return {true, dist_out};
}

} // namespace xdg

#endif // include guard
