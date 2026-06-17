#ifndef _XDG_VEC3_H
#define _XDG_VEC3_H

#include <cmath>
#include <cstdlib>
#include <type_traits>

#include "xdg/vec3/vec3fa.h"
#include "xdg/vec3/vec3da.h"

namespace xdg {

#if defined(XDG_SINGLE_PRECISION)
    using Vertex    = Vec3fa;
    using Scalar    = float;
    using Position  = Vec3fa;
    using Direction = Vec3fa;
    constexpr bool IsSinglePrecision = true;
#else
    using Vertex    = Vec3da;
    using Scalar    = double;
    using Position  = Vec3da;
    using Direction = Vec3da;
    constexpr bool IsSinglePrecision = false;
#endif

inline Direction rand_dir() {
    using std::sin; using std::cos; using std::acos;

    Scalar theta = static_cast<Scalar>(drand48()) * 2.0 * M_PI;
    Scalar u     = 2.0 * static_cast<Scalar>(drand48()) - 1.0;
    Scalar phi   = acos(u);

    return Direction(sin(phi) * cos(theta), sin(phi) * sin(theta), cos(phi)).normalize();
}

template <typename T>
inline bool lower(const T& a, const T& b) {
    for (size_t i = 0; i < 3; ++i) {
        if (a[i] != b[i]) return a[i] < b[i];
    }
    return false;
}

} // end namespace xdg

namespace fmt {
    template<typename T>
    struct XdgVecFormatter : fmt::formatter<std::string> {
        template<typename FormatContext>
        auto format(const T& v, FormatContext& ctx) const {
            return fmt::formatter<std::string>::format(
                fmt::format("({}, {}, {})", v.x, v.y, v.z), ctx);
        }
    };

    template<> struct formatter<xdg::Vec3fa> : XdgVecFormatter<xdg::Vec3fa> {};
    template<> struct formatter<xdg::Vec3da> : XdgVecFormatter<xdg::Vec3da> {};
} // end namespace fmt

#endif // _XDG_VEC3_H
