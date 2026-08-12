# sort

A C++23 research laboratory for **theoretical and empirical study of sorting algorithms**. The project is designed to answer a more useful question than “what is the Big-O?”: which algorithm wins, by how much, under which input structure, element size, stability requirement, machine, compiler, and cost model—and how confident are we in that conclusion?

The repository treats raw measurements as research artifacts. It deliberately keeps correctness, abstract operation counts, elapsed time, data movement, statistical inference, and theoretical guarantees separate rather than collapsing them into a single leaderboard.

## Research surface

### Scalar experiment

`sort_lab` benchmarks 12 scalar implementations/families on signed 64-bit integers:

- insertion, selection, bubble, and Ciura-gap Shell sort;
- heapsort and stable mergesort;
- two-way Hoare quicksort and duplicate-aware three-way quicksort;
- custom introsort with heap fallback and insertion cutoff;
- stable byte-wise LSD radix sort;
- `std::sort` and `std::stable_sort` as production-library baselines.

The scalar harness now exposes 15 deterministic workload families: random, sorted, reversed, few-unique, binary, all-equal, nearly-sorted, organ-pipe, sawtooth, ascending runs, descending runs, rotated sorted data, alternating extremes, staggered periodic keys, and plateau data.

### Record experiment

`sort_records` studies a different problem: **sorting key/payload records when moving an element is materially more expensive than comparing its key**.

Nine representative algorithms are instantiated over records containing a signed 64-bit key, original ordinal, and payload. Supported payload sizes are 0, 1, 3, 7, 15, and 31 64-bit words; the executable reports the actual `sizeof(record)` for each build.

Every output record carries its original ordinal. This lets the harness empirically verify:

- sorted key order;
- exact record/payload preservation;
- whether equal-key records retained their original relative order;
- stability guarantees for algorithms that claim them.

The untimed instrumentation pass also reports explicit whole-record moves and `explicit_bytes_moved = moves × sizeof(record)` for project-controlled implementations. These are **algorithmic data-movement observables**, not hardware memory-traffic counters. Library-internal moves are left unreported rather than fabricated.

## Experimental design

Both executables use the same core discipline:

- counter-free timed implementations and separate instrumented passes;
- independent deterministic input per trial;
- identical input for every algorithm within a paired trial;
- deterministic randomized execution order;
- portable seed derivation and input fingerprints;
- correctness verification before any row is accepted;
- raw CSV output rather than in-benchmark ranking.

The record experiment additionally keeps the same key sequence across payload widths for a given `(pattern, n, trial)`, enabling controlled element-width studies.

## Build and validate

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
```

No hosted GitHub Actions are required. The local CTest suite covers scalar correctness, record correctness/stability/payload integrity, smoke experiments, scalar analysis, record analysis, and crossover analysis.

## Scalar benchmark

```sh
./build/sort_lab --trials 31 --warmups 2 \
  --sizes 64,1024,16384,262144 > scalar.csv
```

Target difficult structures explicitly:

```sh
./build/sort_lab \
  --algorithms quick_hoare,quick_3way,intro,std_sort \
  --patterns binary,organ_pipe,rotated,alternating_extremes,plateau \
  --sizes 128,1024,8192,65536 --trials 51 > adversarial.csv
```

## Record-width and stability benchmark

Inspect compiled widths and algorithms:

```sh
./build/sort_records --list-payloads
./build/sort_records --list-algorithms
```

Run a payload-width campaign:

```sh
./build/sort_records \
  --payload-words 0,1,3,7,15,31 \
  --algorithms heap,merge,quick_3way,intro,radix_lsd,std_sort,std_stable_sort \
  --patterns random,few_unique,all_equal,nearly_sorted,plateau \
  --sizes 64,1024,16384,262144 --trials 31 > records.csv
```

`tools/run_experiment.py` can wrap either executable to capture raw CSV, SHA-256, command line, compiler/binary metadata, host metadata, Git commit, and working-tree state.

## Statistical analysis

Scalar results:

```sh
python3 tools/analyze.py scalar.csv --baseline std_sort \
  --bootstrap 5000 --output scalar-summary.csv
```

Record results:

```sh
python3 tools/analyze_records.py records.csv --baseline std_sort \
  --bootstrap 5000 --output record-summary.csv
```

The reducers report robust descriptive statistics and paired baseline comparisons. Milestone 3 adds **paired win rate and an exact two-sided paired sign test** alongside paired median speedup and bootstrap intervals. Statistical detectability is not treated as practical importance.

Candidate crossover brackets can be extracted from either summary schema:

```sh
python3 tools/crossovers.py scalar-summary.csv --output scalar-crossovers.csv
python3 tools/crossovers.py record-summary.csv --output record-crossovers.csv
```

The tool finds adjacent measured sizes where median paired speedup crosses 1.0, estimates the crossing on a log-size/log-speedup scale, and labels the bracket `decisive` only when the adjacent confidence intervals lie on opposite sides of 1.0. Crossover estimates remain exploratory until the bracket is sampled more densely.

## Research documentation

- [`docs/theory.md`](docs/theory.md): taxonomy, complexity model, lower bounds, stability, adaptivity, memory traffic, and hardware-aware interpretation.
- [`docs/methodology.md`](docs/methodology.md): timing, pairing, instrumentation, record experiments, and threats to validity.
- [`docs/research-protocol.md`](docs/research-protocol.md): hypotheses, experiment tiers, statistical rules, crossover policy, and publication gates.
- [`docs/records.md`](docs/records.md): record-width experiment, empirical stability contract, and data-movement semantics.
- [`docs/adversarial-workloads.md`](docs/adversarial-workloads.md): controlled workload taxonomy and what each pattern probes.
- [`docs/statistics.md`](docs/statistics.md): paired inference, sign tests, win rates, crossover detection, and multiplicity cautions.
- [`docs/reproducibility.md`](docs/reproducibility.md): canonical-run requirements and artifact layout.
- [`REFERENCES.md`](REFERENCES.md): primary and foundational literature.

## Scope and limitations

The repository now studies both scalar integer sorting and fixed-size key/payload records, but it still does **not** claim to characterize strings, variable-size objects, indirect/index sorting, external-memory sorting, NUMA, GPUs, parallel sorting, SIMD specialist implementations, allocator behavior, hardware cache/branch counters, or energy.

`explicit_bytes_moved` is not DRAM traffic, cache traffic, or a measured bandwidth quantity. Hardware-counter and allocation/peak-memory work remains a separate milestone because those measurements are platform-specific and should not be disguised as portable metrics.

No benchmark table belongs in this README until it satisfies the publication gate and is backed by raw reproducibility artifacts.

## License

MIT.
