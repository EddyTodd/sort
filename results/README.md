# Benchmark results

This directory is reserved for **real, reproducible research artifacts**. Synthetic, guessed, or copied performance numbers do not belong here.

Recommended canonical layout:

```text
<run-id>/manifest.json
<run-id>/raw.csv
<run-id>/summary.csv
<run-id>/NOTES.md                 # optional
<run-id>/claim-matrix.csv         # optional
<run-id>/crossovers.csv           # optional
<run-id>/tuning.csv               # optional, must identify train/evaluation split
<run-id>/portfolio.csv            # optional, held-out selector evaluation
```

Hardware-counter and allocation campaigns should use distinct run IDs because their measurement contracts differ from canonical wall-time runs.

Before any benchmark statement is promoted into the project README, a paper, or a portfolio, it must satisfy the publication gate in `docs/research-protocol.md`.

Large campaigns may move raw datasets to release assets or an archival repository. If so, this directory must retain schema version, content hashes, exact source/binary identity, analysis-code version, and durable artifact references.
