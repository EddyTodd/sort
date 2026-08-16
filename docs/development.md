# Development

A top-level checkout builds the header-only library, correctness tests, and the small usage example. When `sortlab` is added as a subdirectory of another project, tests, examples, and install rules default off so the dependency does not alter its parent's build surface.

## Normal workflow

```bash
cmake --preset dev
cmake --build --preset dev
ctest --preset dev
```

Use `release` for optimized builds and `sanitize` for ASan/UBSan with warnings-as-errors.

The key options are:

- `SORTLAB_BUILD_TESTS` — deterministic correctness tests;
- `SORTLAB_BUILD_EXAMPLES` — small standalone usage examples;
- `SORTLAB_INSTALL` — install/export/package rules (top-level by default);
- `SORTLAB_ENABLE_SANITIZERS` — ASan/UBSan on supported non-MSVC compilers;
- `SORTLAB_WARNINGS_AS_ERRORS` — promote compiler warnings to errors.

## Installed package validation

Package validation is intentionally separate from the normal test loop:

```bash
cmake --preset package
cmake --build --preset package
ctest --preset package
```

That preset enables `SORTLAB_BUILD_PACKAGE_TESTS`, installs the library into an isolated local prefix during the package-consumer test, relocates it, and verifies a separate `find_package(sortlab CONFIG REQUIRED)` consumer. Ordinary `dev` and `release` tests do not pay that cost.

Machine/compiler overrides belong in ignored `CMakeUserPresets.json`.

Performance experiments belong in `EddyTodd/bench`; algorithm implementations, correctness, instrumentation, and theory remain here.
