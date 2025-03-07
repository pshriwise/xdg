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

  // Const Accessors
  RTCScene embree() const { return std::get<RTCScene>(_ID); } // return the underlying RTCScene 
  GPRTAccel gprt() const { return std::get<GPRTAccel>(_ID); } // return the underlying GPRTAccel 

  // Non-const Accessors
  RTCScene& embree() { return std::get<RTCScene>(_ID); } // return the underlying RTCScene 
  GPRTAccel& gprt() { return std::get<GPRTAccel>(_ID); } // return the underlying GPRTAccel

  // Equality Operator (useful for hash function definition)
  bool operator==(const TreeID& other) const { return _ID == other._ID; }
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

  // Accessors 
  RTCGeometry embree() { return std::get<RTCGeometry>(_geometry); } // return the underlying RTCGeometry
  GPRTGeom gprt() { return std::get<GPRTGeom>(_geometry); }         // return the underlying GPRTGeom

  // Equality Operator (useful for hash function definition)
  bool operator==(const XdgGeometry& other) const { return _geometry == other._geometry; }
};

}; // namespace xdg

namespace std {
// Specialize std::hash for the empty struct GPRTAccel
template <>
struct hash<xdg::GPRTAccel> {
  size_t operator()(const xdg::GPRTAccel& gprt_accel) const noexcept {
    // Use the address of the object as the hash value
    return std::hash<const void*>{}(static_cast<const void*>(&gprt_accel)); 
  }
};

// Specialize std::hash for TreeID to allow TreeID to be used as a key
template <>
struct hash<xdg::TreeID> {
  size_t operator()(const xdg::TreeID& treeID) const noexcept {
    if (treeID.is_embree()) {
        return std::hash<RTCScene>{}(treeID.embree());
    } else if (treeID.is_gprt()) {
        // Since GPRTAccel is still a placeholder we need to hash based on its address (or uintptr_t)
        return std::hash<xdg::GPRTAccel>{}(treeID.gprt());
    } else {
        return 0; // Handle nullptr case
    }
  }
};

}; // namespace std

#endif