#ifndef _XDG_BOUNDING_BOX_H
#define _XDG_BOUNDING_BOX_H

#include "xdg/vec3da.h"
namespace xdg {
union BoundingBox {
struct {
    double min_x;
    double min_y;
    double min_z;
    double max_x;
    double max_y;
    double max_z;
};
double bounds[6];

void update(const Vertex& v) {
    min_x = std::min(min_x, v.x);
    min_y = std::min(min_y, v.y);
    min_z = std::min(min_z, v.z);
    max_x = std::max(max_x, v.x);
    max_y = std::max(max_y, v.y);
    max_z = std::max(max_z, v.z);
}

void update(const std::array<double, 3>& v) {
    min_x = std::min(min_x, v[0]);
    min_y = std::min(min_y, v[1]);
    min_z = std::min(min_z, v[2]);
    max_x = std::max(max_x, v[0]);
    max_y = std::max(max_y, v[1]);
    max_z = std::max(max_z, v[2]);
}

};
} // namespace xdg

#endif // include guard