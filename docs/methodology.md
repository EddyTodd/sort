# Benchmark methodology

## 1. Objective

The benchmark produces falsifiable, reproducible evidence about sorting implementations. It separates five questions:

1. **Correctness:** does the implementation produce the required ordering and preserve every input element?
2. **Abstract work:** how many observable comparisons, swaps, writes, or whole-record moves does it perform?
3. **Semantic behavior:** does it preserve stability where required?
4. **Machine performance:** how long does it take under a defined software/hardware environment?
5. **Statistical evidence:** how stable is the observed difference across independent input instances?

No single metric answers all five.

## 2. Two complementary experiments

`sort_lab` studies signed 64-bit scalar values. It is useful for isolating comparison/partition behavior with a small fixed element type.

`sort_records` studies fixed-size key/payload records. The comparator observes only the signed 64-bit key; the record also carries an original ordinal and deterministic payload. This makes element-width sensitivity, exact payload preservation, and empirical stability directly measurable.

Results from the two executables are different experimental strata and must not be pooled into one undifferentiated ranking.

## 3. Correctness contract

Every timed and instrumented scalar run is checked against an independently sorted reference copy. A failure aborts the experiment; an incorrect implementation never contributes a timing row.

Record verification is stronger: output keys must be nondecreasing, every original ordinal must occur exactly once, and the output record at that ordinal must preserve the exact original key and payload. Algorithms declaring a stability guarantee are asserted to preserve equal-key ordinal order in deterministic tests.

Both executables test counter-disabled and counter-enabled implementations. Scalar self-tests cover all scalar workload families; record self-tests cover all record workload families and representative payload widths.

## 4. Timing versus instrumentation

Operation counters must not contaminate elapsed-time measurements. Each project-controlled algorithm therefore has two compile-time instantiations:

- **timed:** counter updates compile out;
- **instrumented:** counter updates are enabled and the pass is not timed.

For one trial, all algorithms complete timed runs before any instrumentation pass begins. Input generation, copying, verification, hashing, CSV output, and operation-count collection are outside the timed region.

Allocations performed inside the sorting implementation remain inside elapsed time because they are part of end-to-end algorithm cost.

## 5. Independent trials and pairing

A trial seed is deterministically derived from `(experiment_seed, pattern, n, trial)`. Each trial receives a distinct input while every algorithm in that trial receives identical values.

Scalar analysis pairs algorithms by `(pattern, n, trial, input_hash)`.

Record experiments additionally preserve the same generated key sequence across payload widths. `key_hash` identifies the shared key sequence and `input_hash` fingerprints the full record representation. Algorithm-vs-baseline pairing occurs within the same record width.

Seed derivation, bounded RNG mapping, workload generation, shuffling, and hashing use project-defined deterministic operations rather than implementation-defined `std::hash` behavior.

## 6. Execution-order bias

Algorithm order is deterministically shuffled independently for every measured trial. The emitted `execution_order` column makes order effects observable.

Warmups are configurable and occur before measured trials for each experiment cell. Warmups are correctness-checked but not emitted.

Randomized order reduces systematic order bias but does not eliminate thermal drift, scheduling, frequency scaling, or shared-system noise. Canonical runs require additional host controls described in `reproducibility.md`.

## 7. Workload families

The scalar and record harnesses expose the same 15 controlled probes:

- `random`: bounded high-entropy signed values;
- `sorted`: ascending random sample;
- `reversed`: descending random sample;
- `few_unique`: eight keys;
- `binary`: two keys;
- `all_equal`: one key;
- `nearly_sorted`: ascending sample with approximately 1% swaps;
- `organ_pipe`: values rise toward the center and fall symmetrically;
- `sawtooth`: periodic 32-key structure;
- `runs`: ascending runs of length 32;
- `descending_runs`: descending runs of length 32;
- `rotated`: sorted data rotated by roughly one third;
- `alternating_extremes`: alternating low/high extremes;
- `staggered`: deterministic modular periodic sequence;
- `plateau`: symmetric gradients surrounding a large equal-key plateau.

These are controlled probes, not estimates of real-world workload prevalence. See `adversarial-workloads.md`.

## 8. Metrics

### Scalar

`ns` is elapsed wall-clock nanoseconds around only the counter-disabled sort call. `comparisons`, `swaps`, and `writes` come from the untimed instrumentation pass.

### Records

The record harness reports elapsed `ns`, observable comparisons, explicit swaps, `explicit_record_moves`, `explicit_bytes_moved`, `stable_on_trial`, and exact verification status.

`explicit_bytes_moved = explicit_record_moves × sizeof(record)` is an algorithmic movement metric for project-controlled implementations. It is **not** cache traffic or DRAM traffic. Standard-library internal moves are inaccessible and remain unreported.

All operation counters are abstract observables rather than a universal CPU cost model. Cache misses, branch misses, instructions, vectorization, allocator internals, and memory bandwidth require separate measurement.

## 9. Statistical treatment

Raw trials are canonical; summaries are derived artifacts. The scalar and record reducers report robust descriptive summaries, paired median speedup, bootstrap intervals, paired win rate, and an exact two-sided paired sign test.

No automatic outlier removal is performed. A scientifically justified exclusion rule must be defined before inspecting the final result, and original artifacts must be retained.

The sign test is intentionally magnitude-blind. It complements rather than replaces effect-size estimates. Large benchmark grids create a multiple-comparison problem; isolated unadjusted p-values are not sufficient evidence for publication claims.

`tools/crossovers.py` identifies adjacent measured sizes where median paired speedup crosses 1.0 and interpolates a candidate crossing in log-size/log-speedup space. This is a screening tool. Candidate brackets require denser follow-up sampling before a crossover is promoted as a conclusion.

See `statistics.md` for interpretation rules.

## 10. Environment reporting

Canonical results require:

- CPU model, architecture, topology, and relevant cache information;
- OS/kernel version;
- compiler and standard-library version;
- optimization flags and build type;
- source commit and clean/dirty state;
- executable and experiment command line;
- seed, warmups, trial count, workload set, and payload widths where applicable;
- frequency/turbo/affinity controls where applicable;
- raw CSV SHA-256.

`tools/run_experiment.py` captures a portable subset automatically. Platform-specific details that cannot be captured portably must be recorded manually rather than omitted.

## 11. Known measurement threats

The current harness does not yet directly control or measure:

- CPU affinity, governor/turbo state, thermals, interrupts, and context switches;
- cache/TLB/branch hardware counters;
- allocator call counts and peak auxiliary/resident memory;
- timer-call overhead for very small inputs;
- energy and power;
- inter-process interference;
- compiler-generated code size or instruction mix;
- variable-size/string move costs;
- NUMA or external-memory behavior.

Record movement counters improve the data-movement model but do not remove these limitations. Mechanism claims must wait for direct mechanism measurements.
