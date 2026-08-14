# Contributing

`sortlab` v1 is first a reusable C++23 algorithm library. The retained empirical layer is compatibility/evidence material while exact treatments move through `EddyTodd/bench` acceptance.

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

## Retained research compatibility layer

`research/apps/`, `research/include/`, historical campaign/analysis assets, and external-baseline reconstruction tooling remain only for continuity and evidence reproduction. Avoid expanding generic experimental infrastructure here. Generic timing, PMU/allocation collection, orchestration, statistics, reporting, provenance, historical import, and migration acceptance belong in `EddyTodd/bench`.

Bench v0.5 has exact definition/source parity for the 11 retained default empirical executables, but that does not make them deletion-safe. Before removing an empirical source, consult `docs/research-migration-status.md` and require the corresponding bench migration gate to pass with real replacement evidence.

Algorithm correctness/theory/reconstruction contracts do not automatically migrate with empirical executables. For example, the adaptive-record kernel contract and pinned external-source provenance remain subject-owned while they serve those roles.

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

Retained research validation is explicit:

```sh
cmake --preset retained-research
cmake --build --preset retained-research
ctest --preset retained-research
```

The canonical switch is `-DSORTLAB_BUILD_RETAINED_RESEARCH=ON`; `SORTLAB_BUILD_RESEARCH_TOOLS=ON` is only a deprecated compatibility alias.

Before changing install/export logic, also test installation into a temporary prefix and consumption from a separate `find_package(sortlab CONFIG REQUIRED)` project.
