# Benchmark methodology

## 1. Objective

The benchmark produces falsifiable, reproducible evidence about sorting implementations. It deliberately separates four questions:

1. **Correctness:** does the implementation produce the required ordering and preserve the input multiset?
2. **Abstract work:** how many observable comparisons, swaps, and writes does the implementation perform?
3. **Machine performance:** how long does the implementation take under a defined software/hardware environment?
4. **Statistical evidence:** how stable is the observed difference, and does it persist across independent input instances?

No single metric answers all four.

## 2. Correctness contract

Every timed and instrumented run is checked against an independently sorted reference copy. A failure aborts the experiment; an incorrect implementation never contributes a timing row.

`--self-test` exercises every algorithm in both counter-disabled and counter-enabled modes on edge cases and every workload family, including empty ranges, duplicates, signed extremes, structured inputs, and deterministic generated data. It also tests the seed/fingerprint reproducibility contract.

## 3. Timing versus instrumentation

Operation counters must not contaminate elapsed-time measurements. Each algorithm therefore has two compile-time instantiations:

- **timed:** counter updates compile out;
- **instrumented:** counter updates are enabled and the pass is not timed.

For one trial, all algorithms complete their timed runs before any instrumentation pass begins. This prevents an instrumented run from directly warming data for the next timed competitor in the same trial.

Input generation, input copying, verification, CSV output, and operation-count collection are outside the timed region. Allocations performed by the sorting implementation itself remain inside the timed region because they are part of that implementation's end-to-end cost.

## 4. Independent trials and pairing

A trial seed is deterministically derived from `(experiment_seed, pattern, n, trial)`. Consequently, each trial receives a distinct input while every algorithm in that trial receives identical values.

This design enables paired comparisons. `tools/analyze.py` matches algorithms by `(pattern, n, trial, input_hash)` before calculating speedup against a baseline. Paired ratios reduce variance from input-instance difficulty and prevent accidental comparison of unrelated datasets.

The same experiment seed reproduces the same trial seeds and dataset fingerprints. Seed derivation, workload generation, shuffling, and hashing use project-defined deterministic operations rather than `std::hash` or standard-library distributions whose exact mapping is not a cross-implementation experiment identity contract.

## 5. Execution-order bias

Algorithm order is deterministically shuffled independently for every measured trial. The emitted `execution_order` column makes order effects observable and supports later regression/covariate analysis.

Warmups are configurable and occur before measured trials for each `(pattern, n)` cell. Warmups are correctness-checked but not emitted as measurements.

Randomized order does not eliminate thermal drift, scheduler effects, frequency scaling, or shared-system noise. Canonical runs should additionally control the host as described in `reproducibility.md`.

## 6. Workload families

Current deterministic workloads are:

- `random`: bounded signed values from a deterministic MT19937-64 mapping;
- `sorted`: ascending random sample;
- `reversed`: descending random sample;
- `few_unique`: eight keys;
- `all_equal`: one key, isolating duplicate behavior;
- `nearly_sorted`: ascending sample with approximately 1% swaps;
- `organ_pipe`: values rise toward the center and fall symmetrically;
- `sawtooth`: periodic 32-key structure;
- `runs`: independently random data sorted within runs of 32.

These are not claimed to represent all real workloads. They are controlled probes for order, entropy, duplicates, local structure, and partition behavior.

## 7. Metrics

`ns` is elapsed wall-clock nanoseconds from `std::chrono::steady_clock` around only the counter-disabled sort call.

`comparisons`, `swaps`, and `writes` come from the separate instrumented pass. These counters are algorithmic observables, not a universal CPU cost model. In particular:

- standard-library sort internals expose comparisons through the supplied comparator but not internal swaps/writes;
- radix sort performs no ordering comparisons;
- cache misses, branch misses, instructions, vectorization, allocation, and memory bandwidth are not represented by the current counters.

Fields that cannot be observed are not estimated as if they were measurements.

## 8. Statistical treatment

Raw trials are canonical; summaries are derived artifacts. The included dependency-free analyzer reports:

- sample count;
- median;
- first and third quartiles;
- median absolute deviation (MAD);
- percentile-bootstrap 95% confidence interval for the median;
- paired median speedup and paired bootstrap interval versus a selected baseline.

Bootstrap intervals are descriptive uncertainty estimates, not a substitute for experimental design. A publication-quality claim should use enough independent trials to stabilize both the center and interval, report exact sample counts, and show the full size/distribution curve rather than cherry-picking one cell.

No automatic outlier removal is performed. If exclusions are scientifically justified, the rule must be specified before inspecting the result and both raw and filtered analyses must be retained.

## 9. Environment reporting

Canonical results require:

- CPU model, architecture, topology, and relevant cache information;
- OS/kernel version;
- compiler and standard-library version;
- optimization flags and build type;
- source commit and clean/dirty state;
- command line, seed, warmups, and trial count;
- frequency/turbo/affinity controls where applicable;
- raw CSV SHA-256.

`tools/run_experiment.py` captures a portable subset automatically. Platform-specific details that cannot be captured portably must be recorded manually rather than omitted from a publication claim.

## 10. Known measurement threats

The current harness does not yet control or measure:

- CPU affinity, governor/turbo state, thermals, interrupt load, and context switches;
- cache state or cache/TLB/branch hardware counters;
- allocator counts and peak resident memory;
- timer-call overhead for very small inputs;
- energy and power;
- interleaved multi-process interference;
- compiler-generated code size or instruction mix.

These limitations are research backlog items. Results should be scoped accordingly.
