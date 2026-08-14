# Contributing

`sortlab` is an algorithm library. Changes should improve a permanent sorting mechanism, its generic API, correctness coverage, or algorithmic documentation.

## Scope

Keep generic benchmarking, campaign orchestration, performance statistics, hardware counters, result corpora, and external performance comparisons in `EddyTodd/bench`.

Do not add benchmark state to normal algorithm signatures or introduce research-only algorithm copies when the public implementation can expose the required mechanism through a clean API.

## API expectations

Comparison algorithms should use random-access iterators/ranges and support comparators/projections where practical. Preserve documented stability, iterator/value requirements, and move-only support. Distribution algorithms may impose explicit integral/domain constraints.

Instrumentation must remain optional and use the same underlying implementation as the ordinary API.

## Correctness

Algorithm changes should add deterministic regression coverage for relevant edge cases and invariants. Stable algorithms require duplicate-key stability checks. New public headers must compile independently. Architecture- or domain-specific behavior must fail explicitly rather than silently substitute a different algorithm.

## Validation

Before submitting:

```bash
cmake --preset dev
cmake --build --preset dev
ctest --preset dev

cmake --preset sanitize
cmake --build --preset sanitize
ctest --preset sanitize

cmake -P cmake/VerifyReleaseMetadata.cmake
```

For package changes also run the package preset/external consumer test.

## Style

Use C++23, the repository `.clang-format`, target-local warning policy, and existing naming/layout conventions. Prefer small public surfaces and explicit contracts over compatibility layers for temporary development tooling.
