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
    UNSET = -1,
    FORWARD = 0,
    REVERSE = 1
};

// Mesh identifer type
using MeshID = int32_t;

// Invalid
constexpr MeshID ID_NONE {-1};

// for abs(x) >= min_rcp_input the newton raphson rcp calculation does not fail
constexpr float min_rcp_input = std::numeric_limits<float>::min() /* FIX ME */ *1E5 /* SHOULDNT NEED TO MULTIPLY BY THIS VALUE */;
constexpr int BVH_MAX_DEPTH = 64;

// geometric property type (e.g. material assignment or boundary condition)
enum class PropertyType {
    BOUNDARY_CONDITION = -1,
    MATERIAL = 0,
    DENSITY = 1,
    TEMPERATURE = 2
};

struct Property{
  PropertyType type;
  std::string value;
};


}
#endif // include guard