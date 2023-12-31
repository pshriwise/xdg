
#ifdef EMBREE4

#include "xdg/embree4.h"

#elif defined(EMBREE3)

#include "xdg/embree3.h"

#else

#error "No embree version provided to compiler"

#endif