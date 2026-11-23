# Float-only continuation using a fixed straight-flight ray

All files are outside the project. No project source or CMake setting was changed.
The driver and callback experiments use float for ray/position/intersection arithmetic;
there is no double precision fallback. The original simulation's RNG/collision-distance
sampling and track-length tallies retain their original double types.

## Motivation and frozen rim test

For particle 615368's previous frozen ray, global y spacing is ~9.54e-7, too coarse to
store the actual short path through the rim as a new global-origin point. However the
same path is resolvable in a float ray parameter referenced to the last collision origin.
Using the same float vertices/ray with the existing Plucker edge predicates and a float
triangle-plane distance formula, rim.cpp reports:

    entry_hit=1 entry_t=0.31579014658927917
    exit_hit=1  exit_t=0.31579047441482544
    delta_t=3.2782554626464844e-7
    t_ulp=2.9802322387695312e-8
    rounded_origin_exit_hit=0

There are 11 representable float parameter steps between the two calculated crossings.
This computes both events from ONE fixed origin/direction, even though the rounded
world-space point at entry would put the particle outside the destination volume.
The current Plucker barycentric-derived t, queried from the same original ray, can also
retain this shared-edge consistency; the separate plane formula improves t for long,
thin triangles such as horizontal caps.

## Prototype

Keep flight_origin and direction fixed between collisions/reflections. Keep a float
flight_t and cell ID, plus primitive history. Every surface query uses flight_origin
and direction and rejects candidates with t < flight_t. Returned t is a parameter
measured from the fixed origin. The distance through the current cell is t_hit-flight_t.
After a transmission update flight_t and cell, without using a rounded global endpoint
as the origin for the next query. Material collisions advance the parameter by their
sampled distance, compute the new point, change direction, and start a new flight.
Reflections reset the reference ray as well.

The production API would need to expose t_min and propagate it through Embree and the
custom callback. Today EmbreeRayTracer sets tnear=0 and the triangle callback also
passes a literal zero to the primitive's tMin argument. The diagnostic communicates the
minimum through thread-local state only to avoid changing project APIs.

No spatial nudge/normal offset is applied. Zero-length/coincident transitions still
require history/topology handling; a blind fixed epsilon or blindly advancing t_min
by a large amount could erase a real short segment.

Two comparison kernels:
- plane: existing relative Plucker edge tests and barycentric acceptance, with distance
  t=dot(n,v0-origin)/dot(n,direction), n=cross(v1-v0,v2-v0), all float.
- shear: the earlier float-only ray-aligned shear diagnostic with no precision fallback.

These are experiments, not a general robustness guarantee. Initial point classification
also uses the selected callback kernel, so whole-history trajectories change between modes.
The fixed-ray representation does not itself solve every error at collisions/reflections,
unresolvable coincident events, extreme ray lengths, or invalid float-converted geometry.
The previously identified inverted source facet still requires mesh repair/retessellation.

## Reproduction

    sh /tmp/xdg-single-continuation/build.sh
    OMP_NUM_THREADS=6 LD_PRELOAD=/tmp/xdg-single-continuation/plane.so /tmp/xdg-single-continuation/driver 1000000
    OMP_NUM_THREADS=6 LD_PRELOAD=/tmp/xdg-single-continuation/shear.so /tmp/xdg-single-continuation/driver 1000000
    /tmp/xdg-single-continuation/rim

The completed aggregate results are plane.log and shear.log. The corresponding
*-checks.log files report any accepted hit whose parameter was behind the current one.

## Completed results

| Configuration | Lost / 1000000 | Capped at 1000 | Terminated |
|---|---:|---:|---:|
| Fixed flight plus plane-distance kernel | 0 | 29685 | 970315 |
| Fixed flight plus ray-aligned shear kernel | 8 | 29686 | 970306 |

Both minimum-parameter check logs are empty. The plane configuration uses the same
current Plucker edge predicates; only the distance formula and continuation representation
are changed, with no boundary-origin offsets. Earlier controls were 3 losses for the current
implementation, 1 for plane-distance alone, and 1 for fixed-flight/current-distance alone.
Different configurations change histories, so these finite counts are validation evidence,
not a general proof of independent causal fixes or guaranteed future zero losses.

Suggested production shape: expose a minimum ray parameter through XDG::ray_fire,
EmbreeRayTracer::ray_fire, and TriangleIntersectionFunc; use ray.get_tnear() rather than
literal zero in the primitive call; retain a fixed flight origin/direction and parameter
in the particle. Return t_hit relative to that origin and tally t_hit-t_current. Preserve
triangle/surface history for coincident events. Material collisions and reflections are
new rays and still need boundary-aware position reconstruction if uncertainty overlaps a
boundary. The float-converted mesh's inverted/sub-resolution facets remain a separate issue.
