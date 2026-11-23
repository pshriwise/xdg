# Integrated compensated triangle distance

Project changes:
- include/xdg/geometry/triangle_distance.h: configured-Scalar compensated
  arithmetic; vertex differences, cross/dot products, and quotient correction.
- include/xdg/geometry/plucker.h: replace weighted-point distance reconstruction
  with detail::compensated_triangle_distance. The edge/orientation predicates
  are byte-for-byte unchanged, as are barycentric checks and zero tolerance.
  The final interval test now also rejects NaN from a singular triangle plane.
- include/xdg/geometry/dp_math.h: typed FMA wrapper for C++/Slang, and scalar
  alias for Slang. C++ single builds use float throughout the new helper.
- CMakeLists.txt: disable implicit contraction for GNU/Clang via a public
  -ffp-contract=off requirement; explicit FMA remains enabled. No architecture
  flag or precision setting was changed. Track the shared geometry headers
  in shader build dependencies.
- tests/test_triangle_distance.cpp and tests/CMakeLists.txt: cap crossing,
  two cancellation-sensitive oblique hits, winding/permutation checks, and
  rejection of a NaN hit on a collinear triangle.

Validation:
- Release single-precision full build succeeds.
- Separate double-precision full build succeeds.
- New tests pass: 37 assertions in 3 cases, in each precision.
- Full single suite: 82 cases, same 17 pre-existing failures; no new failures.
- Full double suite: 82 cases, same 13 pre-existing failures; no new failures.
  Named failure sets match the prior fixed-flight implementation baselines.
- Direct rebuilt CLI, no LD_PRELOAD:
  bld/tools/particle-sim /home/pshriwise/tmp/teapot_rt/teapot.h5m -n 1 --start-particle-id 463519
  Completes without a loss; raw output is particle-463519.log.
- Million-particle integration: validate_transport.cpp from the prior work,
  recompiled against the updated project headers/library, invokes the actual
  production transport_particles on 8 disjoint ranges covering IDs 0..999999,
  seed 1, max events 1000, mean free path 1, no LD_PRELOAD.
  teapot-million.log: SUMMARY particles=1000000 lost=0 stuck=0.
- Isolated Slang helper compiles to SPIR-V in precise floating-point mode
  (the standalone shader supplies the DBL_EPSILON macro normally supplied by
  the surrounding shader includes). No GPRT runtime validation was performed;
  GPRT is disabled in both configured builds.
- git diff --check passes.

Prior user edits and the fixed-flight transport implementation were preserved.
No commits, installation, or publication performed. before.patch and the
*.before.* files capture the pre-integration state for review.
