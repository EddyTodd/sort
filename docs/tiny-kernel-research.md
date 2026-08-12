# Tiny-sort kernels and hybrid base cases

Insertion sort is a common base case in practical hybrid sorters, but that does not make it a universal optimum. Small-array sorting is dominated by constants, control flow, code size, comparison cost, and data movement rather than asymptotic growth alone.

This track asks a narrower and more defensible question: **for a specific machine/compiler/workload domain, which tiny sorter should a larger hybrid invoke, and at what cutoff?**

## Direct kernels

`sort_tiny` measures four treatments on identical deterministic inputs for `n <= 32`:

- linear insertion sort;
- binary insertion sort;
- a data-oblivious bitonic comparator topology padded to the next power of two;
- `std::sort` as a production-library control.

The bitonic implementation is deliberately described as a **bitonic network**, not an optimal sorting network. Its padded power-of-two gate counts are fixed by construction:

| padded n | comparator gates |
|---:|---:|
| 2 | 1 |
| 4 | 6 |
| 8 | 24 |
| 16 | 80 |
| 32 | 240 |

For non-power-of-two sizes the scalar values are copied into the next supported network with `INT64_MAX` padding and the first `n` sorted outputs are copied back. This preserves the multiset for the scalar experiment, including when real keys equal `INT64_MAX`.

The network has a data-independent comparator topology, but the project does **not** claim the generated machine code is branch-free on every compiler/ISA. Any branch-misprediction explanation requires direct hardware evidence.

## Why this is scientifically distinct from the insertion-cutoff study

The existing cutoff experiment varied only the insertion threshold. That can answer "which insertion cutoff works best?" but cannot answer "is insertion the right leaf algorithm?"

`sort_leaf_hybrids` therefore factorially varies:

- parent family: merge, median-of-three quicksort, introsort;
- leaf kernel: insertion, binary insertion, bitonic network;
- cutoff: an explicit user-supplied grid, capped at 32 whenever the bitonic kernel is present.

The parent algorithms are otherwise held fixed. A treatment is identified by the complete `(family, kernel, cutoff)` tuple.

## Preregistered hypotheses

**H12 — tiny-kernel crossover.** A data-oblivious sorting-network kernel may outperform insertion sort for some small random/unstructured domains, while insertion should retain an advantage on sufficiently ordered inputs because its work adapts to existing order. The crossover is expected to depend on compiler and microarchitecture and is not assumed to match published thresholds.

**H13 — leaf-kernel integration effect.** A tiny kernel that wins in isolation may fail to improve a larger hybrid because code footprint, instruction-cache pressure, partition/merge context, and leaf-size distribution change the effective cost. Therefore direct-kernel results and integrated-hybrid results are separate outcomes.

These are hypotheses, not conclusions.

## Held-out tuning rule

`tools/tune_leaf_kernels.py` prevents two common forms of overclaiming:

1. it selects `(kernel, cutoff)` using training trials only;
2. its held-out baseline is not a fixed folklore insertion cutoff—it is the **best insertion-only cutoff selected from the same training observations**.

This means a network/hybrid result must beat a fairly tuned insertion baseline rather than an intentionally weak constant.

The deterministic split is the same convention used elsewhere in the repository: trials with `trial % 3 == 2` are held out; all others are training observations.

## Direct statistical reduction

`tools/analyze_tiny.py` pairs kernels by `(pattern, n, trial, input_hash)` and reports:

- median elapsed time;
- median logical comparisons;
- median explicit writes where observable;
- paired median speedup versus insertion;
- percentile-bootstrap 95% interval for paired speedup;
- paired win rate.

No automatic outlier deletion is performed.

## Evidence campaign

`campaigns/tiny-kernels-v1.json` contains two Tier-2 experiments:

1. `tiny-direct`: all integer sizes 2 through 32, all 15 workload families, 101 trials, two repetitions;
2. `leaf-hybrids-coarse`: a preregistered coarse size/cutoff/workload grid for the three parent families, 31 trials, two repetitions.

A denser follow-up around a discovered crossover is allowed only after the coarse campaign is complete and a new campaign specification is frozen. The denser run must not replace the coarse artifact.

## Interpretation limits

A direct tiny-kernel speedup does not prove that the same kernel is the best hybrid base case. A hybrid speedup does not prove a branch/cache mechanism. Conversely, a network losing overall does not imply its comparator topology is intrinsically inferior; code generation, padding, copying, and the non-optimal bitonic schedule are part of this implementation treatment.

The project specifically distinguishes this canonical bitonic treatment from future tracks involving optimal networks, architecture-specific conditional-move networks, SIMD register networks, or AlphaDev-derived assembly sequences.

## Literature context

The small-set research track is motivated in part by Bingmann, Marianczuk, and Sanders, *Engineering Faster Sorters for Small Sets of Items* (arXiv:2002.05599). Their work is evidence that insertion sort is a hypothesis-worthy baseline rather than an axiom, and also that integrating a fast small-set sorter into a larger algorithm can behave differently from benchmarking it in isolation. The project re-tests those ideas with its own implementations, workload contract, compilers, and hardware rather than importing their numerical conclusions.
