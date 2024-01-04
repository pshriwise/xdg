#ifndef _XDG_CLOSEST_H
#define _XDG_CLOSEST_H

#include <array>

#include "xdg/vec3da.h"

namespace xdg {

enum class Region {
  Zero,
  One,
  Two,
  Three,
  Four,
  Five,
  Six
};

Region determine_region(std::array<Position, 3> triangle, Position point);

// FROM MOAB, but cleaned up considerably
Position closest_location_on_triangle(const std::array<Position, 3>& vertices,
                                      const Position& point);

} // namespace xdg

#endif // include guard