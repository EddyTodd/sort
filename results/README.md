# Benchmark results

This directory is reserved for **real, reproducible benchmark artifacts**. No synthetic or guessed performance numbers belong here.

Recommended canonical layouts:

```text
<run-id>/manifest.json
<run-id>/raw.csv
<run-id>/summary.csv
<run-id>/crossovers.csv       # optional derived artifact
<run-id>/NOTES.md             # optional
```

For a campaign containing both scalar and record experiments, use explicit names such as `scalar-raw.csv`, `records-raw.csv`, and corresponding summaries rather than mixing incompatible schemas.

Before benchmark tables are promoted into the project README, they must satisfy the publication gate in `docs/research-protocol.md`.

Large campaigns may eventually move raw datasets to release assets or an external archival repository. If that happens, this directory must retain content hashes, schema version, analysis code version, and durable artifact references.
