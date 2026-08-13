# v1 completeness: sequential in-memory sorting library

## Decision

v1 is complete when `sortlab` can be used as a standalone C++23 sorting library without the benchmark harness, when every permanent comparison implementation is generic, when instrumentation is optional, and when the major sequential in-memory mechanisms have representative implementations.

This document is deliberately about **algorithm/library completeness**, not empirical evidence completeness. Benchmark campaigns, statistical inference, provenance bundles, hardware counters, and cross-machine replication are moving to `EddyTodd/bench` and do not block this version.

## Public architecture

The installed package consists of:

- `sortlab/sort.hpp` — umbrella include;
- `sortlab/comparison.hpp` — generic comparison and hybrid sorts;
- `sortlab/adaptive.hpp` — production TimSort/Powersort adaptive stable sorts;
- `sortlab/stable_inplace.hpp` — stable low-extra-memory merge sort;
- `sortlab/distribution.hpp` — LSD radix, MSD American-flag radix, counting sort;
- `sortlab/tiny.hpp` — bitonic network family;
- `sortlab/instrumentation.hpp` — optional observers/counters;
- `sortlab/metadata.hpp` — reusable algorithm metadata;
- `sortlab/version.hpp`;
- `sortlab/detail.hpp` — implementation support required by the headers above.

No benchmark `Stats`, workload generator, CSV schema, campaign tool, or benchmark executable is required by the installed library.

## Representative mechanism coverage

| Mechanism | v1 representative(s) | Status |
|---|---|---|
| direct insertion | insertion, binary insertion | complete |
| selection/exchange | selection, bubble, comb | complete |
| Shell/gapped insertion | Ciura, Pratt | complete |
| heap | binary heapsort | complete |
| ordinary merge | top-down, bottom-up | complete |
| run-adaptive merge | natural merge | complete |
| low-extra-memory stable merge | binary-partition + rotation merge sort | complete |
| two-way quicksort | Hoare | complete |
| duplicate-aware quicksort | three-way | complete |
| sampled-pivot quicksort | median-of-three | complete |
| multi-pivot quicksort | dual-pivot | complete |
| worst-case-safe hybrid | introsort | complete |
| small-leaf hybrids | merge+insertion, quick+insertion | complete |
| production adaptive stable merge | TimSort-style stack + dynamic gallop | complete |
| near-optimal adaptive merge scheduling | Powersort | complete |
| stable integer distribution | LSD radix | complete |
| in-place MSD integer distribution | American-flag-style radix | complete |
| bounded-domain distribution | counting sort | complete |
| data-oblivious tiny sorting | bitonic network family | complete |

## TimSort/Powersort production status

The permanent adaptive algorithms no longer depend on the scalar benchmark treatment code. They support arbitrary sortable random-access values through comparator/projection APIs.

Correctness properties:

- descending runs are detected using strict comparison and reversed without reversing equal-key groups;
- short runs are extended using stable binary insertion;
- TimSort uses repaired run-stack collapse conditions and a final force-collapse;
- Powersort uses integer node-power scheduling;
- the merge kernel buffers only the smaller run;
- forward merging chooses the left run on ties;
- backward merging places the right run into later equal-key positions;
- galloping uses exponential bracketing followed by binary boundary search in both directions;
- `min_gallop` is stateful and dynamically rewarded/penalized during the sort;
- pathological/unbalanced run geometry, duplicate-heavy records, and move-only values are covered by deterministic tests.

This is a TimSort-style implementation, not a claim of source compatibility or byte-for-byte equivalence with CPython/OpenJDK.

## Stable low-extra-memory decision

v1 uses a stable divide-and-conquer merge based on binary partitioning plus in-place rotations. It uses constant element buffer space plus logarithmic recursion stack and has `O(n log^2 n)` worst-case time.

This is a defensible representative of the stable low-extra-memory family. A Grail/Wiki block-buffer implementation is therefore **not required to claim mechanism coverage**. Adding one later may be useful for performance comparison, but it would be another implementation within an already represented mechanism rather than a core architectural blocker.

## Tiny-network decision

The library exposes a generic bitonic network family, and the retained research layer contains the earlier tiny-kernel experiments. v1 does not add a provenance-pinned catalog of optimal comparator networks.

Reason: optimal/specialized schedules are important implementation engineering, but they do not introduce a missing sorting mechanism. Exact optimality also depends on fixed `N`, while ISA-specialized schedules introduce architecture scope that v1 explicitly excludes. They belong in future `bench` comparison work or a later specialized extension.

## Other mechanisms reviewed

Not added as v1 blockers:

- **cycle sort:** genuinely write-minimizing, but quadratic and specialized; selection already provides a low-write quadratic baseline while v1 prioritizes broadly useful mechanisms;
- **smoothsort:** useful adaptive heap variant, but heap-based comparison sorting is already represented and its practical value is implementation-specific;
- **tree/tournament sorts:** usually require auxiliary node/tree structures and do not add a necessary in-memory array-sorting mechanism for this scope;
- **bucket/flash/sample sorts:** domain/distribution families are represented by counting + LSD + in-place MSD radix; further variants are empirical comparison candidates;
- **Grail/Wiki block merges:** useful future stable low-memory implementations, but the low-extra-memory stable mechanism is represented by rotation merge;
- **optimal/AlphaDev/SIMD tiny networks:** optimization/specialization tracks, not core completeness blockers.

Novelty/educational sorts such as bogo, gnome, pancake, and strand sort are explicitly outside the completeness criterion.

## Generic API guarantees

Comparison algorithms use random-access iterators/ranges with customizable comparison and projection where practical.

- Stable algorithms preserve equivalent-element order under the supplied comparator/projection.
- Quicksort-family algorithms snapshot pivots and currently require copy-constructible value types.
- Insertion, heap, merge, stable in-place merge, TimSort, and Powersort support move-only values satisfying normal sortable/permutable requirements.
- Distribution algorithms are intentionally restricted to integral value types.
- Counting sort requires a bounded observed key range and rejects domains larger than `max_domain`.

## Instrumentation separation

The normal API constructs no statistics object. `sortlab::instrumented` wrappers inject an observer into the same internal algorithm implementation. `counting_observer` is provided as a small default adapter; future `bench` code can supply richer observers without changing algorithms.

The operation observer is an algorithmic event interface, not a hardware-traffic model. Cache, branch, energy, and bandwidth instrumentation belongs in `bench`.

## Package completeness

- header-only C++23 target `sortlab::sortlab`;
- `find_package(sortlab 1 CONFIG REQUIRED)` support;
- install/export/version files;
- no required third-party dependency;
- benchmark/research targets are opt-in through `SORTLAB_BUILD_RESEARCH_TOOLS` (default `OFF`);
- core correctness tests independent of benchmark infrastructure;
- sanitizer option usable with core-only builds.

## Explicit v1 non-goals

Parallel, GPU, NUMA, distributed, external-memory, and out-of-core sorting are separate domains. Architecture-specific SIMD, variable-size string/blob algorithms, indirect object graphs, and exhaustive tiny-network optimization are future extensions rather than v1 blockers.

## Remaining core blockers

**None known.**

Remaining work in the repository is either benchmark migration, empirical research, additional implementations within already-covered mechanism families, or future-domain expansion.
