# Benchmark methodology

## 1. Objective

The laboratory produces falsifiable, reproducible evidence about sorting implementations. It separates six questions that are often conflated:

1. **Correctness:** does the implementation produce the required order without corrupting data?
2. **Abstract work:** how many observable comparisons, swaps, writes, or record moves occur?
3. **Machine performance:** how long does the implementation take in a defined environment?
4. **Systems mechanism:** what do directly observed allocation/hardware counters say about plausible causes?
5. **Statistical evidence:** how stable is the difference across independent paired inputs?
6. **Decision quality:** if a cutoff or adaptive selector is tuned, does it improve held-out performance after selection overhead?

No single metric answers all six.

## 2. Correctness contract

Every timed/instrumented scalar result is compared with an independently sorted reference. Record results additionally verify exact ordinal/key/payload preservation and empirical equal-key order. A correctness failure aborts the run rather than contributing a timing.

The self-tests include empty and singleton inputs, signed extremes, duplicates, deterministic generated inputs, structured/adversarial workloads, and equivalence between timed and instrumented implementations.

## 3. Timing versus instrumentation

Operation counters must not contaminate elapsed-time measurements. Project algorithms therefore expose counter-disabled timed forms and separate counter-enabled forms.

For one scalar/record trial:

1. all timed competitors run in randomized order;
2. optional input features are probed **after** all timed competitors;
3. operation instrumentation runs afterward;
4. CSV emission occurs last.

Input generation, copying, verification, CSV output, operation counting, and feature extraction are outside each algorithm's `ns` timing. Allocations made by the sorting implementation remain inside its timed call in the canonical executable.

The feature probe has its own `feature_ns`; adaptive-portfolio analysis adds that cost to the selected sort.

## 4. Independent trials and pairing

A deterministic trial seed is derived from `(experiment_seed, pattern, n, trial)`. Every trial receives a distinct input while all competitors in that trial receive identical values.

The project-defined seed/hash functions avoid using `std::hash` or standard-library random-distribution mappings as cross-platform experiment identity. `input_hash` fingerprints the generated data.

Paired analyses match algorithms on the same input identity. This reduces input-difficulty variance and prevents accidental comparison of unrelated samples.

## 5. Execution-order bias

Algorithm execution order is deterministically shuffled independently for each trial and recorded. Warmups are configurable and correctness-checked but not emitted.

Randomized order does not eliminate thermal drift, scheduler effects, SMT contention, frequency scaling, interrupts, or other host noise. Controlled campaigns should combine randomized order with affinity and host-state recording.

## 6. Workload families

The current deterministic workload set contains 15 controlled probes:

- `random` — bounded signed pseudo-random keys;
- `sorted` — ascending order;
- `reversed` — descending order;
- `few_unique` — eight-key domain;
- `binary` — two-key domain;
- `all_equal` — one key;
- `nearly_sorted` — sorted then approximately 1% random swaps;
- `organ_pipe` — rise toward center then fall;
- `sawtooth` — periodic 32-key structure;
- `runs` — ascending sorted runs;
- `descending_runs` — descending sorted runs;
- `rotated` — sorted sequence rotated by a fixed fraction;
- `alternating_extremes` — alternating low/high extremes;
- `staggered` — deterministic periodic modular keys;
- `plateau` — long equal-valued central plateau.

The workloads are probes of mechanisms, not a statistical model of all production data.

## 7. Scalar metrics

Canonical scalar CSV includes:

- wall-clock `ns` around only the counter-disabled sort;
- comparisons/swaps/writes from the untimed instrumented pass;
- execution order, trial seed, and input fingerprint;
- bounded input-probe metrics and probe time.

For standard-library algorithms, inaccessible internal swaps/writes remain unreported rather than estimated.

## 8. Record metrics

Record experiments vary payload width while keeping key sequences paired across widths. Verification checks record identity and stability.

Project-controlled record algorithms report explicit record moves and derived explicit bytes moved. This is an algorithmic data-movement count—not cache traffic, DRAM traffic, or bandwidth.

## 9. Cutoff and hybrid metrics

`sort_cutoffs` varies insertion thresholds while holding the surrounding merge/quicksort/introsort mechanism fixed. `cutoff=1` serves as the no-insertion-leaf baseline.

Parameter selection and evaluation must be disjoint. `tools/tune_cutoffs.py` uses deterministic training/held-out trials. A tuned threshold is reported with its domain; it is not generalized outside the measured compiler/type/workload/environment.

## 10. Feature-based portfolio metrics

The scalar harness records only cheap, runtime-observable features: `n`, sampled inversion rate, sampled duplicate fraction, and sampled key-range width. The benchmark pattern label is forbidden as a selector feature.

`tools/portfolio.py` fits per-algorithm cost models on training trials and evaluates the selector on held-out trials. Report:

- best single algorithm selected on training data;
- held-out best-single cost;
- held-out portfolio cost including `feature_ns`;
- portfolio speedup/regret;
- held-out per-instance oracle as an unattainable ceiling.

The oracle is diagnostic; it is not an implementable algorithm.

## 11. Hardware counters

`sort_perf` uses Linux `perf_event_open` around only the sorting call. It requests cycles, instructions, branches, branch misses, cache references, and cache misses.

Events can be denied, virtualized, or multiplexed. The harness scales multiplexed events using kernel time-enabled/time-running values and marks the sample unavailable if counters cannot be obtained meaningfully. An unavailable event set is not interpreted as zero work.

Ratios such as CPI or miss rate are mechanism diagnostics, not universal cost functions.

## 12. Allocation measurement

`sort_alloc` is intentionally separate from canonical timing. It overrides ordinary C++ `new`/`delete` only in that executable and tracks allocation calls, requested bytes, peak tracked live bytes, and largest allocation around the sort.

It does not claim to intercept direct `malloc`, custom allocators, every over-aligned path, stack memory, page faults, resident-set size, or kernel allocation. Because allocator interposition changes execution, allocation runs are evidence about allocation behavior—not canonical wall-time evidence.

## 13. Statistical treatment

Raw trials are canonical. The bundled reducers report robust descriptive statistics, percentile-bootstrap uncertainty, paired median speedup, paired win rate, and exact paired sign tests.

No automatic outlier deletion is performed. If an external invalidation rule is scientifically justified, it must be defined before result inspection and the original artifact retained.

Multiple sizes/workloads/widths create families of comparisons. Formal significance claims must account for multiplicity rather than mining isolated cells.

## 14. Crossover treatment

A crossover is not the first noisy sample where a median changes order. Candidate adjacent crossings are identified from paired speedup curves and must be sampled more densely before publication. Report a bracket/uncertainty region rather than false integer precision.

## 15. Reproducibility and host state

`tools/run_experiment.py` captures raw-data and binary SHA-256, exact command, Git state, host metadata, requested CPU affinity, load averages, and available Linux/macOS control information.

Canonical results should additionally document anything material that cannot be captured portably: physical CPU topology, cache hierarchy, SMT state, frequency/turbo controls, thermals, compiler flags, standard-library version, and background-load policy.

## 16. Threats and separate experiment models

The portable core does not pretend one harness can answer every sorting question. Separate tracks are required for:

- SIMD/vectorized algorithms and instruction-set-specific code;
- parallel sorting and scalability;
- NUMA placement;
- GPU sorting;
- external-memory/I/O-complexity sorting;
- variable-size strings/objects and indirect/index sorting;
- energy/power measurement;
- direct memory-bandwidth/DRAM-traffic analysis.

These are different experimental contracts, not hidden missing columns in the scalar CSV.
