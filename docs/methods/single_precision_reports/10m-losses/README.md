# Three losses from the 10-million-particle single-precision run

Update: all three fixes are now integrated, including the compensated Plucker
edge calculation. The rebuilt production executable completes all three
histories. See /tmp/xdg-edge-integration/README.md for current integration and
validation results; the rest of this file records the original diagnosis.

All three IDs reproduce individually with the current production CLI, seed 1,
max events 1000, mean free path 1, and one particle per run. The supplied loss
positions, directions, event counts and backward distances match exactly.
OpenMP scheduling is not required to trigger any of the three.

No project source was changed during this investigation. All experimental
headers, overrides, drivers and output are confined to this directory.

## 9410439: collision coordinate rounding

At event 140, before the collision producing loss event 141:

- Origin: (-0.11484712362289429, -9.35221004486084, -2.413515090942383)
- Direction: (-0.7653573751449585, -0.25083887577056885, 0.59271240234375)
- Surface: 102, face 532245
- Exact surface parameter: 0.25561853007231916
- Returned float surface parameter: 0.25561854243278503 (nearest float)
- Sampled collision distance: 0.2556184221268082
- Collision parameter rounded to float: 0.2556184232234955

The collision really occurs before the surface. Its exact point, using that
float parameter without intermediate coordinate rounding, is:

(-0.3104865690599219, -9.416329082768467, -2.2620068812302634)

Stored point:

(-0.3104865550994873, -9.416329383850098, -2.2620067596435547)

Rounding pushes it outside volume 5. The exact surface parameter from that
stored point along the OLD direction is -6.642410515829902e-7, and along the
new direction is -1.4119736291411984e-6. The new direction points outward;
there is no future exiting intersection. This is collision-origin error, not
an inaccurate t or a triangle-edge rejection.

FMA coordinates improve z but the correctly rounded point is still outside:
(-0.3104865550994873, -9.416329383850098, -2.262006998062134).
The current kernel accepts its tiny negative intersection as zero due to
PLUCKER_ZERO_TOL. Thus FMA avoids this particular loss through the existing
zero tolerance, while not fixing the general wrong-side representation issue.

Moving the original stored y one float ULP toward the interior to
-9.416328430175781 produces a genuine positive exit distance 3.273322818e-6.
The replay then completes at event 248 without loss. This one-case experiment
establishes causality; it is not a proposed unconditional position offset.
A production treatment should preserve position residuals or use bounded,
volume-aware rounding when a collision starts a new flight near a boundary.

## 9008800: false-positive edge predicate and wrong initial volume

The source is (0,0,0), direction
(-0.6568208932876587, 0.053338538855314255, -0.7521578073501587).
The current find_volume returns 1, although the ray starts outside volume 1.

Triangle 379169 on surface 58 is falsely accepted. One exact Plucker edge value
is +2.903280589368027e-8; the float evaluation produces signed zero through
cancellation, even with the intentional edge clamp disabled. The other two
edge values are negative. Exact barycentric weights for the plane hit are:

(0.3320646405755872, -0.005771841945412391, 0.6737072013698252)

The ray misses that triangle. Its neighboring triangle 369599 on surface 56
also gets a false zero on the common edge (exact opposite sign), and is also
falsely accepted. Its exact weights include -0.0003185226724726758.

The spurious EXITING triangle at t=11.077595743474653 is nearer than the true
ENTERING intersection (face 379168) at t=11.077675844783533. Point-in-volume
therefore incorrectly identifies the source as inside volume 1. The particle
undergoes several collisions in that wrong volume before it eventually has
no forward hit.

The compensated distance itself is accurate for both triangle planes; it
cannot repair a false containment decision. A float-only compensated
calculation of the existing Plucker products rejects the false hits. The
initial volume becomes 9, the particle crosses surface 155 at zero distance
into volume 5, and its full single-ID replay completes without loss.
The Plucker formula and canonical shared-edge convention are retained.

## 5531682: redundant normalized barycentric rejection

At event 317, the missed exit is face 361755, surface 193, the inner box's
z=-22.5 plane. All three computed Plucker edge coordinates are positive:

(1120.4610595703125, 0.000030517578125, 781.0602416992188)

The current normalized values are:

u = 0.4107554852962494
v = 0.5892446041107178
u+v = 1.0000001192092896

The post-normalization u+v>1 check rejects the valid triangle after the Plucker
sign tests accepted it. Exact weights are all positive:

(8.204695898710893e-8, 0.41075541440714053, 0.5892445035459005)

Exact distance is 6.54856035124624; the compensated routine returns its nearest
float, 6.54856014251709. The distance calculation is working correctly.
Removing only the normalized barycentric check makes this replay complete.
That check is mathematically redundant after the existing consistent-sign
Plucker tests, and is numerically harmful here.

## Temporary counterfactuals

prototype.h/callback.cpp implement standalone LD_PRELOAD overrides; no changes
were applied to the project intersector. Every arithmetic operation in the
compensated edge prototype uses float, including FMA. The offline references
in exact.py use exact Fraction arithmetic on the stored float inputs.

Variant                         9410439     9008800     5531682
Production                      lost        lost        lost
Skip normalized bary check      lost        lost        completes
Compensate Plucker edges         lost        completes   completes
Both                            lost        completes   completes

restart.cpp separately tests 9410439 at its offending collision. Both the
one-ULP-interior point and the FMA point avoid this particular loss, including
when the edge/barycentric overrides are enabled. The one-ULP point has a
positive true forward exit; the FMA point relies on the existing zero snap.

These are targeted diagnostic replays, not a completed new 10M-particle
validation. A combined production implementation and larger validation remain
to be done if these changes are adopted.

## Files

- particle-ID.log: unmodified CLI verbose reproductions
- trace.cpp / trace-ID.jsonl: production Particle state and reference scans
- exact.py / exact.log: exact-rational confirmation
- initial.cpp / initial-9008800.jsonl: current float false-positive hits
- precise_edge.h / prototype.h / callback.cpp: float-only temporary predicates
- bary.so / edges.so / both.so: isolated experimental callbacks
- MODE-ID.log: individual replay outcomes
- restart.cpp / restart.log / restart-with-edges.log: collision-origin probes
- edges-9008800-verbose.log: corrected initial-volume replay
