# Contributing

## Algorithm contributions

A new sorting implementation must:

1. define its domain (key type and any preconditions);
2. add accurate family/stability/in-place/adaptivity/complexity metadata;
3. provide counter-disabled and counter-enabled instantiations so timing remains uncontaminated;
4. pass `--self-test` across all workload families and edge cases;
5. avoid undefined behavior and platform-dependent experiment identity;
6. document external algorithmic sources or inspiration without copying incompatible code;
7. state which counters are observable and which are not.

Optimizations that change semantics, pivot selection, gap sequence, cutoffs, memory strategy, or worst-case behavior should be treated as distinct algorithm variants when that distinction matters scientifically.

## Benchmark contributions

Do not commit a claimed performance conclusion without raw data and reproducibility metadata. At minimum provide:

- command line and experiment seed;
- source commit and clean/dirty status;
- compiler/build configuration;
- raw CSV plus SHA-256;
- sample count and uncertainty treatment;
- scoped interpretation that names workload, size, and environment.

Do not silently discard inconvenient measurements.

## Local validation

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
```

Sanitizer builds are encouraged when supported by the local toolchain. Hosted GitHub Actions are currently intentionally absent; local validation is the required gate.
