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

These are currently useful but are not permanent library API:

- `src/sort_lab.cpp`;
- `src/sort_records.cpp`;
- `src/sort_cutoffs.cpp`;
- `src/sort_perf.cpp`;
- `src/sort_alloc.cpp`;
- `src/sort_tiny.cpp`;
- `src/sort_leaf_hybrids.cpp`;
- `src/sort_merge_policies.cpp`;
- `src/sort_merge_kernels.cpp`;
- `src/sort_adaptive_records.cpp`;
- workload generators whose purpose is empirical treatment construction;
- `campaigns/`;
- `claims/` evidence state;
- `results/` artifact conventions;
- external comparison bootstrap/provenance files for pdqsort/IPS4o;
- the majority of `tools/*.py` analysis programs.

After migration, bench-side sort experiments should call the generic v1 API and use `sortlab::instrumented` observers when algorithm-level operation counts are desired.

## Temporary build switch

The v1 default is `SORTLAB_BUILD_RESEARCH_TOOLS=OFF`, so a normal configure builds the permanent library/tests only. Set it to `ON` explicitly when the retained pre-migration research executables are needed.

The installed CMake package never exports the benchmark headers/executables.
