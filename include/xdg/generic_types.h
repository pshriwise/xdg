#ifndef _XDG_GENERIC_TYPES_H
#define _XDG_GENERIC_TYPES_H

#include "xdg/embree_interface.h"
#include <variant>

namespace xdg {

// Placeholder for GPRTAccel types until they are avaialable
struct GPRTAccel {}; // --> RTCScene [ TreeID ]
struct GPRTGeom {}; // --> RTCGeometry [ XdgGeometry ]
struct GPRTContext {}; // --> RTCDevice [ XdgDevice ] TODO: is this needed?

// Struct to hold either RTCScene or GPRTAccel
struct TreeID {
  std::variant<std::nullptr_t, RTCScene, GPRTAccel> _ID;

  // Default constructor
  TreeID() : _ID(nullptr) {}  // Initializes _ID with nullptr

  // Explicit constructors
  explicit TreeID(RTCScene scene) : _ID(scene) {}
  explicit TreeID(GPRTAccel accel) : _ID(accel) {}

  // Variant Type checks
  bool is_embree() const { return std::holds_alternative<RTCScene>(_ID); } // return True if _ID RTCScene
  bool is_gprt() const { return std::holds_alternative<GPRTAccel>(_ID); } // return True if _ID GPRTAccel

  // TODO: Should these accessors make use of std::get_if<T> instead of std::get<T>?
  
  // Accessors 
  RTCScene embree() { return std::get<RTCScene>(_ID); } // return the underlying RTCScene
  GPRTAccel gprt() { return std::get<GPRTAccel>(_ID); } // return the underlying GPRTAccel
};

// Struct to hold either RTCGeometry or GPRTGeom
struct XdgGeometry {
  std::variant<std::nullptr_t, RTCGeometry, GPRTGeom> _geometry;

  // Default constructor
  XdgGeometry() : _geometry(nullptr) {}  // Initializes _geometry with nullptr

  // Explicit constructors
  explicit XdgGeometry(RTCGeometry rtc_geometry) : _geometry(rtc_geometry) {}
  explicit XdgGeometry(GPRTGeom gprt_geometry) : _geometry(gprt_geometry) {}

  // Variant Type checks
  bool is_embree() const { return std::holds_alternative<RTCGeometry>(_geometry); } // return True if _geometry RTCGeometry
  bool is_gprt() const { return std::holds_alternative<GPRTGeom>(_geometry); }      // return True if _geometry GPRTGeom

  // TODO: Should these accessors make use of std::get_if<T> instead of std::get<T>?

  // Accessors 
  RTCGeometry embree() { return std::get<RTCGeometry>(_geometry); } // return the underlying RTCGeometry
  GPRTGeom gprt() { return std::get<GPRTGeom>(_geometry); }         // return the underlying GPRTGeom
};


}

#endif