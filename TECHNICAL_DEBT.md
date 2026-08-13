# Technical debt and future domains

This document separates **core v1 library blockers** from empirical/research work that is intentionally moving to `EddyTodd/bench` or belongs to a different execution domain.

## Core v1 status

**Known core correctness/API/architecture blockers: none.**

The permanent library is generic, instrumentable without benchmark state, installable as `sortlab::sortlab`, and has representative coverage of the major sequential in-memory sorting mechanisms in v1 scope.

## Benchmark/research migration debt

| Area | Current state | Destination / action |
|---|---|---|
| benchmark executables | retained for continuity but not installed API | migrate sorting experiments to `EddyTodd/bench` |
| campaign specifications | retained under `campaigns/` | move to bench-side experiment definitions |
| statistics/reducers | retained under `tools/` | consolidate generic analysis in `bench` |
| evidence/claim ledger | historical research state remains | migrate generic evidence framework to `bench` |
| provenance/manifests | current experiment machinery remains | centralize in `bench` |
| hardware counters | Linux-specific research tooling remains | capability-based bench module |
| allocation measurement | research-only executable remains | bench measurement adapter |
| external baselines | pdqsort/IPS4o comparison track retained | bench external-comparison experiment |
| workload generators | synthetic research workloads remain | bench sorting workload module where appropriate |

See `docs/bench-migration.md` for the concrete file boundary.

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

A future item is a core blocker only if it prevents correct reusable use of the declared v1 API or invalidates a declared algorithm guarantee. Performance research, additional variants, architecture-specific optimization, and new execution domains are tracked separately.
