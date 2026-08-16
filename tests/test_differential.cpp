#include <sortlab/sortlab.hpp>
#include <algorithm>
#include <iostream>
#include <random>
#include <stdexcept>
#include <vector>

namespace {
void check(bool ok) { if (!ok) throw std::runtime_error("sort differential parity failed"); }

#define VERIFY(PLAIN, INSTRUMENTED) do { \
  auto a = source; auto b = source; auto expected = source; \
  std::sort(expected.begin(), expected.end()); \
  PLAIN; \
  sortlab::operation_counts counts{}; sortlab::counting_observer observer(counts); \
  INSTRUMENTED; \
  check(a == expected); check(b == expected); check(a == b); \
} while (false)

void verify(const std::vector<int>& source) {
  VERIFY(sortlab::insertion_sort(a), sortlab::instrumented::insertion_sort(b.begin(), b.end(), observer));
  VERIFY(sortlab::binary_insertion_sort(a), sortlab::instrumented::binary_insertion_sort(b.begin(), b.end(), observer));
  VERIFY(sortlab::selection_sort(a), sortlab::instrumented::selection_sort(b.begin(), b.end(), observer));
  VERIFY(sortlab::bubble_sort(a), sortlab::instrumented::bubble_sort(b.begin(), b.end(), observer));
  VERIFY(sortlab::comb_sort(a), sortlab::instrumented::comb_sort(b.begin(), b.end(), observer));
  VERIFY(sortlab::shell_ciura_sort(a), sortlab::instrumented::shell_ciura_sort(b.begin(), b.end(), observer));
  VERIFY(sortlab::shell_pratt_sort(a), sortlab::instrumented::shell_pratt_sort(b.begin(), b.end(), observer));
  VERIFY(sortlab::heap_sort(a), sortlab::instrumented::heap_sort(b.begin(), b.end(), observer));
  VERIFY(sortlab::merge_sort(a), sortlab::instrumented::merge_sort(b.begin(), b.end(), observer));
  VERIFY(sortlab::merge_bottom_up_sort(a), sortlab::instrumented::merge_bottom_up_sort(b.begin(), b.end(), observer));
  VERIFY(sortlab::natural_merge_sort(a), sortlab::instrumented::natural_merge_sort(b.begin(), b.end(), observer));
  VERIFY(sortlab::stable_inplace_merge_sort(a), sortlab::instrumented::stable_inplace_merge_sort(b.begin(), b.end(), observer));
  VERIFY(sortlab::quick_hoare_sort(a), sortlab::instrumented::quick_hoare_sort(b.begin(), b.end(), observer));
  VERIFY(sortlab::quick_3way_sort(a), sortlab::instrumented::quick_3way_sort(b.begin(), b.end(), observer));
  VERIFY(sortlab::quick_median3_sort(a), sortlab::instrumented::quick_median3_sort(b.begin(), b.end(), observer));
  VERIFY(sortlab::dual_pivot_sort(a), sortlab::instrumented::dual_pivot_sort(b.begin(), b.end(), observer));
  VERIFY(sortlab::intro_sort(a), sortlab::instrumented::intro_sort(b.begin(), b.end(), observer));
  VERIFY(sortlab::merge_insertion_sort(a, 17), sortlab::instrumented::merge_insertion_sort(b.begin(), b.end(), observer, 17));
  VERIFY(sortlab::quick_insertion_sort(a, 17), sortlab::instrumented::quick_insertion_sort(b.begin(), b.end(), observer, 17));
  VERIFY(sortlab::timsort(a), sortlab::instrumented::timsort(b, observer));
  VERIFY(sortlab::powersort(a), sortlab::instrumented::powersort(b, observer));
  VERIFY(sortlab::bitonic_sort(a), sortlab::instrumented::bitonic_sort(b.begin(), b.end(), observer));
  VERIFY(sortlab::radix_lsd_sort(a, 7), sortlab::instrumented::radix_lsd_sort(b.begin(), b.end(), observer, 7));
  VERIFY(sortlab::radix_msd_sort(a, 4), sortlab::instrumented::radix_msd_sort(b.begin(), b.end(), observer, 4));
  VERIFY(sortlab::counting_sort(a, 1024), sortlab::instrumented::counting_sort(b.begin(), b.end(), observer, 1024));
}
#undef VERIFY

std::vector<int> make(std::mt19937& rng, std::size_t n, unsigned kind) {
  std::vector<int> v(n);
  for (std::size_t i = 0; i < n; ++i) {
    if (kind == 0) v[i] = static_cast<int>(rng() % 257U) - 128;
    else if (kind == 1) v[i] = static_cast<int>(rng() % 7U) - 3;
    else if (kind == 2) v[i] = static_cast<int>(i);
    else if (kind == 3) v[i] = static_cast<int>(n - i);
    else v[i] = static_cast<int>(i);
  }
  if (kind == 4 && n > 1) for (std::size_t i = 0; i < n / 8 + 1; ++i) std::swap(v[rng() % n], v[rng() % n]);
  return v;
}
}

int main() {
  try {
    std::mt19937 rng{0xD1FF3E7u};
    for (std::size_t n : {0U,1U,2U,3U,7U,16U,31U,64U,127U,257U})
      for (unsigned kind = 0; kind < 5; ++kind) verify(make(rng, n, kind));
    for (unsigned trial = 0; trial < 20; ++trial) verify(make(rng, 1U + rng() % 384U, trial % 5));
    std::cout << "PASS: full-catalog differential and instrumentation parity\n";
  } catch (const std::exception& e) { std::cerr << "FAIL: " << e.what() << '\n'; return 1; }
}
