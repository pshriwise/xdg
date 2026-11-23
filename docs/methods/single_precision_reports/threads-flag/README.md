# particle-sim thread-count flag

Added -t / --threads as a positive integer option. Explicit values call
omp_set_num_threads and override OMP_NUM_THREADS. Omitting the flag leaves
OpenMP's existing environment/runtime defaults in effect. Builds without
OpenMP accept -t 1 and reject requests for more workers. Invalid argument
syntax and nonpositive thread counts return a failure status.

Rebuilt the current bld executable and the separate single-precision executable.
No precision settings were changed. Check scripts and logs are in this directory.

Runtime checks use OMP_DISPLAY_AFFINITY and a custom display format to verify
actual teams: -t 4 overrides environment=1; --threads 3 gives three workers;
omitted flag with environment=2 gives two workers; -t 1 overrides environment=8
(the GNU runtime does not print affinity for its singleton team). The actual
128-particle CLI tallies agree across these thread counts within summation
roundoff. A single-precision run also confirms four actual workers.

Rejected values: zero, negative, noninteger, out-of-range integer, missing value.
The no-OpenMP executable accepts -t 1 and rejects -t 2. --help exits successfully
and lists both flag names. git diff --check passes.
