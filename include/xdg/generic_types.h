#ifndef _XDG_GENERIC_TYPES_H
#define _XDG_GENERIC_TYPES_H

#include "xdg/embree_interface.h"
#include <variant>



namespace xdg {

// Placeholder for GPRTAccel types until they are avaialable
struct GPRTAccel {}; // --> RTCScene [ TreeID ]
struct GPRTGeom {}; // --> RTCGeometry [ XdgGeometry ]
struct GPRTContext {}; // --> RTCDevice [ XdgDevice ]

// Struct to hold either RTCScene or GPRTAccel
struct TreeID {
    std::variant<RTCScene, GPRTAccel> _ID;

    // Default constructor
    TreeID() : _ID(nullptr) {}  // Initializes _ID with nullptr

    // Explicit constructors
    explicit TreeID(RTCScene scene) : _ID(scene) {}
    explicit TreeID(GPRTAccel accel) : _ID(accel) {}

    bool is_embree() const { return std::holds_alternative<RTCScene>(_ID); }
    bool is_gprt() const { return std::holds_alternative<GPRTAccel>(_ID); }

    RTCScene embree() { return std::get<RTCScene>(_ID); }
    GPRTAccel gprt() { return std::get<GPRTAccel>(_ID); }
};

// Struct to hold either RTCGeometry or GPRTGeom
struct XdgGeometry {
    std::variant<RTCGeometry, GPRTGeom> _geometry;

    // Default constructor
    XdgGeometry() : _geometry(nullptr) {}  // Initializes _geometry with nullptr

    // Explicit constructors
    explicit XdgGeometry(RTCGeometry rtc_geometry) : _geometry(rtc_geometry) {}
    explicit XdgGeometry(GPRTGeom gprt_geometry) : _geometry(gprt_geometry) {}

    bool is_embree() const { return std::holds_alternative<RTCGeometry>(_geometry); }
    bool is_gprt() const { return std::holds_alternative<GPRTGeom>(_geometry); }

    RTCGeometry embree() { return std::get<RTCGeometry>(_geometry); }
    GPRTGeom gprt() { return std::get<GPRTGeom>(_geometry); }
};


// // base TreeID
// template <typename T>
// struct TreeID {
//     T treeID;
// };

// // Type traits to check for different RT backends at compile time
// template <typename T>
// struct is_ray_tracing_scene : std::false_type {}; // Default to false to ensure that an RT backend is always set

// template <>
// struct is_ray_tracing_scene<RTCScene> : std::true_type {}; // Embree

// template <>
// struct is_ray_tracing_scene<GPRT> : std::true_type {}; // GPRT 

// template <>
// struct is_ray_tracing_scene<OptixScene> : std::true_type {}; // Optix


}

#endif