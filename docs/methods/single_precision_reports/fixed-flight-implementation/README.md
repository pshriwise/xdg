# Fixed-flight implementation validation

Implemented in the project:
- Optional trailing dist_min parameter through XDG::ray_fire and RayTracer interfaces,
  with default zero preserving existing source calls and parameter results relative to origin.
- Embree sets a conservative float traversal tnear and preserves the configured Scalar
  lower bound for the triangle test. GPRT forwards the minimum to its existing ray record;
  GPRT host overrides now use the base interface's Scalar type.
- TriangleIntersectionFunc forwards ray.get_tnear() instead of a literal zero.
- Particle keeps a flight origin and Scalar parameter across transmitting boundaries;
  uses parameter differences for collision decisions and track-length tallies; resets
  the reference ray after material collisions/reflections. Face history remains intact
  for straight transmissions and is cleared/trimmed as before for direction changes.

User clarification honored: include/xdg/geometry/plucker.h is byte-for-byte the pre-task
version. The initially planned plane-distance change was removed. The existing user edits
in constants.h, dp_math.h, plucker.h, particle_sim.h, and untracked fmt_formatters.h were
preserved. The pre-task tracked diff is preexisting.patch.

Checks:
- Single-precision Release build: successful.
- New API/transport/rim tests: passed for Embree in both single and double precision.
- Minimum-parameter rounding tests in the double build: 6 assertions passed, confirming
  full Scalar lower bound preservation and conservative float BVH bounds.
- Full single suite: 17 failures of 79 cases; these are exactly the same 17 named failures
  recorded before the implementation (baseline-tests.log had 74 cases before new tests).
- Separate double build: successful. Full suite: 13 failures of 79 cases, in existing
  infinity-value assertions, tally_segments and overlap tests. The new range/transport/rim
  tests pass. The infinity assertions fail on existing casts of maximum double to float;
  their implementation was not changed for this task.
- GPRT runtime tests are skipped because XDG_ENABLE_GPRT is OFF; no GPU validation claimed.
- git diff --check: clean.

Actual command completed:

    cd /home/pshriwise/soft/xdg/bld/tools
    ./particle-sim /home/pshriwise/tmp/teapot_rt/teapot.h5m -n 1000000

One loss: ID 463519, event 61, volume 5. This is the previously observed residual loss with
fixed-flight continuation and the retained triangle algorithm. The original baseline had
three losses. The complete CLI output is teapot-million.log. A second validation invokes
the same production transport_particles function on eight disjoint ID ranges and confirms
particles=1000000, lost=1, stuck=0 (teapot-million-parallel.log). Its standalone driver is
validate_transport.cpp; no test-only callback or preload was used for either validation.

The main project remains configured for single precision. The double build is isolated
under /tmp/xdg-fixed-flight-implementation/bld-double. No commit/install/publish performed.
