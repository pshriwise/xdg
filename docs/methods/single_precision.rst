Single-precision particle transport
===================================

The single-precision transport changes address several independent sources of
lost particles: inaccurate triangle distances, rounded boundary positions,
false triangle-edge hits, and an additional barycentric rejection. They retain
the Plucker intersection formulation and use the configured ``Scalar`` type
for the compensated geometry calculations and position bounds. In a C++ build
with ``XDG_SINGLE_PRECISION=ON``, these calculations use floats, including their
rounding residuals; they do not use a double-precision geometry fallback.

The particle simulator's existing random-number outputs, sampled collision
distances, and track-length tallies use doubles. Those components are separate
from the single-precision geometry calculations described here.

The investigation reports, accuracy comparisons, benchmarks, and integration
records are preserved in :doc:`single_precision_reports/index`. Those reports
describe successive experiments; this page describes the final implementation.

Preserving a straight flight
----------------------------

A transmitting boundary changes the tracked volume but does not change the
physical ray. Repeatedly launching from each rounded boundary position changes
that ray slightly, which can erase a short path through a narrow rim or corner.
The particle therefore retains a flight origin and direction and advances its
parameter instead:

.. math::

   \mathbf{r}(t) = \mathbf{o}_{\mathrm{flight}} + t\mathbf{u}.

``XDG::ray_fire`` and the ray-tracer interfaces accept an optional trailing
``dist_min`` argument. Intersections are restricted to the inclusive interval
``[dist_min, dist_limit]``; returned parameters remain relative to the supplied
origin. The default minimum is zero, preserving existing source calls.

The Embree backend uses a conservatively rounded float ``tnear`` for BVH
traversal and retains the full configured-precision minimum for the triangle
test. This matters in double builds when converting the lower bound to float
would otherwise round it upward and skip a valid hit. The custom triangle
callback passes ``ray.get_tnear()`` to the primitive test. The GPRT host
interface forwards the minimum to its existing ray record as well.

After a transmission, the next query uses the same flight origin and direction
with the updated minimum parameter. Track-length scoring uses
``t_hit - t_current``. Material collisions and reflections change direction
and start a new flight. Primitive history continues to handle coincident or
zero-length transitions; the implementation does not blindly advance the
minimum by a spatial epsilon.

For the diagnosed rim crossing of particle 615368, the exact stored-float ray
entered the cap and exited an adjacent wall only about ``3.18e-7`` later in ray
parameter. Reconstructing the cap position moved its y coordinate by about
``4.41e-7``, placing it beyond the wall. Keeping the original ray preserves the
two distinct parameters even though their world-space chord is smaller than
one coordinate ULP. Fixed-flight continuation alone does not resolve every
distance or collision-position error.

Compensated triangle distance
-----------------------------

The former distance calculation reconstructed a point using normalized
Plucker weights and recovered the ray parameter from one coordinate. Accurate
edge signs do not imply accurate weight magnitudes, and errors in those weights
can move a transition appreciably before or after its triangle plane.

``include/xdg/geometry/triangle_distance.h`` evaluates the equivalent plane
parameter while retaining arithmetic residuals:

.. math::

   \mathbf{n} &= (\mathbf{v}_1-\mathbf{v}_0)
                 \mathbin{\times}(\mathbf{v}_2-\mathbf{v}_0), \\
   t &= \frac{\mathbf{n}\mathbin{\cdot}(\mathbf{v}_0-\mathbf{o})}
                  {\mathbf{n}\mathbin{\cdot}\mathbf{u}}.

Each intermediate is represented by a ``CompensatedScalar`` containing a
``value`` and an ``error``, both in the configured scalar type. Sum residuals
are recovered with TwoSum-style arithmetic, product residuals with explicit
FMA, and the final quotient receives a residual correction. Preserving the
vertex differences, normal, and dot products is important: computing a normal
in ordinary float and improving only the final division is insufficient.

For particle 463519, the previous cap distance was
``1.8856683969497681`` instead of approximately ``1.8856989803399955``, an error
of about 257 float ULPs. The compensated calculation returns the nearest float,
``1.8856990337371826``. This prevents a collision while the particle is still
geometrically outside the volume it has just been assigned.

On 55,150 captured nonzero hits, the compensated result matched the nearest
float reference in every sampled case. References used the exact geometry
represented by the stored float inputs, with offline higher-precision and
exact-rational checks. This is sampled accuracy evidence, not a proof of correct
rounding for arbitrary inputs.

The build exports ``-ffp-contract=off`` for GNU and Clang consumers of ``xdg``.
Explicit FMA remains available. Compensation assumes ordinary IEEE arithmetic
without algebraic reassociation; enabling fast-math can invalidate those steps.
No architecture-specific FMA target flag is forced by the project changes.
Shader dependencies include the shared Plucker and triangle-distance headers.

Compensated Plucker edge predicates
-----------------------------------

Distance accuracy does not fix an incorrect inside/outside decision. The edge
test evaluates the existing Plucker product in its origin-relative form:

.. math::

   P_{ab} = \mathbf{u}\mathbin{\cdot}
       \left[(\mathbf{b}-\mathbf{a})\mathbin{\times}
                       (\mathbf{a}-\mathbf{o})\right].

``detail::compensated_plucker_edge`` in
``include/xdg/geometry/plucker.h`` retains residuals through both vertex
differences, the cross product, and the dot product. It receives the original
vertices and origin: translating vertices with ordinary arithmetic before
calling the helper would discard subtraction residuals too early.

Original edge endpoints are ordered canonically using ``first``. Both adjacent
triangles therefore follow the same arithmetic for their shared edge, with a
final sign reversal for the opposite direction. The three edge evaluations
remain in their original positions among the orientation and sign checks.

Particle 9008800 exposed this failure during its initial volume lookup. A
shared-edge value of approximately ``+2.903280589e-8`` cancelled to signed zero;
the neighboring face produced the opposite false zero. Both triangles were
accepted even though the ray missed them. A spurious exit before the true entry
caused initialization in volume 1 rather than volume 9. The particle was then
lost after five collisions in the wrong volume.

The compensated evaluations return approximately ``+2.903280638e-8`` and
``-2.903280638e-8`` in float. Both false hits are rejected, while the valid
neighboring hit remains accepted. The rebuilt simulator initializes the
particle in volume 9 and completes its history.

Two additional rejection policies are disabled in the current implementation:

* Edge coordinates are not deliberately clamped to zero using a global
  tolerance. Such clamping can turn a triangle miss into a hit and can corrupt
  the weight magnitudes used by the former distance calculation.
* The additional normalized barycentric rejection is commented out. For
  particle 5531682, the Plucker signs correctly admitted an exit, but rounded
  normalized coordinates summed to ``1.0000001192`` and rejected it. Their
  exact barycentric weights were all positive.

The existing distance-to-zero snap remains. ``PLUCKER_ZERO_TOL`` is currently
``10 * numeric_limits<float>::epsilon()``, including in double builds. It is
an absolute parameter tolerance, not a derived position-error bound. The final
ordered range check also rejects NaN from a singular triangle plane.

Conservative collision selection
--------------------------------

Even a correctly rounded surface parameter can be followed by a collision
whose reconstructed float position lies outside the current volume. For
particle 9410439, the sampled collision was genuinely before the surface, but
coordinate rounding moved its stored position beyond the selected face.
Changing the precision of the comparison or adding one float ULP to the
collision distance did not resolve that case.

``tools/position_uncertainty.h`` propagates outward-rounded scalar intervals
through collision-distance conversion, addition to the flight parameter,
position reconstruction, and triangle-plane evaluation. The geometric normal
is oriented outward from the current volume using the surface sense. Raw
triangle vertex ordering alone does not supply that volume-dependent sense.

For the interval collision position, evaluate the interior clearance:

.. math::

   g = \mathbf{n}_{\mathrm{out}}\mathbin{\cdot}
       (\mathbf{v}_0-\mathbf{r}_{\mathrm{collision}}).

A nominally earlier collision is accepted only when ``g.lower > 0``. If the
interval touches or crosses the face, the simulator takes the surface event.
This uses the lower uncertainty bound to certify the collision and avoids
normalization or division by a nearly tangent ray's normal-dot-direction.
All interval endpoints and arithmetic use floats in the single-precision build.

``Particle::advance`` returns the selected event, and the transport loop uses
that same decision for dispatch. A surface event advances to the original
returned surface parameter and scores its full segment. The sampled collision
distance is retained; the implementation does not move to an artificially
shortened boundary cutoff or nudge the collision coordinates.

The interval at the problematic decision for particle 9410439 was
``[-4.224421346e-8, +2.582965131e-8]`` in unnormalized interior-plane units.
Selecting the surface lets the history complete. In the earlier prototype's
million-history audit, the guard redirected 56 of approximately 356 million
nominal collision decisions. Two proposed rounded points were on or beyond
their selected face plane.

This bound certifies the side of the selected outgoing facet only. A nearby
second face, a shared corner, or a previously crossed entry face can require
additional checks. It does not establish membership in an arbitrary nonconvex
volume. The interval implementation assumes finite inputs, nonnegative flight
steps, and ordinary IEEE arithmetic.

OpenMP transport and thread control
-----------------------------------

Nonverbose Embree particle histories run in an OpenMP parallel loop with static
scheduling. The shared tally map is populated before the loop; scoring uses
checked lookup and ``omp atomic update`` on its double values. Lost and stuck
counts use integer reductions. Complete lost-particle messages use a named
critical region. Verbose transport remains serial, as do GPRT host queries
because that backend shares query buffers.

Each particle retains a deterministic seed derived from its ID and the event
limit. Its RNG uses a particle-local 48-bit linear congruential state with the
same sequence and outputs as ``erand48``. This avoids shared libc RNG
parameters and makes the random stream independent of worker scheduling.
Atomic addition order can still affect the last bits of accumulated totals.

Use ``-t`` or ``--threads`` to select a positive worker count:

.. code-block:: console

   ./bld/tools/particle-sim ~/tmp/teapot_rt/teapot.h5m -n 10000000 -t 8
   ./bld/tools/particle-sim ~/tmp/teapot_rt/teapot.h5m -n 1 -v --start-particle-id 9008800

An explicit count overrides ``OMP_NUM_THREADS``. Omitting the flag retains the
OpenMP environment/runtime defaults. Builds without OpenMP accept ``-t 1`` and
reject larger requests. Invalid, nonpositive, or out-of-range values return a
failure status.

Validation and measured cost
----------------------------

The final production replays used seed 1, mean free path 1, and a maximum of
1000 events, without a diagnostic callback override:

.. list-table:: Previously lost histories
   :header-rows: 1

   * - Particle
     - Initial volume
     - Final event
     - Vacuum surface
     - Lost
   * - 9008800
     - 9
     - 582
     - 197
     - 0
   * - 9410439
     - 9
     - 442
     - 194
     - 0
   * - 5531682
     - 5
     - 388
     - 193
     - 0

The final rebuilt single-precision executable also completed one million
histories, IDs 0 through 999999, with zero reported losses. Histories reaching
the existing event limit are not followed indefinitely. A complete final
10-million-history rerun was not performed by the integration validation.

The final triangle tests passed 124 assertions in six cases in each precision.
Particle tests passed 234 assertions in single precision and 229 in double
precision, with three unavailable GPRT cases skipped in each build. The full
single-precision suite retained the same 17 failing test names as its recorded
baseline, with no new failing tests. Integration reports preserve the earlier
double-suite results and the exact scope of each validation stage.

A later commit-packaging check used the current Debug single-precision build
and the separate Release double-precision build. Geometry, particle-transport,
and ray-fire cases passed in each. The expanded ray-property checks reported
five existing failures in each configuration, all involving upper-distance
constants rather than the new minimum parameter: Debug uses a smaller
``INFTYF`` than the stored float maximum, while Release double builds can
convert the maximum double to float infinity. The upper-bound assignments and
these assertions predate the transport changes. The new minimum-parameter
sections pass independently in both builds.

The distance-only accepted-triangle microbenchmark increased from about
96.2 ns to 162.9 ns, or 1.69 times. In the full serial simulator on 100000 teapot
histories, distance-only median overhead was 4.51 percent without an explicit
hardware-FMA target and 1.43 percent against a compiler-matched hardware-FMA
callback. These measurements predate the position guard and compensated edge
updates and do not measure their combined cost.

The OpenMP benchmark, also taken before the final edge and guard updates,
measured 40.679 seconds with one worker, 6.996 seconds with eight, and
5.194 seconds with sixteen for 100000 histories. These were observations on a
shared workstation, not controlled performance guarantees. The archived
protocols document compiler flags, setup costs, and measurement limitations.

Interpreting single- versus double-precision tallies
----------------------------------------------------

The user reported the following aggregate track-length totals after the fixes:

.. list-table:: Reported precision comparison
   :header-rows: 1

   * - Cell
     - Double precision
     - Single precision
     - Relative difference, ppm
   * - 1
     - 34815067.83177709
     - 34814725.20866192
     - -9.8412
   * - 2
     - 11310888.010947619
     - 11310932.62407378
     - +3.9443
   * - 5
     - 584487601.4320023
     - 584485549.8148191
     - -3.5101
   * - 8
     - 0
     - 0
     - Not defined; both zero
   * - 9
     - 2933872967.717098
     - 2933867555.4264274
     - -1.8448

The single-precision total is lower by approximately 7761.92, or 2.1776 ppm
(0.00021776 percent). These values are user-reported results, separate from the
integration's one-million-history validation.

Both builds accumulate tallies in double precision. Matching seeds do not
force matching physical histories: direction sampling uses ``Scalar``, rounded
positions and intersections differ, and a different collision/surface decision
can shift subsequent random-number consumption. The position guard also
intentionally redirects uncertain collisions. Aggregate agreement alone does
not measure systematic bias; paired per-particle differences and their variance,
or comparisons across seeds, are needed for that assessment.

Remaining scope
---------------

The compensated predicates are not exact-sign predicates for all inputs, and
the sampled distance results are not a general correctly-rounded guarantee.
Overflow, extreme scale ranges, and degenerate geometry need further treatment.

Vertex quantization can also change the geometry itself. The mesh audit found
one nearly collinear source facet whose normal inverted after rounding its
vertices to float. That concern was not tied to the three final lost histories,
and these arithmetic changes do not repair such facets.

Runtime validation covered the CPU Embree path. GPRT minimum-parameter forwarding
and shared shader dependencies were updated, but no GPU runtime robustness
claim follows from the CPU results. Earlier standalone Slang compilation checks
are recorded in the distance-integration report.

The existing MOAB metadata-error formatting fix is included with the code
changes: the diagnostic now supplies both the group name and token count to
its two format fields.

.. toctree::
   :maxdepth: 1

   single_precision_reports/index
