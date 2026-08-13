# Migration boundary: `sort` → `bench`

The v1 library is designed so the empirical framework can leave this repository without rewriting sorting algorithms.

## Stays in `sort`

Permanent sorting-domain assets:

- `include/sortlab/sort.hpp` and the installed v1 headers;
- generic comparison, adaptive, distribution, stable-low-memory, and tiny algorithms;
- optional algorithm-level observer hooks;
- reusable algorithm metadata;
- `tests/library_tests.cpp` and future deterministic correctness/property tests;
- sorting theory and algorithm-specific correctness documentation;
- sorting-specific references;
- v1 scope/completeness documentation;
- minimal examples needed to demonstrate the public API.

`include/sortlab/` is exclusively the permanent public library surface. Pre-v1 benchmark-only headers live under `research/include/sortlab/` and are added to include paths only when research targets are explicitly enabled.

## Moves to `EddyTodd/bench`

Generic empirical infrastructure:

- `tools/run_experiment.py`;
- `tools/campaign.py`;
- `tools/validate_artifacts.py`;
- generic bootstrap/paired statistical reducers and reporting framework;
- campaign manifests/specification machinery;
- result artifact/provenance schema;
- CPU-affinity/host-state capture;
- generic hardware-counter and allocation measurement plumbing;
- cross-machine repetition/aggregation infrastructure;
- claim/evidence ledger machinery where it is not sorting-specific.

Sorting experiments can remain as bench-side experiment definitions that depend on the installed `sortlab::sortlab` package.

## Sorting-specific benchmark adapters that move with the framework

These retained executables are not permanent library API and now live under `research/apps/`:

- `sort_lab.cpp`;
- `sort_records.cpp`;
- `sort_cutoffs.cpp`;
- `sort_perf.cpp`;
- `sort_alloc.cpp`;
- `sort_tiny.cpp`;
- `sort_leaf_hybrids.cpp`;
- `sort_merge_policies.cpp`;
- `sort_merge_kernels.cpp`;
- `sort_adaptive_records.cpp`;
- `adaptive_record_kernel_contract.cpp`;
- `sort_external.cpp` when the optional external-baseline track is enabled;
- `research/include/sortlab/` legacy benchmark-only algorithm/workload/counter headers;
- workload generators whose purpose is empirical treatment construction;
- `campaigns/`;
- `claims/` evidence state;
- `results/` artifact conventions;
- external comparison bootstrap/provenance files for pdqsort/IPS4o;
- the majority of `tools/*.py` analysis programs.

After migration, bench-side sort experiments should call the generic v1 API and use `sortlab::instrumented` observers when algorithm-level operation counts are desired. The legacy research-only algorithm copies can then be removed as individual experiments are ported to the v1 library surface.

## Temporary build switch

The v1 default is `SORTLAB_BUILD_RESEARCH_TOOLS=OFF`, so a normal configure builds the permanent library/tests only. Set it to `ON` explicitly when the retained pre-migration research executables are needed.

The installed CMake package never exports the benchmark headers/executables.
