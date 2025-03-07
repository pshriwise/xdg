
#include "xdg/config.h"

#ifdef XDG_HAVE_OPENMP
#include "omp.h"
#endif

namespace xdg {

XDGConfig::XDGConfig(int verbosity, bool warnings, int n_threads)
: verbosity_(verbosity), suppress_warnings_(warnings)
{
  // Set the number of threads
  if (n_threads != -1) {
    n_threads_ = n_threads;
  }

  // if threads aren't manually specified,
  // set using OpenMP
  if (n_threads_ == -1) {
  #ifdef XDG_HAVE_OPENMP
    n_threads_ = omp_get_num_threads();
  #else
    n_threads_ = 1;
  #endif
  }


  // Initialize libMesh if enabled
  #ifdef XDG_ENABLE_LIBMESH
  initialize_libmesh();
  #endif

  initialized_ = true;
}

#ifdef XDG_ENABLE_LIBMESH
void XDGConfig::initialize_libmesh() {
  // libmesh requires the program name, so at least one argument is needed
  int argc = 1;
  const std::string argv{"XDG"};
  const char *argv_cstr = argv.c_str();
  libmesh_init_ = std::make_unique<libMesh::LibMeshInit>(argc, &argv_cstr, 0, n_threads_);
}
#endif

std::shared_ptr<XDGConfig> XDGConfig::new_instance_;

} // namespace xdg