
#ifndef _XDG_RAY_H
#define _XDG_RAY_H

#include <set>
#include <vector>

#include "xdg/vec3da.h"

#include "xdg/constants.h"
#include "xdg/embree_interface.h"

namespace xdg {

// forward declaration
class TriangleRef;

// TO-DO: there should be a few more double elements here (barycentric coords)

/*! Stucture that is an extension of Embree's RTCRay with
    double precision versions of the origin, direction
    and intersection distance.
 */
struct RTCDRay: RTCRay {

  RTCDRay() {
    this->tnear = 0.0;
    this->tfar = INFTYF;
    this->mask = -1;
  }

  //! \brief Set both the single and double precision versions of the ray origin
  void set_org(double o[3]) {
    org_x = o[0]; org_y = o[1]; org_z = o[2];
    dorg[0] = o[0]; dorg[1] = o[1]; dorg[2] = o[2];
  }

  //! \brief Set both the single and double precision versions of the ray origin
  void set_org(const double o[3]) {
    org_x = o[0]; org_y = o[1]; org_z = o[2];
    dorg[0] = o[0]; dorg[1] = o[1]; dorg[2] = o[2];
  }

  //! \brief Set both the single and double precision versions of the ray origin
  void set_org(const Vec3da& o) {
    org_x = o[0]; org_y = o[1]; org_z = o[2];
    dorg[0] = o[0]; dorg[1] = o[1]; dorg[2] = o[2];
  }

  //! \brief Set both the single and double precision versions of the ray direction
  void set_dir(double o[3]) {
    dir_x = o[0]; dir_y = o[1]; dir_z = o[2];
    ddir[0] = o[0]; ddir[1] = o[1]; ddir[2] = o[2];
  }

  //! \brief Set both the single and double precision versions of the ray direction
  void set_dir(const double o[3]) {
    dir_x = o[0]; dir_y = o[1]; dir_z = o[2];
    ddir[0] = o[0]; ddir[1] = o[1]; ddir[2] = o[2];
  }

  //! \brief Set both the single and double precision versions of the ray direction
  void set_dir(const Vec3da& o) {
    dir_x = o[0]; dir_y = o[1]; dir_z = o[2];
    ddir[0] = o[0]; ddir[1] = o[1]; ddir[2] = o[2];
  }

  //! \brief Set both the single and double precision versions of the ray max distance
  void set_tfar(double d) {
    tfar = std::min(d, INFTYF);
    dtfar = d;
  }

  //! \brief Set both the single and double precision versions of the ray near distance
  void set_tnear(double d) {
    tnear = d;
  }

  // Member variables
  RayFireType rf_type; //!< Enum indicating the type of query this ray is used for
  Vec3da dorg, ddir; //!< double precision versions of the origin and ray direction
  double dtfar; //!< double precision version of the ray far distance
  HitOrientation orientation; //!< Enum indicating what hits to accept based on orientation
  const std::vector<MeshID>* exclude_primitives {nullptr}; //! < Set of primitives to exclude from the query
};

/*! Structure extending Embree's RayHit to include a double precision version of the primitive normal */
struct RTCDHit : RTCHit {
  RTCDHit() {
    this->geomID = RTC_INVALID_GEOMETRY_ID;
    this->primID = RTC_INVALID_GEOMETRY_ID;
    this->Ng_x = 0.0;
    this->Ng_y = 0.0;
    this->Ng_z = 0.0;
  }

  // data members
  const PrimitiveRef* primitive_ref {nullptr}; //!< Pointer to the primitive reference for this hit
  MeshID surface {ID_NONE}; //!< ID of the surface this hit belongs to
  Vec3da dNg; //!< Double precision version of the primitive normal
};

/*! Stucture combining the ray and ray-hit structures to be passed to Embree queries */
struct RTCDRayHit {
  struct RTCDRay ray; //<! Extended version of the Embree RTCRay struct with double precision values
  struct RTCDHit hit; //<! Extended version of the Embree RTDRayHit struct with double precision values

  //! \brief Compute the dot product of the ray direction and current hit normal
  double dot_prod() {
    return dot(ray.ddir, hit.dNg);
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
    radius = std::min(rad, INFTYF);
    dradius = rad;
  }

  //! \brief Set both the single and double precision versions of the query location
  void set_point(const double xyz[3]) {
    x = xyz[0]; y = xyz[1]; z = xyz[2];
    dblx = xyz[0]; dbly = xyz[1]; dblz = xyz[2];
  }

  void set_point(const Position& xyz) {
    x = xyz[0]; y = xyz[1]; z = xyz[2];
    dblx = xyz[0]; dbly = xyz[1]; dblz = xyz[2];
  }

  unsigned int primID = RTC_INVALID_GEOMETRY_ID; //<! ID of the nearest primitive
  unsigned int geomID = RTC_INVALID_GEOMETRY_ID; //<! ID of the nearest geometry
  double dblx, dbly, dblz; //<! Double precision version of the query location
  const PrimitiveRef* primitive_ref {nullptr}; //!< Pointer to the primitive reference for this hit
  double dradius; //!< Double precision version of the query distance
};

} // namespace xdg


#endif // include guard
