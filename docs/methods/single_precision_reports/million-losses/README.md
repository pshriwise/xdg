# Million-particle investigation

No project code was modified. The directory contains a copy of the user's current single
precision library (libxdg.so) and snapshots of the intersection and constants headers.
The user's code removes edge clamping and calculates Plucker coordinates/intersections
relative to the ray origin. PLUCKER_ZERO_TOL remains 10*float epsilon.

The current batch driver ran IDs 0..999999 with the original Particle header, seed 1,
max_events 1000, mfp 1, original initialization/collision/reflection/volume logic.
Every thread uses its own Particle and tally map; the immutable geometry is shared.
A serial original particle-sim run independently reproduced the first loss (205858),
then was stopped to avoid duplicating the already-completed million-history batch.

## Million-history results

- Current relative-coordinate float implementation: lost 3, capped 29683, terminated 970314.
- Double intersection intermediates, float transport: lost 1, capped 29690, terminated 970309.
- Double intersection intermediates plus accurate boundary placement and outward rounding:
  lost 0, capped 29686, terminated 970314.
- A second placement variant also explicitly projects onto the triangle plane along the
  dominant normal component before rounding and checks the stored point is on the intended
  side: lost 0, capped 29686, terminated 970314. No unresolved placement checks occurred.
  Results are in double-placement-bounded.log and placement-checks.log.

The zero-loss result is empirical for this million-history test, not a general robustness proof.
Histories differ between algorithms, and capped histories have not been followed to completion.
The double kernel is the earlier diagnostic interposition library; it promotes already-float
mesh vertices and ray coordinates for arithmetic. Mesh and particle storage remain float.

## Current failures

### 205858, event 230

Previous crossing: event 229 -> 230, surface 27, triangle 327528, volume 9 -> 1.
Relative float t=0.39229828119277954; double reference t=0.39229872927594345.
The nearly grazing hit is also close to a triangle edge (reference barycentric b1=0.000116777).
After float advancement, the same ray intersects the triangle plane at t=2.1929842061228834e-6
with b1=-0.0001368542390596821 (outside the triangle). The point is still physically in volume 9.
Correcting only this distance avoids loss through the 1000-event limit.
Accurate boundary placement and destination-side rounding at this one crossing also avoids loss.

### 604449, event 151

Previous crossing: event 150 -> 151, surface 82, triangle 477140, volume 9 -> 2.
Relative float t=0.083037964999675751; double reference t=0.08303808982655185.
IMPORTANT: replacing only this t with its correctly rounded float value STILL loses at event 151.
The endpoint r += t*u is itself rounded in float and remains on the wrong side near an edge.
Baseline signed triangle-plane distance magnitude is 1.844750104479151e-7.
A diagnostic reconstructs the intersection from the original ray and triangle in double and
rounds the stored float position into the destination side along the geometric normal.
Applying it only at this crossing moves the point by 5.8400388525114977e-7 and the particle
terminates at event 431, surface 197, without loss.

### 742427, event 16

Previous crossing: event 14 -> 15, surface 155, triangle 678120 (all vertex z coordinates zero),
volume 9 -> 5. Float t=0.6750258207321167; reference t=0.67502710457259996.
Stored z=+2.9802322387695312e-7, still above the surface although volume 5 is below it.
Next collision distance 4.8209300270695353e-7 leaves z=+1.8727703832155385e-7.
New direction points upward; the volume-5 exit is now behind it at -1.2156704984795299e-6,
just outside the fixed distance-to-zero tolerance 1.1920928955078125e-6.
Correcting placement at the one crossing avoids loss; it terminates at event 189.

## Double-kernel-only failure

114777, event 150. The double-intermediate kernel returns a correctly rounded hit distance,
but rounding r += t*u after crossing surface 106, face 610073 (event 148 -> 149) leaves the
particle on the previous side. A short collision distance 4.5394277137564741e-7 is not enough
to repair that; direction changes/history clears. Exit t=-1.2002223355444001e-6 is just beyond
the fixed snap tolerance. Accurate point placement/destination-side rounding applied only
at the preceding crossing avoids the loss, terminating at event 638.

## Implication

Improving the intersection formula alone does not make boundary transitions consistent.
The hit position must be reconstructed accurately, with its rounding uncertainty accounted
for when storing it and assigning the destination volume. Returning only a float t and
recomputing r += t*u cannot guarantee this, even if t was calculated in double.
A production approach should carry the hit point and its error bound (or equivalent boundary
state), then position the next query on the intended side using the geometric normal and
coordinate-scale representable spacing. Keep the physical hit distance for tallying.
The driver is a diagnostic example; a robust implementation must also consider edges,
very thin regions, and reflection, without selecting an arbitrary global spatial epsilon.

## Reproduction

    sh /tmp/xdg-million-losses/build.sh
    OMP_NUM_THREADS=5 /tmp/xdg-million-losses/batch 1000000
    OMP_NUM_THREADS=5 LD_PRELOAD=/tmp/xdg-million-losses/double.so /tmp/xdg-million-losses/batch 1000000
    OMP_NUM_THREADS=5 LD_PRELOAD=/tmp/xdg-million-losses/double.so /tmp/xdg-million-losses/batch-placement 1000000
    OMP_NUM_THREADS=6 LD_PRELOAD=/tmp/xdg-million-losses/double.so /tmp/xdg-million-losses/batch-placement-bounded 1000000

    /tmp/xdg-million-losses/trace 604449 150
    /tmp/xdg-million-losses/trace-place 604449 150 place
    /tmp/xdg-million-losses/trace-place 205858 229 place
    /tmp/xdg-million-losses/trace-place 742427 14 place
    LD_PRELOAD=/tmp/xdg-million-losses/double.so /tmp/xdg-million-losses/trace-place 114777 148 place

The build script compiles against project headers; use matching header settings when rebuilding.
The copied runtime library ensures the already-built diagnostics use a stable library snapshot.
