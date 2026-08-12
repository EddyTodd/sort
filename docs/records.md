# Record-width and stability experiment

## Purpose

Scalar integer benchmarks hide a major systems variable: the cost of moving an element. Sorting an 8-byte integer and sorting a record containing a key plus hundreds of bytes of payload can favor different strategies even when both perform the same number of key comparisons.

`sort_records` isolates this dimension with fixed-size records and a controlled key-only comparator.

## Record model

A record contains:

1. a signed 64-bit `key` used for ordering;
2. a 64-bit `ordinal` identifying its position in the original input;
3. a deterministic fixed-size payload of 64-bit words.

The supported payload-word counts are `0, 1, 3, 7, 15, 31`. The executable reports `record_bytes = sizeof(Record<Words>)`; analyses must use this observed size rather than assuming layout or padding.

Payload contents are deterministic functions of the experiment seed, original ordinal, and payload-word index. This makes corruption or accidental record/key separation detectable.

## Why keep an ordinal?

A sorted key sequence is insufficient to test stability. Stability means that records with equivalent keys remain in their original relative order.

Because each input record is assigned `ordinal = input_index`, verification can inspect every adjacent equal-key pair after sorting. `stable_on_trial=1` means all equal-key records retained their relative order on that concrete trial.

This distinction matters:

- `stable_guarantee=yes` is a property of an algorithm/implementation;
- `stable_on_trial=1` is an empirical observation for one input;
- an unstable algorithm may happen to be stable on a particular input, especially when keys are unique.

Therefore empirical stability rates should be interpreted primarily on duplicate-heavy workloads such as `few_unique`, `binary`, `all_equal`, `sawtooth`, `staggered`, and `plateau`.

## Correctness contract

A record run is accepted only if all of the following hold:

- output length equals input length;
- keys are nondecreasing;
- every original ordinal appears exactly once;
- each output record exactly matches the original key and payload associated with that ordinal.

Stable algorithms are additionally asserted to preserve relative order during the deterministic self-test.

## Timing contract

As in the scalar harness, timing and instrumentation are separate instantiations. The timed region includes only the sorting call; input generation, copying, verification, hashing, and CSV output are excluded.

Allocations performed internally by a sorting implementation remain part of elapsed time.

## Data movement

For project-controlled algorithms, the instrumentation pass counts whole-record assignments and swaps. `explicit_bytes_moved` is defined as:

`explicit_record_moves × sizeof(record)`

This is useful for comparing algorithmic data movement as element width changes. It is deliberately named **explicit** because it is not equivalent to:

- load/store instruction count;
- cache-line traffic;
- bytes transferred to DRAM;
- writeback traffic;
- compiler-elided or vectorized movement;
- library-internal movement that the harness cannot observe.

For `std::sort` and `std::stable_sort`, comparisons are observable through the comparator but internal move counts are not. Their explicit-move fields therefore remain zero rather than being estimated.

## Pairing across payload widths

For a fixed `(experiment_seed, pattern, n, trial)`, all payload widths use the same generated key sequence. The `key_hash` column identifies this shared sequence, while `input_hash` fingerprints the complete record representation for a specific payload width.

This supports two distinct paired questions:

1. algorithm A versus algorithm B at the same record width;
2. the same algorithm as record width increases while key order remains fixed.

The included record analyzer implements the first directly. Cross-width modeling is intentionally left as a derived analysis so that payload-size effects can be modeled explicitly rather than hidden inside a single aggregate.

## Research questions

The record experiment is designed to test questions such as:

- At what record widths do extra moves dominate comparison-count advantages?
- Does the radix-sort crossover move to larger `n` as records widen because each pass moves the entire payload?
- When does an out-of-place stable mergesort become less competitive because of full-record copy volume?
- How much performance is paid for a stability guarantee on duplicate-heavy data?
- Do algorithms with poor locality degrade faster as record size spans more cache lines?

The final question requires hardware counters for mechanism attribution; wall-clock and explicit-move evidence alone cannot establish a cache-miss mechanism.
