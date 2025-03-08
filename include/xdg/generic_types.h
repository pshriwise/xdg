#ifndef _XDG_GENERIC_TYPES_H
#define _XDG_GENERIC_TYPES_H

#include "xdg/embree_interface.h"
#include <variant>
#include <functional>
#include <cstdint>

namespace xdg {

// Placeholder for GPRTAccel types until they are available
struct GPRTAccel {
  bool operator==(const GPRTAccel&) const noexcept {
    return true; // Since GPRTAccel is a placeholder, all instances are considered equal
  }
}; // --> RTCScene [ TreeID ]

struct GPRTGeom {
  bool operator==(const GPRTGeom&) const noexcept {
    return true; // Since GPRTGeom is a placeholder, all instances are considered equal
  }
}; // --> RTCGeometry [ XdgGeometry ]

struct GPRTContext {}; // --> RTCDevice [ XdgDevice ] TODO: is this needed?

// Scene/Tree ID
using TreeID = int32_t;

// Invalid
constexpr TreeID TREEID_NONE {-1};
}

#endif