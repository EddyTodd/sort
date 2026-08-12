# Technical debt and intentionally deferred research

This file records known gaps explicitly. A missing item is not evidence that the current implementation is wrong; it identifies where the scope or measurement contract is narrower than the long-term research objective.

| Area | Current limitation | Impact | Remediation / evidence needed | Status |
|---|---|---|---|---|
| canonical evidence | Versioned Tier-2 campaigns are defined, but no repository-wide canonical dataset has yet passed the publication gate | hypotheses remain untested regardless of exploratory development runs | execute campaigns on controlled hosts, validate hashes/manifests, attach evidence to claim registry | open |
| cross-machine replication | no completed Tier-3 compiler/standard-library/microarchitecture matrix | machine-independent ranking claims are unsupported | repeat frozen campaigns on materially different CPUs and toolchains | open |
| adaptive merge stability | adaptive merge-policy/kernel tracks currently operate on scalar `int64_t` values | stability is established by construction but not yet revalidated with ordinal-carrying records in these tracks | add record/payload adaptive merge harness and empirical equal-key stability checks | open |
| adaptive merge kernel | full-buffer, smaller-run-buffer, linear, and exponential-plus-binary galloping treatments are now implemented under a fixed Powersort/balanced context | the former missing-kernel implementation gap is closed, but performance hypotheses remain untested | execute `merge-kernels-v1`, validate artifacts, attach evidence to H16-H17 | implemented / evidence open |
| adaptive gallop state | gallop activation threshold is fixed within a treatment rather than dynamically rewarded/penalized during a sort | production TimSort-family adaptive `min_gallop` behavior is not represented | add dynamic-threshold policy as a separately named treatment after fixed-threshold evidence | open |
| minrun research | classic and current balanced variable minrun are implemented, but no learned/domain-specific minrun policy | possible additional gains remain unexplored | train candidate policies on held-out data and replicate before promotion | open |
| merge-policy theory | exact alphabetic optimum model is O(r^3) and capped at 64 runs | exact optimal comparison is limited to small run sets | add a proven faster exact/near-exact solver if larger exact models become necessary | open |
| tiny sorting | bitonic network is not an optimal-comparator or architecture-specialized network | small-set frontier is incomplete | add provenance-pinned optimal networks, SIMD/register kernels, and AlphaDev-derived sequences as distinct treatments | open |
| external baselines | first track covers pdqsort and sequential IPS4o only | state-of-the-art comparison universe remains incomplete | add pinned BlockQuicksort, QuickXsort/QuickMergesort, VQSort, stable adaptive libraries where build contracts permit | open |
| portfolio coverage | portfolio evaluator currently consumes the core scalar harness, not every dedicated research track | learned selector cannot yet choose adaptive-merge, external, or specialized tiny treatments | define a unified compatible portfolio schema after each treatment has independent evidence | open |
| comparator cost | primary scalar experiments use cheap signed 64-bit comparisons | results may not transfer to strings, indirect keys, locale comparisons, or expensive user comparators | add controlled comparator-cost and realistic key-type tracks | open |
| record movement | record harness observes project-controlled explicit moves, not actual cache/DRAM traffic | movement mechanism claims remain limited | add memory-traffic/bandwidth-capable measurements where supported | open |
| allocator measurement | global allocation harness measures requested allocation behavior, not complete allocator/RSS semantics | peak live requested bytes is not process peak memory | add dedicated process-RSS/allocator-profiler track if needed | open |
| hardware counters | direct `perf_event_open` counters are Linux-specific and permission-dependent | equivalent mechanism evidence is absent on macOS/Windows | add explicit platform adapters (e.g. supported native tooling) without fabricating cross-platform equivalence | open |
| counter coverage | current counters omit bandwidth, TLB, frontend, energy, and detailed cache hierarchy events | causal mechanism resolution is incomplete | add capability-negotiated event families and document model-specific semantics | open |
| CPU isolation | affinity and host preflight improve control but do not provide full core isolation, fixed thermals, or firmware control | residual benchmark noise remains | document controlled-host procedures; add stronger isolation metadata where available | open |
| compiler matrix | campaign runner does not automatically build a full compiler/optimization/standard-library matrix | compile-time implementation interactions require manual orchestration | add versioned build-matrix manifests and binary identity checks | open |
| statistics | bundled tools provide paired bootstrap intervals and exact sign tests but not hierarchical models or automatic multiplicity correction | large comparison families require additional analysis discipline | add preregistered correction/model scripts while preserving raw data | open |
| real-world traces | workload suite is synthetic/controlled | external validity is limited | add redistributable or reproducibly generated real trace families with provenance/privacy review | open |
| variable-size data | no strings/blobs/indirect object graph benchmark | movement and comparison tradeoffs differ materially | add dedicated variable-size/indirect-record model | open |
| SIMD | no architecture-portable SIMD sorter in the core track | vectorized frontier is absent | add VQSort or equivalent as a pinned external track, plus capability metadata | open |
| parallel sorting | only sequential IPS4o is included | throughput/scaling claims are unsupported | define thread-count, affinity, scheduler, NUMA, and scaling contract before adding parallel algorithms | open |
| NUMA | no NUMA placement/topology experiments | large multi-socket behavior unknown | separate NUMA-aware campaign with allocation and affinity controls | open |
| GPU | no accelerator sorting track | GPU crossover/transfer costs unknown | separate device/transfer/launch-overhead experiment model | open |
| external memory | all algorithms assume in-memory data | disk/SSD/out-of-core sorting is outside current conclusions | define I/O-volume, block-size, cache, storage-device and dataset contracts | open |
| energy | no energy measurement | efficiency claims cannot be made | add platform-supported energy counters with calibration and availability metadata | open |
| standard-library internals | internal swaps/writes/allocations of `std::sort`/`std::stable_sort` are not fabricated | operation-count comparisons are asymmetric for library algorithms | use external instrumentation or implementation-specific adapters only when methodologically defensible | accepted limitation |
| hosted CI | validation is designed to run locally/CTest and does not depend on hosted performance runs | server CI is not the canonical performance environment | retain deterministic local test gates; use controlled dedicated hosts for evidence | accepted design |

## Debt policy

When a new limitation is discovered, add it here before or with the change that exposes it. Closing an item requires either implementation plus validation, or a documented decision that the limitation is outside the repository's intended scope.

Do not erase historical limitations from published evidence. If a later milestone fixes a measurement or algorithmic gap, old artifacts retain their original methodology and version identity.
