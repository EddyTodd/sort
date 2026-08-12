# Reproducibility and canonical benchmark runs

## Artifact layout

Each canonical run should live in a unique directory:

```text
results/<run-id>/
  manifest.json
  raw.csv
  summary.csv
  NOTES.md              # optional manual controls/anomalies
  claim-matrix.csv      # optional preregistered claims
  crossovers.csv        # optional crossover candidates
```

`raw.csv` is immutable once published. Reruns receive new run IDs rather than overwriting evidence.

Hardware-counter, allocation, record-width, cutoff-tuning, and scalar timing campaigns should normally use distinct run IDs because their measurement contracts differ.

## Capture

Example canonical scalar capture:

```sh
python3 tools/run_experiment.py --cpu 2 --settle-ms 1000 \
  ./build/sort_lab results/<run-id> -- \
  --trials 51 --warmups 2 --sizes 64,1024,16384,262144

python3 tools/analyze.py results/<run-id>/raw.csv \
  --bootstrap 5000 --output results/<run-id>/summary.csv
```

The manifest records:

- exact benchmark command;
- raw CSV SHA-256;
- benchmark binary SHA-256;
- binary-reported environment metadata when supported;
- Git commit and dirty-tree state;
- host OS/architecture/Python details;
- requested CPU affinity;
- pre/post load averages and available host-control state;
- relevant thread/allocator environment variables.

## Host controls

`--cpu N` pins the benchmark child on platforms where Python exposes process affinity. Pinning reduces migration but does not isolate the selected core from interrupts, SMT sibling activity, or kernel work.

The wrapper records available Linux governor/performance-policy/perf settings or macOS CPU/thermal metadata. It does **not** silently change system configuration.

For serious claims, `NOTES.md` should additionally record when relevant:

- exact CPU model and physical topology;
- cache hierarchy;
- SMT state and sibling-core policy;
- RAM configuration/speed;
- power mode, governor, turbo/boost policy;
- process/core isolation;
- laptop AC/battery state;
- thermal conditions for long campaigns;
- intentionally disabled background services;
- compiler executable/full version;
- standard-library implementation/version;
- complete compile/link flags.

## Cleanliness

Canonical timing runs use an optimized release build and a clean source tree. A dirty tree is acceptable for exploration but not archival evidence unless the exact patch is also preserved.

Sanitizer builds are correctness tools, not performance builds.

## Data integrity

Raw-data and binary hashes bind the evidence to both the observations and the executable that produced them. Summary files are derived and may be regenerated; raw files should never be edited by hand.

For external algorithm adapters, the manifest/notes must also pin upstream source version/commit and build configuration.

## Tuning reproducibility

A tuned cutoff or adaptive portfolio requires preservation of:

- full raw candidate data;
- deterministic train/evaluation split rule;
- candidate algorithm/parameter set;
- tuning/selection tool version;
- held-out evaluation result.

Do not publish the performance of a value selected on the same observations as independent confirmation.

## Repetition versus reproducibility

A deterministic seed makes the input reproducible, not the timing. Scheduling, temperature, frequency state, interrupts, and microarchitectural state remain stochastic.

Reproducibility means another investigator can recreate the specified experiment and obtain compatible distributions and conclusions—not identical nanosecond values.
