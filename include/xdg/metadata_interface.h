
#ifndef _XDG_METADATA_INTERFACE
#define _XDG_METADATA_INTERFACE

#include <map>
#include <memory>
#include <set>
#include <string>

#include "xdg/constants.h"
#include "xdg/mesh_manager_interface.h"

namespace xdg {

class Metadata
{
public:
  // Constructors
  Metadata(std::shared_ptr<MeshManager> mm) : mesh_manager_(mm) {};

  // Public methods
  virtual std::string get_volume_property(const std::string& property, MeshID vol) const = 0;

  virtual std::string get_surface_property(const std::string& property, MeshID surface) const = 0;

  // Accessors
  const auto& mesh_manager() const { return mesh_manager_; }

private:
  // Private Methods
  virtual void parse_metadata() = 0;

  // Data members
  // TODO: SHOULD BE GEOMETRY TOPOLOGY OBJECT!!!!
  std::shared_ptr<MeshManager> mesh_manager_;

  std::set<std::string> properties_; //! All registered properties

  std::map<std::pair<std::string, MeshID>, std::string> metadata_; //! Metadata entries
};

} // namespace xdg

#endif // include guard