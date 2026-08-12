# sort

A reproducible C++23 laboratory for studying sorting algorithms empirically, not just quoting asymptotic complexity.

The project is designed around a simple question: **which sorting strategy actually wins for a particular input distribution, size, machine, and cost metric?** It keeps textbook algorithms beside production-library baselines so their tradeoffs can be measured under the same harness.

## Milestone 1

The first milestone establishes the experimental foundation:

- 8 implementations: insertion, selection, bubble, heap, merge, quicksort, `std::sort`, and `std::stable_sort`.
- deterministic workloads: random, sorted, reverse-sorted, low-cardinality, and nearly-sorted data.
- exact correctness checking against an independently sorted reference.
- timing with `std::chrono::steady_clock` plus algorithmic counters for comparisons, swaps, and explicit writes where observable.
- repeated trials and machine-readable CSV output.
- a deterministic self-test covering edge cases, duplicates, negatives, and generated inputs.
- CMake/CTest support with no hosted CI requirement.

This is intentionally the start of a research repository rather than a leaderboard with unqualified numbers. Benchmark results are only meaningful with compiler, flags, CPU, OS, workload, sample count, and statistical treatment recorded.

## Build

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
```

Or compile the single source file directly:

```sh
c++ -std=c++23 -O3 -DNDEBUG sort.cpp -o sort_lab
./sort_lab --self-test
```

## Benchmark

```sh
./build/sort_lab > results.csv
./build/sort_lab --trials 15 --sizes 64,1024,16384,262144 > results.csv
```

CSV schema:

```text
algorithm,pattern,n,trial,seed,ns,comparisons,swaps,writes,verified
```

Quadratic algorithms are skipped above 16,384 elements by default so a standard sweep remains practical. The seed is fixed by default and can be changed with `--seed N`.

## Why these workloads?

A single uniform-random benchmark hides behavior that matters in real systems. Sorted and nearly-sorted inputs expose adaptivity; reversed input stresses some naïve strategies; few-unique inputs exposes duplicate handling; increasing sizes reveal scaling and cache effects. Future milestones will add organ-pipe, sawtooth, runs, adversarial quicksort patterns, larger element types, key/value records, stability checks, and external datasets.

## Interpretation rules

Do not compare raw nanoseconds from different machines as if they were interchangeable. Do not infer a universal winner from one input size or distribution. The included operation counters provide a machine-independent complement to elapsed time, but they are not a CPU cost model: cache misses, branches, vectorization, allocation, instruction count, and memory traffic can dominate.

See [`docs/methodology.md`](docs/methodology.md) for the measurement contract and roadmap.

## Roadmap

Planned research includes additional comparison sorts, integer/radix families, sorting networks, parallel and SIMD implementations, branchless variants, stable/unstable and in-place/out-of-place tradeoffs, element-width sensitivity, hardware performance counters, rigorous confidence intervals, effect sizes, crossover-point estimation, compiler/architecture matrices, and reproducible published benchmark datasets.

## License

MIT.
