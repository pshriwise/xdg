#ifndef _XDG_CONSTANTS
#define _XDG_CONSTANTS

#include <limits>

namespace xdg {

using MeshID=uint32_t;

// for abs(x) >= min_rcp_input the newton raphson rcp calculation does not fail
constexpr float min_rcp_input = std::numeric_limits<float>::min() /* FIX ME */ *1E5 /* SHOULDNT NEED TO MULTIPLY BY THIS VALUE */;
constexpr int BVH_MAX_DEPTH = 64;

}
#endif // include guard