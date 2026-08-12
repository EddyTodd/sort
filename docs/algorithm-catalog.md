# Algorithm catalog and coverage policy

The project does not equate "more algorithms" with better research. The catalog is organized by algorithmic mechanism so empirical comparisons cover materially different tradeoffs rather than dozens of cosmetic variants.

## Implemented scalar algorithms

The canonical scalar harness contains 23 implementations/variants.

| Mechanism | Implementations |
|---|---|
| insertion | insertion, binary insertion |
| selection/exchange | selection, bubble, comb |
| Shell | Ciura-gap Shell, Pratt-gap Shell |
| heap | project heapsort, `std` heap baseline |
| merge | top-down merge, bottom-up merge, natural/run-adaptive merge |
| quick | two-way Hoare, three-way, median-of-three, dual-pivot |
| hybrid | introsort, merge+insertion cutoff 24, median-three quicksort+insertion cutoff 24 |
| distribution | 8-bit LSD radix, 11-bit-digit LSD radix |
| library | `std::sort`, `std::stable_sort` |

The standalone cutoff harness treats the insertion threshold as a parameter instead of assuming 24 is universally optimal.

## Tiny-kernel and hybrid-leaf track

The small-set track is intentionally separate from the 23 top-level scalar algorithms because its treatments have an explicit domain (`n <= 32`) and are studied primarily as base cases for larger algorithms.

`sort_tiny` implements:

- linear insertion;
- binary insertion;
- a padded power-of-two bitonic sorting network;
- `std::sort` as a direct small-set control.

`sort_leaf_hybrids` then factorially combines the three project-controlled leaf kernels with merge sort, median-of-three quicksort, and introsort while independently varying the cutoff. This makes **leaf algorithm** and **leaf size** separate experimental variables.

The bitonic network is not labelled optimal. Optimal-size networks, SIMD/register sorting networks, conditional-move-specialized code, and AlphaDev-derived sequences remain distinct future comparison treatments. See `docs/tiny-kernel-research.md`.

## Adaptive stable merge-policy track

`sort_merge_policies` studies stable natural mergesort as a factorial design instead of treating “TimSort” or “Powersort” as indivisible labels.

Merge scheduling treatments:

- adjacent pairwise rounds;
- the repaired TimSort run-stack policy;
- Powersort's node-power schedule.

Minrun treatments:

- no artificial extension;
- classic fixed TimSort-style minrun;
- a balanced variable minrun sequence matching the current CPython design principle.

All nine combinations use the same run detector, stable binary extension, and stable two-run merge kernel. This is deliberate: it isolates merge scheduling and run sizing from galloping and other merge-kernel optimizations.

The track also includes five controlled run-shape families and an exact optimal alphabetic merge-cost model for up to 64 runs. See `docs/adaptive-merge-research.md`.

These implementations are not inserted into the frozen 23-algorithm `canonical-v1` campaign retroactively. They have a separate `merge-policies-v1` Tier-2 contract; integration into a later unified portfolio requires a new versioned campaign.

## Adaptive merge-kernel track

`sort_merge_kernels` studies the next independent layer of the adaptive merge stack. It freezes **Powersort scheduling + balanced variable minrun** as a controlled reference context, then varies the mechanism used to merge two already-selected adjacent runs.

Temporary-buffer treatments:

- `full`: copy both runs into reusable workspace;
- `smaller`: copy only the smaller run and merge forward or backward as required.

Search treatments:

- `linear`: ordinary element-by-element stable merge;
- `gallop`: after a consecutive-win threshold, use exponential search to bracket a boundary followed by binary search, then copy the discovered block.

The canonical campaign treats gallop threshold as a tunable parameter (`4, 7, 12, 16`) rather than creating a permanent top-level sorter name for every threshold. `tools/tune_gallop.py` selects the threshold on training trials and evaluates it held-out against linear merging under the same buffer policy.

The smaller-run buffer has a structural per-merge workspace bound of `min(a,b) <= floor((a+b)/2)`. That fact is reported separately from empirical wall time, cache behavior, or memory-bandwidth claims.

This track has its own `merge-kernels-v1` Tier-2 contract and H16-H17 evidence requirements. It does not modify the frozen H14-H15 scheduler/minrun experiment. See `docs/adaptive-merge-kernels.md`.

## Implemented record algorithms

The record-width laboratory intentionally uses a smaller representative set: insertion, heap, stable merge, two-way quicksort, three-way quicksort, introsort, stable radix, `std::sort`, and `std::stable_sort`. The goal is factorial control over payload width and stability, not duplicating every scalar variant for every record type.

## Provenance-pinned external track

The first external comparison track is implemented as an opt-in build and a separate Tier-2 campaign. It currently contains:

- `orlp/pdqsort` pinned to `b1ef26a55cdb60d236a5cb199c4234c704f46726`: `pdqsort` and `pdqsort_branchless`;
- `ips4o/ips4o` pinned to `08a5b926ee65cef19139057c6bde02bb5542c1cb`: sequential `ips4o::sort`.

These are measured in `sort_external` beside paired internal/library controls on the exact same generated inputs. Upstream source is not vendored into Git history; the bootstrap/provenance tools verify full commit, checkout cleanliness, required paths, license file hash, and Git tree identity before canonical evidence is collected. See `docs/external-baselines.md`.

## Important external/state-of-the-art families

The following remain part of the research universe even when they are not vendored into the core executable:

- Peeksort and multiway Powersort as additional stable merge-policy research;
- BlockQuicksort: branch-misprediction-aware partitioning;
- QuickXsort / QuickMergesort: theoretically analyzed combinations of partitioning with another sorting method;
- VQSort: vectorized, architecture-portable quicksort;
- in-place stable block merges such as WikiSort/Grail-style methods;
- counting, bucket, American-flag/MSD radix, and other bounded-domain distribution sorts;
- optimal, SIMD/register, and architecture-specific tiny sorting networks;
- parallel merge/sample/radix sorts, including parallel IPS4o as its own experiment model;
- external-memory and NUMA-aware sorting.

These are not silently labelled "missing." They are separate comparison tracks because several require external code, architecture-specific intrinsics, parallel runtimes, different input contracts, or materially different memory models. Every adapter must pin the upstream version and preserve license/provenance rather than copying an arbitrary implementation into the benchmark.

## Variant policy

A variant gets its own algorithm name when it changes a scientifically material parameter, including:

- pivot selection or number of pivots;
- duplicate partitioning strategy;
- Shell gap sequence;
- radix digit width;
- insertion/base-case cutoff;
- leaf algorithm;
- merge order/run policy;
- minrun/run-extension policy;
- temporary-buffer strategy;
- merge search/galloping strategy;
- stability or in-place guarantee;
- branchless/vectorized partitioning;
- parallelism or memory strategy.

Continuous/tunable parameters such as cutoffs and gallop thresholds should normally live in a tuning experiment rather than creating hundreds of permanent algorithm names.

## Completion criterion

The core catalog is considered representative when every major mechanism above has at least one controlled implementation or an explicitly versioned external-baseline track. The research objective is then to characterize domains of superiority and crossover regions—not to declare one universal winner.

Known incomplete comparison tracks are listed explicitly in `TECHNICAL_DEBT.md`.
