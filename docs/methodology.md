# Benchmark methodology

## Objective

The benchmark is intended to produce falsifiable, reproducible claims about sorting implementations. It separates three questions that are often conflated: correctness, abstract algorithmic work, and observed machine performance.

## Correctness contract

Every measured result must be nondecreasing and must contain exactly the same values as its input. The harness enforces this by sorting an independent copy with the standard library and requiring exact vector equality. A failed verification aborts the run rather than publishing a timing for an incorrect implementation.

The deterministic self-test covers empty and singleton inputs, duplicates, negative values, ordered and reverse-ordered data, and generated duplicate-heavy cases for every registered algorithm.

## Workloads

Milestone 1 uses five deterministic families:

1. `random`: uniform signed 64-bit values drawn from a bounded range.
2. `sorted`: the random sample sorted ascending.
3. `reversed`: the random sample sorted descending.
4. `few_unique`: values drawn from eight keys, stressing duplicate behavior.
5. `nearly_sorted`: ascending data followed by approximately 1% random swaps.

Inputs are generated once per `(pattern, n, seed)` and copied for each algorithm so competitors receive identical values. The timed region excludes input generation, copying, output, and correctness verification.

## Metrics

`ns` is wall-clock elapsed nanoseconds from `std::chrono::steady_clock`. It is useful for end-to-end local comparisons but sensitive to CPU frequency, scheduling, thermals, memory hierarchy, compiler, and build flags.

`comparisons` counts ordering predicates issued by the implementation. `swaps` counts explicit swaps in implementations controlled by this project. `writes` counts explicit element writes where the implementation exposes them. Standard-library algorithms only expose comparisons through the supplied comparator, so their swap/write fields intentionally remain zero rather than pretending inaccessible operations were measured.

## Repetition and statistics

The executable emits raw trials instead of collapsing them inside the benchmark. Analysis should retain raw samples and report at least median, interquartile range, and a robust uncertainty estimate. For publication-quality comparisons, use enough independent repetitions to stabilize the estimate, randomize or rotate execution order to assess order effects, and report effect sizes and confidence intervals rather than relying only on point estimates.

A later milestone should add bootstrap confidence intervals and explicit crossover analysis: the goal is not merely to rank algorithms, but to estimate where one implementation becomes materially faster than another under each workload.

## Environment reporting

Published results must record:

- CPU model, core topology, and architecture;
- operating system and version;
- compiler and version;
- optimization flags and build type;
- source commit SHA;
- benchmark seed and trial count;
- whether frequency scaling/turbo was controlled;
- relevant memory and cache information when available.

Results without this metadata are exploratory, not canonical.

## Bias controls

The project should progressively add controls for warm-up effects, execution-order bias, CPU affinity, thermal throttling, background load, allocation effects, cache state, compiler dead-code elimination, and benchmark harness overhead. When a control cannot be applied portably, the limitation should be documented instead of hidden.

## Planned experimental expansion

Future milestones will add more algorithm families and input distributions, element widths and record payload sizes, stability verification, memory-allocation accounting, peak auxiliary memory, Linux `perf` counters, branch and cache metrics, SIMD/parallel variants, multiple compilers and optimization levels, sanitizer validation, and scripts that convert raw CSV into versioned summary tables and plots.
