.. _libmesh_format:

libMesh File Format
===================

XDG supports ray tracing of the subdomain interfaces in a :term:`libMesh` mesh.
The input mesh is expected to be volumetric. The faces of elements on the
boundary of these subdomains are treated as surfaces in the XDG geometry by
shelling the volumetric elements of the subdomain. The element faces that
lie between subdomain boundaries (i.e. faces at the interface of two different
subdomains) are discovered automatically and surfaces are created in the XDG
topology with a transmission boundary condition.

Sidesets with names that match supported boundary conditions ("reflective",
"vacuum", etc.) are treated as such in XDG's metadata system. Currently, such
sidesets must entirely contain one of these interfaces. Support for partial
replacement is under development.

Current restrictions:

  - meshes of linear tetrahedra
  - XDG surfaces cannot be a partial set of interfaces. Sidesets must contain
    the entirety of an interface between two subdomains in the mesh.
