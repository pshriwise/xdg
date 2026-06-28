
#ifndef _XDG_RAY_H
#define _XDG_RAY_H

#include <set>
#include <vector>

#include "xdg/vec3/vec3.h"

#include "xdg/constants.h"
#include "xdg/embree_interface.h"
#include "xdg/primitive_ref.h"

namespace xdg {

// forward declaration
class TriangleRef;

// TO-DO: there should be a few more double elements here (barycentric coords)

/*! Stucture that is an extension of Embree's RTCRay with
    double precision versions of the origin, direction
    and intersection distance.
 */
struct RTCDualRay : RTCRay {

  RTCDualRay() {
    this->tnear = 0.0;
    this->tfar = INFTY;
    this->mask = -1;
  }

  //! \brief Set both the single and double precision versions of the ray origin
  void set_org(double o[3]) {
    org_x = o[0]; org_y = o[1]; org_z = o[2];
    dorg_x = o[0]; dorg_y = o[1]; dorg_z = o[2];
  }

  //! \brief Set both the single and double precision versions of the ray origin
  void set_org(const double o[3]) {
    org_x = o[0]; org_y = o[1]; org_z = o[2];
    dorg_x = o[0]; dorg_y = o[1]; dorg_z = o[2];
  }

  //! \brief Set both the single and double precision versions of the ray origin
  void set_org(const Position& o) {
    org_x = o[0]; org_y = o[1]; org_z = o[2];
    dorg_x = o[0]; dorg_y = o[1]; dorg_z = o[2];
  }

  //! \brief Set both the single and double precision versions of the ray direction
  void set_dir(const Direction& o) {
    dir_x = o[0]; dir_y = o[1]; dir_z = o[2];
    ddir_x = o[0]; ddir_y = o[1]; ddir_z = o[2];
  }

  //! \brief Get ray origin
  Position get_org() const {
    if constexpr (IsSinglePrecision) {
      return Position(org_x, org_y, org_z);
    }
    return Position(dorg_x, dorg_y, dorg_z);
  }

  //! \brief Set both the single and double precision versions of the ray direction
  void set_dir(double o[3]) {
    dir_x = o[0]; dir_y = o[1]; dir_z = o[2];
    ddir_x = o[0]; ddir_y = o[1]; ddir_z = o[2];
  }

  //! \brief Set both the single and double precision versions of the ray direction
  void set_dir(const double o[3]) {
    dir_x = o[0]; dir_y = o[1]; dir_z = o[2];
    ddir_x = o[0]; ddir_y = o[1]; ddir_z = o[2];
  }

  //! \brief Get ray direction
  Direction get_dir() const {
    if constexpr (IsSinglePrecision) {
      return Direction(dir_x, dir_y, dir_z);
    }
    return Direction(ddir_x, ddir_y, ddir_z);
  }

  //! \brief Set both the single and double precision versions of the ray max distance
  void set_tfar(Scalar d) {
    tfar = std::min(static_cast<Scalar>(d), INFTY);
    dtfar = d;
  }

  //! \brief Get ray max distance (both precisions valid)
  Scalar get_tfar() const {
    if constexpr (IsSinglePrecision) {
      return tfar;
    }
    return dtfar;
  }

  //! \brief Set both the single and double precision versions of the ray near distance
  void set_tnear(Scalar d) {
    tnear = d;
  }

  //! \brief Get ray near distance (both precisions valid)
  Scalar get_tnear() const {
    return tnear;
  }

  double dorg_x, dorg_y, dorg_z, ddir_x, ddir_y, ddir_z; //!< double precision versions of the origin and ray direction
  double dtfar; //!< double precision version of the ray far distance
};


struct RTCSurfaceDualRay : RTCDualRay {

  // Member variables
  RayFireType rf_type {RayFireType::VOLUME}; //!< Enum indicating the type of query this ray is used for
  HitOrientation orientation {HitOrientation::EXITING}; //!< Enum indicating what hits to accept based on orientation
  const std::vector<MeshID>* exclude_primitives {nullptr}; //! < Set of primitives to exclude from the query
  TreeID volume_tree {ID_NONE}; // volume the ray is being fired in
};

struct RTCElementDualRay : RTCDualRay {
  RTCElementDualRay() {
    this->element = ID_NONE;
  }

  // Member variables
  MeshID element; //!< ID of the element this ray is associated with
};

/*! Structure extending Embree's RayHit to include a double precision version of the primitive normal */
struct RTCDualHit : RTCHit {
  RTCDualHit() {
    this->geomID = RTC_INVALID_GEOMETRY_ID;
    this->primID = RTC_INVALID_GEOMETRY_ID;
    this->Ng_x = 0.0;
    this->Ng_y = 0.0;
    this->Ng_z = 0.0;
  }

  //! \brief Set primitive normal
  void set_Ng(const Direction& n) {
    Ng_x = n[0]; Ng_y = n[1]; Ng_z = n[2];
    dNg_x = n[0]; dNg_y = n[1]; dNg_z = n[2];
  }

  //! \brief Get primitive normal
  Direction get_Ng() const {
    if constexpr (IsSinglePrecision) {
      return Direction(Ng_x, Ng_y, Ng_z);
    }
    return Direction(dNg_x, dNg_y, dNg_z);
  }

  // data members
  const PrimitiveRef* primitive_ref {nullptr}; //!< Pointer to the primitive reference for this hit
  MeshID surface {ID_NONE}; //!< ID of the surface this hit belongs to
  double dNg_x, dNg_y, dNg_z; //!< Double precision version of the primitive normal
};

/*! Stucture combining the ray and ray-hit structures to be passed to Embree queries */
struct RTCDualRayHit {
  struct RTCSurfaceDualRay ray; //<! Extended version of the Embree RTCRay struct with double precision values
  struct RTCDualHit hit; //<! Extended version of the Embree RTDRayHit struct with double precision values

  //! \brief Compute the dot product of the ray direction and current hit normal
  Scalar dot_prod() {
    return dot(ray.get_dir(), hit.get_Ng());
  }

};

/*! Structure extending Embree's RTCPointQuery to include double precision values */
struct RTCDPointQuery : RTCPointQuery {

  RTCDPointQuery() {
    this->set_radius(INFTY);
    this->time = 0.0;
  }

  //! \brief Set both the single and double precision versions of the query radius
  void set_radius(double rad) {
    radius = std::min(static_cast<Scalar>(rad), INFTY);
    dradius = rad;
  }

  //! \brief Get the query radius (both precisions valid)
  Scalar get_radius() const {
    if constexpr (IsSinglePrecision) {
      return radius;
    }
    return dradius;
  }

  //! \brief Set both the single and double precision versions of the query location
  void set_point(const double xyz[3]) {
    x = xyz[0]; y = xyz[1]; z = xyz[2];
    dbl_x = xyz[0]; dbl_y = xyz[1]; dbl_z = xyz[2];
  }

  void set_point(const Position& xyz) {
    x = xyz[0]; y = xyz[1]; z = xyz[2];
    dbl_x = xyz[0]; dbl_y = xyz[1]; dbl_z = xyz[2];
  }

  //! \brief Get the query location
  Position get_point() const {
    if constexpr (IsSinglePrecision) {
      return Position(x, y, z);
    }
    return Position(dbl_x, dbl_y, dbl_z);
  }

  unsigned int primID = RTC_INVALID_GEOMETRY_ID; //<! ID of the nearest primitive
  unsigned int geomID = RTC_INVALID_GEOMETRY_ID; //<! ID of the nearest geometry
  double dbl_x, dbl_y, dbl_z; //<! Double precision version of the query location
  const PrimitiveRef* primitive_ref {nullptr}; //!< Pointer to the primitive reference for this hit
  double dradius; //!< Double precision version of the query distance
};

} // namespace xdg

#endif // include guard
