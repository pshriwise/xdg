#include "xdg/error.h"
#include "xdg/ray_tracing_interface.h"

void error(RTCError code, const char* str) {
  if (code != RTC_ERROR_NONE)
    fatal_error("Embree error: {}", str);
}

namespace xdg
{
  RayTracingInterface::RayTracingInterface() {
    device_ = rtcNewDevice(nullptr);
    error(rtcGetDeviceError(device_), "Failed to create Embree device");
  }
} // namespace xdg