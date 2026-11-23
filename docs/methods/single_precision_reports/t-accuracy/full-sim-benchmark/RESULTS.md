# Full simulator timing results

The new compensated t calculation adds a few percent to total runtime on this
model. The hardware-FMA version has about 1-2% overhead against a callback with
matched compiler flags. Without explicitly enabling FMA, measured median
overhead is about 4.5% against the current project build.

| Callback compiler target | Existing distance | Compensated distance | Change in median |
| --- | ---: | ---: | ---: |
| Current project target (no -mfma) | 49.076 s | 51.288 s | +4.51% |
| Hardware FMA (-mfma -ffp-contract=off) | 48.190 s | 48.878 s | +1.43% |

Each entry is the median of three executions of the actual production
`bld/tools/particle-sim`, on teapot.h5m with `-n 100000 -s 1 -e 1000`.
The build is Release, single precision, MOAB + Embree. Every run is pinned to
the same CPU, 14; the transport loop is serial. Wall-clock times include mesh
loading, BVH preparation, simulation and final output. A separate zero-particle
run measured setup at 2.261 s.

The FMA baseline is a preloaded copy of the unchanged callback, compiled with
exactly the same flags as the new FMA version. The three main variants (stock,
matched control, compensated) ran in three blocks with rotating order. The
current-target compensated tests were performed afterward as a separate set
of three sequential runs; they used the current project target plus
-ffp-contract=off, without -mfma. In the current project target, automatic
contraction cannot use hardware FMA instructions, so -ffp-contract=off does
not remove existing FMA instructions.

The median of paired, per-block new/control overheads was
1.04% (the table above uses ratios of median
wall times). Per-block overheads were 1.73%, 1.04%, 0.36%.

Full wall-time observations, including all outliers:

- Stock: 48.602, 49.076, 50.197 s
- Matched FMA control: 48.047, 48.190, 48.946 s
- Compensated with FMA: 48.878, 48.690, 49.122 s
- Compensated without -mfma: 51.288, 50.519, 55.593 s

These estimates are approximate. The measurements were taken on a shared
workstation with its existing powersave governor; background workload and CPU
frequency were not controlled. The small FMA-enabled overhead is comparable
to some of the observed run-to-run variation. The last generic run was slower
than its first two; it is retained above and in the median calculation.

The existing executable's median (49.076 s) and the new FMA-enabled version
(48.878 s) are effectively similar at this measurement resolution. Their
compiler targets differ, so the matched callback control is the more useful
measure of the t calculation's additional cost.

With -mfma, the helper uses inline float FMA instructions. Without it, its
disassembly contains 37 calls to fmaf through the PLT, explaining an additional
source of cost for that configuration. Neither prototype uses runtime double
or long-double variables in the distance helper.

All twelve 100,000-particle runs exited successfully with zero losses. The
control's cell tallies exactly match stock. Compensated native and generic
cell tallies also match exactly, verifying equivalent results for these builds
on this workload. The new result does change some individual histories
relative to the old distance calculation; these are actual application runs,
not identical frozen-query replays.

No project files or CMake settings were changed. SHA256 checks confirm that
the executable, project library, triangle header, mesh and experimental
libraries remained unchanged during their corresponding measurements.

See README.md for protocol, run_benchmark.py and run_generic.py for drivers,
combined_summary.json for machine-readable data, and the .log/.time files for
raw process output and timing.
