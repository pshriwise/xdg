# Compensated Plucker edge integration

The reviewed proposal is now applied to the project. Its helper is in
include/xdg/geometry/plucker.h:72, and all three edge-coordinate evaluations in
plucker_ray_tri_intersect call it with the original vertices and ray origin.
The helper uses the existing CompensatedScalar operations from
include/xdg/geometry/triangle_distance.h. In the single-precision CPU build,
all fields and arithmetic for these edge evaluations remain float.

Canonical ordering of original edge endpoints gives identical computations
for a shared edge, with opposite final signs when its direction is reversed.
The existing orientation/sign tests, compensated distance calculation, disabled
barycentric check, position-uncertainty guard, fixed flight ray, and OpenMP
atomic tally updates are retained.

## Regression coverage

tests/test_triangle_distance.cpp now checks:

- The two false initial-volume hits from particle 9008800 are rejected, while
  its valid neighboring hit is accepted, for all six vertex permutations.
- The shared edge retains equal-and-opposite nonzero values, checked against
  the previously calculated exact-rational reference.
- Genuine shared-edge and shared-vertex hits remain accepted, for both
  adjacent triangles and all vertex permutations.
- Large ray origins preserve subtraction residuals that would otherwise
  collapse the translated triangle in float.

The reviewed proposal and applied plucker.h are byte-for-byte identical.

## Builds and targeted tests

- Single precision: bld/tools/particle-sim, bld/tests/test_triangle_distance,
  and bld/tests/test_particle_sim rebuilt successfully.
- Double precision compatibility: the same targets rebuilt in
  /tmp/xdg-fixed-flight-implementation/bld-double.
- Triangle tests: 6 cases and 124 assertions pass in each precision.
- Particle tests: single precision 6 cases/234 assertions pass; double
  precision 5 cases/229 assertions pass. Each skips 3 unavailable GPRT cases.
- Full single-precision project build succeeds.
- Full single-precision CTest: 90 tests, with the same 17 failing test names
  as the baseline run taken before rebuilding the edge change. No new failing
  tests. See baseline-failure-comparison.txt and the two CTest logs.
- git diff --check passes.

No GPU runtime validation or full double-precision CTest run was performed for
this change. The actual geometric predicate remains compensated arithmetic,
with no proof of exact signs for all possible inputs.

## Three original lost histories

Each was rerun against the rebuilt single-precision production executable,
without LD_PRELOAD, seed 1, MFP 1, max events 1000:

    bld/tools/particle-sim ~/tmp/teapot_rt/teapot.h5m -n 1 -v --start-particle-id ID

| ID | Initial volume | Final event | Vacuum surface | Losses |
| --- | --- | --- | --- | --- |
| 9008800 | 9 | 582 | 197 | 0 |
| 9410439 | 9 | 442 | 194 | 0 |
| 5531682 | 5 | 388 | 193 | 0 |

See particle-ID.log for each complete replay.

## Production sample

The rebuilt single-precision simulator completed 1,000,000 histories (IDs
0..999999) with zero reported losses, seed 1, MFP 1, max events 1000, 8 workers:

    bld/tools/particle-sim ~/tmp/teapot_rt/teapot.h5m -n 1000000 -t 8

The run used the actual executable without LD_PRELOAD. See million.log.
Elapsed time was 103.57 seconds; the concurrent project build makes this a
validation result rather than a controlled overhead measurement.

A complete 10-million-history run has not been repeated for this integration.
