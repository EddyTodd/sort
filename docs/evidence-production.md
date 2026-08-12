# Evidence production and claim lifecycle

The implementation laboratory is only the first half of this project. This document defines the path from a hypothesis to a publishable result.

## 1. Freeze the experiment contract

A controlled campaign starts from a committed JSON specification in `campaigns/`. The spec fixes binaries, arguments, repetitions, platform restrictions, and post-processing commands before final collection.

Changing the matrix after inspecting final data creates a **new campaign version**. Do not silently edit `canonical-v1.json` and reuse its old results.

## 2. Capture immutable raw evidence

`tools/campaign.py` delegates every measured run to `tools/run_experiment.py`, preserving the existing raw/binary SHA-256, Git state, command line, host metadata, and control-state capture.

The campaign runner adds:

- deterministic run IDs;
- repeated-run structure;
- resumability based on raw hash validity;
- explicit platform skips;
- fixed analyzer invocation;
- a campaign manifest containing the campaign-spec hash.

Raw CSV is never regenerated in place merely because an analyzer changed. Derived summaries may be regenerated.

## 3. Validate before interpretation

Run:

```sh
python3 tools/validate_artifacts.py results/campaigns/canonical-v1
```

Validation checks manifest readability, raw SHA-256, binary hash presence, source-tree cleanliness, non-empty CSV data, and `verified` rows when that column exists.

A failed integrity check blocks claim promotion. It is not an outlier to delete.

## 4. Separate evidence state from prose

`claims/registry.json` is the source of truth for H1-H11. Each claim has:

- a precise hypothesis;
- an evidence status;
- explicit evidence requirements;
- attached artifact paths only after evidence exists.

Allowed statuses are:

- `untested`;
- `exploratory`;
- `tier2-supported`;
- `tier3-replicated`;
- `rejected`;
- `inconclusive`.

Generate a readable ledger with:

```sh
python3 tools/evidence_ledger.py --output results/EVIDENCE.md
```

Changing a status requires reviewing the publication gate in `docs/research-protocol.md`; the registry is not a substitute for scientific judgment.

## 5. Report absence of evidence explicitly

`tools/report.py` summarizes completed/resumed/failed campaign runs and claim statuses. It deliberately does **not** infer winners from whichever CSV files happen to exist.

```sh
python3 tools/report.py --output results/STATUS.md
```

This prevents the repository from drifting into a README where exploratory timings quietly become conclusions.

## 6. Tier-3 replication

Run the same versioned campaign on materially different environments. Keep each environment in its own artifact root, preserving binary hashes and manifests. A cross-machine statement requires compatible conclusions across those roots; combining raw timings across unlike machines into one distribution is generally invalid.

## 7. External implementation tracks

SIMD, parallel, GPU, NUMA, external-memory, and third-party state-of-the-art sorters remain separate tracks. When added, they should receive their own campaign specs and provenance metadata rather than being smuggled into `canonical-v1` after data collection.
