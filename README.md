# sort

A C++23 research laboratory for **theoretical and empirical study of sorting algorithms**. The repository is built to answer a more useful question than “what is the Big-O?”: which algorithm wins, by how much, under which data distribution, size, machine, compiler, and cost model—and how confident are we in that conclusion?

The code intentionally keeps pedagogical algorithms, engineered comparison sorts, a distribution sort, and production-library baselines under one reproducible harness. Raw measurements are first-class research artifacts; headline rankings without workload and environment context are not.

## Current research surface

Milestone 2 contains 12 implementations/families:

- insertion, selection, bubble, and Ciura-gap Shell sort;
- heapsort and stable mergesort;
- two-way Hoare quicksort and duplicate-aware three-way quicksort;
- a custom introsort with depth-limited heap fallback and insertion cutoff;
- stable byte-wise LSD radix sort for signed 64-bit integers;
- `std::sort` and `std::stable_sort` as library baselines.

Nine deterministic workload families cover random, sorted, reversed, few-unique, all-equal, nearly-sorted, organ-pipe, sawtooth, and pre-sorted-run structure.

Run `sort_lab --list-algorithms` for machine-readable algorithm metadata including family, stability, in-place status, adaptivity, asymptotic bounds, and auxiliary-space class.

## What changed in the research design

The harness separates **elapsed-time measurement from operation instrumentation**. Timed implementations compile with counters disabled; a second untimed pass measures comparisons, explicit swaps, and explicit writes. This prevents the counter bookkeeping from becoming part of the performance result.

Each trial receives a distinct deterministic dataset. Algorithm order is independently shuffled for each trial to reduce order bias. Dataset seeds and fingerprints use project-defined deterministic functions rather than implementation-defined `std::hash`, making experiment identity portable across standard-library implementations. Every timed and instrumented result is independently verified before emission.

## Build and validate

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
```

The implementation is split into a small research harness plus header-only algorithm/workload modules so algorithms, experiment design, and CLI concerns can evolve independently without obscuring the benchmark contract.

## Run an experiment

```sh
./build/sort_lab --trials 31 --warmups 2 --sizes 64,1024,16384,262144 > raw.csv
```

Target a subset:

```sh
./build/sort_lab \
  --algorithms intro,radix_lsd,std_sort \
  --patterns random,few_unique,nearly_sorted \
  --sizes 1024,16384,262144 \
  --trials 51 > raw.csv
```

Quadratic algorithms are skipped above 16,384 elements by default; change this with `--quadratic-limit N`.

For a reproducibility bundle:

```sh
python3 tools/run_experiment.py ./build/sort_lab results/run-001 -- \
  --trials 31 --warmups 2 --sizes 64,1024,16384,262144
```

This writes `raw.csv` plus `manifest.json` containing the SHA-256 of the raw data, host metadata, binary build metadata, command line, Git commit, and working-tree state.

## Statistical analysis

```sh
python3 tools/analyze.py results/run-001/raw.csv --baseline std_sort \
  --bootstrap 5000 --output results/run-001/summary.csv
```

The reducer reports median, quartiles, MAD, bootstrap 95% confidence intervals for the median, and **paired** speedup estimates against the chosen baseline. Pairing uses `(pattern, n, trial, input_hash)`, so algorithms are compared on the same generated input rather than unrelated samples.

The analysis intentionally preserves raw observations. It does not silently remove outliers or present a universal winner.

## CSV schema

```text
schema_version,algorithm,pattern,n,trial,experiment_seed,trial_seed,input_hash,execution_order,ns,comparisons,swaps,writes,verified
```

`comparisons`, `swaps`, and `writes` describe the untimed instrumentation pass. For library algorithms, only comparisons are externally observable; inaccessible library writes/swaps remain zero rather than being fabricated. Radix sort has zero comparison count by construction.

## Research documentation

- [`docs/theory.md`](docs/theory.md): taxonomy, complexity model, lower bounds, stability, adaptivity, memory traffic, and hardware-aware interpretation.
- [`docs/methodology.md`](docs/methodology.md): benchmark contract and bias controls.
- [`docs/research-protocol.md`](docs/research-protocol.md): hypotheses, experiment matrix, statistical rules, and publication gates.
- [`docs/reproducibility.md`](docs/reproducibility.md): canonical-run requirements and artifact layout.
- [`REFERENCES.md`](REFERENCES.md): primary and foundational literature used to guide the project.
- [`results/README.md`](results/README.md): policy for benchmark datasets; the repository does not ship invented results.

## Scope and limitations

This milestone is a rigorous **single-threaded, in-memory, signed-64-bit** benchmark. It does not yet claim to characterize records with expensive moves, indirect sorting, strings, external-memory sorting, NUMA effects, GPUs, SIMD/vectorized specialist implementations, parallel algorithms, cache/branch hardware counters, allocator behavior, or energy consumption. Those are explicit research dimensions, not hidden omissions.

Library implementations are treated as versioned black-box baselines. Their precise strategy can vary by standard library and release, so published results must include compiler/library metadata and must not generalize one implementation to all C++ environments.

## Contribution standard

New algorithms must add truthful theoretical metadata, pass all deterministic workload tests, preserve the timing/instrumentation separation, and document any domain constraints. New benchmark claims require raw data and a reproducibility manifest. See [`CONTRIBUTING.md`](CONTRIBUTING.md).

## License

MIT.
