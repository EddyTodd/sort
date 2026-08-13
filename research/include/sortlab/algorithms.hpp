#pragma once

#include "sortlab/common.hpp"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <functional>
#include <string_view>
#include <vector>

namespace sortlab {

template <bool Count>
static void insertion_sort(std::vector<Value>& a, Stats& stats) {
  for (std::size_t i = 1; i < a.size(); ++i) {
    const Value key = a[i];
    std::size_t j = i;
    while (j > 0) {
      if (!lessv<Count>(key, a[j - 1], stats)) break;
      writev<Count>(a[j], a[j - 1], stats);
      --j;
    }
    writev<Count>(a[j], key, stats);
  }
}

template <bool Count>
static void selection_sort(std::vector<Value>& a, Stats& stats) {
  for (std::size_t i = 0; i < a.size(); ++i) {
    std::size_t minimum = i;
    for (std::size_t j = i + 1; j < a.size(); ++j) {
      if (lessv<Count>(a[j], a[minimum], stats)) minimum = j;
    }
    swapv<Count>(a[i], a[minimum], stats);
  }
}

template <bool Count>
static void bubble_sort(std::vector<Value>& a, Stats& stats) {
  for (std::size_t n = a.size(); n > 1; --n) {
    bool changed = false;
    for (std::size_t i = 1; i < n; ++i) {
      if (lessv<Count>(a[i], a[i - 1], stats)) {
        swapv<Count>(a[i], a[i - 1], stats);
        changed = true;
      }
    }
    if (!changed) break;
  }
}

static std::vector<std::size_t> shell_gaps(std::size_t n) {
  static constexpr std::size_t ciura[] = {1, 4, 10, 23, 57, 132, 301, 701, 1750};
  std::vector<std::size_t> gaps;
  for (const auto gap : ciura) {
    if (gap < n) gaps.push_back(gap);
  }
  std::size_t gap = 1750;
  while (gap < n / 2) {
    gap = static_cast<std::size_t>(static_cast<double>(gap) * 2.25);
    if (gap < n) gaps.push_back(gap);
  }
  std::sort(gaps.begin(), gaps.end(), std::greater<>());
  return gaps;
}

template <bool Count>
static void shell_sort(std::vector<Value>& a, Stats& stats) {
  for (const auto gap : shell_gaps(a.size())) {
    for (std::size_t i = gap; i < a.size(); ++i) {
      const Value value = a[i];
      std::size_t j = i;
      while (j >= gap && lessv<Count>(value, a[j - gap], stats)) {
        writev<Count>(a[j], a[j - gap], stats);
        j -= gap;
      }
      writev<Count>(a[j], value, stats);
    }
  }
}

template <bool Count>
static void sift_down(std::vector<Value>& a, std::size_t base, std::size_t root,
                      std::size_t n, Stats& stats) {
  for (;;) {
    std::size_t child = root * 2 + 1;
    if (child >= n) return;
    if (child + 1 < n && lessv<Count>(a[base + child], a[base + child + 1], stats)) ++child;
    if (!lessv<Count>(a[base + root], a[base + child], stats)) return;
    swapv<Count>(a[base + root], a[base + child], stats);
    root = child;
  }
}

template <bool Count>
static void heap_sort_range(std::vector<Value>& a, std::size_t lo, std::size_t hi, Stats& stats) {
  const std::size_t n = hi - lo;
  for (std::size_t i = n / 2; i > 0; --i) sift_down<Count>(a, lo, i - 1, n, stats);
  for (std::size_t remaining = n; remaining > 1; --remaining) {
    swapv<Count>(a[lo], a[lo + remaining - 1], stats);
    sift_down<Count>(a, lo, 0, remaining - 1, stats);
  }
}

template <bool Count>
static void heap_sort(std::vector<Value>& a, Stats& stats) {
  heap_sort_range<Count>(a, 0, a.size(), stats);
}

template <bool Count>
static void merge_rec(std::vector<Value>& a, std::vector<Value>& tmp, std::size_t lo,
                      std::size_t hi, Stats& stats) {
  if (hi - lo < 2) return;
  const std::size_t mid = lo + (hi - lo) / 2;
  merge_rec<Count>(a, tmp, lo, mid, stats);
  merge_rec<Count>(a, tmp, mid, hi, stats);
  std::size_t i = lo;
  std::size_t j = mid;
  std::size_t k = lo;
  while (i < mid && j < hi) {
    if (lessv<Count>(a[j], a[i], stats)) writev<Count>(tmp[k++], a[j++], stats);
    else writev<Count>(tmp[k++], a[i++], stats);
  }
  while (i < mid) writev<Count>(tmp[k++], a[i++], stats);
  while (j < hi) writev<Count>(tmp[k++], a[j++], stats);
  for (k = lo; k < hi; ++k) writev<Count>(a[k], tmp[k], stats);
}

template <bool Count>
static void merge_sort(std::vector<Value>& a, Stats& stats) {
  std::vector<Value> tmp(a.size());
  merge_rec<Count>(a, tmp, 0, a.size(), stats);
}

template <bool Count>
static std::size_t hoare_partition(std::vector<Value>& a, std::size_t lo, std::size_t hi,
                                   Stats& stats) {
  const Value pivot = a[lo + (hi - lo) / 2];
  std::size_t i = lo;
  std::size_t j = hi - 1;
  for (;;) {
    while (i < hi && lessv<Count>(a[i], pivot, stats)) ++i;
    while (j > lo && lessv<Count>(pivot, a[j], stats)) --j;
    if (i >= j) return i;
    swapv<Count>(a[i], a[j], stats);
    ++i;
    if (j > 0) --j;
  }
}

template <bool Count>
static void quick_rec(std::vector<Value>& a, std::size_t lo, std::size_t hi, Stats& stats) {
  while (hi - lo > 1) {
    std::size_t cut = hoare_partition<Count>(a, lo, hi, stats);
    if (cut <= lo) cut = lo + 1;
    if (cut >= hi) cut = hi - 1;
    if (cut - lo < hi - cut) {
      quick_rec<Count>(a, lo, cut, stats);
      lo = cut;
    } else {
      quick_rec<Count>(a, cut, hi, stats);
      hi = cut;
    }
  }
}

template <bool Count>
static void quick_sort(std::vector<Value>& a, Stats& stats) {
  if (!a.empty()) quick_rec<Count>(a, 0, a.size(), stats);
}

template <bool Count>
static void quick3_rec(std::vector<Value>& a, std::size_t lo, std::size_t hi, Stats& stats) {
  while (hi - lo > 1) {
    const Value pivot = a[lo + (hi - lo) / 2];
    std::size_t lt = lo;
    std::size_t i = lo;
    std::size_t gt = hi;
    while (i < gt) {
      if (lessv<Count>(a[i], pivot, stats)) {
        swapv<Count>(a[lt++], a[i++], stats);
      } else if (lessv<Count>(pivot, a[i], stats)) {
        swapv<Count>(a[i], a[--gt], stats);
      } else {
        ++i;
      }
    }
    if (lt - lo < hi - gt) {
      quick3_rec<Count>(a, lo, lt, stats);
      lo = gt;
    } else {
      quick3_rec<Count>(a, gt, hi, stats);
      hi = lt;
    }
  }
}

template <bool Count>
static void quick3_sort(std::vector<Value>& a, Stats& stats) {
  quick3_rec<Count>(a, 0, a.size(), stats);
}

template <bool Count>
static void insertion_range(std::vector<Value>& a, std::size_t lo, std::size_t hi, Stats& stats) {
  for (std::size_t i = lo + 1; i < hi; ++i) {
    const Value key = a[i];
    std::size_t j = i;
    while (j > lo && lessv<Count>(key, a[j - 1], stats)) {
      writev<Count>(a[j], a[j - 1], stats);
      --j;
    }
    writev<Count>(a[j], key, stats);
  }
}

template <bool Count>
static void intro_rec(std::vector<Value>& a, std::size_t lo, std::size_t hi,
                      unsigned depth_limit, Stats& stats) {
  constexpr std::size_t insertion_cutoff = 24;
  while (hi - lo > insertion_cutoff) {
    if (depth_limit == 0) {
      heap_sort_range<Count>(a, lo, hi, stats);
      return;
    }
    --depth_limit;
    std::size_t cut = hoare_partition<Count>(a, lo, hi, stats);
    if (cut <= lo) cut = lo + 1;
    if (cut >= hi) cut = hi - 1;
    if (cut - lo < hi - cut) {
      intro_rec<Count>(a, lo, cut, depth_limit, stats);
      lo = cut;
    } else {
      intro_rec<Count>(a, cut, hi, depth_limit, stats);
      hi = cut;
    }
  }
  insertion_range<Count>(a, lo, hi, stats);
}

template <bool Count>
static void intro_sort(std::vector<Value>& a, Stats& stats) {
  if (a.size() < 2) return;
  const auto bits = std::bit_width(a.size());
  const unsigned depth = static_cast<unsigned>(2 * (bits - 1));
  intro_rec<Count>(a, 0, a.size(), depth, stats);
}

template <bool Count>
static void radix_lsd_sort(std::vector<Value>& a, Stats& stats) {
  if (a.size() < 2) return;
  std::vector<Value> tmp(a.size());
  constexpr std::uint64_t sign = std::uint64_t{1} << 63;
  for (unsigned pass = 0; pass < 8; ++pass) {
    std::size_t count[256]{};
    const unsigned shift = pass * 8;
    for (const Value value : a) {
      const auto key = static_cast<std::uint64_t>(value) ^ sign;
      ++count[(key >> shift) & 0xffU];
    }
    std::size_t position[256]{};
    for (std::size_t i = 1; i < 256; ++i) position[i] = position[i - 1] + count[i - 1];
    for (const Value value : a) {
      const auto key = static_cast<std::uint64_t>(value) ^ sign;
      const auto bucket = static_cast<std::size_t>((key >> shift) & 0xffU);
      writev<Count>(tmp[position[bucket]++], value, stats);
    }
    a.swap(tmp);
  }
}

template <bool Count>
static void std_sort_impl(std::vector<Value>& a, Stats& stats) {
  std::sort(a.begin(), a.end(), [&](Value x, Value y) {
    if constexpr (Count) ++stats.comparisons;
    return x < y;
  });
}

template <bool Count>
static void std_stable_sort_impl(std::vector<Value>& a, Stats& stats) {
  std::stable_sort(a.begin(), a.end(), [&](Value x, Value y) {
    if constexpr (Count) ++stats.comparisons;
    return x < y;
  });
}

using SortFn = void (*)(std::vector<Value>&, Stats&);

struct Algorithm {
  std::string_view name;
  std::string_view family;
  SortFn timed;
  SortFn instrumented;
  bool quadratic;
  bool comparison_based;
  std::string_view stable;
  std::string_view in_place;
  std::string_view adaptive;
  std::string_view best;
  std::string_view average;
  std::string_view worst;
  std::string_view auxiliary;
};

#define ALGORITHM(NAME, FAMILY, FN, QUAD, CMP, STABLE, INPLACE, ADAPT, BEST, AVG, WORST, AUX) \
  Algorithm{NAME, FAMILY, FN<false>, FN<true>, QUAD, CMP, STABLE, INPLACE, ADAPT, BEST, AVG, WORST, AUX}

static const std::vector<Algorithm>& algorithms() {
  static const std::vector<Algorithm> table = {
      ALGORITHM("insertion", "insertion", insertion_sort, true, true, "yes", "yes", "yes",
                "O(n)", "O(n^2)", "O(n^2)", "O(1)"),
      ALGORITHM("selection", "selection", selection_sort, true, true, "no", "yes", "no",
                "O(n^2)", "O(n^2)", "O(n^2)", "O(1)"),
      ALGORITHM("bubble", "exchange", bubble_sort, true, true, "yes", "yes", "yes",
                "O(n)", "O(n^2)", "O(n^2)", "O(1)"),
      ALGORITHM("shell_ciura", "shell", shell_sort, false, true, "no", "yes", "yes",
                "gap-dependent", "gap-dependent", "gap-dependent", "O(1)"),
      ALGORITHM("heap", "heap", heap_sort, false, true, "no", "yes", "no",
                "O(n log n)", "O(n log n)", "O(n log n)", "O(1)"),
      ALGORITHM("merge", "merge", merge_sort, false, true, "yes", "no", "no",
                "O(n log n)", "O(n log n)", "O(n log n)", "O(n)"),
      ALGORITHM("quick_hoare", "quick", quick_sort, false, true, "no", "yes", "no",
                "O(n log n)", "O(n log n)", "O(n^2)", "O(log n) expected stack"),
      ALGORITHM("quick_3way", "quick", quick3_sort, false, true, "no", "yes", "duplicates",
                "O(n) on all-equal", "O(n log n)", "O(n^2)", "O(log n) expected stack"),
      ALGORITHM("intro", "hybrid", intro_sort, false, true, "no", "yes", "cutoff",
                "O(n log n)", "O(n log n)", "O(n log n)", "O(log n) stack"),
      ALGORITHM("radix_lsd", "distribution", radix_lsd_sort, false, false, "yes", "no", "no",
                "O(8n)", "O(8n)", "O(8n)", "O(n + 256)"),
      ALGORITHM("std_sort", "library", std_sort_impl, false, true, "not guaranteed", "implementation", "implementation",
                "implementation-defined", "implementation-defined", "O(n log n) comparisons", "implementation-defined"),
      ALGORITHM("std_stable_sort", "library", std_stable_sort_impl, false, true, "yes", "implementation", "implementation",
                "implementation-defined", "implementation-defined", "O(n log^2 n) comparisons fallback", "implementation-defined")};
  return table;
}

#undef ALGORITHM

}  // namespace sortlab
