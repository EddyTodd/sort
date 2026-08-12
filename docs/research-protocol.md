# Research protocol

This document defines how the project moves from implementation checks to defensible empirical claims.

## Research tiers

### Tier 0 — correctness

Every algorithm passes deterministic edge/workload tests in timed and instrumented forms. Record algorithms additionally pass payload-integrity checks; stable algorithms pass empirical stability assertions. Sanitizer runs should be clean when supported.

### Tier 1 — exploratory performance

Local runs identify candidate crossovers, cutoff regions, payload-width interactions, hardware mechanisms, and surprising workload effects. They guide hypotheses but are not promoted as general conclusions.

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

## Portfolio protocol

`sort_lab` records a small runtime-observable feature probe after timed competitors have completed. The portfolio evaluator uses `n`, sampled inversion/disorder, sampled duplicate fraction, and sampled key-range width. It must not use the workload-generator pattern label as a predictor.

`tools/portfolio.py`:

1. trains per-algorithm cost models on training trials;
2. chooses an algorithm for each held-out input using only observable features;
3. adds measured feature-extraction time to selector cost;
4. compares against a training-selected best single algorithm;
5. reports the unattainable held-out per-instance oracle as an upper-bound reference.

A portfolio is interesting only if it reduces held-out total cost after probe overhead and continues to do so under replication.

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
10. held-out cutoff performance and held-out portfolio regret/speedup.

Energy, memory bandwidth, NUMA, SIMD, parallel speedup, and external-memory I/O are separate experiment tracks because they require materially different measurement contracts.

## Statistical rules

- Preserve and publish raw trials.
- Prefer paired comparisons on identical trial inputs.
- Report uncertainty intervals with point estimates.
- Report practical effect sizes alongside p-values.
- Do not delete inconvenient outliers post hoc.
- If a run is invalidated by an external event, document the invalidation rule and retain the original artifact separately.
- Treat multiple sizes/workloads/widths as families of comparisons when making significance claims.
- Replicate surprising results before interpreting mechanisms.
- Do not infer cache, branch, bandwidth, or allocation mechanisms from wall time alone.
- Never tune a cutoff/selector and report its performance on the same observations as if they were independent evidence.

The bundled inference tools are deliberately dependency-free and auditable. More advanced work may use hierarchical models, mixed effects, robust regression, or multiplicity corrections in a separate environment, but raw schemas and experiment identity must remain intact.

## Crossover estimation

A crossover is not the first noisy cell where one median becomes smaller. `tools/crossovers.py` identifies adjacent brackets and labels them exploratory or decisive using endpoint uncertainty.

Candidate regions must then be sampled more densely. Report a bracket or fitted estimate with uncertainty rather than false integer precision. If the crossing does not replicate, it remains an exploratory artifact.

## Mechanism-claim gate

A causal statement requires a direct measurement of the proposed mechanism on the same scoped experiment:

- branch/cycle/cache claim → hardware counters;
- allocation claim → allocation harness;
- data-movement claim → record move instrumentation or stronger traffic measurement;
- memory-bandwidth claim → bandwidth-capable counter/tool, not explicit move count alone.

Counter availability must be explicit. Unavailable counters are not interpreted as zeros.

## Publication gate

A benchmark statement can enter the README, a paper, or a portfolio as an empirical conclusion only when it has:

1. precise scope: algorithm versions, type/record width, workload, size range, machine/compiler, and stability requirement;
2. raw CSV;
3. manifest and SHA-256 for raw data and benchmark binary;
4. analysis command/configuration;
5. adequate independent samples;
6. uncertainty and effect-size reporting;
7. no correctness failures;
8. at least one repeat run showing the result is not a one-run artifact;
9. multiplicity handled when formal significance claims span a comparison family;
10. training/held-out separation for tuned thresholds or selectors;
11. direct mechanism measurements for causal hardware/allocation claims.

Cross-machine claims additionally require Tier 3 replication.
