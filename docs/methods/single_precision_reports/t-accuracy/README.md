# Float-only evaluation of triangle hit parameter t

These are standalone diagnostic files. No project source was changed for this
investigation. The project retains its existing Plucker edge tests and distance
calculation. The prototype retains the edge/orientation/barycentric decisions,
zero-distance tolerance and parameter bounds; it substitutes only t evaluation.

## Residual lost particle

Input: /home/pshriwise/tmp/teapot_rt/teapot.h5m
Current single-precision build with fixed-flight continuation, seed 1.
Particle 463519 crosses cap triangle 678109 (surface 155), event 59.
All cap vertices have z=0. The exact hit parameter for the stored float ray is
30026824 / 15923445 = 1.8856989803399955224...

Current t:                 1.8856683969497680664
Homogeneous weighted t:    1.8856682777404785156
FMA weighted t:            1.8856682777404785156
Compensated edge weights:  1.8857012987136840820
Ordinary plane t:          1.8856989145278930664
Compensated determinant t: 1.8856990337371826172 (nearest float)

Current error: -3.0583390227456e-5, or -256.552 ULPs of t.
The reconstructed position has z=7.2419643402099609e-6 when the volume changes
to 5, before the ray actually reaches the cap. A collision occurs while z is
still positive; its new direction points up and the next volume-5 exit query
misses. This explains why fixed-flight continuation alone does not cure this
particular loss: the intervening collision starts a new flight before the
geometric entry has actually occurred.

The final weighted sum has condition number 1 in this case. The primary error
is in the rounded Plucker coordinate magnitudes feeding the weights, rather
than cancellation in that final sum. Merely delaying normalization or using
FMA for the last weighted sum does not fix it. The edge signs still agree.

## Candidate calculation

For e1=v1-v0, e2=v2-v0, n=e1 cross e2:

    t = dot(n, v0-origin) / dot(n, direction)

`t_compensated.h` evaluates this expression using a {value,error} pair of
Scalar values, preserving residuals through vertex subtraction, cross and dot
products, and correcting the final quotient. In a single build every operation
is float. FMA recovers product residuals; TwoSum recovers addition/subtraction
residuals. No runtime double or long-double fallback is used by this helper.
This does keep additional significand information in extra float values.

Compilation used -O3 -mfma -ffp-contract=off, with no fast-math. The explicit
FMA operations are intentional; algebraic reassociation would invalidate the
compensation steps. Disassembly of t_accuracy::distance contains 36 scalar
float FMA instructions and no double arithmetic or external calls on this CPU.
This is a prototype for the finite mesh/rays tested, not a general error bound
or a complete treatment of degenerate triangles, overflow or subnormals.

## Frozen-ray accuracy comparison

`capture.cpp` records production Particle rays and hit vertices. `samples.jsonl`
contains 55,278 accepted hits from 128 trajectories (IDs (1931*i)%1000000).
After excluding 128 zero-distance initialization hits, 55,150 remain.

`evaluate.cpp` compares multiple float-only candidates on identical inputs.
Only its offline reference calculations use long double. Its reference is the
exact geometry represented by the original float vertices, origin and direction,
without first rounding translated vertices. 257 difficult/near-rounding-tie
cases were also checked against Python Fraction exact-rational arithmetic,
including the worst old error and the worst partial-compensation error; these
checks found no discrepancies in the fully compensated results.

Method                      p99 error (ULPs)   max error (ULPs)
Current weighted point          15.637              7455.460
Homogeneous weighted point      15.122              3443.460
FMA homogeneous point           15.070              6777.460
Compensated edge coordinates     9.596             14239.540
Ordinary plane ratio             1.945               284.628
Compensated normal/dot           1.467               174.628
Compensated determinant+divide    0.495                 0.500

The fully compensated result matched the nearest float reference for all
55,150 sampled nonzero hits. This is an observation on these samples, not a
proof of correct rounding for arbitrary inputs. Simple plane arithmetic is
also not uniformly more accurate than the original routine on every ray.

## Transport experiment

`callback_compensated.cpp` is a temporary copy of the current Embree triangle
callback. It calls the prototype in `plucker_compensated.h`. The callback is
interposed using LD_PRELOAD; the project library itself is unchanged.

    /usr/bin/time -f 'elapsed_seconds=%e' env \
      LD_PRELOAD=/tmp/xdg-t-accuracy/callback_compensated.so \
      /tmp/xdg-fixed-flight-implementation/validate_transport

The driver invokes the real production transport_particles function with seed
1 on eight disjoint ID ranges covering particles 0 through 999999. Result:

    SUMMARY particles=1000000 lost=0 stuck=0
    elapsed_seconds=68.05

`teapot-million-compensated.log` is the complete output. Without the override,
the prior production run reported one lost particle, ID 463519 (see the previous
/tmp/xdg-fixed-flight-implementation/teapot-million-parallel.log).
This is evidence for this mesh and seed; other meshes and longer runs remain
unmeasured. GPRT/Slang was not modified or tested.

## Cost

`benchmark.cpp` evaluates full accepted-triangle routines, including the same
edge tests and final filters, for 128 passes over the captured inputs. All
variants used identical -O3 -mfma -ffp-contract=off compiler options.

Current routine:              96.2 ns / accepted triangle
Ordinary plane routine:       79.2 ns / accepted triangle
Compensated routine:         162.9 ns / accepted triangle

The prototype cost was about 1.69x the current accepted-triangle routine in this
microbenchmark. This is not a measurement of total application slowdown; BVH
traversal, rejected triangles and other transport work are not represented by
this benchmark. No matched end-to-end speed comparison was performed.

## Useful files

- t_compensated.h: float-only candidate implementation
- callback_compensated.cpp, plucker_compensated.h: temporary intersection override
- capture.cpp, lost-463519.jsonl, samples.jsonl: production frozen-ray capture
- evaluate.cpp, summarize.py, summary.txt: candidate comparisons and statistics
- exact-check.txt: independent exact-rational spot-check result
- benchmark.cpp, benchmark.log: accepted-triangle timing
- distance-disassembly.txt: single-precision distance helper instructions
- plucker.before.h: byte-for-byte snapshot of the unchanged project header
