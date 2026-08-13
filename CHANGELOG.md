# Changelog

All notable changes to `sortlab` are recorded here. The installed header-only C++ library follows semantic versioning. Historical empirical studies remain outside the permanent package surface until their individual `bench` parity gates are satisfied.

## [Unreleased]

### Added

- Standard `dev`, `release`, `sanitize`, and checkout-local `package` CMake presets.
- External installed-package consumer smoke testing.
- Target-owned 18-header public API file set and build-time independent compilation of every installed header.

### Changed

- Installed package/license ownership is explicit and the header-only export is validated from a separate consumer project.
- The planned removal of historical perf/allocation study plumbing is governed by a non-destructive empirical parity gate rather than a long-lived deletion branch.

## [1.0.0] - 2026-08-12

### Added

- Stable C++23 header-only generic sorting library over random-access iterators/ranges with comparator/projection customization where applicable.
- Comparison algorithms including elementary sorts, Shell variants, heap/merge/quick families, introsort, dual-pivot quicksort, TimSort, PowerSort, stable in-place merge, and related adaptive mechanisms.
- Integer/domain-specific counting and radix LSD/MSD algorithms plus tiny/network mechanisms.
- Observer-based instrumentation separated from ordinary uninstrumented algorithm execution.
- Reusable algorithm metadata and deterministic correctness/conformance tests.
- Installable `sortlab::sortlab` CMake package and public version metadata.

### Architecture

- Permanent algorithms, observer hooks, algorithm metadata, correctness tests, and sorting theory remain owned by `sort`.
- Generic benchmark orchestration, statistical analysis, provenance, PMU/allocation collection, reporting, and cross-machine evidence migrate to `EddyTodd/bench` only after study-specific parity gates pass.
