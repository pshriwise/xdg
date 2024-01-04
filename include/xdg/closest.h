#ifndef _XDG_CLOSEST_H
#define _XDG_CLOSEST_H

#include <array>

#include "xdg/vec3da.h"

namespace xdg {

// FROM MOAB, but cleaned up considerably
Position closest_location_on_triangle(const std::array<Position, 3>& vertices,
                                      const Position& point);

} // namespace xdg

#endif // include guard