# Technical debt and future domains

This document separates **core v1 library blockers** from empirical/research work that is intentionally moving to `EddyTodd/bench` or belongs to a different execution domain.

## Core v1 status

**Known core correctness/API/architecture blockers: none.**

The permanent library is generic, instrumentable without benchmark state, installable as `sortlab::sortlab`, and has representative coverage of the major sequential in-memory sorting mechanisms in v1 scope.

## Benchmark/research migration debt

Bench v0.5.0 commit `acd9a77f9aa0fbb6edd569eb24c78b4694b442ed` has exact definition/source parity for all 11 retained default empirical executables. The remaining debt is therefore primarily **evidence acceptance and cleanup**, not inventing more generic benchmark machinery inside this repository.

| Area | Current state | Destination / action |
|---|---|---|
| empirical executables | 11/11 default treatments have exact bench definitions; sources retained | execute replacement campaigns, pass bench migration gate, then delete study-by-study |
| campaign specifications | historical subject campaign contracts remain | preserve until accepted evidence/reconstruction no longer needs them; new orchestration belongs in bench |
| statistics/reducers | historical sorting-specific tools retained | generic inference/reporting is in bench; retain only sorting-specific theory/model logic after cleanup |
| evidence/claim ledger | historical contract/state retained; no committed raw result datasets | bench owns generic evidence indexing/import/acceptance |
| provenance/manifests | subject reconstruction metadata remains | bench owns generic provenance/integrity; subject keeps algorithm/vendor identity needed for reconstruction |
| hardware counters | historical Linux-specific study retained | exact bench perf treatment exists; delete only after evidence acceptance |
| allocation measurement | historical allocation study retained | exact bench allocation treatment exists; delete only after evidence acceptance |
| external baselines | pinned pdqsort/IPS4o reconstruction retained | bench owns the empirical comparison; subject currently retains pinned checkout/bootstrap provenance |
| workload generators | sorting-specific historical inputs remain shared by retained treatments | remove only after no retained empirical/correctness contract uses them |

See `docs/research-migration-status.md` for the study-by-study gate and `docs/bench-migration.md` for the file-ownership boundary.

The old `SORTLAB_BUILD_RESEARCH_TOOLS` CMake switch is now only a deprecated compatibility alias. `SORTLAB_BUILD_RETAINED_RESEARCH` and the `retained-research` preset make the evidence-reproduction role explicit while keeping the default/package surface clean.

## Future sorting domains, not v1 blockers

| Domain | Why separate |
|---|---|
| parallel sorting | requires thread-count, scheduler, affinity, scaling, and synchronization contracts |
| NUMA sorting | requires topology-aware placement and memory-allocation controls |
| GPU sorting | device transfer, launch overhead, device memory, and accelerator-specific algorithms |
| distributed sorting | network, partitioning, fault tolerance, and cluster topology dominate the contract |
| external-memory sorting | I/O volume, storage device, block/cache policy, and dataset size change the model |
| architecture-specific SIMD | ISA/capability-specific implementations should be isolated from portable core |
| variable-size strings/blobs | comparison, indirection, allocation, and key-extraction costs need a distinct API/experiment model |

## Deliberately deferred implementations within represented mechanisms

These may be valuable later, but they do not represent missing v1 mechanisms:

- Grail/Wiki-style internal-buffer block merge: stable low-extra-memory sorting is represented by `stable_inplace_merge_sort`; block-buffer methods are additional implementations within that family.
- optimal/AlphaDev/ISA-specific tiny networks: bitonic sorting represents the data-oblivious network family; exact fixed-N/ISA optimization is future specialization.
- smoothsort: heap-family adaptivity is interesting but heapsort already represents worst-case in-place heap sorting.
- cycle sort: specialized write-minimizing quadratic sorting can be added if write-count research justifies it; it is not needed for the general-purpose v1 catalog.
- additional sample/bucket/flash sorts: distribution sorting is represented by counting, LSD radix, and in-place MSD radix.

## Research questions intentionally left open

No performance ranking is part of the v1 definition of done. Existing untested hypotheses, crossovers, cutoff surfaces, cross-machine replication, and mechanism explanations remain empirical work. Their absence is **not** a core library blocker.

## Debt policy

A future item is a core blocker only if it prevents correct reusable use of the declared v1 API or invalidates a declared algorithm guarantee. Performance research, additional variants, architecture-specific optimization, evidence campaigns, and new execution domains are tracked separately.
