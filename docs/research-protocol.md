# Research protocol

This document defines how the project moves from exploratory benchmarks to defensible published claims.

## Research tiers

### Tier 0 — correctness

All algorithms pass deterministic edge/workload tests in both timed and instrumented instantiations. Sanitizer runs should be clean when platform tooling is available.

### Tier 1 — exploratory performance

Local runs identify candidate crossover points and suspicious workload interactions. Results may guide hypotheses but are not promoted as general conclusions.

### Tier 2 — controlled replication

A fixed experiment matrix is run repeatedly on a controlled host with environment manifests, randomized algorithm order, independent trial inputs, and raw-data hashes. The matrix and analysis parameters are fixed before final data collection.

### Tier 3 — cross-environment replication

The same protocol is repeated across compilers/standard libraries and at least two materially different CPU microarchitectures. Conclusions are classified as environment-specific or replicated.

## Core hypotheses for the next empirical campaign

H1. Insertion sort has a measurable small-`n` region where lower constant overhead offsets quadratic scaling, especially on nearly-sorted data.

H2. Three-way quicksort increasingly outperforms two-way Hoare quicksort as duplicate entropy decreases, with the strongest effect on all-equal and few-unique inputs.

H3. LSD radix sort has a size-dependent crossover against `std::sort` on 64-bit integer keys and is sensitive to memory hierarchy because it performs repeated full-array passes and uses linear auxiliary storage.

H4. Heapsort performs more poorly in wall-clock time than its comparison complexity alone predicts on modern cache hierarchies, despite its deterministic `O(n log n)` bound.

H5. Algorithm rankings and crossover points are not invariant across input structure, so a universal “fastest sorting algorithm” claim will be rejected unless explicitly scoped.

These hypotheses are directional research targets, not conclusions.

## Default experiment matrix

For controlled work, use logarithmically spaced sizes dense around suspected crossovers. A starting matrix is:

`n = 8, 16, 24, 32, 48, 64, 96, 128, 192, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536, 131072, 262144, 524288, 1048576`

Run all nine workload families. Quadratic algorithms should stop at a preregistered threshold rather than timing out opportunistically after results are seen.

Use at least 31 independent trials for exploratory controlled runs and increase the sample count when confidence intervals remain too wide for the effect size under study. Sample size should be justified by precision/effect requirements rather than a ritual fixed number.

## Primary outcomes

1. elapsed time distribution and median;
2. paired speedup versus a named baseline;
3. comparison count where meaningful;
4. explicit write/swap count for project-controlled implementations;
5. crossover location as a function of `n` and workload.

Future primary outcomes include branch misses, cache misses, cycles, instructions, peak auxiliary memory, and energy.

## Statistical rules

- Preserve and publish raw trials.
- Prefer paired comparisons on identical trial inputs.
- Report uncertainty intervals with point estimates.
- Do not delete outliers post hoc simply because they are inconvenient.
- If a run is invalidated by an external event (thermal event, host load, configuration error), record the invalidation rule and retain the original artifact separately.
- Treat multiple sizes/workloads as a family of comparisons when making significance claims; do not mine hundreds of cells for isolated low p-values.
- Distinguish statistical detectability from practical effect size.
- Replicate surprising results before interpreting mechanisms.

The included bootstrap reducer is intentionally simple. More advanced inferential work may use blocked/paired models, robust regression, hierarchical models, or multiple-comparison corrections in a separate analysis environment, but must preserve the raw schema and experiment identity.

## Crossover estimation

A crossover is not the first noisy sample where one median happens to be lower. Candidate crossover regions should be sampled more densely, and the paired speedup curve with uncertainty should be inspected across adjacent `n`. Report a region or fitted estimate with uncertainty rather than false integer precision.

## Publication gate

A benchmark statement can enter the README or portfolio as an empirical conclusion only when it has:

1. a precise scope (algorithm versions, value type, workload, size range, machine/compiler);
2. raw CSV;
3. manifest and SHA-256;
4. analysis command/configuration;
5. adequate independent samples;
6. uncertainty/effect-size reporting;
7. no correctness failures;
8. at least one repeat run demonstrating the conclusion is not a one-run artifact.

Cross-machine claims additionally require Tier 3 replication.
