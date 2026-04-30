Glossary
========

.. _glossary:


.. glossary::

    Acceleration Structure
        A general term for ray tracing data structures that are used to accelerate 
        ray tracing operations by partitioning geometric primitives (i.e. faces/elements) 
        in a way that allows for efficient traversal and intersection testing.

    BLAS
        Bottom-Level Acceleration Structure. A lower-level acceleration
        structure, commonly a BVH, built over mesh primitives such as triangles.
        In a two-level ray tracing acceleration structure, BLAS instances are
        referenced by a :term:`TLAS`.

    BVH
        Bounding Volume hierarchy

    CAD
        Computer-Aided Design

    CAE
        Computer-Aided Engineering

    CSG
        Constructive Solid Geometry

    `Coreform Cubit <https://coreform.com/products/coreform-cubit/>`_
        A commercial software package for mesh generation, geometry
        simplification, and mesh-based simulations.

    `Cubit <https://cubit.sandia.gov/>`_
        A Sandia National Laboratory software package for mesh generation, geometry
        simplification, and mesh-based simulations.

    `DAGMC <https://svalinn.github.io/DAGMC/index.html>`_
        Direct Accelerated Geometry Monte Carlo toolkit

    `Double-Down <https://double-down.readthedocs.io/en/latest/>`_
        A ray tracing extension for DAGMC that provides support for double-precision
        ray tracing on CAD-based geometries using Intel Embree.

    `DPRT <https://github.com/NVIDIA/deepeeRT>`_
        DeePeeRT (Double Precision Ray Tracing Toolkit) - A "Basics-only" Ray Tracing Library
        intended specifically for Double-Precision Ray Tracing being developed by NVIDIA.

    `Embree <https://www.embree.org/>`_
        A collection of high-performance CPU ray tracing kernels developed by Intel.

    EntitySet
        An arbitrary collection of entities in MOAB, including other
        `EntitySet`'s. Parent-child relationships between `EntitySet`'s can also
        be established. Synonymous with the term :term:`MeshSet`.

    FEA
        Finite Element Analysis

    `GPRT <https://github.com/gprt-org/GPRT>`_
        General Purpose Raytracing Toolkit - A vulkan based GPU accelerated ray tracing
        library capable of both GPU software and hardware accelerated ray tracing.

    `libMesh <https://libmesh.github.io/>`_
        A C++ finite element library that provides a framework for the
        development of parallel adaptive finite element methods.

    MeshSet
        A collection of entities in MOAB. Synonymous with :term:`EntitySet`.

    `MOAB <https://sigma.mcs.anl.gov/moab-library/>`_
        Mesh-Oriented datABase

    `OpenMC <https://docs.openmc.org>`_
        An open-source Monte Carlo code for neutron and photon transport.

    RT hardware acceleration
        The use of specialized hardware, such as dedicated ray tracing cores,
        avalaible on GPUs to signifciantly accelerate ray tracing operations.
        Such hardware can perform ray-triangle intersections and BVH traversal
        much faster than even GPU software implementations of ray tracing algorithms.
        However, they are limited to single precision support and often require
        vendor-specific APIs to target.

    subdomain
        A region of a mesh that is bounded by surfaces. In the context of XDG,
        a subdomain is a region of a mesh that is bounded by surfaces that are
        treated as interfaces between different materials. Also refferred to as
        a mesh block.

    Tag
        A named data field that can be associated with entities in MOAB. Each
        tag has an immutable data type. The size of the data can be fixed or
        variable.

    TLAS
        Top-Level Acceleration Structure. A higher-level acceleration structure
        built over one or more :term:`BLAS` instances. A TLAS is used to cull
        larger groups of geometry before traversing the lower-level structures.

    XDG
        Accelerated Discretized Geometry
