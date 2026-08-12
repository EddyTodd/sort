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

Every record retains its original ordinal and deterministic payload, allowing the harness to verify nondecreasing key order, exact record/payload preservation, empirical stability on equal keys, and stability guarantees for algorithms that claim them.

Project-controlled algorithms also expose explicit record moves and `explicit_bytes_moved`. These are algorithmic observables, not estimates of cache or DRAM traffic.

## Hybrid and cutoff research

A central research question is whether a practical sorter should combine mechanisms rather than use one algorithm everywhere.

`sort_cutoffs` treats the insertion-sort base-case threshold as an experimental variable for merge sort + insertion leaves, median-of-three quicksort + insertion leaves, and introsort + insertion leaves.

```sh
./build/sort_cutoffs \
  --cutoffs 1,4,8,12,16,20,24,32,48,64,96,128 \
  --sizes 8,12,16,24,32,48,64,96,128,192,256,512,1024,2048,4096,8192 \
  --patterns random,sorted,few_unique,nearly_sorted,runs \
  --trials 51 > cutoffs.csv
python3 tools/tune_cutoffs.py cutoffs.csv --output cutoff-summary.csv
```

The tuning tool selects a cutoff on training trials and evaluates it on a deterministic held-out split. The repository does **not** assume one folklore threshold is universally optimal.

`tools/claim_matrix.py` evaluates preregistered paired claims including insertion-vs-merge crossover, linear-vs-binary insertion, hybrid-vs-pure merge/quicksort, two-way-vs-three-way quicksort, radix digit width, and run-adaptive-vs-fixed merge. See [`docs/hybrid-research.md`](docs/hybrid-research.md).

## Experimental algorithm portfolios

The scalar benchmark records a small, deterministic input-feature probe **after all timed competitors have run**, preventing the probe from warming a later timed sort. It records feature-extraction time plus sampled disorder, duplicate density, and key-range width.

`tools/portfolio.py` uses only observable features and `n`—never the benchmark's hidden workload label—to fit per-algorithm cost models on training trials and evaluate the selector on held-out trials. Portfolio cost includes the measured feature-probe overhead.

```sh
./build/sort_lab \
  --algorithms intro,natural_merge,quick_3way,radix_lsd_11,std_sort \
  --patterns random,sorted,few_unique,nearly_sorted,runs,plateau \
  --sizes 16,32,64,128,256,512,1024,4096,16384 --trials 60 > portfolio.csv
python3 tools/portfolio.py portfolio.csv \
  --algorithms intro,natural_merge,quick_3way,radix_lsd_11,std_sort \
  --output portfolio-summary.csv
```

The output compares the held-out selector against the best single training-selected algorithm and the unattainable held-out per-instance oracle.

## Hardware counters

`sort_perf` wraps the sorting call with Linux `perf_event_open` counters for cycles, retired instructions, branch instructions/misses, and cache references/misses.

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
  --patterns random,nearly_sorted --sizes 1024,16384,262144 --trials 11 > alloc.csv
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

The local test suite covers scalar correctness, record correctness/stability/payload integrity, hybrid cutoffs, hardware-counter behavior, allocation accounting, robust statistical reducers, crossover detection, held-out cutoff tuning, claim definitions, and held-out portfolio evaluation.

## Reproducibility bundle

`tools/run_experiment.py` wraps any benchmark executable and captures raw CSV plus a manifest containing raw-data SHA-256, binary SHA-256, command line, Git state, host metadata, requested affinity, and available Linux/macOS control state.

```sh
python3 tools/run_experiment.py --cpu 2 --settle-ms 1000 \
  ./build/sort_lab results/canonical-001 -- \
  --trials 51 --warmups 2 --sizes 64,1024,16384,262144
```

The wrapper records host state; it does not silently change governors, turbo, thermal policy, or OS security settings.

## Statistical analysis

Scalar and record reducers report robust descriptive statistics, bootstrap uncertainty, paired speedup, paired win rate, and exact paired sign tests. `tools/crossovers.py` identifies candidate adjacent size brackets where paired speedup crosses 1.0 and distinguishes exploratory crossings from brackets whose endpoint intervals lie on opposite sides of parity.

Raw trials remain canonical. No automatic outlier deletion is performed.

## Research documentation

- [`docs/theory.md`](docs/theory.md): lower bounds, taxonomy, stability, adaptivity, memory traffic, and hardware-aware interpretation.
- [`docs/algorithm-catalog.md`](docs/algorithm-catalog.md): implemented mechanisms, variant policy, and external comparison tracks.
- [`docs/hybrid-research.md`](docs/hybrid-research.md): insertion cutoffs, hybrid hypotheses, held-out tuning, and portfolio research.
- [`docs/methodology.md`](docs/methodology.md): benchmark contract and threats to validity.
- [`docs/research-protocol.md`](docs/research-protocol.md): hypotheses, experiment tiers, statistical rules, and publication gates.
- [`docs/records.md`](docs/records.md): record-width and empirical-stability contract.
- [`docs/adversarial-workloads.md`](docs/adversarial-workloads.md): workload taxonomy.
- [`docs/statistics.md`](docs/statistics.md): paired inference and crossover interpretation.
- [`docs/hardware-measurement.md`](docs/hardware-measurement.md): hardware counters, allocation semantics, affinity, and mechanism-claim gates.
- [`docs/reproducibility.md`](docs/reproducibility.md): canonical-run artifact requirements.
- [`REFERENCES.md`](REFERENCES.md): foundational, primary, and production-source references.

## What “complete” means here

The core repository is a complete **single-threaded, in-memory sorting research laboratory**, not an assertion that every known sorter should be reimplemented locally. Architecture-specific SIMD sorters, parallel algorithms, GPUs, external-memory sorting, NUMA studies, variable-size strings/objects, and third-party state-of-the-art implementations are separate experiment models. They should enter through versioned/provenance-preserving comparison tracks rather than being mixed into the portable core without controlling their different contracts.

No performance table belongs in this README until it satisfies the publication gate with raw data, a manifest, uncertainty, and replication.

## License

MIT.
