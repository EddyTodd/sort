# Adaptive stable merge research over records

This track extends the scalar adaptive-merge laboratory to **ordinal-carrying records with configurable inline payload width**.

The purpose is not merely to prove that the merge implementation can sort structs. It is to test whether conclusions drawn with cheap 64-bit scalar movement remain valid when every comparison moves a record whose size ranges from a key/ordinal pair to hundreds of bytes.

The experiment is intentionally factorial but split into two frozen substudies so scheduler effects are not confounded with merge-kernel effects.

## Record model

The existing record contract is reused directly:

```text
Record<Words> = key + original ordinal + inline payload[Words]
```

Supported payload widths are:

`0, 1, 3, 7, 15, 31` 64-bit words.

The benchmark always emits the ABI-specific `record_bytes`; conclusions must use that field rather than assuming a fixed object layout.

The ordinal is not a secondary sort key. Comparisons use `key` only. The ordinal exists so the verifier can test whether equal-key records retain their original relative order.

Payload words are deterministic functions of input identity and ordinal. Verification therefore checks all three properties simultaneously:

1. nondecreasing keys;
2. exact record/payload preservation;
3. equal-key ordinal order.

A row that is sorted but unstable is invalid evidence.

## Why record width is a separate experimental dimension

For a scalar key, comparison/control overhead can dominate. For a wide record, moving data can dominate even when the comparison is still one signed 64-bit integer comparison.

That changes the relative importance of several adaptive-merge decisions:

- merge scheduling changes how often records participate in merges;
- minrun extension changes how much insertion-style record shifting occurs;
- full buffering versus smaller-run buffering changes temporary record copies;
- galloping changes comparison/search control but may not reduce record movement by the same proportion;
- a treatment that wins on narrow records can lose when movement cost grows.

The project therefore does not extrapolate scalar adaptive-merge rankings to records.

## Implementation contract

`include/sortlab/adaptive_merge_records.hpp` implements the same major factors as the scalar adaptive tracks:

### Merge scheduling

- `pairwise` — exploratory/control scheduler;
- `timsort_stack` — repaired TimSort-style stack-collapse policy;
- `powersort` — node-power scheduling.

The Tier-2 record policy campaign uses `timsort_stack` and `powersort`. Pairwise remains available for correctness and exploratory work but is not part of the canonical record-policy comparison.

### Run extension

- `none`;
- `classic` fixed TimSort-style minrun;
- `balanced` variable minrun.

### Temporary-buffer strategy

- `full` — both adjacent runs are copied to reusable temporary storage;
- `smaller` — only the smaller run is copied.

For a merge of lengths `a` and `b`, the smaller-run treatment requests exactly `min(a,b)` records of merge workspace, so the requested temporary record count for that merge is at most half the combined run length.

This is a structural space statement. It is **not** a cache, bandwidth, or wall-time conclusion.

### Merge search

- `linear`;
- `gallop` with an explicit activation threshold.

Galloping uses exponential search to bracket a boundary followed by binary search inside the bracket. Thresholds are treatments rather than constants imported as folklore.

## Stability-sensitive merge directions

The smaller-run strategy requires two implementations.

### Left run buffered

The merge proceeds forward. On equal keys, the left run wins the tie so earlier input records remain earlier.

### Right run buffered

The merge proceeds backward. On equal keys, the right run is written into the later output position. This is the reverse-direction equivalent of stable left-first ordering.

Gallop boundary semantics follow the same rule:

- forward left streaks use an upper-bound boundary;
- forward right streaks use a lower-bound boundary;
- backward left streaks use an upper-bound boundary;
- backward right streaks use a lower-bound boundary.

These distinctions matter because an incorrect lower/upper-bound choice can produce a perfectly sorted array while silently reordering equal keys.

`sort_adaptive_records --self-test` therefore includes direct merge contracts that force both forward and backward smaller-run merges with duplicates.

## Instrumentation semantics

The timed path and the instrumented path are separate template instantiations.

The instrumented path reports:

- key comparisons;
- record swaps;
- explicit record moves;
- explicit bytes moved (`record_moves × record_bytes`);
- raw/effective/reversed runs;
- insertion-extension elements;
- merge count;
- scheduled weighted merge cost;
- maximum pending-run count;
- requested temporary records at peak;
- requested temporary bytes at peak;
- actual reusable-vector capacity at peak;
- temporary records copied;
- temporary bytes copied;
- gallop entries;
- records consumed by galloping.

Explicit moves are an algorithmic accounting convention. They are **not** measurements of cache-line transfers, DRAM traffic, writeback traffic, compiler-generated copies, or process RSS.

Local automatic copies used to hold an insertion key follow the same accounting convention as the existing record laboratory and are not redefined solely for this track.

## Workloads

The adaptive record suite contains duplicate-heavy, ordered, random, and run-structured probes:

- random;
- sorted;
- reversed;
- few-unique;
- binary;
- all-equal;
- nearly-sorted;
- 32-element ascending runs;
- 32-element descending runs;
- plateau;
- equal controlled runs;
- long/short controlled runs;
- power-skewed runs;
- Fibonacci-like runs;
- alternating run directions.

Duplicate-heavy patterns are mandatory because stability cannot be empirically exercised when every key is unique.

Controlled run-shape inputs reuse the exact run-geometry construction from the scalar adaptive-merge track, then attach ordinal/payload records without changing key order.

## Tier-2 campaign design

`campaigns/adaptive-records-v1.json` contains two experiments.

### 1. Scheduler/minrun factorial

The fixed factors are:

- merge kernel: full-buffer linear;
- record key type: signed 64-bit;
- payload widths: all six supported widths.

The varying factors are:

- scheduler: repaired TimSort stack vs Powersort;
- minrun: none, classic, balanced.

This is a **2 × 3** policy factorial.

The naive pairwise scheduler is excluded from canonical record-policy evidence. It remains useful as an exploratory structural control, but the record-width study should not hinge on incidental overhead in a deliberately simple baseline implementation.

### 2. Merge-kernel factorial

The fixed factors are:

- scheduler: Powersort;
- minrun: balanced;
- record key type: signed 64-bit.

The varying factors are:

- buffer: full vs smaller-run;
- search: linear vs gallop;
- gallop thresholds: `4, 7, 12, 16`.

Linear merging appears once per buffer policy. Galloping appears once per `(buffer, threshold)` pair, for **10 kernel treatments** total.

## H18 — adaptive merge policy under record width

The relative performance of repaired TimSort-stack versus Powersort scheduling, and of none/classic/balanced run extension, can change as record width increases.

A valid H18 result must:

- compare policies within the same payload width;
- use identical-key paired trials;
- retain explicit record-move/byte context;
- report width-specific effects rather than pooling widths into one ranking;
- pass stability and payload-integrity verification on every analyzed row.

## H19 — adaptive merge buffering under record width

Smaller-run buffering reduces requested temporary **record count** structurally, but the byte savings scale with `record_bytes` and the wall-time effect may be nonlinear.

A valid H19 result must compare full and smaller buffering under the same scheduler, minrun, search policy, workload, size, and payload width.

Requested temporary bytes are direct algorithmic accounting. Claims about cache or memory bandwidth still require appropriate hardware measurements.

## H20 — galloping threshold transfer across record widths

A gallop threshold selected for narrow records is not assumed to transfer to wide records.

`tools/tune_record_gallop.py` therefore selects thresholds independently for each:

```text
merge policy × minrun × buffer × pattern × n × payload width
```

Training trials satisfy `trial % 3 != 2`. Held-out trials satisfy `trial % 3 == 2`.

The selected gallop treatment is compared against linear merging under the **same** merge policy, minrun policy, buffer policy, workload, size, and payload width on the same trial/input identity.

## Analysis tools

### Paired within-width effects

```sh
python3 tools/analyze_adaptive_records.py raw.csv \
  --baseline powersort_balanced_full_linear \
  --bootstrap 5000 \
  --output summary.csv
```

The reducer refuses unverified or unstable rows.

### Payload-width effects

```sh
python3 tools/analyze_record_width_effects.py raw.csv \
  --baseline-payload-words 0 \
  --output width-effects.csv
```

Pairs are formed by treatment, workload, size, trial, and `key_hash`. `input_hash` is intentionally not used across payload widths because payload bytes differ even when the key sequence is identical.

The output reports paired time slowdown, record-move ratio, explicit-byte ratio, and temporary-byte ratios relative to the zero-payload record.

### Held-out gallop tuning

```sh
python3 tools/tune_record_gallop.py raw.csv \
  --output gallop-heldout.csv
```

No threshold chosen on training observations is evaluated on those same observations as independent evidence.

## Publication boundaries

This milestone implements the experiment and correctness contract. It does not establish that:

- Powersort is faster than TimSort-stack for wide records;
- balanced minrun is preferable for records;
- smaller-run buffering is faster;
- threshold 7 is optimal;
- galloping becomes more or less useful as records widen.

Those are empirical questions for the frozen campaign.

Cross-machine defaults require Tier-3 replication.

## Threats to validity and remaining scope

The first record-adaptive campaign intentionally keeps several dimensions fixed or out of scope:

- comparison cost is still one cheap signed-integer key comparison;
- payloads are inline fixed-size words, not strings or indirect object graphs;
- actual cache/DRAM traffic is not measured;
- dynamic TimSort-style `min_gallop` adaptation is not implemented;
- allocator/RSS effects are not equivalent to requested temporary bytes;
- SIMD, parallel, NUMA, GPU, and external-memory sorting use different measurement contracts.

These limitations are tracked in `TECHNICAL_DEBT.md` and must not be hidden by a favorable wall-time result.
