# Bench parity cleanup gate

This draft records the evidence gate for removing the now-generic pre-v1 sorting system-study harnesses. **It is intentionally non-destructive and must remain blocked until empirical parity evidence exists.**

## Planned removal after authorization

Once the gate below is satisfied, a fresh cleanup patch may remove:

- `research/apps/sort_perf.cpp`;
- `research/apps/sort_alloc.cpp`;
- `research/include/sortlab/perf_counters.hpp`;
- `research/include/sortlab/allocation_tracker.hpp`;
- `tools/analyze_perf.py`;
- `tools/analyze_alloc.py`;
- the associated CMake research targets/self-tests and the `sort_alloc` sanitizer exception.

These responsibilities are represented in `EddyTodd/bench` v0.4 through `bench-sort-legacy`, `sort-legacy-perf.toml`, and `sort-legacy-allocation.toml`.

The retained `research/include/sortlab/{algorithms,extended_algorithms,workloads,...}` files are **not** generic infrastructure and are not authorized for removal by this gate. The temporary bench parity adapter needs them to preserve historical implementation/workload identity, and other specialized subject experiments still depend on them.

## Required before destructive cleanup

- [ ] Execute `campaigns/subjects/sort-legacy-perf.toml` from `bench` commit `b9eb75935491d9b2ef97dbb8ca952b854286ff23` or a documented descendant that preserves the same treatment definitions. If historical PMU evidence used CPU pinning, apply the corresponding `cpu_affinity` treatment.
- [ ] Execute `campaigns/subjects/sort-legacy-allocation.toml`.
- [ ] Verify both evidence bundles with `benchctl verify`.
- [ ] Compare treatment identity fields (`algorithm`, pattern, `n`, trial, trial seed, input hash) against retained pre-v1 evidence.
- [ ] Compare allocation observations and old `analyze_alloc.py` conclusions against the bench metric analysis.
- [ ] Compare perf/timing observations and old `analyze_perf.py` conclusions where host/counter comparability permits it.
- [ ] Record the bench commit, sort commit, host/provenance, result roots, and comparison outcome below.
- [ ] Only after all checks pass, generate a fresh destructive cleanup branch from then-current `sort/main` and review that exact deletion diff.

## Evidence record

Pending. Replace this section with concrete result bundle identifiers, host provenance, source revisions, and the comparison decision before destructive cleanup is authorized.

## Why the gate is non-destructive

Library/package hygiene continues to evolve independently of historical benchmark migration. Keeping the evidence gate as documentation rather than a long-lived deletion patch avoids repeated rebases that could accidentally drop unrelated correctness, package, or header-hygiene improvements. The destructive patch should be short-lived and generated only after the evidence decision is complete.
