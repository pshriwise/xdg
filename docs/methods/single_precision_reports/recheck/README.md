# Reassessment: normals, mesh validity, and boundary continuation

No project source files were modified. All drivers use a copied current single-precision
library and the existing Particle implementation unless a stated diagnostic replaces a part.
Runtime intersection and position alternatives below use float, with no double fallback.
The source-versus-quantized-mesh comparison uses double ONLY as an offline audit of the
original H5M coordinates, not as an intersection or transport remedy.

## Normal contract

MOABMeshManager::face_vertices and MBDirectAccess::get_mb_coords return stored vertex order.
TriangleIntersectionFunc passes these vertices to plucker_ray_tri_intersect unchanged.
Only AFTER that function returns, the callback calculates face_normal and flips it when
ray.volume_tree == reverse_vol. Geometry is shared by both adjacent volumes.
Thus, a normal derived from the primitive's input vertices is NOT already oriented out of
the currently tracked volume. For example:

- Surface 27: forward volume 1, reverse volume 9; triangle 327528 raw normal
  (0.00936023,0.71790886,-0.69607413).
- Surface 82: forward volume 2, reverse volume 9; triangle 477140 raw normal
  (0.15329352,0.74381715,-0.65056676).
- Surface 155: forward volume 5, reverse volume 9; raw normal +z on its cap.
  Exiting volume 9 into volume 5 needs -z, the opposite of the raw vertex normal.

This matters for the proposed primitive-level rounding. The current callback does correctly
adjust its culling normal, so this finding by itself does not explain the existing losses.
EmbreeRayTracer::ray_fire also negates the stored hit normal after traversal, but currently
returns only distance/surface and does not use that normal for point placement.

## Mesh audits

Every volume has zero topologically open edges, zero edges with >2 incident faces,
zero inconsistent edge windings, and zero float triangles with exactly zero computed area.
Those checks do NOT establish absence of self-intersections or overlaps.
The maximum longest-edge/altitude ratio is 567040 (triangle 678219, surface 117).
Three float triangles have an altitude below the largest coordinate ULP of their vertices:
678219 (surface 117), 677643 and 677810 (surface 154).

Source coordinate audit: triangle 677810 on surface 154 is essentially collinear already
in the input. Its source normal z component is +6.8846837952829532e-18. After rounding its
vertices to float, even recomputing the geometry's normal accurately gives z=-2.9990587790962309e-9.
The ordinary float cross product agrees in sign (-2.9976945370435715e-9).
Therefore this is an actual geometric inversion during vertex quantization, rather than
just an inaccurate cross product on unchanged geometry. It is the only detected normal
inversion / normal change over about 8 degrees in 678219 unique source triangles.
See quantization-audit.log for all coordinates. We have NOT tied the current three lost
histories to this particular facet; it is a separate confirmed mesh validity concern.

## Alternative float intersection calculations, no position offsets

One million IDs, seed 1, max_events 1000, mfp 1:

| Implementation | Lost | Capped | Terminated |
|---|---:|---:|---:|
| Current relative Plucker baseline (previous run) | 3 | 29683 | 970314 |
| Remove only redundant normalized barycentric rejection | 3 | 29683 | 970314 |
| Ray-aligned shear / edge test, no double fallback | 6 | 29697 | 970297 |
| Float Moller-Trumbore | 164 | 29604 | 970232 |
| Current edge predicates, direct triangle-plane t | 1 | 29688 | 970311 |
| Native Embree RTC_GEOMETRY_TYPE_TRIANGLE + robust scenes | 5 | 29687 | 970308 |
| Fixed origin per straight flight + increasing minimum t | 1 | 29688 | 970311 |

These are diagnostics, not rankings or production fixes. Histories change with arithmetic.
The custom-kernel interposition alternatives also affect initial point classification.
Native Embree uses the original XDG initialization, native triangle intersection, standard
RTCRayHit layout, and an intersection filter reproducing exiting-normal and face-history
checks. The native ray tnear is zero; unlike the custom code it cannot recover a negative
intersection that the existing PLUCKER_ZERO_TOL would snap to zero. Small positive returned
native distances retain the existing snap. Therefore absolute counts are not directly a
controlled measure of relative kernel quality. Native loss of 205858 and other mid-history
boundary failures establish that the custom primitive is not necessary for such a failure.

The fixed-flight experiment holds the last collision origin/direction fixed, advances a
float ray parameter, filters candidate distances below the current parameter, and computes
world positions from the original ray. It reduces repeated-origin drift but leaves one loss
(463519), so it does not solve all boundary-placement issues. There were no behind-parameter
candidates accepted in the completed fixed-flight-near run.
The initial fixed-flight.cpp experiment without a minimum-distance filter was invalid and
was stopped; only fixed-flight-near.cpp / fixed-flight-near.log are meaningful results.

## Concrete corner failure: 615368

The plane-distance and native-Embree variants both lose this history at event 174.
The plane variant intersects triangle 678198 (surface 155, z=0) at event 173:

- Correct t on the same stored float ray/geometry: 0.31579014428662222.
- Float triangle-plane t: 0.31579014658927917 (correctly rounded).
- The true intersection's near-edge barycentric coordinate is positive, 8.840631667e-7.
- After the float r += t*u update, the corresponding coordinate is NEGATIVE, -3.829667271e-7.
- The stored point is exactly on the cap plane (z=0) but just outside the cap polygon.
- The particle switches volume 9 -> 5, the face is excluded from future intersections,
  and the following query finds no exiting intersection for volume 5.

Thus, the hit test and t can be correct, yet rounding the global endpoint changes its
relation to an adjacent boundary at a corner. Moving along the cap normal alone is not
an all-purpose repair: the example's reconstruction normal-error extent is zero (all
vertex z values are exactly zero), while the uncertainty is in x/y near the cap edge.
This is why successful offset experiments on changed trajectories do not prove a complete
fix for every edge/corner case.

For this frozen ray, direct float barycentric reconstruction gives a point whose y is one
ULP farther inside the cap, and passes its cap predicate. Adding the relative point to the
ray origin gives an outside point. See corner.log and corner.cpp for the exact values.

## Interpretation

The finite tests support two distinct requirements: consistent triangle predicates and a
boundary continuation policy that accounts for hit-point uncertainty, adjacent faces, and
cell membership. A robust nearest-hit kernel does not guarantee a rounded world-space
point lies on the desired side of every face at an edge or vertex. A global normal bias
alone should not be presented as a universal solution.
Float-only implementation remains possible; neither an API requirement for double nor a
blanket epsilon increase follows from these results. Before selecting a production remedy,
repair near-degenerate / inverted quantized facets, define a consistent normal contract,
and validate hit point / boundary state handling on frozen failing trajectories.

The ray-aligned algorithm reference is Woop/Benthin/Wald, JCGT 2013:
https://jcgt.org/published/0002/01/05/paper.pdf
Our diagnostic deliberately omits its optional double fallback and makes no claim of the
paper's full robustness guarantees. PBRT explicitly models hit-point uncertainty:
https://www.pbr-book.org/4ed/Shapes/Managing_Rounding_Error

## Direction check requested for the rim loss (615368)

The cap edge of face 678198 shared by vertex IDs 266801 and 192887 is also an edge of
face 511063 on surface 102. These are the cap and adjacent side wall of volume 5.
The direction is (0.25883090496063232,-0.96054893732070923,-0.10174677520990372).

An offline exact-rational calculation on the STORED FLOAT ray and STORED FLOAT vertices
(edge-direction-exact.py) establishes:

- Cap outward normal (from volume 5): (0,0,+1). n.dot(unit_direction)=-0.1017467699.
  Direction is 5.839766 degrees below the cap plane.
- Wall outward normal approximately (0.13230151,-0.91160226,0.38920116).
  n.dot(unit_direction)=+0.8702823484: direction heads strongly out through the side wall.
- Exact cap point is (1.15665690312023,-8.52867845275745,0).
- The original ray exits the wall triangle 3.1808807301e-7 in ray parameter later
  (physical chord length 3.1808808965e-7). Its wall barycentrics are all positive.
- Global cap t=0.31579014428662217; global wall exit t=0.3157904623746952.
- Stored rounded cap point is (1.1566568613052368,-8.528678894042969,0).
  The y error is -4.41285518335e-7; x error is -4.18149932610e-8.
- At the exact cap point, wall signed distance is -2.76826449648e-7 (inside).
- At the rounded point, wall signed distance is +1.19918263070e-7 (outside).
- From the rounded point, that wall-plane intersection is behind the ray at
  t=-1.37792357873e-7 (and above the finite side-wall triangle's edge).

This is a short rim clip. The mathematical ray enters volume 5 through the cap and almost
immediately exits through the side wall into volume 1. Rounding skips the entire short
segment and the rounded ray moves away from volume 5; it is not positioned to re-enter.
This confirms the user's proposed glancing/edge exception. Merely being outside a tracked
volume would not, by itself, establish that the continuation cannot enter it.
Exact arithmetic here is exclusively an offline diagnostic; there is no precision fallback
in the particle tracker. See edge-direction.jsonl for the two facets and exact log for metrics.
