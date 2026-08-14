# Release process

A release is a validated snapshot of the permanent `sortlab` library.

## Required checks

Before tagging:

```bash
cmake --preset release
cmake --build --preset release
ctest --preset release

cmake --preset sanitize
cmake --build --preset sanitize
ctest --preset sanitize

cmake --preset package
cmake --build --preset package
ctest --preset package

cmake -P cmake/VerifyReleaseMetadata.cmake
```

The release commit must keep the CMake project version, public version header, `CITATION.cff`, and `CHANGELOG.md` synchronized.

## Scope review

Verify that:

- every installed header is declared in the target-owned public header file set;
- generic comparison APIs preserve comparator/projection contracts;
- declared stability and domain constraints are covered by deterministic tests;
- no benchmark framework, campaign code, or performance result corpus has entered the package/library tree;
- package relocation and downstream consumption pass.

## Tagging

Create an immutable semantic-version tag only after the release commit passes the complete local validation matrix. Performance evidence is versioned independently by `EddyTodd/bench` and is not a prerequisite for the algorithm library tag.
