# Contributing

`sortlab` v1 is first a reusable C++23 algorithm library. The retained benchmark/research layer is temporary and will move to `EddyTodd/bench`.

## Core algorithm contributions

A permanent algorithm contribution must:

1. use the public generic iterator/range model when the mechanism is comparison based;
2. expose comparator/projection customization where it is meaningful;
3. keep benchmark state out of the normal function signature;
4. route optional instrumentation through the observer layer rather than duplicating the algorithm;
5. state truthful stability, storage, domain, adaptivity, and asymptotic metadata;
6. document any stronger type requirement such as copy-constructible pivot values;
7. pass edge cases, randomized duplicate-heavy cases, custom comparator/projection tests, and applicable move-only/stability/domain-boundary tests;
8. avoid required third-party dependencies in the portable core unless the design benefit clearly outweighs the cost.

New variants should represent a materially different mechanism or contract, not merely increase the catalog size.

## Correctness before performance

A core PR is accepted on algorithm/API correctness, genericity, packaging, and maintainability. Benchmark wins are not required and should not be used to weaken a correctness contract.

Stable algorithms must preserve comparator-equivalent order. Adaptive merge changes must cover strict descending-run reversal, equal keys, tiny and unbalanced runs, both forward and backward merge directions, and any galloping state they introduce.

Distribution algorithms must test signed/unsigned boundaries and reject invalid domain/configuration parameters explicitly.

## Instrumentation

`sortlab::instrumented` functions use the same implementation as their normal counterpart. Observer events are algorithmic events only. Do not add cache, branch, energy, timing, affinity, or machine-provenance logic to the core library; those belong in `bench`.

## Research compatibility layer

The current `src/sort_*`, workload generators, campaigns, analysis scripts, counters, and evidence machinery are retained for continuity. Avoid expanding that framework here. Generic experimental infrastructure should be implemented in `EddyTodd/bench`; sorting-specific experiments there should consume the installed `sortlab::sortlab` target.

External algorithms such as pdqsort and IPS4o remain opt-in comparison dependencies and must not become required core dependencies merely to increase coverage.

## Local validation

Core-only validation is the default:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
```

Sanitizers:

```sh
cmake -S . -B build-san \
  -DSORTLAB_ENABLE_SANITIZERS=ON \
  -DCMAKE_BUILD_TYPE=Debug
cmake --build build-san -j
ctest --test-dir build-san --output-on-failure
```

The retained research targets are opt-in with `-DSORTLAB_BUILD_RESEARCH_TOOLS=ON`.

Before changing install/export logic, also test installation into a temporary prefix and consumption from a separate `find_package(sortlab CONFIG REQUIRED)` project.
