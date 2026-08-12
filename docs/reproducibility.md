# Reproducibility and canonical benchmark runs

## Artifact layout

Each canonical run should live in a unique directory:

```text
results/<run-id>/
  manifest.json
  raw.csv
  summary.csv
  NOTES.md          # optional manual controls/anomalies
```

`raw.csv` is immutable once published. If the experiment must be rerun, create a new run ID rather than overwriting evidence.

## Capture

Use:

```sh
python3 tools/run_experiment.py ./build/sort_lab results/<run-id> -- <benchmark args>
python3 tools/analyze.py results/<run-id>/raw.csv --bootstrap 5000 --output results/<run-id>/summary.csv
```

The manifest captures portable host details, exact command, binary-reported compiler metadata, Git commit, working-tree state, and the SHA-256 of `raw.csv`.

## Manual host record for serious claims

Portable APIs cannot capture every relevant condition. `NOTES.md` should record, when available:

- exact CPU model and physical core topology;
- cache hierarchy;
- RAM configuration/speed;
- power mode, CPU governor, turbo/boost policy;
- process affinity or isolation;
- laptop AC/battery state;
- room/CPU temperature considerations for long campaigns;
- background services intentionally disabled;
- compiler executable and full version;
- standard library implementation/version;
- complete compile/link flags.

## Cleanliness

Canonical runs should use an optimized release build and a clean source tree. A dirty tree is allowed for exploration but should not be treated as archival evidence unless the patch itself is captured.

## Data integrity

The raw CSV hash in `manifest.json` is the experiment's content identity. Analyses should verify the hash before publication or aggregation. Summary files are derived and may be regenerated; raw files should not be edited by hand.

## Repetition versus reproducibility

A deterministic seed makes the *input* reproducible but not the *timing*. OS scheduling, temperature, frequency scaling, interrupts, and microarchitectural state remain stochastic. Reproducibility therefore means another investigator can recreate the specified experiment and obtain compatible distributions/conclusions—not identical nanosecond values.
