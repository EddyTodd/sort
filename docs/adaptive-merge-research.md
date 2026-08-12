# Adaptive merge-policy research

Stable adaptive mergesorts are not one algorithm. Performance depends on at least four separable choices:

1. how natural runs are detected;
2. whether short runs are extended, and to what sizes;
3. how adjacent runs are scheduled for merging;
4. how the actual two-run merge is implemented.

This track deliberately holds item 4 constant while factorially varying items 2 and 3. That isolates merge-tree and run-sizing effects before adding galloping, specialized temporary-buffer strategies, or implementation-specific tricks.

## Design-source provenance

The project implementation is original and does not vendor CPython or OpenJDK code. The policy definitions were reviewed against primary production sources and primary literature:

- Munro and Wild, *Nearly-Optimal Mergesorts: Fast, Practical Sorting Methods That Optimally Adapt to Existing Runs*, ESA 2018, DOI `10.4230/LIPIcs.ESA.2018.63`.
- CPython `Objects/listsort.txt` and `Objects/listobject.c` at repository revision `b11e749f7590e9a0907db908fa3e7e76c772c28f`, inspected 2026-08-12. This revision documents/implements Powersort and the variable balanced minrun generator.
- OpenJDK `java.util.TimSort` at repository revision `86d80bd392c9b1b801394d657a8c7fdf095761e1`, inspected 2026-08-12. The project uses the repaired stack-collapse invariant reflected by that implementation.

These are design references, not imported benchmark evidence. Upstream performance claims are not treated as results for this repository.

## Run detection

`sort_merge_policies` recognizes monotone natural runs from left to right.

- ascending runs are nondecreasing;
- descending runs are **strictly** decreasing and are reversed in place.

Strict descending detection is intentional: reversing a run containing equal keys would reverse their relative order and violate stability. The common merge kernel chooses the left run on equality, and binary run extension inserts after existing equal values. Together those rules make the scalar algorithm stable by construction.

The current track operates on scalar `int64_t` values, so record/ordinal stability replication remains separate technical debt. The existing record laboratory continues to be the empirical stability oracle for record-capable algorithms.

## Minrun treatments

### `none`

No artificial extension. Every detected natural run enters the merge scheduler at its discovered length.

This exposes pure merge-policy behavior but can create very many short runs on random inputs.

### `classic`

A fixed TimSort-style minrun is computed once from `n`, using the conventional power-of-two rounding rule that produces a value below 64. A short natural run is extended by stable binary insertion to that fixed target, capped by the remaining input.

### `balanced`

The target minrun varies slightly from run to run. The integer accumulator distributes rounding error across the sequence so, when natural runs are short enough for extension to control the boundaries, effective runs are as evenly sized as possible.

The implementation self-test includes the documented `n = 315` sequence:

`39, 39, 40, 39, 39, 40, 39, 40`

The treatment is evaluated independently from merge scheduling. For example, `timsort_stack + balanced` and `powersort + classic` are valid cells. This prevents attributing the effect of one factor to the other.

## Merge-policy treatments

All policies merge only adjacent runs, which is required to preserve stable order.

### `pairwise`

Effective runs are retained and merged in adjacent rounds from left to right. This is a simple balanced-by-position baseline, not an online stack algorithm.

Its `max_pending_runs` has a different interpretation from the online policies: it records the full set of effective runs retained before the first merge.

### `timsort_stack`

Runs are pushed online and merged according to the repaired TimSort stack-collapse inequalities used by current OpenJDK. This track intentionally studies the **merge scheduling policy**, not a byte-for-byte TimSort implementation.

The merge kernel does not implement TimSort galloping. Short-run extension and merge scheduling are separately parameterized. Therefore results should be labelled `timsort_stack`, not simply “TimSort”.

### `powersort`

The scheduler computes the boundary power of adjacent runs using the exact integer bit-by-bit `powerloop` formulation described by CPython and merges while the pending-run power invariant requires it.

As with the TimSort treatment, the merge kernel is common to the experiment. The purpose is to test scheduling quality without allowing a different low-level merge routine to become a confounder.

## Controlled run-shape workloads

The dedicated harness reuses representative core workloads and adds five deterministic run-geometry probes:

| Pattern | Construction | Property probed |
|---|---|---|
| `run_equal32` | equal 32-element monotone runs | balanced merge tree |
| `run_long_short` | alternating 8 / 128 | severe local length imbalance |
| `run_power_skew` | repeating power-of-two-like lengths | hierarchical imbalance |
| `run_fibonacci` | Fibonacci-like lengths | stack-policy sensitivity |
| `run_alternating_direction` | equal runs alternating ascending/descending | reversal plus run scheduling |

Blocks occupy descending key bands so boundaries are genuine run breaks rather than accidental continuations. These are controlled probes, not claims about real-world frequency.

The raw input decomposition is computed once per trial and reported separately from the runs actually discovered after minrun extension. This distinction matters because extending a short run consumes elements that otherwise would have formed later natural runs.

## Timed versus instrumented paths

The timed path compiles operation and merge-policy metrics out via `if constexpr`. It measures the same algorithmic treatment without comparison/write/run counters contaminating the timed region.

The separate instrumented pass reports:

- comparisons;
- swaps and explicit writes;
- natural runs encountered by the treatment;
- effective runs after extension;
- reversed runs;
- elements consumed by run extension;
- number of scheduled merges;
- weighted scheduled merge cost;
- peak pending-run count;
- entropy of the effective run-length distribution.

The harness also reports raw input natural-run count and raw run entropy outside the treatment.

## Merge cost and entropy

For effective run lengths `L_i`, a merge schedule has weighted merge cost equal to the sum of the two run lengths at every internal merge. Equivalently, it is the weighted external path length of the alphabetic merge tree.

`run_entropy_bits` is

`H = sum_i (L_i / n) * log2(n / L_i)`.

`tools/analyze_merge_policies.py` reports `merge_cost / n - H` as a schedule-redundancy diagnostic. It is a theoretical structural measure, not elapsed time.

A lower scheduled merge cost can reduce potential data movement, but wall time also contains run detection, insertion extension, comparisons, memory effects, and policy overhead. Therefore merge-cost superiority is not automatically a performance conclusion.

## Exact optimal alphabetic model

`tools/merge_policy_model.py` separates scheduling theory from the sorter entirely. For up to 64 runs it computes the exact optimal alphabetic merge cost using dynamic programming:

`OPT(i,j) = weight(i,j) + min_k (OPT(i,k) + OPT(k+1,j))`.

It then simulates `pairwise`, `timsort_stack`, and `powersort` on the same run lengths and reports each ratio to the exact optimum.

The versioned suite `models/merge-policy-sequences-v1.json` includes balanced, left/right/center-heavy, long-short, Fibonacci-like, and power-skewed examples.

Example:

```sh
python3 tools/merge_policy_model.py --suite models/merge-policy-sequences-v1.json
python3 tools/merge_policy_model.py --lengths 10,1,1
```

The exact model is structural evidence. It does not substitute for wall-time replication.

## H14 — adaptive merge scheduling

For skewed run distributions, merge schedule should materially change weighted merge cost. Powersort is expected to reduce redundancy relative to naive pairwise scheduling in many such cases and may differ from the repaired TimSort stack policy.

The hypothesis does **not** assume Powersort universally wins wall time. Policy overhead and machine effects remain empirical questions.

## H15 — balanced minrun scheduling

A balanced variable minrun sequence should reduce avoidable run-size imbalance when natural runs are short enough for extension to control effective boundaries. Its wall-time benefit is uncertain because more favorable merge trees can trade against insertion-extension cost and policy overhead.

## Tier-2 campaign

`campaigns/merge-policies-v1.json` freezes the first controlled experiment:

- 3 merge policies;
- 3 minrun policies;
- 12 workload families;
- a dense size grid including `n = 315` and powers-of-two boundaries;
- 51 independent trials;
- 2 controlled repetitions;
- paired analysis with `powersort + balanced` as a named baseline;
- 5,000 bootstrap replications in the preregistered reducer.

The baseline is a comparison reference, not a declared winner.

## Interpretation limits

This milestone intentionally does **not** implement:

- galloping merges;
- copying only the smaller run as temporary storage;
- record-width adaptive merge variants;
- expensive user-defined comparator regimes;
- multiway Powersort;
- parallel merge scheduling;
- architecture-specific vectorized merge kernels.

Those can materially change rankings. They are documented in `TECHNICAL_DEBT.md` and require their own controlled treatments rather than being silently folded into this experiment.
