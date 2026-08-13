# Library API

## Entry point

Most consumers should include:

```cpp
#include <sortlab/sort.hpp>
```

The package is header-only and exports the CMake target `sortlab::sortlab`.

## Comparison algorithms

Comparison algorithms accept random-access iterator pairs. Most also provide a random-access range overload.

```cpp
sortlab::merge_sort(first, last);
sortlab::merge_sort(range);
```

Comparator/projection form:

```cpp
sortlab::timsort(records, std::ranges::less{}, &record::key);
sortlab::intro_sort(records, 24, std::ranges::greater{}, &record::score);
```

The comparator is applied to projected values via `std::invoke` semantics.

## Value-type requirements

The common baseline is the C++ `std::sortable`/permutable random-access contract.

Algorithms that do not need a pivot snapshot support move-only value types when their move construction/assignment and swapping satisfy that contract. The test suite exercises move-only values through insertion, heap, merge, stable in-place merge, TimSort, and Powersort.

The quicksort family currently stores pivot values and therefore additionally requires `std::copy_constructible<value_type>`. This is an explicit API contract, not an accidental benchmark limitation.

## Stability

Stable comparison algorithms:

- insertion sort;
- binary insertion sort;
- bubble sort;
- top-down and bottom-up merge sort;
- natural merge sort;
- merge+insertion hybrid;
- stable in-place rotation merge sort;
- TimSort;
- Powersort.

Stable algorithms use comparator equivalence: neither `comp(proj(a), proj(b))` nor `comp(proj(b), proj(a))` is true.

## Adaptive options

`sortlab::adaptive_options` controls the initial TimSort/Powersort adaptive state:

```cpp
sortlab::adaptive_options options;
options.min_gallop = 7;
options.min_merge = 32;
sortlab::timsort(values, std::ranges::less{}, std::identity{}, options);
```

`min_gallop` is only the initial threshold. The production merge kernel adjusts it dynamically based on whether gallop phases are productive.

## Distribution algorithms

Distribution algorithms operate directly on integral values rather than comparators/projections.

```cpp
sortlab::radix_lsd_sort(values);       // default 8-bit digits
sortlab::radix_lsd_sort(values, 11);   // configurable digit width
sortlab::radix_msd_sort(values);       // American-flag-style in-place MSD
sortlab::counting_sort(values, 4096);  // maximum accepted observed domain
```

`radix_lsd_sort` accepts digit widths 1..12. `radix_msd_sort` accepts 1..8. Signed keys are transformed into monotonic unsigned key space so minimum/maximum signed values sort correctly.

`counting_sort` never allocates based on an unchecked full integer domain. If `max(key)-min(key)+1` exceeds `max_domain`, it throws `std::length_error`.

## Optional instrumentation

A normal algorithm call has no instrumentation argument.

The `sortlab::instrumented` namespace exposes wrappers that call the same internal implementations with an observer:

```cpp
sortlab::operation_counts counts;
sortlab::counting_observer observer(counts);
sortlab::instrumented::intro_sort(first, last, observer);
```

Observer events cover comparisons, swaps, logical writes, rotations, gallop batches, and dynamic `min_gallop` changes. The included counter is intentionally simple. Benchmark systems can provide richer observers without making measurement state part of the algorithm API.

Instrumentation events describe algorithm-level operations; they do not claim to equal cache, DRAM, branch, or energy events.

## Metadata

`sortlab::algorithm_catalog` is a constexpr array of `algorithm_info` entries describing family, stability, auxiliary-storage class, domain, adaptivity, and asymptotic complexity strings.

This metadata is suitable for API discovery and benchmark registration. It is intentionally independent of CSV schemas or campaign tooling.
