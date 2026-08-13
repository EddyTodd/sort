# Release process

A `sortlab` release freezes the installed generic sorting API and its correctness/complexity contracts. Empirical benchmark publication and cross-machine claims remain `bench` responsibilities.

## Version surfaces

Before tagging, synchronize:

- `project(sortlab VERSION ...)` in `CMakeLists.txt`;
- `include/sortlab/version.hpp`;
- `version` and `date-released` in `CITATION.cff`;
- the matching `CHANGELOG.md` entry.

Published release tags are immutable.

## Required validation

Run the standard standalone preset matrix from a clean checkout:

```sh
cmake --preset dev
cmake --build --preset dev
ctest --preset dev

cmake --preset release
cmake --build --preset release
ctest --preset release

cmake --preset sanitize
cmake --build --preset sanitize
ctest --preset sanitize

cmake --preset package
cmake --build --preset package
ctest --preset package
```

Because `sortlab` is header-only, every installed header in the target-owned `public_headers` file set must compile independently. The package graph must install the project and compile a separate consumer through `find_package(sortlab CONFIG REQUIRED)`.

Algorithm changes must preserve deterministic tests for ordering, comparator/projection behavior, duplicates, empty/singleton inputs, stability where promised, integer-domain restrictions, and observer/non-observer equivalence where instrumentation exists. Changes to adaptive algorithms should include adversarial run structures relevant to their mechanism.

## Public API review

Before release, review changes to:

- algorithm names and range/iterator constraints;
- stability and memory-complexity guarantees;
- comparator/projection semantics;
- observer event semantics;
- integer/domain-specific preconditions;
- installed `detail/` headers that are currently part of the package surface;
- algorithm metadata consumed by downstream tooling.

Breaking changes require a semantic-version increment and explicit changelog entry.

## Research migration gate

Never delete a retained empirical study merely because a bench-side adapter exists. Destructive cleanup requires executed and verified parity evidence for the exact treatment definitions. In particular, the historical perf/allocation study removal remains blocked until the documented `bench` campaigns and comparison checklist pass.

The permanent library must not depend on `bench` or on retained research executables.

## Tag and GitHub release

After validation and merge:

1. create annotated tag `vMAJOR.MINOR.PATCH` at the exact validated `main` commit;
2. create the GitHub Release from that tag;
3. use the matching changelog entry as the release-note basis;
4. state any compiler/platform limitations explicitly;
5. verify rendered citation metadata matches the release version/date.

Do not move a published tag. Release corrections under a new version.
