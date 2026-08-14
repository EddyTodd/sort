# Migration boundary: `sort` → `bench`

The v1 library is designed so generic empirical infrastructure can leave this repository without rewriting permanent sorting algorithms.  Bench v0.5.0 commit `acd9a77f9aa0fbb6edd569eb24c78b4694b442ed` now provides source/treatment **definition parity** for every retained default empirical executable at this subject revision.

Definition parity is not deletion safety.  The retained sources remain until bench's executed-evidence acceptance gate passes.

## Stays permanently in `sort`

Permanent sorting-domain assets:

- `include/sortlab/sort.hpp` and all installed v1 headers;
- generic comparison, adaptive, distribution, stable-low-memory, and tiny algorithms;
- optional algorithm-level observer hooks;
- reusable algorithm metadata;
- `tests/library_tests.cpp`, adaptive-option/package-consumer/header tests, and future deterministic correctness/property tests;
- sorting theory and algorithm-specific correctness documentation;
- sorting-specific references;
- v1 scope/completeness documentation;
- minimal examples needed to demonstrate the public API.

`include/sortlab/` is exclusively the permanent public library surface. Retained benchmark-only headers live under `research/include/sortlab/` and are added to include paths only when retained research targets are explicitly enabled.

Algorithm-level correctness contracts are not empirical migration units merely because they currently live under `research/apps/`. In particular, `adaptive_record_kernel_contract.cpp` remains subject-owned unless/until its underlying mechanism is deliberately retired or converted into a permanent test.

## Generic infrastructure owned by `bench`

The following responsibilities no longer belong in the long-term subject architecture:

- generic campaign execution/orchestration;
- generic bootstrap, paired/blocked reducers, inference, crossover estimation, and reporting;
- benchmark result/provenance/integrity schemas;
- CPU-affinity, process-resource, PMU, and generic allocation collectors;
- cross-machine replication/aggregation mechanics;
- generic claim/evidence indexing;
- historical-evidence import and migration-acceptance gates.

Bench-side sort experiment definitions may depend on the pinned/installed `sortlab::sortlab` package and, only when exact historical implementation identity requires it, retained `research/include/sortlab/` headers.

## Empirical executable migration units

The machine-readable inventory is `bench/migrations/sort.toml`.  These 11 subject executables have complete bench-side definition parity:

- `research/apps/sort_lab.cpp`;
- `research/apps/sort_records.cpp`;
- `research/apps/sort_cutoffs.cpp`;
- `research/apps/sort_perf.cpp`;
- `research/apps/sort_alloc.cpp`;
- `research/apps/sort_tiny.cpp`;
- `research/apps/sort_leaf_hybrids.cpp`;
- `research/apps/sort_merge_policies.cpp`;
- `research/apps/sort_merge_kernels.cpp`;
- `research/apps/sort_adaptive_records.cpp`;
- `research/apps/sort_external.cpp` when the pinned external-baseline track is available.

See [`research-migration-status.md`](research-migration-status.md) for the exact campaign mapping and current acceptance state.

The current-v1 `sort-i64`, cutoff, and generic record campaigns are useful new research treatments but are **not** substitutes for historical implementation parity.  Bench therefore retains separate exact treatments for `sort_lab`, historical cutoffs, records, tiny kernels, adaptive/merge studies, and external baselines.

## Shared research assets

The following are not deleted as a single bulk migration unit:

- `research/include/sortlab/` implementation/workload/counter headers;
- `campaigns/`, `claims/`, and `results/` historical contract/state;
- sorting-specific analysis scripts and model assets;
- external baseline manifest, bootstrap script, license metadata, and vendor reconstruction path.

Shared assets are removed only when no retained empirical executable or subject correctness/reconstruction contract needs them.  Generic functionality should move to bench; sorting-specific theory/correctness/reconstruction state stays here.

The external pdqsort/IPS4o track is intentionally fail-closed. Bench owns the empirical treatment, but currently requires subject-managed vendor checkouts at the pinned commits; therefore `external/baselines.json` and `tools/bootstrap_external.py` remain subject-owned reconstruction provenance for now.

## Build boundary

The ordinary v1 build does not configure retained empirical targets.

Canonical opt-in:

```sh
cmake -S . -B build-research \
  -DSORTLAB_BUILD_RETAINED_RESEARCH=ON \
  -DCMAKE_BUILD_TYPE=Release
```

or use:

```sh
cmake --preset retained-research
cmake --build --preset retained-research
ctest --preset retained-research
```

`SORTLAB_BUILD_RESEARCH_TOOLS=ON` remains a deprecated compatibility alias.  The installed CMake package never exports retained benchmark headers or executables.

## Deletion gate

An empirical executable may be removed only after its bench study is deletion-safe. Bench v0.5 requires, at minimum:

1. complete treatment definition and an existing declared campaign;
2. checksum-clean, fully executed non-empty replacement evidence;
3. clean provenance tied to this exact subject commit;
4. `bench-analysis-v3` analysis and a checksummed report;
5. if historical evidence is registered, a verified legacy import and an explicit accepted comparison bound to both artifact identities.

At this revision there are no committed raw historical result datasets under `results/`, only the artifact contract.  The bench manifest therefore uses `historical_policy = "if-present"`: history is not invented, but once registered it automatically activates the comparison gate.

No current source deletion is implied by the completed definition migration.  Run the authoritative bench command before any cleanup PR:

```sh
benchctl migration-status migrations/sort.toml \
  --evidence migrations/evidence.toml \
  --require-deletion-safe
```
