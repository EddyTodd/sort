# Hybrid sorting and cutoff research

## Why hybrids are a first-class research question

Production sorting algorithms routinely combine mechanisms because asymptotic complexity alone does not determine wall-clock performance. Small partitions have different constant factors, structured inputs contain exploitable runs, duplicate-heavy inputs reward different partitioning, and worst-case protection can require a different fallback algorithm.

The repository therefore treats **algorithm composition and switching thresholds as hypotheses to measure**, not folklore constants.

## Preregistered claims

The default claim matrix contains seven directional hypotheses:

1. **Insertion vs merge:** insertion sort can beat top-down mergesort below a workload/machine-dependent `n` despite quadratic average complexity.
2. **Linear vs binary insertion:** binary insertion reduces comparisons but can lose when comparisons are cheap because its control flow is more expensive; the direction can reverse as `n` grows or comparisons become expensive.
3. **Merge + insertion:** replacing tiny recursive merge leaves with insertion sort should improve practical mergesort over a non-hybrid base case.
4. **Quicksort + insertion:** insertion sorting small partitions should improve a median-of-three quicksort.
5. **Three-way vs two-way quicksort:** explicit equal-key handling should increasingly help as key entropy falls.
6. **Radix digit width:** larger radix digits reduce passes but increase histogram footprint, implying an architecture/size-dependent crossover.
7. **Natural vs fixed mergesort:** run-adaptive merging should benefit inputs with existing monotone structure.

These claims are encoded in `tools/claim_matrix.py`; adding or changing a claim changes the analysis contract and should be reviewed before canonical data collection.

## Cutoff experiment

`sort_cutoffs` varies insertion thresholds for three families while holding the rest of each algorithm fixed:

- top-down merge + insertion leaves;
- median-of-three quicksort + insertion leaves;
- introsort + insertion leaves.

Suggested first sweep:

```sh
./build/sort_cutoffs \
  --cutoffs 1,4,8,12,16,20,24,32,48,64,96,128 \
  --sizes 8,12,16,24,32,48,64,96,128,192,256,512,1024,2048,4096,8192 \
  --patterns random,sorted,few_unique,nearly_sorted,runs \
  --trials 51 > cutoffs.csv
```

`cutoff=1` is the no-insertion-leaf baseline for the three parameterized families.

## Avoiding tuning-set overfit

Do not select a cutoff and evaluate it on the same trials. `tools/tune_cutoffs.py` deterministically reserves every third trial as held-out data, chooses the best cutoff using only the training trials for a `(family, pattern, n)` cell, and then reports held-out performance versus cutoff 1.

A production/default cutoff should require a broader decision rule than "the best cell in one benchmark." Candidate defaults should be evaluated across:

- all relevant sizes;
- multiple workload families;
- scalar and record widths where applicable;
- compilers/standard libraries;
- at least two microarchitectures.

A single universal cutoff may not exist. If the optimal region moves materially, the result should be reported as a conditional surface rather than averaged into a misleading constant.

## Exploratory validation result

During development, a non-canonical local run reproduced the expected phenomenon: ordinary insertion sort beat the project's top-down mergesort on random 64-bit integers through small sizes, then crossed over as `n` grew; insertion remained especially strong on sorted/nearly-sorted inputs. Merge+insertion and quicksort+insertion hybrids were substantially faster than recurse-to-one baselines in that same environment. Binary insertion also showed a distinct crossover against linear insertion, and 8-bit versus 11-bit radix passes changed ranking with size.

Those observations are **engineering validation only**. They are intentionally not committed as benchmark evidence because they were collected in an uncontrolled development environment. Canonical numerical claims require the publication gate.

## Algorithm portfolios

The repository now includes an experimental portfolio evaluator. `sort_lab` records a bounded input probe after every timed competitor has finished so the probe cannot warm a later timed sort. The probe measures its own elapsed cost and records sampled inversion rate, duplicate fraction, and key-range width.

`tools/portfolio.py` fits one small cost model per candidate algorithm on training trials, chooses among them on held-out trials using only observable features plus `n`, and adds the measured probe cost to the selector. The benchmark generator's `pattern` label is never a runtime feature.

The selector is compared against:

- the best single algorithm chosen from training data;
- the unattainable per-instance held-out oracle, which quantifies the maximum headroom of the candidate portfolio.

A portfolio is not considered successful merely because it predicts the fastest algorithm often. It must reduce **total held-out time after feature cost**, and the result must replicate across environments. A learned policy that overfits one machine is an environment-specific result, not a universal sorter.

## Literature context

The cutoff question is well established in algorithm engineering, but published constants are implementation-specific evidence rather than portable laws. Historical and current OpenJDK dual-pivot quicksort sources use explicit small-array insertion thresholds. Introsort formalizes a different hybrid dimension by falling back from quicksort to a worst-case-efficient stopper. QuickXsort studies compositions in which partitioning is combined with another sorter. Work on small-set sorting networks shows that even insertion sort is not automatically the optimal base case on every architecture.

The repository therefore tests the mechanism—switching sorters as the problem regime changes—while refusing to import another implementation's threshold as a conclusion.
