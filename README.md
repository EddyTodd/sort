# sort

A C++23 library and study of **how different sorting algorithms actually behave**.

The repository is useful on its own: it contains reusable algorithms, deterministic correctness tests, operation instrumentation, and the theory needed to understand the implementations. Cross-algorithm performance experiments live in [`EddyTodd/bench`](https://github.com/EddyTodd/bench).

## Use it

```cpp
#include <sortlab/sort.hpp>

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

A small compile-checked example is built in a top-level checkout. `dev`, `release`, and `sanitize` run the normal library/correctness workflow; the separate `package` preset performs downstream install/relocation validation.

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

`bench` answers empirical questions such as crossover sizes, input-distribution sensitivity, insertion cutoffs, stability and payload costs, operation/allocation tradeoffs, and adaptive behavior:

```bash
./bench run sort
```

## Read more

- [`docs/algorithm-catalog.md`](docs/algorithm-catalog.md) — mechanisms and guarantees
- [`docs/theory.md`](docs/theory.md) — why the algorithms work and how they differ
- [`docs/library-api.md`](docs/library-api.md) — public API and instrumentation
- [`docs/development.md`](docs/development.md) — build options and package validation
- [`docs/scope.md`](docs/scope.md) — deliberate v1 boundary
- [`docs/references.md`](docs/references.md) — literature

## License

MIT.
