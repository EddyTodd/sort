# Research migration status

This document is the subject-side view of migration to [`EddyTodd/bench`](https://github.com/EddyTodd/bench).  The machine-readable acceptance manifest lives in `bench/migrations/sort.toml` as of bench v0.5.0 commit `acd9a77f9aa0fbb6edd569eb24c78b4694b442ed` and pins this repository at `d3a9446f1c4371799af8e55a8007733daf598ccf`.

## Current gate

All retained **default empirical executable definitions** have bench-native treatment/source parity.  That is not the same as deletion safety.

At this repository revision:

- definition parity: **11 / 11 empirical executable studies**;
- executed replacement evidence accepted through bench v0.5: **0 / 11 in this repository**;
- deletion-safe empirical executables: **0 / 11 until external evidence is registered and accepted**.

The last figure is intentionally conservative.  `sort` does not embed or infer the state of external benchmark result directories.  The authoritative cleanup decision is:

```sh
benchctl migration-status migrations/sort.toml \
  --evidence migrations/evidence.toml \
  --require-deletion-safe
```

run from the corresponding `bench` checkout/evidence registry.

## Empirical executable mapping

| Subject executable | Bench study | Bench campaign | Definition parity | Deletion now? |
|---|---|---|---|---|
| `research/apps/sort_lab.cpp` | `sort-lab` | `sort-lab-exact-session.toml` | complete | no |
| `research/apps/sort_records.cpp` | `sort-records` | `sort-records-exact-session.toml` | complete | no |
| `research/apps/sort_cutoffs.cpp` | `sort-cutoffs` | `sort-cutoffs-exact-session.toml` | complete | no |
| `research/apps/sort_perf.cpp` | `sort-perf` | `sort-legacy-perf.toml` | complete | no |
| `research/apps/sort_alloc.cpp` | `sort-allocation` | `sort-legacy-allocation.toml` | complete | no |
| `research/apps/sort_tiny.cpp` | `sort-tiny` | `sort-tiny-session.toml` | complete | no |
| `research/apps/sort_leaf_hybrids.cpp` | `sort-leaf-hybrids` | `sort-leaf-hybrids-session.toml` | complete | no |
| `research/apps/sort_merge_policies.cpp` | `sort-merge-policies` | `sort-merge-policies-session.toml` | complete | no |
| `research/apps/sort_merge_kernels.cpp` | `sort-merge-kernels` | `sort-merge-kernels-session.toml` | complete | no |
| `research/apps/sort_adaptive_records.cpp` | `sort-adaptive-records` | `sort-adaptive-records-session.toml` | complete | no |
| `research/apps/sort_external.cpp` | `sort-external` | `sort-external-session.toml` | complete | no |

The exact `sort_lab` treatment matters: the historical executable has a 23-algorithm table, times input-feature extraction separately, shuffles the full table within one process, and omits five quadratic algorithms only at the largest default size.  A current-v1 `sort-i64` campaign is therefore not a substitute for this historical treatment.

## What stays in `sort`

Migration of an empirical executable does **not** imply that every file it includes moves to bench.

Permanent subject-owned material includes:

- installed/public C++23 algorithms and metadata under `include/sortlab/`;
- deterministic v1 correctness, conformance, stability, package-consumer, and adaptive-option tests;
- algorithmic theory, references, scope/completeness documentation, and correctness invariants;
- sorting-specific workload/implementation assets still needed to reproduce a retained treatment until its deletion gate passes;
- `research/apps/adaptive_record_kernel_contract.cpp` while it remains an algorithm-level correctness contract rather than an empirical claim;
- pinned external-baseline manifest/bootstrap/license provenance required to reconstruct pdqsort/IPS4o source identity.

Generic campaign orchestration, statistics, reporting, provenance collection, evidence indexing, PMU/allocation framework logic, and migration acceptance belong in `bench`.

## Why retained research is still buildable

The empirical sources remain available solely for evidence reproduction/comparison while the migration gate is unresolved.  Build them explicitly:

```sh
cmake --preset retained-research
cmake --build --preset retained-research
ctest --preset retained-research
```

The ordinary `dev`, `release`, `sanitize`, and `package` presets do not enable retained empirical targets.

`SORTLAB_BUILD_RETAINED_RESEARCH=ON` is the canonical option.  `SORTLAB_BUILD_RESEARCH_TOOLS=ON` remains a deprecated compatibility alias so old reproduction instructions do not silently stop working.

## Deletion sequence

When bench reports a study deletion-safe:

1. retain the accepted replacement artifact and any historical import/comparison attestation;
2. remove only the study's declared empirical executable first;
3. run the permanent library/header/package correctness suite;
4. remove shared `research/include` or Python assets only after no remaining retained empirical or correctness contract uses them;
5. keep external-vendor provenance until the external treatment no longer depends on the subject-managed reconstruction path;
6. update this document and `docs/bench-migration.md` with the accepted evidence identity.

This prevents a broad `research/` deletion from erasing algorithm-specific correctness or reconstruction assets merely because similarly named timing logic exists in bench.
