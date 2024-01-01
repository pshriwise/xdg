#include "xdg/error.h"
#include "xdg/triangle_ref.h"
#include "xdg/ray_tracing_interface.h"

namespace xdg {

void error(void* dum, RTCError code, const char* str) {
  if (code != RTC_ERROR_NONE)
    fatal_error("Embree error: {}", str);
}


RayTracingInterface::RayTracingInterface()
{
  device_ = rtcNewDevice(nullptr);
  rtcSetDeviceErrorFunction(device_, (RTCErrorFunction)error, nullptr);
}

RayTracingInterface::~RayTracingInterface()
{
  rtcReleaseDevice(device_);
}

void
RayTracingInterface::register_volume(const std::shared_ptr<MeshManager> mesh_manager,
                                     MeshID volume_id)
{
  
}

void RayTracingInterface::init()
{

}
} // namespace xdg