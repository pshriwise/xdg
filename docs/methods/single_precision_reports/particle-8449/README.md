# Particle 8449 diagnosis

All files here are outside the project. No project sources or build settings were modified.
The drivers link to the existing single precision bld/libxdg.so and use the current project headers.

Build:

    sh /tmp/xdg-particle-8449/build.sh

Reproduce baseline:

    /tmp/xdg-particle-8449/diagnose

Correct only the surface distance selected with n_events_ == 134, using a double precision
Moller-Trumbore intersection over the SAME float vertices, position, and direction:

    /tmp/xdg-particle-8449/diagnose correct

Correct only that same distance with float Plucker math, preserving the raw edge coordinates:

    /tmp/xdg-particle-8449/diagnose float

Evaluate the frozen ray and triangle with float/clamp, float/no-clamp, and double/no-clamp:

    /tmp/xdg-particle-8449/frozen-ray

Baseline loses at event 136. Both single-distance counterfactuals terminate at vacuum surface 196
at event 431 with zero losses. These are targeted causal experiments, not production fixes.

Cause: plucker_edge_test zeros the valid third edge coordinate 1.9338913261890411e-6 because
DBL_ZERO_TOL is 20*float epsilon = 2.384185791015625e-6. This affects barycentric reconstruction
and the returned distance (0.163863405585289 rather than reference 0.15081870105300785).
A second real crossing on the same surface is at 0.15509506480995816, so the wrong distance
skips both crossings but changes the tracked volume only once, from 9 to 1.
At the subsequent collision, find_volume reports 9 but the particle tracks volume 1.

The raw float calculation without clamping returns 0.15077459812164307. This retains some
arithmetic error, but correcting only this distance also prevents this particle's loss.
The reference calculation promotes already-rounded float geometry to double; it does not
require recovering original double precision mesh vertices.
