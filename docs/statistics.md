# Statistical analysis policy

## Unit of replication

The independent replication unit is the generated trial input, not repeated timing of the exact same array. Algorithms are paired on the same input instance so input difficulty is blocked rather than left as uncontrolled variance.

## Point estimates and uncertainty

The included reducers report medians and percentile-bootstrap confidence intervals. Medians are used because wall-clock benchmark distributions are often skewed and may contain scheduler-induced long tails.

A confidence interval is not a binary truth test. Its width is evidence about precision and should influence whether more trials are needed.

## Paired speedup

For algorithm `A` against baseline `B`, each paired trial contributes:

`speedup = time(B) / time(A)`

Values above 1 mean `A` was faster on that paired input. The reducer reports the median paired speedup and a bootstrap interval over the paired ratios.

Pairing keys are:

- scalar: `(pattern, n, trial, input_hash)`;
- records: `(pattern, n, record_bytes, trial, key_hash)`.

## Paired win rate

`paired_win_rate` is the proportion of paired trials where the candidate beats the baseline, with ties contributing one half. It answers a different question from median speedup: how consistently did the candidate win across trial inputs?

A large speedup driven by a minority of inputs and a small but consistent speedup can therefore be distinguished.

## Exact paired sign test

The reducers include an exact two-sided sign-test p-value. Ties are excluded from the binomial count. This test asks whether wins and losses are imbalanced under a 50/50 null without assuming a parametric timing distribution.

It intentionally discards magnitude information, so it must be reported with effect sizes. A tiny p-value does not imply a practically important speedup.

## Multiple comparisons

A benchmark grid contains many algorithms, workloads, sizes, payload widths, and possible baselines. The repository does **not** treat unadjusted cell-level p-values as permission to mine for isolated “significant” results.

For confirmatory campaigns, hypotheses and comparison families should be fixed before final collection. Where formal significance claims span many cells, use an appropriate family-wise or false-discovery correction in a derived analysis and retain the original raw data.

## Crossover detection

`tools/crossovers.py` scans a summary curve for adjacent measured sizes where the median paired speedup crosses 1.0. It interpolates between the two points in log(`n`) / log(speedup) space.

This produces a **candidate crossover estimate**, not exact truth. A bracket is labeled:

- `decisive` when the confidence interval at one endpoint lies wholly below 1 and the other lies wholly above 1;
- `exploratory` otherwise.

The correct response to a candidate crossover is to sample more densely inside and around the bracket, rerun the paired experiment, and then fit/report uncertainty. The interpolated value must not be presented with false integer precision.

## Mechanism claims

Timing and operation counts can identify performance phenomena but usually cannot establish a hardware mechanism. Claims such as “algorithm A loses because of cache misses” require direct cache-counter or equivalent evidence. The same rule applies to branch prediction, TLB behavior, memory bandwidth, and energy.
