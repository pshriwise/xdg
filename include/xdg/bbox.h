#ifndef _XDG_BOUNDING_BOX_H
#define _XDG_BOUNDING_BOX_H

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
  };
} // namespace xdg

#endif // include guard