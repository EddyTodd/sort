# Development workflow

`sortlab` uses the shared subject-repository developer workflow.

## Standard presets

```sh
cmake --preset dev
cmake --build --preset dev
ctest --preset dev
```

Optimized configuration:

```sh
cmake --preset release
cmake --build --preset release
ctest --preset release
```

Strict local sanitizer configuration:

```sh
cmake --preset sanitize
cmake --build --preset sanitize
ctest --preset sanitize
```

The sanitizer preset enables `SORTLAB_ENABLE_SANITIZERS=ON` and `SORTLAB_WARNINGS_AS_ERRORS=ON`.

## Install-package check

The `package` preset installs a Release build into `build/package-prefix`:

```sh
cmake --preset package
cmake --build --preset package
ctest --preset package
```

Because `sortlab::sortlab` is header-only, the install check is especially important: it exercises the exported INTERFACE target, permanent public/detail header lists, relocatable package/version files, and installed license without relying on the source-tree include path.

Normal non-sanitized standalone CTest graphs also run `sortlab.package-consumer`. The test installs the current build into an isolated prefix, configures and builds a completely separate project after `find_package(sortlab 1 CONFIG REQUIRED)`, and then runs that consumer's CTest suite. The downstream executable sorts a real `std::array<int, 5>` with installed `sortlab::intro_sort` and verifies the resulting order. This directly detects omitted installed headers, source-tree-only include assumptions, and runtime/template regressions in the exported header-only target. Sanitizer configurations omit the distribution smoke; the permanent in-tree correctness tests remain sanitizer-instrumented.

## Header-only development

`sortlab::sortlab` is an `INTERFACE` library, so the normal build graph primarily compiles deterministic correctness/contract tests. This is intentional: historical benchmark executables live under `research/apps/` and are disabled by default.

Enable retained research targets only for migration/parity work:

```sh
cmake --preset dev -DSORTLAB_BUILD_RESEARCH_TOOLS=ON
```

Generic empirical orchestration and new campaign infrastructure belong in `EddyTodd/bench`.

## User-local configuration

Put machine/compiler/SDK overrides in `CMakeUserPresets.json`; it is intentionally ignored by Git. Shared presets must remain portable and should not embed local external-baseline checkout paths or architecture-specific flags.
