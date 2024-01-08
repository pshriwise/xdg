#include "xdg/error.h"
#include "xdg/geometry_data.h"
#include "xdg/ray_tracing_interface.h"
#include "xdg/ray.h"
#include "xdg/triangle_ref.h"

namespace xdg {

void error(void* dum, RTCError code, const char* str) {
  if (code != RTC_ERROR_NONE)
    fatal_error("Embree error: {}", str);
}


RayTracer::RayTracer()
{
  device_ = rtcNewDevice(nullptr);
  rtcSetDeviceErrorFunction(device_, (RTCErrorFunction)error, nullptr);
}

RayTracer::~RayTracer()
{
  rtcReleaseDevice(device_);
}

void RayTracer::init()
{

}

void
RayTracer::register_volume(const std::shared_ptr<MeshManager> mesh_manager,
                                     MeshID volume_id)
{
  // allocate storage for this volume
  auto volume_elements = mesh_manager->get_volume_elements(volume_id);
  this->triangle_storage_map_[volume_id].resize(volume_elements.size());
  auto& triangle_storage = this->triangle_storage_map_[volume_id];

  auto volume_surfaces = mesh_manager->get_volume_surfaces(volume_id);
  int storage_offset {0};
  for (auto& surface_id : volume_surfaces) {
    // get the sense of this surface with respect to the volume
    Sense triangle_sense {Sense::UNSET};
    auto surf_to_vol_senses = mesh_manager->get_parent_volumes(surface_id);
    if (volume_id == surf_to_vol_senses.first) triangle_sense = Sense::FORWARD;
    else if (volume_id == surf_to_vol_senses.second) triangle_sense = Sense::REVERSE;
    else fatal_error("Volume {} is not a parent of surface {}", volume_id, surface_id);

    auto surface_elements = mesh_manager->get_surface_elements(surface_id);
    for (int i = 0; i < surface_elements.size(); ++i) {
      auto& triangle_ref = triangle_storage[i + storage_offset];
      triangle_ref.triangle_id = surface_elements[i];
      triangle_ref.surface_id = surface_id;
      triangle_ref.sense = triangle_sense;
    }
    storage_offset += surface_elements.size();
 }

  auto volume_scene = rtcNewScene(device_);
  rtcSetSceneFlags(volume_scene, RTC_SCENE_FLAG_ROBUST);
  rtcSetSceneBuildQuality(volume_scene, RTC_BUILD_QUALITY_HIGH);
  this->volume_map_[volume_id] = volume_scene;

  TriangleRef* tri_ref_ptr = triangle_storage.data();

  // compute the bounding box of the volume
  auto volume_bounding_box = mesh_manager->volume_bounding_box(volume_id);

  // determine the bump distance for this volume based on the maximum distance a ray will travel
  // to an intersection
  double dx = volume_bounding_box.max_x - volume_bounding_box.min_x;
  double dy = volume_bounding_box.max_y - volume_bounding_box.min_y;
  double dz = volume_bounding_box.max_z - volume_bounding_box.min_z;

  double max_distance = std::sqrt(dx*dx + dy*dy + dz*dz);
  double bump = max_distance * std::pow(10, -std::numeric_limits<float>::digits10);

  // create a new geometry for each surface
  int buffer_start = 0;
  for (auto surface : volume_surfaces) {
    auto surface_triangles = mesh_manager->get_surface_elements(surface);
    RTCGeometry surface_geometry = rtcNewGeometry(device_, RTC_GEOMETRY_TYPE_USER);
    rtcSetGeometryUserPrimitiveCount(surface_geometry, surface_triangles.size());
    unsigned int embree_surface = rtcAttachGeometry(volume_scene, surface_geometry);

    GeometryUserData surface_data;
    surface_data.mesh_manager = mesh_manager.get();
    surface_data.tri_ref_buffer = tri_ref_ptr + buffer_start;
    user_data_map_[surface_geometry] = surface_data;

    rtcSetGeometryUserData(surface_geometry, &user_data_map_[surface_geometry]);

    for (int i = 0; i < surface_triangles.size(); ++i) {
      auto& triangle_ref = surface_data.tri_ref_buffer[i];
      // triangle_ref.embree_surface = embree_surface;
    }
    buffer_start += surface_triangles.size();

    // set the bounds function
    rtcSetGeometryBoundsFunction(surface_geometry, (RTCBoundsFunction)&TriangleBoundsFunc, nullptr);
    rtcSetGeometryIntersectFunction(surface_geometry, (RTCIntersectFunctionN)&TriangleIntersectionFunc);
    rtcSetGeometryOccludedFunction(surface_geometry, (RTCOccludedFunctionN)&TriangleOcclusionFunc);

    rtcCommitGeometry(surface_geometry);
  }
  rtcCommitScene(volume_scene);
}

bool RayTracer::point_in_volume(MeshID volume,
                                const Position& point,
                                const Direction* direction,
                                const std::vector<MeshID>* exclude_primitives)
{
  RTCScene scene = volume_map_[volume];

  RTCDRayHit rayhit;
  rayhit.ray.set_org(point);
  if (direction != nullptr) rayhit.ray.set_dir(*direction);
  else rayhit.ray.set_dir({1. / std::sqrt(2.0), 1 / std::sqrt(2.0), 0.0});
  rayhit.ray.rf_type = RayFireType::VOLUME;
  rayhit.ray.orientation = HitOrientation::ANY;
  rayhit.ray.set_tfar(INFTY);
  rayhit.ray.set_tnear(0.0);
  if (exclude_primitives != nullptr) rayhit.ray.exclude_primitives = exclude_primitives;

  {
    rtcIntersect1(scene, (RTCRayHit*)&rayhit);
  }

  // if the ray hit nothing, the point is outside of the volume
  if (rayhit.hit.geomID == RTC_INVALID_GEOMETRY_ID) return false;

  // use the hit triangle normal to determine if the intersection is
  // exiting or entering
  return rayhit.ray.ddir.dot(rayhit.hit.dNg) > 0.0;
}

void
RayTracer::ray_fire(MeshID volume,
                    const Position& origin,
                    const Direction& direction,
                    double& distance,
                    const std::vector<MeshID>* exclude_primitves)
{
  RTCScene scene = volume_map_[volume];

  RTCDRayHit rayhit;
  // set ray data
  rayhit.ray.set_org(origin);
  rayhit.ray.set_dir(direction);
  rayhit.ray.set_tfar(INFTY);
  rayhit.ray.set_tnear(0.0);
  rayhit.ray.rf_type = RayFireType::VOLUME;
  rayhit.ray.orientation = HitOrientation::EXITING;
  rayhit.ray.mask = -1; // no mask
  if (exclude_primitves != nullptr) rayhit.ray.exclude_primitives = exclude_primitves;

  // fire the ray
  {
    rtcIntersect1(scene, (RTCRayHit*)&rayhit);
    // TODO: I don't quite understand this...
    rayhit.hit.Ng_x *= -1.0;
    rayhit.hit.Ng_y *= -1.0;
    rayhit.hit.Ng_z *= -1.0;
  }

  if (rayhit.hit.geomID == RTC_INVALID_GEOMETRY_ID)
    distance = INFTY;
  else
    distance = rayhit.ray.dtfar;
}

void RayTracer::closest(MeshID volume,
                                  const Position& point,
                                  double& distance)
{
  RTCScene scene = volume_map_[volume];

  RTCDPointQuery query;
  query.set_point(point);

  RTCPointQueryContext context;
  rtcInitPointQueryContext(&context);

  rtcPointQuery(scene, &query, &context, (RTCPointQueryFunction)&TriangleClosestFunc, &scene);

  if (query.geomID == RTC_INVALID_GEOMETRY_ID) {
    distance = INFTY;
    return;
  }

  distance = query.dradius;
}

bool RayTracer::occluded(MeshID volume,
                         const Position& origin,
                         const Direction& direction,
                         double& distance) const
{
  RTCScene scene = volume_map_.at(volume);

  RTCDRay ray;
  ray.set_org(origin);
  ray.set_dir(direction);
  ray.set_tfar(INFTY);
  ray.set_tnear(0.0);
  ray.rf_type = RayFireType::FIND_VOLUME;
  ray.orientation = HitOrientation::ANY;
  ray.flags = 0;
  ray.mask = -1; // no mask

  // fire the ray
  {
    rtcOccluded1(scene, (RTCRay*)&ray);
  }

  distance = ray.dtfar;
  return distance != INFTY;
}

} // namespace xdg