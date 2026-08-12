# Selected references

The project uses primary and foundational literature to guide both implementation questions and experimental design. This is a working bibliography, not a claim that every cited algorithm is already implemented.

1. J. L. Bentley and M. D. McIlroy, **“Engineering a Sort Function,”** *Software: Practice and Experience* 23(11), 1249–1265, 1993. DOI: `10.1002/spe.4380231105`. Robust quicksort engineering, pivot sampling, three-way partitioning, and adversarial certification.
2. D. R. Musser, **“Introspective Sorting and Selection Algorithms,”** *Software: Practice and Experience* 27(8), 983–993, 1997. DOI: `10.1002/(SICI)1097-024X(199708)27:8<983::AID-SPE117>3.0.CO;2-#`. Introduces introsort: fast partition sorting with a depth limit and worst-case stopper.
3. C. Martínez and S. Roura, **“Optimal Sampling Strategies in Quicksort and Quickselect,”** *SIAM Journal on Computing* 31(3), 683–705, 2001. DOI: `10.1137/S0097539700382108`. Analyzes pivot sampling under different comparison/exchange cost regimes.
4. S. Edelkamp and A. Weiß, **“BlockQuicksort: How Branch Mispredictions Don't Affect Quicksort,”** ESA 2016 / arXiv:`1604.06697`; journal version DOI: `10.1145/3274660`. Connects partition design with branch-misprediction cost and modern hardware behavior.
5. C. Martínez, M. E. Nebel, and S. Wild, **“Analysis of Branch Misses in Quicksort,”** arXiv:`1411.2059`. Gives analytical treatment of branch prediction effects in classical and dual-pivot quicksort variants.
6. S. Edelkamp and A. Weiß, **“Worst-Case Efficient Sorting with QuickMergesort,”** arXiv:`1811.00833`. Explores engineered hybrids that combine practical performance with stronger worst-case comparison guarantees.
7. ISO C++ working draft, **Sorting and related operations**, clauses `[alg.sort]` and `[stable.sort]`: `https://eel.is/c++draft/alg.sort`. Used for the current `std::sort` / `std::stable_sort` complexity and stability contracts; library implementation strategy remains deliberately unspecified.

## Citation policy

Theoretical claims in project documentation should be either elementary and derived in the text, linked to a standards contract, or grounded in appropriate literature. Implementation inspiration must not be confused with copied code: project implementations are original unless a file explicitly states otherwise.
