# Development

The build is intentionally small. A top-level checkout builds the header-only library, correctness tests, and example; when embedded with `add_subdirectory`, tests, examples, and install rules default off.

## Build and test

```bash
cmake --preset dev
cmake --build --preset dev
ctest --preset dev
```

Use the `release` preset for optimized builds.

Only three project options are needed:

- `SORTLAB_BUILD_TESTS` — build correctness tests;
- `SORTLAB_BUILD_EXAMPLES` — build the usage example;
- `SORTLAB_INSTALL` — enable install and `find_package` support.

Compiler warnings, sanitizers, coverage, and other developer policies are deliberately left to the parent project or ordinary CMake/compiler flags instead of being wrapped in repository-specific CMake helpers.

## Install

```bash
cmake --preset release
cmake --build --preset release
cmake --install build/release --prefix build/install
```

The installed package exports `sortlab::sortlab` for `find_package(sortlab CONFIG REQUIRED)`.

Performance measurement does not live in this repository. This project owns the algorithms, correctness tests, optional algorithm instrumentation, and theory.
