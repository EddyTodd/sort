# Sorting theory and algorithm taxonomy

## Comparison model and lower bound

For general comparison sorting of `n` distinct keys, the decision-tree model has at least `n!` leaves. Therefore a worst-case comparison sorter needs at least `ceil(log2(n!))` comparisons, which is `n log2 n - Θ(n)` by Stirling's approximation. This is the sense in which `Θ(n log n)` comparison sorts are asymptotically optimal in the comparison model.

That lower bound does **not** apply to algorithms that exploit key representation. Radix/counting families trade assumptions about the key universe, word representation, and auxiliary memory for non-comparison complexity. The repository therefore never places comparison counts and radix passes on one theoretical axis as if they were equivalent operations.

## Properties that matter beyond Big-O

### Stability

A stable sort preserves the relative order of records with equivalent keys. Stability is semantically important for multi-key pipelines and can carry memory/performance costs. Current benchmark values contain keys only; stability metadata is theoretical in this milestone. A later record-payload benchmark will verify stability empirically.

### In-place and auxiliary memory

“In-place” is used here in the practical algorithmic sense of constant auxiliary element storage, excluding recursion stack unless stated. Stack depth and external buffers are documented separately because `O(log n)` stack and `O(n)` temporary arrays have materially different cache and memory-pressure consequences.

### Adaptivity

An adaptive algorithm can exploit existing order, duplicates, or runs. Best-case asymptotics alone do not fully characterize adaptivity: a method may react strongly to duplicate entropy but not presortedness, or vice versa. Workload families intentionally probe these dimensions separately.

### Data movement

Comparisons can be cheap for integers but expensive for strings, locale-aware keys, or indirect records. Conversely, swaps/moves can dominate for large payloads. The current `int64_t` experiment is therefore one cost regime, not a universal ranking.

### Branch and cache behavior

Classical operation counts can predict poorly on modern processors when branch mispredictions, cache misses, memory bandwidth, and vectorization dominate. Research on engineered quicksort variants explicitly demonstrates the importance of branch behavior. Hardware-counter instrumentation is consequently a required later milestone rather than an optional optimization detail.

## Current algorithm table

| Algorithm | Family | Stable | In-place | Adaptive | Best | Average | Worst | Auxiliary |
|---|---|---:|---:|---:|---|---|---|---|
| insertion | insertion | yes | yes | yes | O(n) | O(n²) | O(n²) | O(1) |
| selection | selection | no | yes | no | O(n²) | O(n²) | O(n²) | O(1) |
| bubble | exchange | yes | yes | yes | O(n) | O(n²) | O(n²) | O(1) |
| shell_ciura | shell | no | yes | yes | gap-dependent | gap-dependent | gap-dependent | O(1) |
| heap | heap | no | yes | no | O(n log n) | O(n log n) | O(n log n) | O(1) |
| merge | merge | yes | no | no | O(n log n) | O(n log n) | O(n log n) | O(n) |
| quick_hoare | partition | no | yes | no | O(n log n) | O(n log n) | O(n²) | expected O(log n) stack |
| quick_3way | partition | no | yes | duplicate-adaptive | O(n) on equal keys | O(n log n) | O(n²) | expected O(log n) stack |
| intro | hybrid | no | yes | cutoff-adaptive | O(n log n) | O(n log n) | O(n log n) | O(log n) stack |
| radix_lsd | distribution | yes | no | no | O(8n) | O(8n) | O(8n) | O(n + 256) |
| std_sort | library | not guaranteed | implementation | implementation | implementation | implementation | O(n log n) comparisons | implementation |
| std_stable_sort | library | yes | implementation | implementation | implementation | implementation | standard-library contract | implementation |

The table distinguishes project-controlled algorithms from library contracts. `std::sort` and `std::stable_sort` implementation strategies are not assumed to be identical across libstdc++, libc++, MSVC STL, or versions.

## Why include deliberately slow algorithms?

Insertion, selection, and bubble sort are retained as experimental controls and pedagogical baselines. They expose different relationships among comparisons, writes, adaptivity, and input order. They are not candidates for large general-purpose arrays; the harness caps their default input size.

## Research questions enabled by the current suite

1. At what `n` does insertion sort lose to `O(n log n)` algorithms for each structure level?
2. How strongly does three-way partitioning outperform two-way partitioning as key entropy falls?
3. Where does radix sort cross over against comparison-based library sorting for 64-bit integers?
4. What data-movement premium does stable mergesort pay relative to unstable in-place methods?
5. How large is the empirical penalty of heapsort's stronger worst-case guarantee on current hardware?
6. Does custom introsort's worst-case protection materially change average performance versus plain Hoare quicksort?
7. Which conclusions survive changes in compiler, standard library, CPU microarchitecture, and optimization level?

These questions should be answered with preregistered experiment cells and confidence intervals, not one-off stopwatch runs.
