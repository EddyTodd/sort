# Selected references

The project uses primary literature, standards contracts, and production source code to guide implementation and experiment design. A citation does not imply that the cited implementation has been copied or vendored.

## Algorithm engineering and hybrids

1. J. L. Bentley and M. D. McIlroy, **“Engineering a Sort Function,”** *Software: Practice and Experience* 23(11), 1249–1265, 1993. DOI: `10.1002/spe.4380231105`.
2. D. R. Musser, **“Introspective Sorting and Selection Algorithms,”** *Software: Practice and Experience* 27(8), 983–993, 1997. DOI: `10.1002/(SICI)1097-024X(199708)27:8<983::AID-SPE117>3.0.CO;2-#`.
3. C. Martínez and S. Roura, **“Optimal Sampling Strategies in Quicksort and Quickselect,”** *SIAM Journal on Computing* 31(3), 683–705, 2001. DOI: `10.1137/S0097539700382108`.
4. S. Edelkamp and A. Weiß, **“QuickXsort: Efficient Sorting with n log n − 1.399n + o(n) Comparisons on Average,”** arXiv:`1307.3033`, 2013.
5. S. Edelkamp, A. Weiß, and S. Wild, **“QuickXsort — A Fast Sorting Scheme in Theory and Practice,”** arXiv:`1811.01259`, 2018.
6. S. Edelkamp and A. Weiß, **“BlockQuicksort: How Branch Mispredictions Don't Affect Quicksort,”** ESA 2016, DOI: `10.4230/LIPIcs.ESA.2016.38`; journal DOI: `10.1145/3274660`.
7. S. Edelkamp and A. Weiß, **“Worst-Case Efficient Sorting with QuickMergesort,”** arXiv:`1811.00833`.
8. T. Bingmann, J. Marianczuk, and P. Sanders, **“Engineering Faster Sorters for Small Sets of Items,”** arXiv:`2002.05599`, 2020.

## Run adaptivity and modern comparison sorting

9. J. I. Munro and S. Wild, **“Nearly-Optimal Mergesorts: Fast, Practical Sorting Methods That Optimally Adapt to Existing Runs,”** ESA 2018, DOI: `10.4230/LIPIcs.ESA.2018.63`.
10. O. Peters, **pattern-defeating quicksort (pdqsort)**, primary implementation repository and design notes: `https://github.com/orlp/pdqsort`.

## Hardware-aware and parallel sorting

11. **“Analysis of Branch Misses in Quicksort,”** arXiv:`1411.2059`.
12. M. Axtmann, S. Witt, D. Ferizovic, and P. Sanders, **“In-place Parallel Super Scalar Samplesort (IPS4o),”** arXiv:`1705.02257`.
13. M. Blacher, J. Giesen, P. Sanders, and J. Wassenberg, **“Vectorized and Performance-Portable Quicksort,”** *Software: Practice and Experience* 52(12), 2684–2699, 2022. DOI: `10.1002/spe.3142`; arXiv:`2205.05982`.
14. M. Axtmann, S. Witt, D. Ferizovic, and P. Sanders, **“Engineering In-place (Shared-memory) Sorting Algorithms,”** *ACM Transactions on Parallel Computing* 9(1), 2022. DOI: `10.1145/3505286`; arXiv:`2009.13569`.

## Production-source evidence for cutoff/hybrid behavior

15. OpenJDK `java.util.DualPivotQuicksort` source/tuning history. Historical/current variants explicitly use small-array insertion thresholds and different mechanisms for different domains: `https://cr.openjdk.org/~bchristi/8226297/webrev10/src/java.base/share/classes/java/util/DualPivotQuicksort.java.udiff.html`.
16. CPython `Objects/listobject.c` and list-sorting development history. Python's stable adaptive list sort uses specialized small-run/base-case logic; recent Tim Peters development notes discuss plain versus binary insertion tradeoffs when comparisons are cheap: `https://github.com/python/cpython/blob/main/Objects/listobject.c`.

## Standards contracts

17. ISO C++ working draft, sorting clauses `[alg.sort]` and `[stable.sort]`: `https://eel.is/c++draft/alg.sort`.

## v1 library mechanism references

18. Tim Peters, **`listsort.txt` / TimSort design notes**, CPython source history. The v1 `timsort` follows the stable natural-run + stack-collapse + adaptive galloping design family but is an independent C++23 implementation.
19. OpenJDK, **`java.util.TimSort`**, production reference for repaired run-stack invariants and dynamic `minGallop` behavior.
20. Peter M. McIlroy, Keith Bostic, and M. Douglas McIlroy, **“Engineering Radix Sort,”** *Computing Systems* 6(1), 5–27, 1993. Primary reference for American-flag-style in-place MSD radix distribution.
21. F. P. Preparata, **“A Fast Stable Sorting Algorithm with Absolutely Minimum Storage,”** *Theoretical Computer Science* 1(2), 185–190, 1975. DOI `10.1016/0304-3975(75)90019-5`.
22. Christian Siebert, **“Simple in-place yet comparison-optimal Mergesort,”** arXiv:`2509.24540`, 2025. Modern context for practical stable in-place merge sorting via rotation/co-ranking ideas; cited for the low-extra-memory mechanism family rather than as source code provenance.

## Citation and provenance policy

Production-source thresholds are evidence that hybridization is practical engineering, not universal constants for this repository. External baselines must pin upstream version/commit and preserve license/provenance. Project implementations are original unless a file explicitly states otherwise.
