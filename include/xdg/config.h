
#ifndef XDG_CONFIG_H
#define XDG_CONFIG_H

#include <memory>
#include <string>
#include <unordered_map>

#include "xdg/error.h"

#ifdef XDG_ENABLE_LIBMESH
#include "libmesh/libmesh.h"
#endif

namespace xdg {
class XDGConfig {
  friend class ConfigCreator;
public:

  static void initialize(int verbosity = -1, bool warnings = true, int n_threads = -1) {
    if (new_instance_ != nullptr) {
      write_message("XDGConfig instance already exists", 7);
    } else {
      XDGConfig* p = new XDGConfig(verbosity, warnings, n_threads);
      new_instance_.reset(p);
    }
  }

  static const std::shared_ptr<XDGConfig>& get_config() {
    if (new_instance_ == nullptr) {
      write_message("XDGConfig instance not initialized, using defaults to initialize", 10);
      initialize();
    }
    return new_instance_;
  }

  // Delete copy constructor and assignment operator
  XDGConfig(const XDGConfig&) = delete;
  XDGConfig& operator=(const XDGConfig&) = delete;

  #ifdef XDG_ENABLE_LIBMESH
  const auto& libmesh_init() const { return libmesh_init_; }
  #endif

  ~XDGConfig() {
    #ifdef XDG_ENABLE_LIBMESH
    libmesh_init_.reset();
    #endif
  }

protected:
  // Private constructor
  XDGConfig(int verbosity = -1, bool warnings = true, int n_threads = -1);


  // Data members
  int verbosity_ {7};
  bool suppress_warnings_ {false};
  int n_threads_ {-1};
  bool initialized_ {false};

  #ifdef XDG_ENABLE_LIBMESH
  void initialize_libmesh();
  std::unique_ptr<libMesh::LibMeshInit> libmesh_init_ {nullptr};
  #endif

  static std::shared_ptr<XDGConfig> new_instance_;
};

} // namespace xdg


#endif // XDG_CONFIG_H
