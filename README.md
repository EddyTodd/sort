# sort

A C++23 laboratory for the **theoretical, empirical, and systems-level study of sorting algorithms**.

The project does not ask only which algorithm has the best asymptotic bound. It asks which implementation wins, by how much, under which input structure, element width, stability requirement, compiler, standard library, and microarchitecture—and what mechanism plausibly explains that result.

Raw measurements are first-class research artifacts. Correctness, asymptotic theory, operation counts, elapsed time, record movement, allocation behavior, hardware counters, statistical uncertainty, and tuning decisions are kept separate rather than collapsed into a single leaderboard.

## Research surface

### Scalar algorithms

`sort_lab` currently exposes **23 algorithms and scientifically material variants** over signed 64-bit integers:

- insertion: linear insertion and binary insertion;
- exchange/selection: selection, bubble, and comb sort;
- Shell: Ciura-gap and Pratt-gap variants;
- heap: project heapsort and a standard-library heap baseline;
- merge: top-down, bottom-up, natural/run-adaptive, and merge+insertion;
- quicksort: two-way Hoare, duplicate-aware three-way, median-of-three, dual-pivot, and quicksort+insertion;
- hybrid: introsort plus parameterized insertion-cutoff families;
- distribution: stable 8-bit LSD radix and an 11-bit-digit LSD radix variant;
- library: `std::sort` and `std::stable_sort`.

Run `sort_lab --list-algorithms` for machine-readable metadata covering family, stability, in-place status, adaptivity, asymptotic bounds, and auxiliary space. [`docs/algorithm-catalog.md`](docs/algorithm-catalog.md) explains the coverage policy and external state-of-the-art comparison tracks.

### Controlled workloads

The scalar and record laboratories expose 15 deterministic workload families: random, sorted, reversed, few-unique, binary, all-equal, nearly-sorted, organ-pipe, sawtooth, ascending runs, descending runs, rotated sorted data, alternating extremes, staggered periodic keys, and plateau data.

These are controlled probes, not claims about the frequency of real-world workloads. Their purpose is to expose adaptivity, duplicate handling, partition behavior, run exploitation, and adversarial sensitivity.

### Record width and stability

`sort_records` benchmarks nine representative algorithms over fixed-size key/payload records. Payload widths span 0, 1, 3, 7, 15, and 31 64-bit words; the executable records the actual ABI `sizeof(record)`.

Every record retains its original ordinal and deterministic payload, allowing the harness to verify:

- nondecreasing key order;
- exact record/payload preservation;
- empirical stability on equal keys;
- stability guarantees for algorithms that claim them.

Project-controlled algorithms also expose explicit record moves and `explicit_bytes_moved`. These are algorithmic observables, not estimates of cache or DRAM traffic.

## Hybrid and cutoff research

A central research question is whether a practical sorter should combine mechanisms rather than use one algorithm everywhere.

`sort_cutoffs` treats the insertion-sort base-case threshold as an experimental variable for:

- merge sort + insertion leaves;
- median-of-three quicksort + insertion leaves;
- introsort + insertion leaves.

```sh
./build/sort_cutoffs \
  --cutoffs 1,4,8,12,16,20,24,32,48,64,96,128 \
  --sizes 8,12,16,24,32,48,64,96,128,192,256,512,1024,2048,4096,8192 \
  --patterns random,sorted,few_unique,nearly_sorted,runs \
  --trials 51 > cutoffs.csv

python3 tools/tune_cutoffs.py cutoffs.csv --output cutoff-summary.csv
```

The tuning tool selects a cutoff on training trials and evaluates it on a deterministic held-out split. The repository does **not** assume one folklore threshold is universally optimal.

`tools/claim_matrix.py` evaluates preregistered paired claims including insertion-vs-merge crossover, linear-vs-binary insertion, hybrid-vs-pure merge/quicksort, two-way-vs-three-way quicksort, radix digit width, and run-adaptive-vs-fixed merge.

See [`docs/hybrid-research.md`](docs/hybrid-research.md).

## Tiny-sort kernels and hybrid leaves

The next question is stricter than “what insertion cutoff is best?”: **is insertion sort actually the best leaf algorithm?**

`sort_tiny` directly compares linear insertion, binary insertion, a padded power-of-two bitonic sorting network, and `std::sort` for every small-array treatment up to 32 elements. The bitonic implementation has a data-oblivious comparator topology; the project does not assume that implies branch-free machine code on every compiler/ISA.

```sh
./build/sort_tiny \
  --sizes 2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,24,32 \
  --patterns random,sorted,reversed,few_unique,nearly_sorted \
  --trials 101 > tiny.csv

python3 tools/analyze_tiny.py tiny.csv \
  --baseline insertion --bootstrap 5000 --output tiny-summary.csv
```

`sort_leaf_hybrids` then jointly varies **parent family × leaf kernel × cutoff** for merge, median-of-three quicksort, and introsort:

```sh
./build/sort_leaf_hybrids \
  --families merge,quick,intro \
  --kernels insertion,binary_insertion,bitonic_network \
  --cutoffs 4,8,12,16,20,24,28,32 \
  --patterns random,sorted,reversed,few_unique,nearly_sorted,runs \
  --sizes 64,128,256,512,1024,4096,16384 \
  --trials 31 > leaf-hybrids.csv

python3 tools/tune_leaf_kernels.py leaf-hybrids.csv \
  --output leaf-tuning.csv
```

The joint tuner selects `(kernel, cutoff)` on training trials and evaluates it held-out against the **best insertion-only cutoff selected from the same training observations**. This prevents a sorting network from appearing superior merely because insertion was compared with a poor fixed folklore threshold.

`campaigns/tiny-kernels-v1.json` is the Tier-2 evidence contract for H12-H13. See [`docs/tiny-kernel-research.md`](docs/tiny-kernel-research.md).

## Adaptive stable merge scheduling

A stable adaptive mergesort is itself a composition of decisions. `sort_merge_policies` isolates two of them as independent experimental factors:

- **merge schedule:** adjacent pairwise rounds, repaired TimSort stack policy, Powersort;
- **run extension:** none, classic fixed TimSort-style minrun, balanced variable minrun.

All **9 combinations** share the same monotone-run detector, stable binary extension, and stable two-run merge kernel. That prevents a faster galloping merge or a different temporary-buffer strategy from being mistaken for a better merge schedule.

The dedicated workload suite reuses ordinary inputs and adds controlled run geometries such as equal runs, alternating long/short runs, Fibonacci-like lengths, power-skewed lengths, and alternating run directions.

```sh
./build/sort_merge_policies \
  --policies pairwise,timsort_stack,powersort \
  --minruns none,classic,balanced \
  --patterns random,sorted,nearly_sorted,run_long_short,run_power_skew,run_fibonacci \
  --sizes 32,64,128,315,1024,4096,16384,65536 \
  --trials 51 > merge-policies.csv

python3 tools/analyze_merge_policies.py merge-policies.csv \
  --baseline-policy powersort --baseline-minrun balanced \
  --bootstrap 5000 --output merge-policy-summary.csv
```

The instrumented path reports raw/effective run counts, comparisons, writes, weighted scheduled merge cost, pending-run depth, and run entropy. These structural metrics are **not** substituted for elapsed time.

A separate theory tool compares the scheduling policies with the exact optimal alphabetic merge cost for up to 64 runs:

```sh
python3 tools/merge_policy_model.py \
  --suite models/merge-policy-sequences-v1.json
```

This distinguishes “better merge tree” from “faster implementation.” `campaigns/merge-policies-v1.json` is the Tier-2 evidence contract for H14-H15. See [`docs/adaptive-merge-research.md`](docs/adaptive-merge-research.md).

## Experimental algorithm portfolios

The scalar benchmark records a small, deterministic input-feature probe **after all timed competitors have run**, preventing the probe from warming a later timed sort. It records feature-extraction time plus sampled disorder, duplicate density, and key-range width.

`tools/portfolio.py` uses only observable features and `n`—never the benchmark's hidden workload label—to fit per-algorithm cost models on training trials and evaluate the selector on held-out trials. Portfolio cost includes the measured feature-probe overhead.

```sh
./build/sort_lab \
  --algorithms intro,natural_merge,quick_3way,radix_lsd_11,std_sort \
  --patterns random,sorted,few_unique,nearly_sorted,runs,plateau \
  --sizes 16,32,64,128,256,512,1024,4096,16384 \
  --trials 60 > portfolio.csv

python3 tools/portfolio.py portfolio.csv \
  --algorithms intro,natural_merge,quick_3way,radix_lsd_11,std_sort \
  --output portfolio-summary.csv
```

The output compares the held-out selector against the best single training-selected algorithm and the unattainable held-out per-instance oracle. This provides a disciplined way to ask whether a portfolio can beat a fixed algorithm without leaking generator labels into runtime decisions.

Dedicated research tracks are not silently folded into the selector. A future unified portfolio that includes adaptive-merge, tiny-kernel, and external treatments will use a new compatible schema and frozen campaign after those candidates have independent evidence.

## Hardware counters

`sort_perf` uses Linux `perf_event_open` for:

- cycles;
- retired instructions;
- branch instructions and branch misses;
- cache references and cache misses.

Wall time is collected in a counter-free pass. Each hardware event is then measured in its own **fresh sort pass over the identical input**, avoiding the requirement that all six events fit simultaneously in the CPU's programmable-counter budget.

```sh
./build/sort_perf --cpu 2 \
  --algorithms intro,merge_insertion_24,dual_pivot,radix_lsd_11,std_sort \
  --patterns random,few_unique,nearly_sorted \
  --sizes 1024,16384,262144 --trials 31 > perf.csv

python3 tools/analyze_perf.py perf.csv --output perf-summary.csv
```

The harness fails closed when counters are unavailable or unusable. Unsupported/denied events are marked unavailable; zeros are never silently presented as measurements. On non-Linux systems the rest of the repository remains usable.

## Allocation behavior

`sort_alloc` is a separate allocation-only executable that tracks ordinary C++ `new`/`delete` around the sort call. It reports allocation calls, requested bytes, peak live requested bytes, largest allocation, and tracked bytes still live when measurement ends.

```sh
./build/sort_alloc \
  --algorithms heap,merge,natural_merge,radix_lsd,std_sort,std_stable_sort \
  --patterns random,nearly_sorted --sizes 1024,16384,262144 \
  --trials 11 > alloc.csv

python3 tools/analyze_alloc.py alloc.csv --output alloc-summary.csv
```

The allocation harness is intentionally separate from canonical timing because replacing the allocator changes the measured program.

## Build and validate

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
```

Optional sanitizer validation:

```sh
cmake -S . -B build-san -DSORTLAB_ENABLE_SANITIZERS=ON
cmake --build build-san -j
ctest --test-dir build-san --output-on-failure
```

The local test suite covers scalar correctness, record correctness/stability/payload integrity, insertion cutoffs, tiny-kernel correctness, hybrid leaf treatments, adaptive merge scheduling/minrun treatments, hardware-counter behavior, allocation accounting, robust statistical reducers, exact merge-policy models, crossover detection, held-out cutoff/kernel tuning, claim definitions, and held-out portfolio evaluation.

## Reproducibility bundle

`tools/run_experiment.py` wraps any benchmark executable and captures the raw CSV plus a manifest containing raw-data SHA-256, binary SHA-256, command line, Git state, host metadata, requested affinity, and available Linux/macOS control state.

```sh
python3 tools/run_experiment.py --cpu 2 --settle-ms 1000 \
  ./build/sort_lab results/canonical-001 -- \
  --trials 51 --warmups 2 --sizes 64,1024,16384,262144
```

The wrapper records host state; it does not silently change governors, turbo, thermal policy, or OS security settings.

## Statistical analysis

Scalar and record reducers report robust descriptive statistics, bootstrap uncertainty, paired speedup, paired win rate, and exact paired sign tests. `tools/crossovers.py` identifies candidate adjacent size brackets where paired speedup crosses 1.0 and distinguishes exploratory crossings from brackets whose endpoint intervals lie on opposite sides of parity.

```sh
python3 tools/analyze.py scalar.csv --baseline std_sort --bootstrap 5000 --output summary.csv
python3 tools/crossovers.py summary.csv --output crossovers.csv
```

Raw trials remain canonical. No automatic outlier deletion is performed.

## Research documentation

- [`docs/theory.md`](docs/theory.md): lower bounds, taxonomy, stability, adaptivity, memory traffic, and hardware-aware interpretation.
- [`docs/algorithm-catalog.md`](docs/algorithm-catalog.md): implemented mechanisms, variant policy, and external comparison tracks.
- [`docs/hybrid-research.md`](docs/hybrid-research.md): insertion cutoffs, hybrid hypotheses, held-out tuning, and portfolio research.
- [`docs/tiny-kernel-research.md`](docs/tiny-kernel-research.md): sorting networks, tiny-array comparisons, H12-H13, and joint leaf-kernel/cutoff tuning.
- [`docs/adaptive-merge-research.md`](docs/adaptive-merge-research.md): run detection, minrun policies, TimSort/Powersort scheduling, exact merge-cost models, H14-H15, and evidence boundaries.
- [`docs/methodology.md`](docs/methodology.md): benchmark contract and threats to validity.
- [`docs/research-protocol.md`](docs/research-protocol.md): hypotheses, experiment tiers, statistical rules, and publication gates.
- [`docs/records.md`](docs/records.md): record-width and empirical-stability contract.
- [`docs/adversarial-workloads.md`](docs/adversarial-workloads.md): workload taxonomy.
- [`docs/statistics.md`](docs/statistics.md): paired inference and crossover interpretation.
- [`docs/hardware-measurement.md`](docs/hardware-measurement.md): hardware counters, allocation semantics, affinity, and mechanism-claim gates.
- [`docs/reproducibility.md`](docs/reproducibility.md): canonical-run artifact requirements.
- [`docs/external-baselines.md`](docs/external-baselines.md): pinned external implementation provenance and comparison contract.
- [`TECHNICAL_DEBT.md`](TECHNICAL_DEBT.md): explicit known limitations, impact, and remediation paths.
- [`REFERENCES.md`](REFERENCES.md): foundational, primary, and production-source references.

## What “complete” means here

The portable core is a mature **single-threaded, in-memory sorting research laboratory**, not an assertion that every known sorter or machine model has already been exhausted. Architecture-specific SIMD sorters, parallel algorithms, GPUs, external-memory sorting, NUMA studies, variable-size strings/objects, additional production merge kernels, and more third-party state-of-the-art implementations remain separate experiment models. Their current status is explicit in [`TECHNICAL_DEBT.md`](TECHNICAL_DEBT.md).

No performance table belongs in this README until it satisfies the publication gate with raw data, a manifest, uncertainty, and replication.

## License

MIT.
