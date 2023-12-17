#ifndef _XDG_CONSTANTS
#define _XDG_CONSTANTS

#include <limits>

namespace xdg {

// Whether information pertains to a surface or volume
enum class GeometryType {
    SURFACE = 2,
    VOLUME = 3
};

// Surface to Volume sense values (may differ from mesh-specific values)
enum class Sense {
    SENSE_UNSET = -1,
    SENSE_FORWARD = 0,
    SENSE_REVERSE = 1
};

// Mesh identifer type
using MeshID = int32_t;

// Invalid
constexpr MeshID ID_NONE {-1};

// for abs(x) >= min_rcp_input the newton raphson rcp calculation does not fail
constexpr float min_rcp_input = std::numeric_limits<float>::min() /* FIX ME */ *1E5 /* SHOULDNT NEED TO MULTIPLY BY THIS VALUE */;
constexpr int BVH_MAX_DEPTH = 64;

}
#endif // include guard