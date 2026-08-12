# Contributing

This repository is a research artifact. New code must preserve both software correctness and experimental validity.

## Algorithm contributions

A new sorting implementation must:

1. define its domain, key type, and preconditions;
2. provide truthful family/stability/in-place/adaptivity/complexity metadata;
3. provide counter-disabled and counter-enabled forms when operation instrumentation applies;
4. pass deterministic edge cases and all applicable workload families;
5. avoid undefined behavior and platform-dependent experiment identity;
6. cite algorithmic inspiration and preserve external licensing/provenance;
7. state which operation/allocation/hardware counters are observable and which are not.

Changes to pivot selection, gap sequence, radix width, run policy, base-case cutoff, stability, memory strategy, or worst-case behavior are scientifically material variants and should be named or parameterized explicitly.

## Hybrid and tuning contributions

A tuned parameter is not evidence unless selection and evaluation are separated. New tuning/selection work must define the parameter search space before the canonical run, use disjoint training/evaluation observations, include selection/probe overhead in adaptive policies, avoid hidden generator labels, report untuned/best-single baselines and useful oracle ceilings, and avoid hard-coding one machine's winning value as a universal constant.

## Benchmark contributions

Do not commit a performance conclusion without raw evidence and reproducibility metadata. At minimum provide exact command/seed, source commit and clean/dirty state, compiler/build configuration, benchmark binary SHA-256, raw CSV plus SHA-256, sample count/uncertainty treatment, and scoped interpretation naming workload, size/type width, and environment.

Do not silently discard inconvenient measurements.

## Mechanism claims

Wall time alone does not establish why an algorithm won. Claims about branches/caches require hardware-counter evidence; allocation claims require allocation evidence; memory-traffic claims require a suitable traffic/bandwidth measurement. Unavailable measurements remain unavailable rather than being encoded as zero.

## External algorithm baselines

Architecture-specific or third-party state-of-the-art algorithms should normally enter through a versioned adapter/comparison track. Pin the upstream commit/version, retain its license, document build flags/domain restrictions, and do not copy code into the core merely to increase algorithm count.

## Local validation

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
```

Sanitizer validation is available with `-DSORTLAB_ENABLE_SANITIZERS=ON` when the local compiler supports ASan/UBSan.
