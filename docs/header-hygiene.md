# Public-header hygiene

`sortlab` is header-only, so public-header independence is part of the library's core correctness and usability contract.

The 18 installed headers are now carried by the `sortlab::sortlab` target as a CMake `public_headers` file set. When `SORTLAB_BUILD_TESTS=ON`, CMake reads that file set directly and generates one translation unit per header. Each generated source contains only that sortlab include, and the `sortlab-header-self-containment` object target compiles all of them.

This includes the installed `detail/` headers because they are part of the current supported package surface and are transitively required by public algorithms. Each must therefore parse without relying on another sortlab header having been included first.

The check complements `sortlab.package-consumer`: self-containment validates every header independently, while the external consumer validates the installed INTERFACE target and full package layout.
