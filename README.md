# sortlab

`sortlab` is a header-only C++23 library of sequential in-memory sorting algorithms. It focuses on reusable generic APIs, representative algorithm mechanisms, deterministic correctness, stability semantics, and optional operation instrumentation. Empirical performance research lives in [`EddyTodd/bench`](https://github.com/EddyTodd/bench).

## Use

```cpp
#include <sortlab/sort.hpp>

#include <vector>

std::vector<int> values{5, 1, 4, 1, 3};
sortlab::intro_sort(values);
```

Comparison algorithms accept random-access iterators or ranges and support comparators and projections where applicable:

```cpp
struct row {
    int key;
    std::string payload;
};

std::vector<row> rows = /* ... */;
sortlab::timsort(rows, std::ranges::less{}, &row::key);
```

Instrumentation uses the same algorithm implementations rather than benchmark-only copies:

```cpp
sortlab::operation_counts counts;
sortlab::counting_observer observer(counts);
sortlab::instrumented::merge_sort(values.begin(), values.end(), observer);
```

## Scope

Version 1 covers sequential in-memory CPU sorting. The catalog includes:

- insertion, selection, bubble, comb, Shell, heap, merge, quicksort, and introspective families;
- merge/insertion and quick/insertion hybrids with configurable leaf cutoffs;
- natural merge sort and stable in-place divide/rotate merge sort;
- TimSort and PowerSort with shared adaptive stable merging;
- stable LSD radix, in-place MSD radix permutation, and bounded-domain counting sort for integral domains;
- generic bitonic sorting-network construction for tiny/data-oblivious workloads.

Parallel, GPU, distributed, external-memory, architecture-specific SIMD sorters, and exhaustive optimal-network catalogs are outside v1.

## Correctness

The deterministic suite covers empty/tiny inputs, sorted/reversed/equal/duplicate-heavy/random data, signed integer extremes, comparator/projection behavior, declared stability, move-only values where supported, adaptive run geometry, galloping, radix boundaries, counting-sort domain rejection, metadata consistency, and instrumentation contracts.

## Build

```bash
cmake --preset dev
cmake --build --preset dev
ctest --preset dev
```

`release`, `sanitize`, and `package` presets use the same interface.

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

## Documentation

- [`docs/library-api.md`](docs/library-api.md) — public API and instrumentation
- [`docs/algorithm-catalog.md`](docs/algorithm-catalog.md) — algorithm families and guarantees
- [`docs/theory.md`](docs/theory.md) — algorithmic background and tradeoffs
- [`docs/scope.md`](docs/scope.md) — v1 completeness and scope boundary
- [`docs/references.md`](docs/references.md) — literature

Workload campaigns, cutoff inference, crossover analysis, external implementation comparisons, statistics, provenance, and reports are intentionally centralized in [`EddyTodd/bench`](https://github.com/EddyTodd/bench).

## License

MIT.
