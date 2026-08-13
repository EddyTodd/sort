# v1 algorithm catalog and coverage policy

The permanent v1 catalog is organized by **mechanism**, not by benchmark count. Comparison algorithms are generic C++23 random-access iterator/range algorithms with comparator/projection customization where practical. Distribution algorithms intentionally retain integral-domain constraints.

The older benchmark-specific registry remains in the source tree only for pre-migration research compatibility and is not installed as public API.

## Direct insertion and exchange

| Algorithm | Stable | Storage | Adaptive note |
|---|---:|---|---|
| `insertion_sort` | yes | in-place | linear on already ordered data |
| `binary_insertion_sort` | yes | in-place | reduces comparison search; movement remains quadratic |
| `selection_sort` | no | in-place | fixed quadratic scan |
| `bubble_sort` | yes | in-place | early-exit on ordered data |
| `comb_sort` | no | in-place | shrinking-gap exchange treatment |

## Shell / gapped insertion

- `shell_ciura_sort`
- `shell_pratt_sort`

The gap sequence is a scientifically material part of Shell sorting, so both representative sequences remain explicit.

## Heap

- `heap_sort` — in-place binary heapsort with `O(n log n)` worst-case comparisons.

Additional heap variants such as smoothsort are not required for v1 mechanism completeness.

## Merge families

- `merge_sort` — stable top-down merge sort with one reusable workspace per sort.
- `merge_bottom_up_sort` — stable iterative merge sort with reusable workspace.
- `natural_merge_sort` — stable natural-run adaptive merging.
- `merge_insertion_sort` — stable merge sort with configurable insertion leaves.
- `stable_inplace_merge_sort` — stable binary-partition/rotation merge sort using no linear element buffer; `O(log n)` recursion stack and `O(n log^2 n)` worst-case time.

The last algorithm is the v1 representative of the serious stable low-extra-memory merge family. Grail/Wiki-style block-buffer implementations remain useful future alternatives within an already represented mechanism.

## Quicksort and hybrid comparison sorting

- `quick_hoare_sort` — two-way Hoare partitioning.
- `quick_3way_sort` — duplicate-aware three-way partitioning.
- `quick_median3_sort` — median-of-three pivot treatment.
- `dual_pivot_sort` — two-pivot partitioning.
- `quick_insertion_sort` — configurable insertion leaves.
- `intro_sort` — median-of-three quicksort with heap fallback and insertion leaves; `O(n log n)` worst case.

These pivot-snapshot implementations require copy-constructible value types. Other move-friendly library algorithms do not inherit that requirement.

## Production adaptive stable sorting

### `timsort`

The permanent implementation includes:

- natural monotone-run discovery;
- strict descending-run reversal, so equal keys are never reversed;
- stable binary extension to minrun;
- repaired TimSort-style stack-collapse invariants and final force collapse;
- smaller-run temporary buffering;
- stable forward and backward merge directions;
- exponential-search-plus-binary-search galloping in both directions;
- stateful/dynamic `min_gallop` reward/penalty behavior.

It is an independent TimSort-style C++23 implementation, not source compatibility with CPython/OpenJDK.

### `powersort`

`powersort` uses the same production-quality generic stable merge kernel and natural-run/minrun machinery but chooses adjacent merges using integer node-power scheduling.

## Distribution sorting

| Algorithm | Domain | Stable | Storage |
|---|---|---:|---|
| `radix_lsd_sort` | integral | yes | linear buffer + histogram |
| `radix_msd_sort` | integral | no | in-place American-flag-style permutation + recursion/histograms |
| `counting_sort` | bounded integral | yes | `O(n + k)` buffer/counts |

Signed integers are mapped monotonically to unsigned key space, so minimum/maximum signed values retain numerical ordering. Counting sort refuses an observed domain larger than caller-controlled `max_domain` rather than accidentally allocating an unbounded table.

## Tiny/data-oblivious sorting

- `bitonic_sort` — generic arbitrary-`N` bitonic network-family treatment using a greatest-power-of-two merge construction.

v1 deliberately does not add an exhaustive table of fixed-`N` optimal or ISA-specialized networks. Those are valuable specialization/benchmark tracks, not a missing major sorting mechanism.

## Standard-library and external baselines

`std::sort`, `std::stable_sort`, pdqsort, and IPS4o remain comparison/reference treatments in the research layer. They are **not** wrappers exported as permanent `sortlab` algorithms, and external implementations do not become required package dependencies.

## Coverage criterion

For v1, sequential in-memory CPU sorting is mechanism-complete when the library has representative implementations for:

- insertion/exchange and Shell;
- heap;
- ordinary, natural, adaptive, hybrid, and low-extra-memory stable merge families;
- two-way, duplicate-aware, sampled-pivot, multi-pivot, and worst-case-safe quick/hybrid sorting;
- bounded counting plus stable LSD and in-place MSD integer distribution;
- a data-oblivious tiny network family.

Parallel, GPU, NUMA, distributed, and external-memory sorting use different execution/memory contracts and are explicit future domains, not missing v1 mechanisms.

The machine-readable permanent metadata lives in `sortlab::algorithm_catalog` in `<sortlab/metadata.hpp>`.
