# Development workflow

The repository has one build surface: the permanent header-only library and its deterministic correctness/package tests.

## Debug

```bash
cmake --preset dev
cmake --build --preset dev
ctest --preset dev
```

## Release

```bash
cmake --preset release
cmake --build --preset release
ctest --preset release
```

## Sanitizers

```bash
cmake --preset sanitize
cmake --build --preset sanitize
ctest --preset sanitize
```

The sanitizer preset enables ASan/UBSan and warnings-as-errors on supported non-MSVC compilers.

## Package validation

```bash
cmake --preset package
cmake --build --preset package
ctest --preset package
```

The package test installs the header-only target, relocates the install tree, and consumes it from a separate CMake project. Public-header self-containment is compiled from the target-owned header file set.

Machine/compiler overrides belong in ignored `CMakeUserPresets.json`.

Performance experiments and empirical tooling belong in `EddyTodd/bench`, not in this repository.
