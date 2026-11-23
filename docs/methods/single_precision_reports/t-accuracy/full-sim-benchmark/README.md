# Full particle-sim timing on the teapot model

The benchmark runs the existing production executable, not a custom transport
driver. No project source or build outputs are changed.

Command for each timed run:

    taskset -c 14 /home/pshriwise/soft/xdg/bld/tools/particle-sim \
      /home/pshriwise/tmp/teapot_rt/teapot.h5m -n 100000 -s 1 -e 1000

The executable is Release, XDG_SINGLE_PRECISION=ON, MOAB + Embree. No verbose
logging or implicit-complement graveyard is requested. The mean free path is
the CLI default, 1. The simulator's particle loop is serial. Each entire process
is pinned to CPU 14, with OMP_NUM_THREADS=1 and OMP_DYNAMIC=FALSE.

Three variants:

- stock: existing executable and project library, without LD_PRELOAD.
- control: unchanged current triangle callback loaded using LD_PRELOAD.
- compensated: callback selecting the compensated distance calculation.

The control and compensated libraries use identical -O3 -DNDEBUG -mfma
-ffp-contract=off -fPIC -shared -fopenmp flags and the same single-precision
project feature definitions and link dependencies. After normalizing which
intersector is selected, the two callback sources are identical. The control
helps distinguish compiler/loading differences from the t calculation itself.

Three sequential blocks rotate the variant order so each variant runs once
in each position. No two simulator runs overlap. Wall time includes process
startup, mesh loading, BVH preparation, transport and final tally output. Input
file pages are expected to be cached by the OS from preceding investigations.
No system caches, CPU governor settings or user services are changed.

`run_benchmark.py` is the complete timing orchestrator. `metadata.json` records
arguments, CPU model, ordering and SHA256 digests of the executable, library,
triangle header, mesh and experimental shared objects. `results.json` records
every completed run; `summary.json` contains final medians and ratios. The
*.log files are raw CLI output, and *.time files are GNU time wall/user/system
seconds and peak resident memory in kB. `progress.jsonl` records progress.

The same particle count and random seed are used for every variant. Different
t values can change individual histories, so this measures actual application
cost for the workload, not a replay of identical triangle queries. The earlier
frozen-ray microbenchmark separately measures the triangle-routine cost on
identical inputs.

The completed results are in RESULTS.md. A secondary experiment in
run_generic.py measures the compensated callback without -mfma and a
zero-particle setup run; its data are generic_results.json and
generic_summary.json. combined_summary.json collects both experiments.
