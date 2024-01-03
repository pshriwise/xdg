
#ifdef XDG_EMBREE4

#include "xdg/embree4.h"

#elif defined(XDG_EMBREE3)

#include "xdg/embree3.h"

#else

#error "No embree version for XDG provided to compiler"

#endif