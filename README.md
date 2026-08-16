# sort

A C++23 study of **how different sorting algorithms actually behave**.

The repository keeps the algorithms, their correctness tests, and the theory needed to understand them. Performance experiments live in [`EddyTodd/bench`](https://github.com/EddyTodd/bench), so the sorting code stays easy to inspect and reuse.

## Try it

```cpp
#include <sortlab/sort.hpp>

#include <vector>

std::vector<int> values{5, 1, 4, 1, 3};
sortlab::intro_sort(values);
```

Comparison algorithms use ordinary random-access iterators/ranges and support comparators and projections where appropriate.

Build and run the correctness suite:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

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

`bench` answers the empirical questions: crossover sizes, input-distribution sensitivity, insertion cutoffs, stability and payload costs, operation/allocation tradeoffs, adaptive behavior, and external baselines.

From the `bench` repository, the default sorting study is intended to be runnable as:

```bash
python3 -m benchctl run sort
```

## Read more

- [`docs/algorithm-catalog.md`](docs/algorithm-catalog.md) — mechanisms and guarantees
- [`docs/theory.md`](docs/theory.md) — why the algorithms work and how they differ
- [`docs/library-api.md`](docs/library-api.md) — public API and instrumentation
- [`docs/scope.md`](docs/scope.md) — deliberate v1 boundary
- [`docs/references.md`](docs/references.md) — literature

## License

MIT.
