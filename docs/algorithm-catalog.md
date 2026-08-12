# Algorithm catalog and coverage policy

The project does not equate "more algorithms" with better research. The catalog is organized by algorithmic mechanism so empirical comparisons cover materially different tradeoffs rather than dozens of cosmetic variants.

## Implemented scalar algorithms

The canonical scalar harness contains 23 implementations/variants.

| Mechanism | Implementations |
|---|---|
| insertion | insertion, binary insertion |
| selection/exchange | selection, bubble, comb |
| Shell | Ciura-gap Shell, Pratt-gap Shell |
| heap | project heapsort, `std` heap baseline |
| merge | top-down merge, bottom-up merge, natural/run-adaptive merge |
| quick | two-way Hoare, three-way, median-of-three, dual-pivot |
| hybrid | introsort, merge+insertion cutoff 24, median-three quicksort+insertion cutoff 24 |
| distribution | 8-bit LSD radix, 11-bit-digit LSD radix |
| library | `std::sort`, `std::stable_sort` |

The standalone cutoff harness treats the insertion threshold as a parameter instead of assuming 24 is universally optimal.

## Implemented record algorithms

The record-width laboratory intentionally uses a smaller representative set: insertion, heap, stable merge, two-way quicksort, three-way quicksort, introsort, stable radix, `std::sort`, and `std::stable_sort`. The goal is factorial control over payload width and stability, not duplicating every scalar variant for every record type.

## Important external/state-of-the-art families

The following belong in the research universe even when they are not vendored into the core executable:

- Timsort and Powersort/Peeksort: stable run-adaptive merging and merge-policy research;
- Pattern-defeating quicksort (pdqsort): robust quicksort engineering for patterns and duplicates;
- BlockQuicksort: branch-misprediction-aware partitioning;
- QuickXsort / QuickMergesort: theoretically analyzed combinations of partitioning with another sorting method;
- IPS4o: cache/branch-aware in-place sample sort, including parallel variants;
- VQSort: vectorized, architecture-portable quicksort;
- in-place stable block merges such as WikiSort/Grail-style methods;
- counting, bucket, American-flag/MSD radix, and other bounded-domain distribution sorts;
- sorting networks for very small fixed sizes and vector registers;
- parallel merge/sample/radix sorts;
- external-memory and NUMA-aware sorting.

These are not silently labelled "missing." They are separate comparison tracks because several require external code, architecture-specific intrinsics, parallel runtimes, different input contracts, or materially different memory models. A future adapter must pin the upstream version and preserve license/provenance rather than copying an arbitrary implementation into the benchmark.

## Variant policy

A variant gets its own algorithm name when it changes a scientifically material parameter, including:

- pivot selection or number of pivots;
- duplicate partitioning strategy;
- Shell gap sequence;
- radix digit width;
- insertion/base-case cutoff;
- merge order/run policy;
- stability or in-place guarantee;
- branchless/vectorized partitioning;
- parallelism or memory strategy.

Continuous/tunable parameters should normally live in a tuning experiment rather than creating hundreds of permanent algorithm names.

## Completion criterion

The core catalog is considered representative when every major mechanism above has at least one controlled implementation or an explicitly versioned external-baseline track. The research objective is then to characterize domains of superiority and crossover regions—not to declare one universal winner.
