# Research protocol

This document defines how the project moves from implementation checks to defensible empirical claims.

## Research tiers

### Tier 0 — correctness

Every algorithm passes deterministic edge/workload tests in timed and instrumented forms. Record algorithms additionally pass payload-integrity checks; stable algorithms pass empirical stability assertions. Sanitizer runs should be clean when supported.

### Tier 1 — exploratory performance

Local runs identify candidate crossovers, cutoff regions, payload-width interactions, hardware mechanisms, merge-policy effects, merge-kernel effects, and surprising workload effects. They guide hypotheses but are not promoted as general conclusions.

### Tier 2 — controlled replication

A fixed experiment matrix is run repeatedly on a controlled host with environment manifests, independent paired inputs, randomized execution order, raw-data hashes, and analysis parameters fixed before final collection.

### Tier 3 — cross-environment replication

The same protocol is repeated across compilers/standard libraries and at least two materially different CPU microarchitectures. Conclusions are labelled environment-specific or replicated.

## Core hypotheses

H1. **Small-subproblem insertion crossover.** Plain insertion sort has a measurable small-`n` region where low overhead offsets quadratic scaling. The crossover depends on input structure, machine, compiler, and element cost.

H2. **Binary versus linear insertion.** Binary insertion reduces comparison count but can lose when comparisons are cheap or input is already ordered because it performs more control work and does not exploit the same one-comparison fast path. The ranking can reverse as `n` or comparison cost grows.

H3. **Insertion-leaf hybrids.** Merge sort, median-of-three quicksort, and introsort can improve by using insertion sort for sufficiently small leaves/partitions. The optimal cutoff is a surface, not assumed to be one universal constant.

H4. **Duplicate-aware partitioning.** Three-way quicksort increasingly outperforms two-way partitioning as duplicate entropy decreases, with the strongest effect expected on all-equal, binary, and few-unique inputs.

H5. **Radix digit width.** Wider radix digits reduce pass count but increase histogram footprint, producing size- and architecture-dependent crossover behavior.

H6. **Run adaptivity.** Natural/run-adaptive merge strategies outperform fixed merge schedules when existing monotone structure is sufficiently strong, but may pay overhead on structureless inputs.

H7. **Heap mechanism.** Heapsort can underperform comparison-count expectations in wall-clock time because locality and branch behavior differ from more cache-friendly alternatives. Any causal explanation requires direct hardware-counter evidence.

H8. **Record width.** Increasing record width changes rankings because whole-record movement becomes a larger component of total cost. Algorithms with more movement should generally degrade faster when comparison cost is held constant.

H9. **Cost of stability.** The performance cost of requiring stability depends on duplicate density, record width, `n`, and implementation; no single stable-versus-unstable penalty is assumed.

H10. **No universal winner.** Algorithm rankings and crossover points are not invariant across input structure, element type, or environment. A universal “fastest sorting algorithm” claim is rejected unless its domain is precisely scoped.

H11. **Feature-based portfolio.** A cheap input probe may improve held-out performance by selecting among complementary algorithms, but the benefit must exceed feature-extraction cost and must be measured without using benchmark generator labels as runtime oracle information.

H12. **Tiny-kernel crossover.** A data-oblivious bitonic sorting network may outperform insertion sort in some small-array domains, while insertion can retain an advantage on sufficiently ordered inputs because its work adapts to existing order. Any crossover is environment-dependent.

H13. **Leaf-kernel integration effect.** A tiny sorter that wins in isolation may not be the best base case inside merge, quick, or introsort because integration changes code footprint, surrounding control flow, and the distribution of leaf sizes.

H14. **Adaptive merge scheduling.** For skewed natural-run distributions, merge schedule materially changes weighted merge cost. Powersort should often reduce structural redundancy relative to naive pairwise scheduling and may differ from the repaired TimSort stack policy, without implying universal wall-time dominance.

H15. **Balanced minrun scheduling.** A variable balanced minrun sequence can reduce avoidable effective-run imbalance and merge-tree redundancy when short natural runs are extended, but the wall-time effect depends on `n`, input structure, insertion-extension cost, compiler, and machine.

H16. **Smaller-run merge buffering.** Buffering only the smaller adjacent run strictly reduces requested temporary workspace for each merge. Its wall-time effect can still vary because it changes copy volume, direction, overlap behavior, and memory access patterns.

H17. **Galloping merge crossover.** Exponential-plus-binary galloping can reduce comparison/control work when one run wins in long streaks, but can add search overhead on interleaved runs. The best activation threshold is workload- and environment-dependent.

These are directional research targets, not conclusions.

## Default scalar experiment matrix

Use logarithmically spaced sizes with denser sampling around suspected crossover regions:

`n = 8, 12, 16, 24, 32, 48, 64, 96, 128, 192, 256, 384, 512, 768, 1024, 2048, 4096, 8192, 16384, 32768, 65536, 131072, 262144, 524288, 1048576`

Run all 15 workload families unless a hypothesis preregisters a narrower stratum. Quadratic algorithms stop at a preregistered threshold rather than timing out opportunistically after results are observed.

Use at least 31 independent trials for exploratory controlled work and increase sample count when uncertainty remains too wide for the effect under study. Sample size is justified by precision/effect requirements rather than ritual.

## Hybrid cutoff protocol

`sort_cutoffs` varies insertion thresholds for merge+insertion, median-of-three quicksort+insertion, and introsort+insertion.

Rules:

1. include cutoff `1` as the no-insertion-leaf baseline;
2. select cutoffs using training trials only;
3. evaluate the selected cutoff on held-out trials;
4. repeat across workloads and sizes rather than averaging immediately to one constant;
5. do not promote a default cutoff until it replicates across the intended compiler/CPU/type domain.

`tools/tune_cutoffs.py` implements a deterministic train/held-out split. If the optimal region varies materially by domain, report a conditional surface rather than hiding that interaction behind one mean.

## Tiny-kernel and leaf-integration protocol

`sort_tiny` directly compares linear insertion, binary insertion, a padded power-of-two bitonic sorting network, and `std::sort` for `n <= 32` on paired deterministic inputs. The bitonic treatment has a fixed data-oblivious comparator topology, but the repository does not assume the compiler emits branch-free machine code.

`sort_leaf_hybrids` then holds the parent algorithm fixed while varying both leaf kernel and cutoff for merge, median-of-three quicksort, and introsort.

Rules:

1. direct tiny-kernel performance and integrated-hybrid performance are separate outcomes;
2. the bitonic leaf cutoff may not exceed 32;
3. select `(kernel, cutoff)` using training trials only;
4. evaluate the selected treatment on held-out trials;
5. compare against the **best insertion-only cutoff chosen from the same training data**, not one arbitrary folklore constant;
6. a direct network win does not imply an integrated network win;
7. branch/cache/code-footprint explanations require direct mechanism measurements rather than wall-time speculation;
8. a denser crossover follow-up must be frozen as a new campaign artifact after the coarse campaign, not retrofitted into the original contract.

`tools/analyze_tiny.py` reports paired bootstrap uncertainty for the direct experiment. `tools/tune_leaf_kernels.py` implements the joint train/held-out kernel-plus-cutoff selection. See `docs/tiny-kernel-research.md`.

## Adaptive merge-policy protocol

`sort_merge_policies` studies stable natural mergesort with a factorial design. It crosses merge scheduling (`pairwise`, `timsort_stack`, `powersort`) with run-extension policy (`none`, `classic`, `balanced`) while holding the run detector, stable binary extension, and stable two-run merge kernel constant.

Rules:

1. **isolate one factor at a time:** compare merge policies at the same minrun treatment, and compare minrun treatments under the same merge policy;
2. all nine treatments for a trial receive the exact same input and randomized execution order;
3. compute the untreated input's natural-run decomposition once and report it separately from treatment-specific runs, because minrun extension consumes future boundaries;
4. treat `scheduled_merge_cost` and `merge_cost / n - run_entropy` as structural merge-tree outcomes, not as substitutes for elapsed time;
5. the exact alphabetic merge-cost model is theoretical evidence and does not count as replicated wall-time evidence;
6. the `timsort_stack` treatment isolates the repaired stack scheduling policy; it must not be labelled a complete production TimSort because the common merge kernel intentionally omits other production-specific behavior;
7. no default merge/minrun combination is promoted until Tier-2 evidence replicates on the intended domain; cross-machine defaults require Tier 3;
8. mechanism statements about cache behavior, branch behavior, allocation, or memory traffic require direct measurements from an appropriately scoped follow-up;
9. a denser follow-up prompted by the coarse campaign must be frozen as a new campaign rather than editing `merge-policies-v1` after seeing results.

`tools/analyze_merge_policies.py` performs paired inference on identical `(pattern, n, trial, input_hash)` observations. `tools/merge_policy_model.py` computes exact optimal alphabetic merge cost for preregistered run-length sequences with at most 64 runs and compares each policy's structural cost to that optimum. See `docs/adaptive-merge-research.md`.

## Adaptive merge-kernel protocol

`sort_merge_kernels` freezes adaptive scheduling at Powersort with balanced variable minrun, then varies only two-run merge mechanics.

Buffer treatments:

- `full`: copy both adjacent runs into reusable workspace;
- `smaller`: copy only the smaller adjacent run and merge forward or backward as required.

Search treatments:

- `linear`: one comparison/selection step at a time;
- `gallop`: after a fixed winning streak, exponential search brackets a boundary and binary search finishes it before copying the discovered block.

Rules:

1. run detection, minrun policy, and merge schedule remain fixed within `merge-kernels-v1` so kernel effects are not confused with H14/H15 effects;
2. compare buffer policies at the same search policy and gallop threshold;
3. compare linear versus galloping under the same buffer policy;
4. the smaller-run memory bound is structural evidence, but locality/cache/bandwidth or speed explanations still require empirical/mechanism evidence;
5. train gallop thresholds only on trials where `trial % 3 != 2` and evaluate the selected threshold only on held-out `trial % 3 == 2` observations;
6. held-out gallop comparisons must pair the same `(pattern, n, trial, input_hash)` and the same buffer policy;
7. `smaller_gallop_7` is a preregistered broad-analysis reference, not a declared winner or universal default;
8. `gallop_entries` and `gallop_elements` are separate because an attempted gallop can legitimately discover no additional block;
9. record-level equal-key stability remains a separate validation requirement before production-stability claims about this exact kernel track;
10. a dynamic/adaptive gallop threshold is a future separate treatment and must not overwrite fixed-threshold evidence.

`tools/analyze_merge_kernels.py` provides paired broad analysis. `tools/tune_gallop.py` performs train/held-out threshold selection. See `docs/adaptive-merge-kernels.md`.

## Portfolio protocol

`sort_lab` records a small runtime-observable feature probe after timed competitors have completed. The portfolio evaluator uses `n`, sampled inversion/disorder, sampled duplicate fraction, and sampled key-range width. It must not use the workload-generator pattern label as a predictor.

`tools/portfolio.py`:

1. trains per-algorithm cost models on training trials;
2. chooses an algorithm for each held-out input using only observable features;
3. adds measured feature-extraction time to selector cost;
4. compares against a training-selected best single algorithm;
5. reports the unattainable held-out per-instance oracle as an upper-bound reference.

A portfolio is interesting only if it reduces held-out total cost after probe overhead and continues to do so under replication.

Dedicated tracks such as adaptive merge policies/kernels, external sorters, and tiny kernels are not automatically injected into this portfolio. A unified selector requires a new compatible schema and versioned campaign after the candidate treatments themselves have independent evidence.

## Default record experiment matrix

Use the scalar `n` grid where practical and payload words `0, 1, 3, 7, 15, 31`. Always record actual `record_bytes` because object layout is implementation-dependent.

A full cross-product can be expensive. A staged design is acceptable:

1. broad exploratory scan with fewer sizes/trials;
2. identify candidate algorithm × workload × width interactions;
3. fix the final analysis plan;
4. collect a denser Tier 2 matrix around those regions.

Duplicate-heavy workloads are mandatory for stability research because unique-key inputs cannot expose stability violations.

## Primary outcomes

1. elapsed-time distribution and median;
2. paired speedup versus a named baseline;
3. paired win rate and exact paired sign-test evidence;
4. comparison count where meaningful;
5. explicit writes/swaps or whole-record movement for project-controlled implementations;
6. empirical stable-trial rate for record experiments;
7. crossover location as a function of `n`, workload, and record width;
8. allocation calls/requested bytes/peak tracked live bytes in the allocation-only harness;
9. cycles, instructions, branch misses, and cache-event diagnostics where hardware counters are available;
10. held-out cutoff performance and held-out portfolio regret/speedup;
11. direct tiny-kernel paired speedup with logical comparator/write context;
12. held-out speedup of jointly selected leaf kernel/cutoff versus the best training-selected insertion-only cutoff;
13. adaptive-merge scheduled merge cost, merge cost per element, effective/raw run counts, run entropy, and structural redundancy;
14. paired adaptive merge-policy/minrun wall-time effects with comparison/write and pending-run context;
15. adaptive merge temporary-workspace peak/capacity and copied-element context;
16. held-out tuned-gallop speedup versus linear merge under the same buffer policy, with gallop activation/elements and comparison context.

Energy, memory bandwidth, NUMA, SIMD, parallel speedup, and external-memory I/O are separate experiment tracks because they require materially different measurement contracts.

## Statistical rules

- Preserve and publish raw trials.
- Prefer paired comparisons on identical trial inputs.
- Report uncertainty intervals with point estimates.
- Report practical effect sizes alongside p-values.
- Do not delete inconvenient outliers post hoc.
- If a run is invalidated by an external event, document the invalidation rule and retain the original artifact separately.
- Treat multiple sizes/workloads/widths/policy cells as families of comparisons when making significance claims.
- Replicate surprising results before interpreting mechanisms.
- Do not infer cache, branch, bandwidth, allocation, or code-footprint mechanisms from wall time alone.
- Never tune a cutoff, kernel, merge policy, minrun policy, gallop threshold, or selector and report its performance on the same observations as if they were independent evidence.
- Structural optimality of a merge tree or temporary-space bound does not imply wall-time optimality of the corresponding sorter.

The bundled inference tools are deliberately dependency-free and auditable. More advanced work may use hierarchical models, mixed effects, robust regression, or multiplicity corrections in a separate environment, but raw schemas and experiment identity must remain intact.

## Crossover estimation

A crossover is not the first noisy cell where one median becomes smaller. `tools/crossovers.py` identifies adjacent brackets and labels them exploratory or decisive using endpoint uncertainty.

Candidate regions must then be sampled more densely. Report a bracket or fitted estimate with uncertainty rather than false integer precision. If the crossing does not replicate, it remains an exploratory artifact.

## Mechanism-claim gate

A causal statement requires a direct measurement of the proposed mechanism on the same scoped experiment:

- branch/cycle/cache claim → hardware counters;
- allocation claim → allocation harness;
- data-movement claim → record move instrumentation or stronger traffic measurement;
- memory-bandwidth claim → bandwidth-capable counter/tool, not explicit move count alone;
- instruction-cache/code-footprint claim → appropriate code-size/instruction-cache evidence, not elapsed time alone.

Counter availability must be explicit. Unavailable counters are not interpreted as zeros.

## Publication gate

A benchmark statement can enter the README, a paper, or a portfolio as an empirical conclusion only when it has:

1. precise scope: algorithm/policy versions, type/record width, workload, size range, machine/compiler, stability requirement, and any minrun/leaf/cutoff/buffer/gallop treatment;
2. raw CSV;
3. manifest and SHA-256 for raw data and benchmark binary;
4. analysis command/configuration;
5. adequate independent samples;
6. uncertainty and effect-size reporting;
7. no correctness failures;
8. at least one repeat run showing the result is not a one-run artifact;
9. multiplicity handled when formal significance claims span a comparison family;
10. training/held-out separation for tuned thresholds, leaf kernels, policies, or selectors;
11. direct mechanism measurements for causal hardware/allocation/code-footprint claims;
12. explicit separation of structural/theoretical merge-cost or memory-bound evidence from empirical wall-time evidence where applicable.

Cross-machine claims additionally require Tier 3 replication.
