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

## Header-only development

`sortlab::sortlab` is an `INTERFACE` library, so the normal build graph primarily compiles deterministic correctness/contract tests. This is intentional: historical benchmark executables live under `research/apps/` and are disabled by default.

Enable retained research targets only for migration/parity work:

```sh
cmake --preset dev -DSORTLAB_BUILD_RESEARCH_TOOLS=ON
```

Generic empirical orchestration and new campaign infrastructure belong in `EddyTodd/bench`.

## User-local configuration

Put machine/compiler/SDK overrides in `CMakeUserPresets.json`; it is intentionally ignored by Git. Shared presets must remain portable and should not embed local external-baseline checkout paths or architecture-specific flags.
