# Benchmark results

This directory is reserved for **real, reproducible research artifacts**. Synthetic, guessed, copied, or development-only performance numbers do not belong here.

Single-run artifacts use:

```text
<run-id>/manifest.json
<run-id>/raw.csv
<run-id>/summary.csv
<run-id>/NOTES.md                 # optional
<run-id>/claims.csv               # optional
<run-id>/crossovers.csv           # optional
<run-id>/cutoff-summary.csv       # optional, must preserve train/evaluation split
<run-id>/portfolio-summary.csv    # optional, held-out selector evaluation
```

Versioned multi-run campaigns use:

```text
campaigns/<campaign-id>/campaign-manifest.json
campaigns/<campaign-id>/<experiment-id>/rep-001/manifest.json
campaigns/<campaign-id>/<experiment-id>/rep-001/raw.csv
campaigns/<campaign-id>/<experiment-id>/rep-001/<derived analysis files>
...
```

The committed experiment contract lives in `campaigns/*.json`. `campaign-manifest.json` records its content hash and the status of every planned repetition. `tools/validate_artifacts.py` must pass before a run is attached to `claims/registry.json` as evidence.

Hardware-counter and allocation campaigns retain distinct experiment IDs because their measurement contracts differ from canonical wall-time runs. A platform skip is absence of that measurement, never a zero-valued observation.

Before any benchmark statement is promoted into the project README, a paper, or a portfolio, it must satisfy the publication gate in `docs/research-protocol.md` and the claim-specific requirements in `claims/registry.json`.

Large campaigns may move raw datasets to release assets or an archival repository. If so, this directory must retain schema version, content hashes, exact source/binary identity, analysis-code version, campaign-spec hash, and durable artifact references.
