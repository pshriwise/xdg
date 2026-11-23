# OpenMP particle transport

Implemented in tools/particle_sim.h:
- OpenMP parallel-for over particle IDs for nonverbose Embree transport, with
  static scheduling. OMP_NUM_THREADS controls worker count.
- The shared cell_tracks map is populated before entering the parallel loop.
  Each score obtains an existing value with at() and uses omp atomic update.
  No private track-length maps or final tally-map merge are used.
- Lost/stuck counts use integer reductions and retain pre-existing totals.
- Per-particle seed assignment is unchanged. RNG state uses the same 48-bit LCG
  and the same double output as erand48, computed entirely from particle-local
  state to avoid libc RNG global parameters. Tests check exact sequence equality.
- Verbose transport stays serial, and lost diagnostics have a named critical
  region so complete messages stay together. GPRT host queries remain serial
  because that backend shares query buffers.
- Standalone Particle::advance calls retain the original map-insertion behavior;
  transport passes shared_tallies=true to select checked lookup and atomic scoring.
- Builds without OpenMP retain the serial loop and ordinary additions.

CLI help documents OMP_NUM_THREADS and serial verbose behavior. The existing
OpenMP linkage for tools was sufficient; test_particle_sim now links OpenMP
when available so it exercises the threaded implementation.

Checks:
- Existing bld (currently double precision): particle-sim and test_particle_sim
  build successfully. Tests: 195 assertions pass, 4 cases pass, 2 unavailable
  GPRT cases skip.
- Separate /tmp/xdg-openmp/bld-single build: same test results in single precision.
- Unit tests exercise multiple actual geometry-query workers, 257 particles
  with a nonzero starting ID, atomic cell tallies, existing counters/tallies,
  a zero-particle call, and exact RNG parity against erand48.
- Old/new double-precision serial CLIs produce exactly identical tallies for
  1000 particles. The 8-thread output differs only by summation roundoff.
- A manually compiled no-OpenMP CLI matches the OpenMP 1-thread single build
  exactly for 1000 particles.
- Verbose CLI with 2 particles / 3 events and OMP_NUM_THREADS=8 retains ordered
  complete particle histories.
- The project library's hash is unchanged. No precision setting was changed;
  the existing bld stays double precision. The user's already-running serial
  job was not interrupted.
- git diff --check passes.

End-to-end performance, actual single-precision CLI, teapot.h5m, n=100000,
seed=1, max events=1000, mfp=1, no verbose, OMP_DYNAMIC=FALSE:

Threads  Wall seconds  Relative speedup
1        40.679        1.00
8         6.996        5.81
16        5.194        7.83

These are single timing observations including geometry loading and setup,
on the same workstation while the user's serial run was also active. All
three runs lost zero particles and their tallies agree within floating-point
summation roundoff. Atomic summation order can change the last bits of totals;
particle RNG histories are independent of the worker schedule.

benchmark.py, benchmark.json, benchmark-summary.json, and benchmark-progress.log
contain the reproducible timing procedure and data. cli-checks.json records
comparison with the old executable. tests-current.log/tests-single.log record
unit results. before/ and before.patch preserve the pre-change state.

Use:

    OMP_NUM_THREADS=16 ./bld/tools/particle-sim ~/tmp/teapot_rt/teapot.h5m -n 10000000

The isolated single-precision executable is:
/tmp/xdg-openmp/bld-single/tools/particle-sim
