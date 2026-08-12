# Provenance-pinned external baselines

A research repository should compare against strong external implementations, but copying source into the core or building a floating upstream branch makes results hard to reproduce. The external track therefore separates **provenance**, **materialization**, **adaptation**, and **measurement**.

## Baseline v1

### Pattern-defeating quicksort

`orlp/pdqsort` is pinned to `b1ef26a55cdb60d236a5cb199c4234c704f46726`. The upstream project describes pdqsort as a deterministic quicksort/heapsort hybrid with insertion-sort handling of small partitions and pattern-detection mechanisms; it also exposes an explicitly branchless entry point. The repository is distributed under the zlib license.

The track measures both `pdqsort` and `pdqsort_branchless` because branchless partitioning is a scientifically material implementation choice, not an alias.

### IPS4o

`ips4o/ips4o` is pinned to `08a5b926ee65cef19139057c6bde02bb5542c1cb`, under BSD-2-Clause. Baseline v1 invokes only sequential `ips4o::sort`. Its parallel interface is intentionally not compared against single-threaded algorithms because changing thread count changes the experiment model.

## Preregistered external-track hypotheses

**E1 — engineered quicksort behavior.** Pattern-defeating quicksort and its branchless variant should change the quicksort performance frontier on at least some structured or branch-sensitive domains, but neither is assumed to dominate every workload. Any explanation based on branch prediction requires direct counter evidence rather than timing alone.

**E2 — sequential sample-sort crossover.** Sequential IPS4o may become competitive or superior only beyond a workload- and machine-dependent size region. The project will estimate crossover regions rather than report one universal threshold.

Both hypotheses begin as untested. The `external-v1` campaign is the first Tier-2 evidence contract for them.

## Measurement contract

`sort_external` uses the same `Value`, workload generators, trial-seed derivation, input fingerprint, randomized execution order, and correctness verifier as the core scalar harness. External implementations and internal controls therefore see the same input within each paired trial.

External/library internal comparisons, swaps, writes, or allocations are not inferred. This track's primary portable outcome is wall time; hardware/allocation mechanism claims require corresponding dedicated measurements.

Every raw row identifies algorithm origin, pinned upstream commit, and declared license. Canonical campaign artifacts should also retain `external/provenance.json`, whose license hashes and Git tree hashes prove which checkout supplied the benchmark build.

## Updating a baseline

An upstream update is a new experimental treatment. Change the pinned commit intentionally, review upstream license/interface changes, regenerate provenance, and use a new campaign namespace when comparing old and new versions. Never update a commit merely because upstream `master` moved.
