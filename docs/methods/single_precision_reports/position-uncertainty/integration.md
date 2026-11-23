# Production integration

The previously standalone surface-first position-uncertainty guard now runs in
the particle simulator. Project changes for this integration:

- tools/position_uncertainty.h: outward-rounded Scalar intervals through sampled
  distance conversion, absolute flight parameter, position reconstruction,
  triangle normal, and interior plane clearance. All bound arithmetic is float
  when XDG_SINGLE_PRECISION is enabled.
- tools/particle_sim.h: accept a nominally earlier collision only when the
  clearance lower bound is positive. Otherwise advance to and score the original
  surface parameter. advance() returns the selected event, and the transport
  loop uses that same selection for collision/surface dispatch. Verbose output
  identifies when an uncertain collision becomes a surface event.
- tests/test_particle_sim.cpp: regression coverage for forward/reverse surface
  senses, unchanged interior collisions, full surface advancement/tallying, and
  the frozen oblique collision from particle 9410439.

The triangle intersection routine and the user's disabled normalized
barycentric check were not changed by this integration. OpenMP tally atomics
and the existing particle RNG remain in place.

## Rebuilt simulator replays

Built bld/tools/particle-sim with XDG_SINGLE_PRECISION=ON. Each replay uses the
actual rebuilt executable, without any LD_PRELOAD override, seed 1, MFP 1,
max events 1000:

    bld/tools/particle-sim ~/tmp/teapot_rt/teapot.h5m -n 1 -v --start-particle-id ID

| Particle | Outcome |
| --- | --- |
| 9410439 | Guard triggers at original event 140; exits vacuum surface 194 at event 442; no loss. |
| 5531682 | Exits vacuum surface 193 at event 388; no loss. |
| 9008800 | Same separate predicate/initial-volume problem; lost at event 5. |

See integrated-ID.log for each verbose history. The second particle benefits
from rebuilding the user's previously disabled barycentric check; the new guard
does not trigger for it.

## Larger production run

The rebuilt single-precision simulator completed 1,000,000 histories, IDs
0..999999, with zero reported losses, seed 1, MFP 1, max events 1000, 8 workers:

    bld/tools/particle-sim ~/tmp/teapot_rt/teapot.h5m -n 1000000 -t 8

Elapsed time: 83.35 seconds, including loading/preparing the model. This is a
validation run, not a matched overhead measurement. integrated-million.log
contains the output and final track-length tallies. No LD_PRELOAD override was
used.

## Regression tests

Both precision builds compile particle-sim and test_particle_sim successfully.

- Single precision: 6 passing cases, 234 passing assertions; 3 GPRT cases skipped
  because that backend is not enabled.
- Double precision compatibility: 5 passing cases, 229 passing assertions; 3 GPRT
  cases skipped. No double precision arithmetic is used to support single
  precision geometric bounds.
- git diff --check passes.

## Scope

The bound tests the selected outgoing facet only. Other nearby or entry facets
are not covered. The separate 9008800 predicate problem is intentionally left
unchanged. The three replay outcomes establish that one of the three reported
losses remains; they do not establish the total number of losses in a complete
10-million-history rerun.
