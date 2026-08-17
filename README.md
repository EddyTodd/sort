# sort

A C++23 library and study of **how different sorting algorithms actually behave**.

This repository is useful independently: it contains reusable algorithms, deterministic correctness tests, operation instrumentation, and the theory needed to understand the implementations. Cross-algorithm performance research lives in [`EddyTodd/bench`](https://github.com/EddyTodd/bench).

## Use it

```cpp
#include <sortlab/sortlab.hpp>

#include <vector>

std::vector<int> values{5, 1, 4, 1, 3};
sortlab::intro_sort(values);
```

Comparison algorithms accept random-access iterators or ranges and support comparators and projections where appropriate.

As a CMake subdirectory:

```cmake
add_subdirectory(path/to/sort)
target_link_libraries(app PRIVATE sortlab::sortlab)
```

Or consume an installed package:

```cmake
find_package(sortlab 1 CONFIG REQUIRED)
target_link_libraries(app PRIVATE sortlab::sortlab)
```

## Build and test

```bash
cmake --preset release
cmake --build --preset release
ctest --preset release
```

The build system is intentionally minimal: `dev` and `release` are the only shared presets, and project-specific CMake is limited to building the library, tests/examples, and optional install rules.

## Algorithms

The catalog is intentionally representative rather than exhaustive:

- insertion, selection, bubble, comb, and Shell sort;
- heap sort;
- merge, natural merge, stable in-place merge, and merge/insertion hybrids;
- Hoare, three-way, median-of-three, dual-pivot, hybrid quicksort, and introsort;
- TimSort and PowerSort;
- stable LSD radix, in-place MSD radix, and bounded counting sort;
- bitonic sorting networks for tiny/data-oblivious cases.

Optional instrumentation counts operations using the same permanent implementations; there are no benchmark-only copies of the algorithms.

## Research

Performance measurement belongs outside this repository. `bench` can consume the normal public API, but `sort` does not depend on it.

## Documentation

- [`docs/algorithm-catalog.md`](docs/algorithm-catalog.md) — mechanisms and guarantees
- [`docs/theory.md`](docs/theory.md) — why the algorithms work and how they differ
- [`docs/api.md`](docs/api.md) — public API and instrumentation
- [`docs/development.md`](docs/development.md) — build and install
- [`docs/scope.md`](docs/scope.md) — deliberate v1 boundary
- [`docs/references.md`](docs/references.md) — literature

## License

MIT.
