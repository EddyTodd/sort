# Workload taxonomy and adversarial probes

The project uses synthetic workloads as **controlled probes**, not as claims about the frequency of real production data. A useful benchmark suite must vary order, entropy, duplicate density, local run structure, and partition shape instead of treating uniform random data as universal.

## Current families

| Workload | Construction | Primary property probed |
|---|---|---|
| `random` | bounded deterministic pseudo-random keys | high-entropy baseline |
| `sorted` | ascending random sample | presorted behavior and adaptivity |
| `reversed` | descending random sample | opposite monotone order |
| `few_unique` | eight keys | duplicate-heavy partition behavior |
| `binary` | two keys | extreme low entropy without complete equality |
| `all_equal` | one key | degenerate duplicate case |
| `nearly_sorted` | sorted sample with about 1% swaps | adaptivity to small disorder |
| `organ_pipe` | rises to center, then falls | symmetric partition stress |
| `sawtooth` | periodic 32-key sequence | periodic duplicates/local structure |
| `runs` | independent ascending runs of length 32 | existing ordered runs |
| `descending_runs` | independent descending runs of length 32 | locally reversed runs |
| `rotated` | globally sorted then rotated by roughly one third | one discontinuity in otherwise sorted data |
| `alternating_extremes` | alternating minimum/maximum ranks | repeated extreme oscillation |
| `staggered` | modular arithmetic periodic sequence | deterministic non-random permutation/duplicates |
| `plateau` | organ-pipe shape capped at a plateau | large equal region plus gradients |

## “Adversarial” does not mean universally worst-case

An input is adversarial relative to a particular algorithm and implementation. For example, all-equal data is favorable for a well-designed three-way partition but can expose unnecessary work in a two-way partition. A rotated or organ-pipe sequence may stress one pivot policy while being unremarkable for another.

The repository therefore avoids naming a workload “quicksort killer” unless it is derived for the exact pivot/partition implementation under test and accompanied by a documented construction argument.

## Experimental use

Workloads should be analyzed as separate strata. Do not average all 15 families into one headline runtime: such an average assigns an arbitrary prior probability to each synthetic pattern and can hide important interactions.

For controlled comparisons:

- use the same pattern definition and seed across competitors;
- inspect the entire size curve;
- pair algorithms on the exact same generated input;
- add denser sizes around suspected crossovers;
- replicate surprising algorithm × workload interactions;
- distinguish a deliberately constructed stress case from a representative production distribution.

## Future workload research

Candidate additions include implementation-specific pivot killers, Zipf-like key frequencies, configurable run-length distributions, inversion-count-controlled permutations, partially sorted blocks, heavy-tailed values, real trace-derived key sets, and external-memory distributions. Each new family should state the property it isolates and avoid redundant aliases for existing probes.
