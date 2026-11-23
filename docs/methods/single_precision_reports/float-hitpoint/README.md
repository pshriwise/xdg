# Float-only returned-hit-position experiment

No project files were modified. All intersection, hit-point reconstruction, error-bound,
and offset arithmetic in these diagnostic alternatives uses float (no double fallback).
The existing simulation's RNG, collision sampling, and tallies are retained as supplied.
Those pre-existing components use doubles, but no double arithmetic is used to improve
intersection or boundary-position accuracy here. The geometry callback shared object has
no addsd/subsd/mulsd/divsd instructions (checked with objdump on this build).

The current relative-coordinate Plucker kernel, with edge clamping disabled, is copied
into a diagnostic namespace and extended to return two candidate hit points and an error
vector. Its reciprocal uses 1.0f rather than 1.0. The original float geometry library is
copied into this directory as libxdg.so to keep runtime behavior stable.

The barycentric weights are:

    b0 = plucker_coord1 * inverse_sum
    b1 = plucker_coord2 * inverse_sum
    b2 = plucker_coord0 * inverse_sum

The original triangle coordinates, BEFORE translating by -ray_origin, are v0,v1,v2.
The standard triangle-interpolated point is:

    p = b0*v0 + b1*v1 + b2*v2

At a surface event the driver sets the particle position to this p instead of computing
origin + t*direction. It retains t for collision decisions and track length tallying.
Hit-point payloads are propagated by thread-local storage in the Embree callback; the
production API is not changed. The callback updates the payload only when it actually
accepts a closer hit, after orientation and primitive-history filters.

Three million-history experiments use IDs 0..999999, seed=1, max_events=1000, mfp=1:

- mode 0: directly assign the barycentrically reconstructed point above.
- mode 1: directly assign ray_origin + the relative-coordinate reconstructed point.
  This also avoids dividing to obtain t then multiplying by direction to recover the point.
- mode 2: use mode-0 point plus a reconstruction-roundoff bound and directed rounding.

For mode 2 the componentwise reconstruction error estimate follows PBRT's triangle formula:

    u = 0.5f * numeric_limits<float>::epsilon()
    gamma7 = (7.f*u)/(1.f-7.f*u)
    error[j] = gamma7 * (abs(b0*v0[j])+abs(b1*v1[j])+abs(b2*v2[j]))

The unit geometric normal n points from the old volume into the new volume; reflection
reverses the selected side. The driver computes extent=dot(abs(n),error), offsets by
extent*n, and applies nextafter to each changed coordinate away from the uncertainty box.
This bound concerns hit-point reconstruction; it is not a proof that the existing float
Plucker predicate always identifies the correct triangle or accurately bounds t.
Zero losses in a finite set of histories is not a proof of correctness for all rays.

References:
https://www.pbr-book.org/4ed/Shapes/Triangle_Meshes
https://www.pbr-book.org/4ed/Shapes/Managing_Rounding_Error

Build and run:

    sh /tmp/xdg-float-hitpoint/build.sh
    OMP_NUM_THREADS=4 LD_PRELOAD=/tmp/xdg-float-hitpoint/point.so /tmp/xdg-float-hitpoint/batch 1000000 0 0
    OMP_NUM_THREADS=4 LD_PRELOAD=/tmp/xdg-float-hitpoint/point.so /tmp/xdg-float-hitpoint/batch 1000000 0 1
    OMP_NUM_THREADS=4 LD_PRELOAD=/tmp/xdg-float-hitpoint/point.so /tmp/xdg-float-hitpoint/batch 1000000 0 2

The results are mode-0.log, mode-1.log, and mode-2.log. Each reports loss count,
number of histories still alive at the event cap, and number terminated normally.

## Targeted tests preserving the original trajectory until its failing crossing

604449: assigning only the float barycentric hit point at event 150 STILL loses at event 151.
The returned point is (1.2107303142547607, 5.6348409652709961, 2.9595980644226074).
Its reconstruction-error estimate is (5.05156e-7, 2.35104e-6, 1.23484e-6).
Applying the error-bound-based offset at this one crossing prevents the loss.
See targeted-604449.log and targeted-bounded-604449.log.

742427: assigning the float barycentric point at event 14 fixes this loss; the stored
z coordinate becomes exactly zero, matching the triangle's three zero z coordinates.
205858: assigning the float barycentric point at event 229 avoids loss through event 1000.

Targeted tests are important because changing every prior crossing also changes the
trajectory: an aggregate zero-loss run alone would not demonstrate a specific repair.

## Completed million-history results

| Navigation update | Lost | Capped at 1000 | Terminated |
|---|---:|---:|---:|
| Current distance-only baseline (prior run) | 3 | 29683 | 970314 |
| Mode 0: direct triangle-barycentric point | 2 | 29697 | 970301 |
| Mode 1: origin plus reconstructed relative point | 1 | 29690 | 970309 |
| Mode 2: triangle point plus error bound and directed rounding | 0 | 29674 | 970326 |

Mode 0 losses: 847510,969665. Mode 1 loss: 615368.
Mode 2's zero-loss result is limited to the tested million histories; it does not
establish correctness of all Plucker predicates, nearly degenerate triangles, or
arbitrarily thin cells. Histories at the event cap are not followed to completion.
