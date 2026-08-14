# sortlab

`sortlab` is a C++23 header-only library of sequential in-memory sorting algorithms. It focuses on algorithm mechanisms, generic reusable APIs, deterministic correctness, and sorting theory.

Empirical performance research lives in [`EddyTodd/bench`](https://github.com/EddyTodd/bench). This repository intentionally contains no benchmark runner, campaign framework, performance result corpus, or benchmark-specific algorithm copy.

## Use

```cpp
#include <sortlab/sort.hpp>

#include <vector>

std::vector<int> values{5, 1, 4, 1, 3};
sortlab::intro_sort(values);
```

Comparison algorithms accept random-access iterators/ranges and, where applicable, comparators and projections:

```cpp
struct row {
  int key;
  std::string payload;
};

std::vector<row> rows = /* ... */;
sortlab::timsort(rows, std::ranges::less{}, &row::key);
```

Optional instrumentation uses the same algorithm implementations rather than benchmark-specific copies:

```cpp
sortlab::operation_counts counts;
sortlab::counting_observer observer(counts);
sortlab::instrumented::merge_sort(values.begin(), values.end(), observer);
```

See `docs/library-api.md`.

## Algorithm catalog

### Elementary

- insertion sort;
- binary insertion sort;
- selection sort;
- bubble sort;
- comb sort;
- Ciura and Pratt Shell sorts.

### Heap and merge families

- heap sort;
- top-down merge sort;
- bottom-up merge sort;
- natural merge sort;
- merge/insertion hybrid with configurable leaf cutoff;
- stable in-place divide/rotate merge sort.

### Quicksort and introspective families

- Hoare quicksort;
- three-way quicksort;
- median-of-three quicksort;
- dual-pivot quicksort;
- quick/insertion hybrid with configurable leaf cutoff;
- introsort with insertion leaves and heap fallback.

### Adaptive stable merges

- TimSort with run detection, binary extension, stack invariants, smaller-run buffering, forward/backward stable merging, galloping, and dynamic `min_gallop` adaptation;
- PowerSort using the same stable adaptive merge kernel with node-power scheduling.

### Distribution sorting

Integral types:

- stable LSD radix sort;
- in-place MSD radix permutation;
- bounded-domain stable counting sort.

Signed integer ordering is handled across the full representable range.

### Tiny/data-oblivious

- generic bitonic sorting network construction, including non-power-of-two lengths.

See `docs/algorithm-catalog.md`, `REFERENCES.md`, and `V1_COMPLETENESS.md`.

## Correctness

The permanent deterministic suite covers:

- empty/singleton/tiny inputs;
- sorted, reversed, equal, duplicate-heavy, and randomized data;
- signed integer extremes;
- custom comparators and projections;
- declared stability guarantees;
- move-only types where supported;
- pathological TimSort/PowerSort run geometries;
- forward/backward adaptive merges and galloping;
- radix digit-width and signed-boundary behavior;
- counting-sort domain rejection;
- metadata consistency;
- optional operation instrumentation.

## Build and test

```bash
cmake --preset dev
cmake --build --preset dev
ctest --preset dev
```

Release and sanitizer presets are also provided:

```bash
cmake --preset release
cmake --build --preset release
ctest --preset release

cmake --preset sanitize
cmake --build --preset sanitize
ctest --preset sanitize
```

## Install

```bash
cmake --preset package
cmake --build --preset package
```

Consumer:

```cmake
find_package(sortlab 1 CONFIG REQUIRED)
target_link_libraries(your_target PRIVATE sortlab::sortlab)
```

The installed package is header-only and has no required third-party runtime dependency.

## Scope

Version 1 is sequential, in-memory CPU sorting. Parallel/GPU/distributed/external-memory sorting, architecture-specific SIMD sorters, and exhaustive optimal sorting-network catalogs are separate future domains rather than v1 blockers.

Performance comparisons, workload campaigns, cutoff inference, external implementation comparisons, and result reporting are intentionally performed in `EddyTodd/bench` against this public API.

## License

MIT.
