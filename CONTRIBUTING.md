# Contributing

## Algorithm contributions

A new sorting implementation must:

1. define its domain, key type, record compatibility, and preconditions;
2. add accurate family/stability/in-place/adaptivity/complexity metadata;
3. provide counter-disabled and counter-enabled instantiations so timing remains uncontaminated;
4. pass deterministic tests across relevant workload families and edge cases;
5. avoid undefined behavior and platform-dependent experiment identity;
6. document external algorithmic sources or inspiration without copying incompatible code;
7. state which counters are observable and which are not.

Optimizations that change semantics, pivot selection, gap sequence, cutoffs, memory strategy, or worst-case behavior should be treated as distinct algorithm variants when that distinction matters scientifically.

Record-compatible algorithms must preserve key/payload association exactly. Algorithms claiming stability must pass ordinal-based stability assertions on duplicate-heavy inputs and representative record widths.

## Workload contributions

A workload generator must document the property it probes. Do not label an input as an algorithmic “killer” or worst case without an argument tied to the exact implementation under test. Workload definitions must be deterministic under the project seed contract.

## Benchmark contributions

Do not commit a claimed performance conclusion without raw data and reproducibility metadata. At minimum provide:

- command line and experiment seed;
- source commit and clean/dirty status;
- compiler/build configuration;
- raw CSV plus SHA-256;
- sample count and uncertainty treatment;
- scoped interpretation naming workload, size, element/record type, and environment.

Do not silently discard inconvenient measurements. Do not infer hardware mechanisms from wall-clock timing alone.

## Local validation

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
```

Sanitizer builds are encouraged when supported by the local toolchain. Hosted GitHub Actions are currently intentionally absent; local validation is the required gate.
