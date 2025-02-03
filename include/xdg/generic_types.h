#ifndef _XDG_GENERIC_TYPES_H
#define _XDG_GENERIC_TYPES_H

#include "xdg/embree_interface.h"




namespace xdg {


using TreeID = RTCScene;
using XdgDevice = RTCDevice;
using XdgGeometry = RTCGeometry;

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
// struct is_ray_tracing_scene<GPRTScene> : std::true_type {}; // GPRT 

// template <>
// struct is_ray_tracing_scene<OptixScene> : std::true_type {}; // Optix


}

#endif