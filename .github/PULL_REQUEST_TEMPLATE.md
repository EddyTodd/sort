## Scope

Describe the sorting mechanism, API/metadata change, correctness change, documentation update, or research-migration work in this PR.

## Library contract

- [ ] Iterator/range/comparator/projection constraints are unchanged or explicitly documented.
- [ ] Stability, memory complexity, and integer/domain preconditions remain correct.
- [ ] Observer/instrumented semantics remain equivalent to uninstrumented algorithm behavior where promised.
- [ ] Installed `detail/` headers and algorithm metadata are changed only intentionally.
- [ ] Header-only consumers remain independent of benchmark/research types.

## Correctness

- [ ] Deterministic tests cover empty/singleton, duplicates, adversarial patterns, and changed algorithm mechanisms.
- [ ] Comparator/projection behavior is tested when generic API code changed.
- [ ] Stability is verified for algorithms that promise it.
- [ ] Adaptive changes include relevant run-structure cases.
- [ ] All installed headers remain independently includable and the external package consumer remains valid.

## Research boundary

- [ ] Generic timing/statistics/campaign/provenance/PMU/allocation/reporting machinery remains in `EddyTodd/bench`.
- [ ] Any deletion of retained research assets has executed treatment/correctness/evidence parity.
- [ ] Historical perf/allocation plumbing is not removed merely because source-level bench parity exists.

## Validation performed

List exact commands, compilers/platforms, algorithms/patterns, sanitizer configuration, and results.

## Release impact

- [ ] No release-note entry required.
- [ ] `CHANGELOG.md` updated for a release-impacting change.
- [ ] Version/citation changes, if any, follow `docs/release-process.md`.
