# v1 completeness

`sortlab` 1.0 is complete when it provides a representative, reusable catalog of sequential in-memory CPU sorting mechanisms, generic C++23 APIs, deterministic correctness/stability coverage, optional instrumentation, and an installable header-only package independent of benchmarking infrastructure.

## Included mechanisms

- elementary insertion/selection/exchange families;
- Shell sorting with representative gap sequences;
- heap sort;
- top-down, bottom-up, natural, hybrid, and stable in-place merge families;
- Hoare, three-way, median-of-three, dual-pivot, hybrid quicksort, and introsort;
- production adaptive stable TimSort and PowerSort mechanisms;
- stable LSD radix, in-place MSD radix, and bounded counting sort for integral domains;
- generic bitonic network sorting for tiny/data-oblivious treatment;
- comparator/projection support for generic comparison algorithms;
- optional operation-count instrumentation sharing the permanent implementations.

## Correctness boundary

The deterministic test suite covers edge sizes, duplicates, order pathologies, custom comparators/projections, declared stability, move-only support where valid, adaptive-run invariants, galloping, signed distribution-sort boundaries, bounded counting domains, metadata, and instrumentation behavior.

Quicksort-family pivot snapshotting intentionally requires copy-constructible values. Distribution algorithms intentionally carry integral/domain constraints.

## Deliberately deferred

These are separate domains, not v1 defects:

- parallel and distributed sorting;
- GPU sorting;
- external-memory/out-of-core sorting;
- NUMA-specific scheduling;
- architecture-specific SIMD sorting;
- specialized variable-length string/blob sorters;
- exhaustive optimal sorting-network catalogs.

## Empirical research boundary

Performance experiments are not part of core completeness. `EddyTodd/bench` owns timing, workloads used for performance questions, cutoff/crossover studies, record-width experiments, allocation/PMU collection, external performance baselines, statistics, provenance, evidence, and reports.

`sortlab` owns only the algorithms and algorithm-specific correctness/theory required to make those experiments meaningful.

**Known v1 core blockers: none.**
