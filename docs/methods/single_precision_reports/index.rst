Single-precision investigation reports
======================================

These reports preserve the explanations and results from successive stages
of the investigation. Read :doc:`../single_precision` for the final
implementation and its current scope. Statements such as "no project files
were modified" or "one particle remains lost" describe the stage recorded
in that report and may be superseded by later integrations.

The original report text is retained, including temporary paths, commands,
and references to diagnostic drivers or raw logs. Those drivers, binaries,
and raw logs are not included in this archive. Production regression tests
are part of the repository under ``tests/``. The teapot model is an external
input, not an included test fixture.

The links below download the preserved Markdown or text reports. Within the
repository, the same files can be read directly in their named subdirectories.

Initial diagnoses and continuation experiments
----------------------------------------------

* :download:`Particle 8449: edge clamping <particle-8449/README.md>`

* :download:`Broader loss investigation <many-losses/README.md>`

* :download:`Million-history boundary-placement investigation <million-losses/README.md>`

* :download:`Float-only returned-hit-position experiments <float-hitpoint/README.md>`

* :download:`Normals, mesh validity, and boundary continuation <recheck/README.md>`

* :download:`Fixed-flight continuation experiments <single-continuation/README.md>`

* :download:`Fixed-flight implementation and validation <fixed-flight-implementation/README.md>`

Compensated distance and measured cost
--------------------------------------

* :download:`Float-only evaluation of the triangle parameter <t-accuracy/README.md>`

* :download:`Frozen-ray accuracy summary <t-accuracy/summary.txt>`

* :download:`Full-simulator benchmark protocol <t-accuracy/full-sim-benchmark/README.md>`

* :download:`Full-simulator benchmark results <t-accuracy/full-sim-benchmark/RESULTS.md>`

* :download:`Compensated-distance integration and validation <t-integration/README.md>`

Parallel transport and thread control
-------------------------------------

* :download:`OpenMP transport, atomic tallies, and timings <openmp/README.md>`

* :download:`Thread-count flag and CLI checks <threads-flag/README.md>`

Residual losses and final integrations
--------------------------------------

* :download:`The three losses from ten million histories <10m-losses/README.md>`

* :download:`Position-uncertainty prototype and lower-bound policy <position-uncertainty/README.md>`

* :download:`Frozen position-bound interpretation <position-uncertainty/frozen-bounds.txt>`

* :download:`Position-uncertainty production integration <position-uncertainty/integration.md>`

* :download:`Compensated Plucker edge integration <edge-integration/README.md>`

* :download:`Named baseline failures <edge-integration/baseline-failure-comparison.txt>`
