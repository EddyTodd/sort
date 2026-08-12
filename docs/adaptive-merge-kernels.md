# Adaptive merge-kernel research

Milestone 8 isolated **which adjacent runs are merged and how runs are extended**. This track freezes those decisions and studies the next layer down: **how two already-selected adjacent sorted runs are physically merged**.

The canonical kernel experiment fixes:

- run detection: the adaptive monotone-run detector from `adaptive_merge.hpp`;
- run extension: balanced variable minrun;
- merge schedule: Powersort;
- value type: signed 64-bit scalar values.

Only the merge kernel changes. This is a controlled reference context, not a claim that Powersort + balanced minrun is already the fastest policy.

## Experimental factors

### Temporary-buffer policy

`full`

Copies both adjacent runs into a reusable workspace and merges from that copy back into the source array. For a merge of lengths `a` and `b`, the requested temporary workspace is `a + b` elements.

`smaller`

Copies only the smaller adjacent run. If the left run is smaller, merging proceeds forward. If the right run is smaller, merging proceeds backward.

For every individual merge, the requested temporary workspace is therefore

`min(a, b) <= floor((a + b) / 2)`.

This is a strict structural memory reduction for the merge operation. It does **not** change the asymptotic auxiliary-space class of the complete worst-case sort: both policies remain `O(n)` in the worst case. The experiment reports requested peak elements, actual `std::vector` capacity peak, capacity bytes, and cumulative elements copied into temporary workspace.

### Search policy

`linear`

Chooses one next element at a time by comparing the current run heads (or tails for a backward merge).

`gallop`

Begins identically to linear merging. After one run wins `gallop_threshold` consecutive selections, the implementation uses exponential search to bracket the next insertion boundary and then binary search inside that bracket. A block is copied in one logical gallop phase, after which ordinary merging resumes.

The canonical coarse threshold candidates are `4, 7, 12, 16`. Seven is included because production TimSort-family implementations have historically used a threshold in this region, but the repository does not treat it as a universal constant.

## Stability rules

The scalar harness cannot observe equal-key ordinals directly, so the full adaptive-record stability replication remains separate technical debt. Nevertheless, the kernel itself follows the stability ordering required by a stable merge:

- forward merge: the left run wins equal-key ties;
- backward merge: the right run is placed later on equal-key ties;
- left-side gallops use an upper-bound boundary when left elements may precede an equal right key;
- right-side gallops use a lower-bound boundary when only strictly smaller right elements may precede the current left key;
- the reverse-direction gallops use the corresponding tail boundaries.

These rules are deliberate. Replacing an upper bound with a lower bound in the wrong direction can preserve sortedness while silently breaking stability.

## Timed versus instrumented paths

As elsewhere in the repository, timing and operation accounting are separate executions over identical input.

The timed template does not increment:

- comparison/write counters;
- merge-kernel counters;
- gallop counters;
- temporary-space metrics.

The instrumented pass records those observables after the timed competitors have completed. Workspace allocation/growth is part of the timed algorithm because it is part of the treatment being measured.

## Metrics

`sort_merge_kernels` emits:

- elapsed nanoseconds;
- comparisons, swaps, and explicit writes;
- natural/effective run counts and fixed-schedule merge cost;
- temporary elements requested at peak;
- actual reusable workspace capacity at peak;
- peak capacity bytes;
- cumulative elements copied into temporary workspace;
- gallop entry count;
- elements skipped/copied through gallop phases;
- correctness verification.

A zero `gallop_elements` value does not mean galloping was disabled. A gallop attempt can find that no additional block belongs before the opposing current key. `gallop_entries` and `gallop_elements` are therefore reported separately.

## H16 — smaller-run buffering

**Hypothesis.** Copying only the smaller run strictly reduces requested temporary workspace and may reduce data movement or improve locality, but its forward/backward merge asymmetry and different copy pattern can make wall-time effects workload- and machine-dependent.

The memory bound is structural. A timing or cache claim is empirical and must not be inferred from the bound alone.

Required evidence includes repeated `merge-kernels-v1` data, same-input paired comparison under the same search policy, explicit temporary-space metrics, and hardware evidence before causal cache/bandwidth language is used.

## H17 — galloping and threshold selection

**Hypothesis.** Galloping can reduce comparison/control work when one run wins in long streaks, while adding search overhead on highly interleaved runs. The best activation threshold is not universal.

`tools/tune_gallop.py` prevents threshold leakage:

1. trials where `trial % 3 != 2` are training observations;
2. the fastest gallop threshold is selected separately for each buffer × pattern × `n` cell;
3. `trial % 3 == 2` observations are held out;
4. the selected gallop treatment is paired against the linear treatment using the same buffer, trial, and input hash.

The held-out comparison answers “does tuned galloping beat linear merging in this scope?” It does not answer whether that threshold transfers to another compiler, CPU, value type, or workload.

## Paired broad analysis

`tools/analyze_merge_kernels.py` provides descriptive and paired statistics for every named treatment. The default reference treatment, `smaller_gallop_7`, is a preregistered comparison anchor, **not a declared winner**.

Report fields include median/quartiles, paired median speedup, percentile-bootstrap interval, win rate, and exact sign-test evidence, alongside comparison/write/temp/gallop context.

## Canonical campaign

`campaigns/merge-kernels-v1.json` freezes:

- 2 buffer policies;
- one linear treatment per buffer;
- gallop thresholds `4, 7, 12, 16` per buffer;
- all 12 adaptive-merge workload families;
- 18 sizes from 64 through 1,048,576, including `n=315` and denser small/medium points;
- 51 independent trials;
- two controlled repetitions;
- paired 5,000-replicate bootstrap analysis;
- held-out threshold tuning.

A denser follow-up around any observed crossover must be a new campaign artifact rather than editing this contract after results are seen.

## Production-source design context

The production sources inspected in the adaptive-merge program remain pinned for methodological context:

- CPython `listobject.c` at `b11e749f7590e9a0907db908fa3e7e76c772c28f` includes forward/backward merge specializations and adaptive galloping in its Powersort-based list sort;
- OpenJDK `TimSort.java` at `86d80bd392c9b1b801394d657a8c7fdf095761e1` includes smaller-run temporary-buffer merge directions and galloping logic.

This repository does not copy those implementations and does not import their benchmark numbers. The code here is an original controlled implementation designed to isolate factors under one C++ measurement contract.

## Evidence boundary

This milestone does **not** establish that the resulting combination is “production TimSort,” “CPython sort,” or universally optimal. Production implementations combine additional details such as adaptive gallop-state updates, specialized copying, object/comparator semantics, allocation strategies, and years of platform-specific engineering.

It also does not close adaptive-record stability validation. That remains explicit until an ordinal-carrying record harness exercises these exact kernels.
