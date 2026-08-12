# Research protocol

This document defines how the project moves from exploratory benchmarks to defensible published claims.

## Research tiers

### Tier 0 — correctness

All algorithms pass deterministic edge/workload tests in both timed and instrumented instantiations. Record algorithms additionally pass payload-integrity checks and stable algorithms pass empirical stability assertions. Sanitizer runs should be clean when platform tooling is available.

### Tier 1 — exploratory performance

Local runs identify candidate crossover points, payload-width interactions, and suspicious workload effects. Results guide hypotheses but are not promoted as general conclusions.

### Tier 2 — controlled replication

A fixed experiment matrix is run repeatedly on a controlled host with environment manifests, randomized algorithm order, independent trial inputs, raw-data hashes, and analysis parameters fixed before final collection.

### Tier 3 — cross-environment replication

The same protocol is repeated across compilers/standard libraries and at least two materially different CPU microarchitectures. Conclusions are classified as environment-specific or replicated.

## Core hypotheses

H1. Insertion sort has a measurable small-`n` region where lower constant overhead offsets quadratic scaling, especially on nearly-sorted data.

H2. Three-way quicksort increasingly outperforms two-way Hoare quicksort as duplicate entropy decreases, with the strongest effect on all-equal, binary, and few-unique inputs.

H3. LSD radix sort has a size-dependent crossover against `std::sort` on 64-bit integer keys and is sensitive to memory hierarchy because it performs repeated full-array passes and uses linear auxiliary storage.

H4. Heapsort performs more poorly in wall-clock time than its comparison complexity alone predicts on modern cache hierarchies, despite its deterministic `O(n log n)` bound.

H5. Algorithm rankings and crossover points are not invariant across input structure, so a universal “fastest sorting algorithm” claim will be rejected unless explicitly scoped.

H6. Increasing record width changes algorithm rankings because whole-record movement becomes a larger component of total cost; algorithms with more explicit movement should generally degrade faster when comparison cost is held constant.

H7. The performance cost of requiring stability depends on duplicate density, record width, and `n`; no single stable-versus-unstable penalty should be assumed across workloads.

These are directional research targets, not conclusions.

## Default scalar experiment matrix

For controlled work, use logarithmically spaced sizes dense around suspected crossovers. A starting matrix is:

`n = 8, 16, 24, 32, 48, 64, 96, 128, 192, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536, 131072, 262144, 524288, 1048576`

Run all 15 workload families unless a hypothesis preregisters a narrower stratum. Quadratic algorithms stop at a preregistered threshold rather than timing out opportunistically after results are seen.

Use at least 31 independent trials for exploratory controlled runs and increase sample count when uncertainty remains too wide for the effect under study. Sample size should be justified by precision/effect requirements rather than a ritual fixed number.

## Default record experiment matrix

Use the same `n` grid where practical and payload words `0, 1, 3, 7, 15, 31`. Always record actual `record_bytes` because object layout is implementation-dependent.

A full cross-product can be expensive. It is acceptable to preregister a staged design:

1. broad exploratory scan with fewer sizes/trials;
2. identify candidate algorithm × workload × width interactions;
3. collect a denser Tier 2 matrix around those regions with the final analysis plan fixed before collection.

Duplicate-heavy workloads are mandatory for stability research because unique-key inputs cannot expose stability violations.

## Primary outcomes

1. elapsed-time distribution and median;
2. paired speedup versus a named baseline;
3. paired win rate and uncertainty;
4. comparison count where meaningful;
5. explicit write/swap or whole-record movement for project-controlled implementations;
6. empirical stable-trial rate for record experiments;
7. crossover location as a function of `n`, workload, and record width.

Future primary outcomes include branch misses, cache misses, cycles, instructions, allocation counts, peak auxiliary memory, and energy.

## Statistical rules

- Preserve and publish raw trials.
- Prefer paired comparisons on identical trial inputs.
- Report uncertainty intervals with point estimates.
- Report practical effect sizes alongside p-values.
- Do not delete outliers post hoc simply because they are inconvenient.
- If a run is invalidated by an external event, record the invalidation rule and retain the original artifact separately.
- Treat multiple sizes/workloads/widths as families of comparisons when making significance claims.
- Replicate surprising results before interpreting mechanisms.
- Do not infer cache, branch, bandwidth, or allocator mechanisms from timing alone.

The bundled inference tools are intentionally dependency-free and auditable. More advanced work may use paired hierarchical models, robust regression, mixed effects, or multiple-comparison corrections in a separate environment, but must preserve raw schemas and experiment identity.

## Crossover estimation

A crossover is not the first noisy sample where one median happens to be lower. `tools/crossovers.py` identifies candidate adjacent brackets and labels them exploratory or decisive based on endpoint intervals.

Candidate regions must then be sampled more densely. Report a bracket or fitted estimate with uncertainty rather than false integer precision. If the apparent crossing does not replicate, it remains an exploratory artifact.

## Publication gate

A benchmark statement can enter the README or portfolio as an empirical conclusion only when it has:

1. precise scope: algorithm versions, value/record type, workload, size range, machine/compiler, and stability requirement;
2. raw CSV;
3. manifest and SHA-256;
4. analysis command/configuration;
5. adequate independent samples;
6. uncertainty and effect-size reporting;
7. no correctness failures;
8. at least one repeat run demonstrating the conclusion is not a one-run artifact;
9. multiplicity handled when formal significance claims are made across a comparison family.

Cross-machine claims additionally require Tier 3 replication. Hardware-mechanism claims additionally require direct mechanism measurements.
