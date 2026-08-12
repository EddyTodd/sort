# Sorting theory and algorithm taxonomy

## Comparison model and lower bound

For general comparison sorting of `n` distinct keys, the decision-tree model has at least `n!` leaves. Therefore a worst-case comparison sorter needs at least `ceil(log2(n!))` comparisons, which is `n log2 n - Θ(n)` by Stirling's approximation. This is the sense in which `Θ(n log n)` comparison sorts are asymptotically optimal in the comparison model.

That lower bound does **not** apply to algorithms that exploit key representation. Radix/counting families trade assumptions about the key universe, word representation, and auxiliary memory for non-comparison complexity. The repository therefore never places comparison counts and radix passes on one theoretical axis as if they were equivalent operations.

## Sorting networks and tiny sets

A sorting network is a fixed sequence/topology of compare-exchange operations whose connectivity does not depend on input values. This data-oblivious structure is theoretically distinct from insertion sort, whose executed comparison/move count depends strongly on input order.

The canonical tiny-kernel treatment uses a bitonic sorting network on the next power-of-two size. For `N = 2^k`, the implementation has:

- comparator-gate count `N * k * (k + 1) / 4`;
- comparator depth `k * (k + 1) / 2` if independent comparators within a stage are considered parallel;
- fixed comparator topology for a given padded `N`;
- two explicit scalar stores per comparator in the project implementation, plus copy-in/copy-out for padded ranges.

For the supported powers of two this gives:

| N | gates | depth |
|---:|---:|---:|
| 2 | 1 | 1 |
| 4 | 6 | 3 |
| 8 | 24 | 6 |
| 16 | 80 | 10 |
| 32 | 240 | 15 |

These counts create testable predictions. On highly ordered data, linear insertion can execute close to linear work while the network cannot reduce its comparator schedule. On less structured tiny inputs, the network may trade more predictable control/data flow against insertion's data-dependent loop. The compiler and ISA determine whether those source-level properties become conditional moves, branches, vector instructions, or something else, so machine-level mechanism claims require direct evidence.

The bitonic treatment is **not** an optimal-comparator network. Optimal networks, SIMD/register networks, conditional-move-specialized kernels, and AlphaDev-derived sequences are separate future treatments.

## Why hybrid algorithms are theoretically reasonable

Asymptotic notation suppresses constants. Recursive `O(n log n)` algorithms create many small subproblems for which partition/recursion/merge overhead can dominate. Replacing the base case with a different sorter does not change the parent algorithm's asymptotic bound when the cutoff is bounded, but it can change constant factors substantially.

There are therefore two separate parameters:

1. **when** to stop the parent algorithm (`cutoff`);
2. **what** algorithm sorts the leaf (`leaf kernel`).

A fixed insertion cutoff answers only the first question. The H12-H13 track treats both as experimental variables and uses held-out evaluation so the same observations are not used to select and validate the pair.

## Properties that matter beyond Big-O

### Stability

A stable sort preserves the relative order of records with equivalent keys. Stability is semantically important for multi-key pipelines and can carry memory/performance costs. `sort_records` verifies stability empirically by retaining each record's original ordinal and testing duplicate-heavy inputs.

### In-place and auxiliary memory

“In-place” is used here in the practical algorithmic sense of constant auxiliary element storage, excluding recursion stack unless stated. Stack depth and external buffers are documented separately because `O(log n)` stack and `O(n)` temporary arrays have materially different cache and memory-pressure consequences. The allocation harness separately measures ordinary C++ allocation behavior rather than inferring it from asymptotic notation.

### Adaptivity

An adaptive algorithm can exploit existing order, duplicates, or runs. Best-case asymptotics alone do not fully characterize adaptivity: a method may react strongly to duplicate entropy but not presortedness, or vice versa. Workload families intentionally probe these dimensions separately.

Insertion sort and a fixed sorting network illustrate this difference sharply: insertion's executed work can collapse on ordered input, whereas a sorting network's comparator topology is fixed for a chosen size.

### Data movement

Comparisons can be cheap for integers but expensive for strings, locale-aware keys, or indirect records. Conversely, swaps/moves can dominate for large payloads. `sort_records` therefore varies payload width and reports explicit record movement for project-controlled implementations instead of treating the scalar `int64_t` ranking as universal.

### Branch and cache behavior

Classical operation counts can predict poorly on modern processors when branch mispredictions, cache misses, instruction-cache footprint, memory bandwidth, and vectorization dominate. The repository includes direct Linux hardware-counter experiments for cycles, instructions, branches, branch misses, cache references, and cache misses. Wall time alone is not accepted as evidence for a causal hardware explanation.

A particularly important distinction for tiny sorters is **isolated kernel performance versus integration performance**. A larger or more specialized tiny kernel may win when called alone yet lose when embedded in a parent algorithm because instruction-cache pressure and surrounding control flow differ. H13 exists specifically to prevent those two claims from being conflated.

## Algorithm coverage

The portable scalar harness currently contains 23 top-level algorithms/variants spanning insertion, exchange/selection, Shell, heap, merge, quicksort, hybrid, radix/distribution, and standard-library families. The record, cutoff, tiny-kernel, allocation, hardware, portfolio, and provenance-pinned external tracks are deliberately separate factorial experiments rather than being flattened into one enormous leaderboard.

See `docs/algorithm-catalog.md` and `sort_lab --list-algorithms` for the current machine-readable catalog. External baselines currently include pinned pdqsort variants and sequential IPS4o; architecture-specific and parallel families use distinct experiment contracts.

## Why include deliberately slow algorithms?

Insertion, selection, and bubble sort are retained as experimental controls and pedagogical baselines. They expose different relationships among comparisons, writes, adaptivity, and input order. They are not candidates for large general-purpose arrays; the harness caps their default input size.

## Research questions enabled by the current suite

1. Where is the direct insertion-versus-`O(n log n)` crossover for each input structure?
2. Is linear insertion, binary insertion, a sorting network, or `std::sort` best for each tiny-set domain?
3. Does the best isolated tiny kernel remain best when integrated into merge, quicksort, or introsort?
4. How much does the optimal leaf cutoff move when the leaf algorithm changes?
5. How strongly does three-way partitioning outperform two-way partitioning as key entropy falls?
6. Where does radix sort cross over against comparison-based library sorting for 64-bit integers, and how does radix digit width change the frontier?
7. How do record width and stability requirements reshape algorithm rankings?
8. How large is the empirical penalty of heapsort's stronger worst-case guarantee, and do direct counters support branch/cache explanations?
9. Can a held-out feature-based portfolio beat the best fixed sorter after paying feature-probe cost?
10. Do modern pinned external implementations such as pdqsort and IPS4o move the observed frontier relative to project and standard-library controls?
11. Which conclusions survive changes in compiler, standard library, CPU microarchitecture, and optimization level?

These questions should be answered with preregistered experiment cells, paired evidence, uncertainty intervals, and replication—not one-off stopwatch runs.
