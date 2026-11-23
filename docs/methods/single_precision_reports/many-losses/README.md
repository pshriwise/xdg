# Broader single precision loss investigation

No project files were modified. All drivers and alternative callbacks are in this directory.
The current user build has both epsilon multipliers set to 10. Source snapshots are *.original.*.
The existing particle-sim executable reproduced 9 losses out of 100000 histories.
The batch driver independently reproduced the same IDs, states, and event counts.

## Results

| Kernel | Lost | Reached 1000 event cap | Terminated |
|---|---:|---:|---:|
| Existing build | 9 | 2914 | 97077 |
| Disable edge clamping only | 2 | 2916 | 97082 |
| Disable edge clamping, double intersection intermediates | 0 | 2917 | 97083 |
| Disable edge clamping, float intersection in ray origin coordinates | 0 | 2916 | 97084 |

Loss IDs, existing kernel: 2870,25907,30657,32781,45705,45767,59680,61288,92922.
Without edge clamping: 45705,45767.
Each run uses IDs 0..99999, base seed 1, max_events 1000, mfp 1, and the same particle header.
Position, direction, vertices, normals, Embree BVH, and transport stay single precision in ALL runs.
The double variant promotes already-rounded float vertices/ray values for intersection arithmetic.
All alternate kernels keep the existing distance-to-zero snap (10*float epsilon).
The local variant translates triangle vertices by -ray_origin, evaluates the same Plucker kernel
without edge clamping, and reconstructs intersection coordinates relative to the ray origin.
These variants change trajectories, so counts alone do not prove each eliminated loss's cause.
Specific histories were examined below to establish causes.

## Additional failure: bad surface location without edge clamping

45767, event 166, surface 155, triangle 678049, a triangle on z=0:
- Float Plucker distance: 0.68100059032440186
- Double reference on SAME float vertices/ray: 0.68111159097243623
- Advancing by float result leaves z=+4.4852495193481445e-5, before the plane.
- The code nevertheless changes volume 9 -> 5 and excludes the hit triangle.
- Next collision distance 4.6880135217614447e-5 is insufficient to reach z=0.
- At collision z=+2.5914525394910015e-5; direction changes and history clears.
- Actual volume is 9, tracked volume is 5, and its outward surface lies behind it.
- Loss at event 168. This also occurs without edge clamping; no edge coordinate was clamped.
- Correcting ONLY the event-166 distance in the no-clamp replay avoids loss, terminating at event 435.

45705, event 30, surface 83, triangle 428557:
- Float Plucker distance: 1.0295432806015015
- Double reference: 1.0295438511728832
- Premature volume 9 -> 2 crossing, immediately followed by collision of distance 3.3597914348843247e-8.
- Float position does not move on that collision; the particle remains physically outside volume 2.
- Direction change clears history; relevant exit distance is -1.8016874767863034e-6 (double).
- Loss at event 32. Correcting ONLY the event-30 distance avoids loss, terminating at event 814.

The global-coordinate edge formula subtracts terms that can be much larger than the result;
errors in normalized barycentric weights then contaminate the reconstructed intersection.
The returned t uses only the largest-direction component, so the barycentric point need not
agree with origin+t*direction in the other components. For 45767 the two points differ by
5.5067474022507668e-5 even though all triangle vertices have z exactly zero.
The origin-relative and double variants reduce these arithmetic errors in this experiment.

## Edge clamping also creates false positives

For 2870, event 244, triangle 372199 has one negative raw edge coordinate (-2.332963e-7)
and two positive ones. Clamping it to zero falsely accepts a triangle outside the ray.
Later, event 247, triangle 379861 has raw edges (-3.600772e-5,-2.177805e-5,+1.011416e-6).
Clamping the positive one admits another false hit, switching into the wrong volume.
This shows the predicate itself needs attention, beyond preserving raw weights for t.

## Reproduction

    sh /tmp/xdg-many-losses/build.sh
    OMP_NUM_THREADS=3 /tmp/xdg-many-losses/batch 100000
    OMP_NUM_THREADS=3 LD_PRELOAD=/tmp/xdg-many-losses/unclamp.so /tmp/xdg-many-losses/batch 100000
    OMP_NUM_THREADS=3 LD_PRELOAD=/tmp/xdg-many-losses/double.so /tmp/xdg-many-losses/batch 100000
    OMP_NUM_THREADS=3 LD_PRELOAD=/tmp/xdg-many-losses/local.so /tmp/xdg-many-losses/batch 100000

    LD_PRELOAD=/tmp/xdg-many-losses/unclamp.so /tmp/xdg-many-losses/trace 45767
    LD_PRELOAD=/tmp/xdg-many-losses/unclamp.so /tmp/xdg-many-losses/trace 45767 correct
    LD_PRELOAD=/tmp/xdg-many-losses/unclamp.so /tmp/xdg-many-losses/trace 45705
    LD_PRELOAD=/tmp/xdg-many-losses/unclamp.so /tmp/xdg-many-losses/trace 45705 correct
    /tmp/xdg-many-losses/frozen 45767

The preload libraries replace only the existing triangle callbacks in the running process.
They are diagnostic experiments, not a validated production fix. Zero losses above is limited
to these 100000 histories; approximately 2900 histories reach the existing event limit.
