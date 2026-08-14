# sortlab

`sortlab` is a C++23 library of sequential in-memory sorting algorithms.  Its pre-v1 empirical programs are retained temporarily for evidence reproduction while their exact treatments migrate to [`EddyTodd/bench`](https://github.com/EddyTodd/bench).

Version 1 separates those concerns deliberately:

- **permanent library:** generic sorting algorithms, correctness contracts, metadata, instrumentation hooks, CMake package;
- **retained research compatibility layer:** historical empirical executables, workload/mechanism assets, and reconstruction tooling kept outside the installed API until bench's evidence-acceptance gate permits cleanup.

The installed package contains only the permanent library headers. Retained research headers and executables are not part of the installed API or the default package surface.

## Use the library

```cpp
#include <sortlab/sort.hpp>

#include <vector>

std::vector<int> values{5, 1, 4, 1, 3};
sortlab::intro_sort(values);
```

Comparison algorithms accept random-access iterators/ranges and, where applicable, a comparator and projection:

```cpp
struct row {
  int key;
  std::string payload;
};

std::vector<row> rows = /* ... */;
sortlab::timsort(rows, std::ranges::less{}, &row::key);
```

The ordinary API has no benchmark state. Optional instrumentation uses the same underlying implementations:

```cpp
sortlab::operation_counts counts;
sortlab::counting_observer observer(counts);
sortlab::instrumented::merge_sort(values.begin(), values.end(), observer);
```

See [`docs/library-api.md`](docs/library-api.md) for iterator/range requirements, projections, move-only support, instrumentation semantics, and distribution-sort domain constraints.

## v1 algorithm catalog

### Insertion / exchange / selection

- `insertion_sort` — stable, adaptive, in-place;
- `binary_insertion_sort` — stable; fewer comparison opportunities at the cost of the same quadratic movement bound;
- `selection_sort` — in-place selection baseline;
- `bubble_sort` — stable adaptive exchange baseline;
- `comb_sort` — gap-based exchange treatment.

### Shell sorting

- `shell_ciura_sort`;
- `shell_pratt_sort`.

### Heap sorting

- `heap_sort` — in-place `O(n log n)` worst case.

### Merge families

- `merge_sort` — top-down stable merge sort;
- `merge_bottom_up_sort` — iterative stable merge sort;
- `natural_merge_sort` — detects monotone input runs;
- `merge_insertion_sort` — configurable insertion leaves;
- `stable_inplace_merge_sort` — stable low-extra-memory divide/rotate merge family with `O(log n)` stack data and `O(n log^2 n)` worst-case time.

### Quicksort / hybrid partition families

- `quick_hoare_sort`;
- `quick_3way_sort` — duplicate-aware three-way partitioning;
- `quick_median3_sort`;
- `dual_pivot_sort`;
- `quick_insertion_sort` — configurable insertion leaves;
- `intro_sort` — median-of-three partitioning, insertion leaves, heap fallback for `O(n log n)` worst-case time.

The quicksort-family implementations snapshot pivots and therefore require copy-constructible value types. Other generic algorithms support move-only types when their standard iterator/permutation requirements are satisfied.

### Production adaptive stable merges

- `timsort` — strict descending-run reversal, stable binary run extension, repaired TimSort stack-collapse invariants, smaller-run buffering, forward/backward stable merging, exponential/binary galloping, and **dynamic `min_gallop` adaptation**;
- `powersort` — the same stable adaptive merge kernel under Powersort node-power scheduling.

These are permanent algorithms, not only benchmark treatments. The retained research executables still expose fixed policy/threshold experiments for historical studies, but they are separate from the v1 API.

### Distribution sorting

Integral types only:

- `radix_lsd_sort` — stable LSD radix; configurable digit width;
- `radix_msd_sort` — in-place American-flag-style MSD radix permutation;
- `counting_sort` — stable bounded-domain counting sort; rejects domains larger than the caller-supplied `max_domain` instead of allocating unbounded memory.

Signed integers are ordered correctly across their full representable range by mapping the sign bit into monotonic unsigned key space.

### Tiny / data-oblivious sorting

- `bitonic_sort` — generic bitonic network family, including non-power-of-two sizes via the greatest-power-of-two merge construction.

The older padded-bitonic benchmark treatment remains in the retained research layer. v1 deliberately does **not** claim an optimal-comparator small-N network catalog; specialized optimal/ISA-specific networks are implementation-optimization research, not a missing sorting mechanism. See [`V1_COMPLETENESS.md`](V1_COMPLETENESS.md).

## Stability and records

Stable v1 algorithms are tested with custom record types carrying duplicate keys, original ordinals, and nontrivial payloads. Projections replace the old need for separate record-specific algorithm implementations:

```cpp
sortlab::powersort(records, std::ranges::less{}, &record::key);
```

Equal-key order is preserved by insertion, binary insertion, bubble, merge variants, natural merge, stable in-place merge, TimSort, Powersort, and stable LSD/counting distribution where the domain model applies.

## Build, test, install

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
```

The default build contains only the permanent library/test/package surface; retained empirical targets are disabled.

Sanitizers:

```sh
cmake -S . -B build-san \
  -DSORTLAB_ENABLE_SANITIZERS=ON \
  -DCMAKE_BUILD_TYPE=Debug
cmake --build build-san -j
ctest --test-dir build-san --output-on-failure
```

To reproduce the retained pre-migration research programs intentionally:

```sh
cmake --preset retained-research
cmake --build --preset retained-research
ctest --preset retained-research
```

Equivalent direct configuration uses `-DSORTLAB_BUILD_RETAINED_RESEARCH=ON`. `SORTLAB_BUILD_RESEARCH_TOOLS=ON` remains a deprecated compatibility alias for old reproduction instructions.

Install and consume with CMake:

```sh
cmake --install build --prefix /your/prefix
```

```cmake
find_package(sortlab 1 CONFIG REQUIRED)
target_link_libraries(my_target PRIVATE sortlab::sortlab)
```

The package is header-only and has no required third-party runtime dependency.

## Correctness contract

`sortlab_v1_tests` validates, among other cases:

- empty, singleton, tiny, sorted, reverse-sorted, all-equal, duplicate-heavy, and signed-extreme data;
- randomized duplicate-heavy cases across every permanent comparison sorter;
- custom descending comparators;
- custom record projections;
- empirical stability for every comparison algorithm that declares stability;
- move-only types for algorithms whose API permits them;
- pathological/unbalanced TimSort and Powersort run geometries;
- forward/backward adaptive merging and dynamic gallop activation;
- LSD/MSD radix digit-width and signed-boundary behavior;
- bounded counting-sort rejection behavior;
- metadata uniqueness/completeness;
- optional instrumentation without changing the normal API.

The library test target compiles with strong warnings and `-Werror` on GCC/Clang builds. ASan/UBSan can be enabled locally with `SORTLAB_ENABLE_SANITIZERS`.

## Scope and non-goals

v1 is intentionally **sequential, in-memory, CPU sorting**.

Not v1 blockers:

- parallel sorting;
- GPU sorting;
- NUMA-specific placement/scaling;
- distributed sorting;
- external-memory/out-of-core sorting;
- architecture-specific SIMD sorting;
- variable-size string/blob specialization;
- exhaustive optimal sorting-network catalogs.

Those require different execution, memory, or evidence contracts and remain future research domains.

## Research material and `bench` migration

Bench v0.5.0 commit `acd9a77f9aa0fbb6edd569eb24c78b4694b442ed` has bench-native **definition/source parity for all 11 retained default empirical executables** at this repository's pinned revision, including the historically distinct 23-algorithm `sort_lab` treatment.

That does **not** authorize deletion yet. Replacement campaigns must actually be executed, checksum-verified, analyzed with `bench-analysis-v3`, reported, and accepted through bench's machine-readable migration gate. If historical evidence is registered, its verified import and explicit accepted comparison are also required.

The study-by-study subject view and ownership rules are in [`docs/research-migration-status.md`](docs/research-migration-status.md). The broader migration boundary is in [`docs/bench-migration.md`](docs/bench-migration.md).

Algorithm implementations, correctness contracts, API documentation, theory/references, and sorting-specific reconstruction assets remain subject-owned. Generic campaign/statistics/report/provenance/evidence-acceptance mechanics belong in `bench`.

## Completeness

[`V1_COMPLETENESS.md`](V1_COMPLETENESS.md) records the v1 completion decision, representative mechanism audit, deliberate deferrals, and the distinction between core blockers and future research.

**Core blockers at v1: none known.**

## License

MIT.
