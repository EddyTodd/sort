# Benchmark results

This directory is reserved for **real, reproducible benchmark artifacts**. No synthetic or guessed performance numbers belong here.

Canonical run layout:

```text
<run-id>/manifest.json
<run-id>/raw.csv
<run-id>/summary.csv
<run-id>/NOTES.md   # optional
```

Before benchmark tables are promoted into the project README, they must satisfy the publication gate in `docs/research-protocol.md`.

Large campaigns may eventually move raw datasets to release assets or an external archival repository. If that happens, this directory must retain content hashes, schema version, analysis code version, and durable artifact references.
