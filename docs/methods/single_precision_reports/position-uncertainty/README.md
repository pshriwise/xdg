# Conservative collision selection from position uncertainty

Update: the guard is now integrated in tools/particle_sim.h, with interval
arithmetic in tools/position_uncertainty.h. See integration.md for production
build and replay results. The remaining text records the original standalone
prototype investigation.

Requested policy: prefer a surface event when the possible collision position
straddles the surface. Use the lower uncertainty limit to certify a collision.
These files are standalone prototypes. Project source was not modified.

## Decision

1. Keep the existing nominal comparison: sampled collision distance < returned
   local surface distance. Otherwise choose the surface immediately.
2. Convert the sampled distance to the configured Scalar. Bound that conversion
   using neighboring Scalar values; include the original sampled distance.
3. Propagate outward-rounded intervals through the accumulated flight parameter,
   multiplication by direction, and addition of the fixed flight origin. The
   resulting position box encloses the nominal exact point and the actual stored
   point, including their intermediate rounding.
4. Form an interval normal from the selected face's stored vertices. Orient it
   outward from the current volume using surface_sense, rather than assuming
   the raw triangle normal has the desired volume orientation.
5. Bound g = dot(n_out, vertex0 - collision_position). Its positive side is the
   interior halfspace. Accept a collision only if the entire interval is strictly
   positive, i.e. g.lower > 0. Otherwise take the surface event.

The sign test avoids dividing by n dot direction for a nearly tangent ray.
It is the plane-clearance version of conservatively lowering the acceptable
collision cutoff. The interval operations use Scalar and nextafter throughout;
Single precision uses no double/long-double arithmetic for these bounds.
The separate reference checker uses long double only for independent auditing.

A single event decision controls BOTH advancement and collision/surface dispatch.
The sampled collision distance is never biased or overwritten. A surface event
advances to the returned absolute surface parameter and scores the corresponding
actual segment, not to an artificially shortened lower bound. A certified
collision retains the sampled distance and the usual position reconstruction.

The interval routines use outward rounding after each basic operation, including
bounds on cross products and dot products. The arithmetic assumptions are finite,
nonnegative flight inputs and ordinary IEEE floating-point arithmetic; the tested
mesh and rays are well within the normal finite range. No global epsilon or
fixed number of distance ULPs is used for the decision.

## Particle 9410439

The only ambiguous decision in the original history occurs at event 140:

sampled collision distance: 0.255618422126808187
returned surface distance:  0.255618542432785034

The bounded unnormalized inside-plane value is:

[-4.224421346066265e-8, +2.582965130670800e-8]

Zero lies in the interval, so the collision is not certified inside the face.
The surface-first replay completes at event 442 without loss. The original
policy loses the particle at event 141. No coordinate nudge was performed.

The position box is:

x [-0.310486644506454468, -0.310486495494842529]
y [-9.41633033752441406,  -9.41632843017578125]
z [-2.26200723648071289, -2.26200652122497559]

For interpretation only, normalization by the exact face-normal magnitude in
an offline calculation gives interior clearance approximately
[-1.272032046e-6, +7.777667404e-7]. Its equivalent along-ray clearance interval
is approximately [-3.046980200e-6, +1.863034713e-6]. The guard itself needs
neither normalization nor this division. frozen-bounds.txt records these values.

## Larger sample

A separate run follows 1,000,000 histories, IDs 0..999999, seed 1, mean free path
1, max events 1000, 8 CPU workers. The temporary bary.so callback disables the
legacy normalized BC rejection, matching the user's source edit without changing
the working build. The Plucker edge arithmetic is otherwise unchanged.

SUMMARY particles=1000000 candidates=356135121 guarded=56 outside=2 unsafe_accepted=0 lost=0
seconds=93.11

The guarded fraction is approximately 1.57e-7 of nominal collision candidates.
Two guarded proposed stored points are on or beyond their selected face plane;
the other cases are inside but not certified by the conservative interval.
An independent long-double plane calculation found no accepted collision on
or beyond its selected outgoing face. This is an empirical audit as well as
an interval-based certificate; it is not a complete proof about the whole
simulation or arbitrary meshes. The timing includes diagnostic reference work
and is not a matched production overhead measurement.

The 10,000-history preliminary run had 3,541,928 nominal collision candidates,
zero guarded decisions and zero losses (sample.log).

The predicate problem for particle 9008800 remains separate; this position
policy does not repair its wrong initial volume.

## Scope of the guarantee

The interval test certifies the side of the SELECTED OUTGOING FACET. It does
not establish membership in an arbitrary nonconvex volume. At a shared edge,
corner or very nearby second surface, a full-volume guarantee would require
checking other facets whose geometry intersects the position-uncertainty box.
It also does not establish that a previously crossed entry face is safely behind
the reconstructed point; that can matter for collisions immediately after entry.
Those are necessary extensions before claiming arbitrary-geometry robustness.

This prototype tests the requested lower-side, surface-first policy against
the observed case. No production implementation or complete 10M validation has
yet been performed for it.

## Files

- uncertainty.h: Scalar-only interval arithmetic, point box and plane bound
- prototype.cpp: independent single-ID replay and parallel sample/audit driver
- individual.log: original versus guarded results for the three supplied IDs
- sample.log / million.log: validation summaries
- frozen-bounds.txt: offline interpretation of the interval for 9410439

The general use of position error boxes and normal projection is described in:
https://www.pbr-book.org/4ed/Shapes/Managing_Rounding_Error
