#pragma once

#include "sortlab/algorithms.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace sortlab {

template <bool Count>
static void binary_insertion_range(std::vector<Value>& values, std::size_t lo,
                                   std::size_t hi, Stats& stats) {
  if (hi - lo < 2) return;

  for (std::size_t i = lo + 1; i < hi; ++i) {
    const Value value = values[i];
    std::size_t left = lo;
    std::size_t right = i;
    while (left < right) {
      const std::size_t mid = left + (right - left) / 2;
      if (lessv<Count>(value, values[mid], stats)) {
        right = mid;
      } else {
        left = mid + 1;
      }
    }
    for (std::size_t j = i; j > left; --j) {
      writev<Count>(values[j], values[j - 1], stats);
    }
    writev<Count>(values[left], value, stats);
  }
}

template <bool Count>
static void binary_insertion_sort(std::vector<Value>& values, Stats& stats) {
  binary_insertion_range<Count>(values, 0, values.size(), stats);
}

template <bool Count>
static void comb_sort(std::vector<Value>& values, Stats& stats) {
  std::size_t gap = values.size();
  bool swapped = true;
  while (gap > 1 || swapped) {
    gap = (gap * 10) / 13;
    if (gap < 1) gap = 1;
    swapped = false;
    for (std::size_t i = 0; i + gap < values.size(); ++i) {
      if (lessv<Count>(values[i + gap], values[i], stats)) {
        swapv<Count>(values[i], values[i + gap], stats);
        swapped = true;
      }
    }
  }
}

static std::vector<std::size_t> pratt_gaps(std::size_t n) {
  std::vector<std::size_t> gaps;
  for (std::size_t power_two = 1; power_two < n;) {
    for (std::size_t gap = power_two; gap < n;) {
      gaps.push_back(gap);
      if (gap > (n - 1) / 3) break;
      gap *= 3;
    }
    if (power_two > (n - 1) / 2) break;
    power_two *= 2;
  }
  std::sort(gaps.begin(), gaps.end());
  gaps.erase(std::unique(gaps.begin(), gaps.end()), gaps.end());
  std::reverse(gaps.begin(), gaps.end());
  return gaps;
}

template <bool Count>
static void shell_pratt_sort(std::vector<Value>& values, Stats& stats) {
  for (const std::size_t gap : pratt_gaps(values.size())) {
    for (std::size_t i = gap; i < values.size(); ++i) {
      const Value value = values[i];
      std::size_t j = i;
      while (j >= gap && lessv<Count>(value, values[j - gap], stats)) {
        writev<Count>(values[j], values[j - gap], stats);
        j -= gap;
      }
      writev<Count>(values[j], value, stats);
    }
  }
}

template <bool Count>
static void merge_ranges(std::vector<Value>& values, std::vector<Value>& temp,
                         std::size_t lo, std::size_t mid, std::size_t hi,
                         Stats& stats) {
  std::size_t left = lo;
  std::size_t right = mid;
  std::size_t out = lo;
  while (left < mid && right < hi) {
    if (lessv<Count>(values[right], values[left], stats)) {
      writev<Count>(temp[out++], values[right++], stats);
    } else {
      writev<Count>(temp[out++], values[left++], stats);
    }
  }
  while (left < mid) writev<Count>(temp[out++], values[left++], stats);
  while (right < hi) writev<Count>(temp[out++], values[right++], stats);
  for (std::size_t i = lo; i < hi; ++i) {
    writev<Count>(values[i], temp[i], stats);
  }
}

template <bool Count>
static void merge_bottom_up_sort(std::vector<Value>& values, Stats& stats) {
  if (values.size() < 2) return;
  std::vector<Value> temp(values.size());
  for (std::size_t width = 1; width < values.size();) {
    for (std::size_t lo = 0; lo < values.size(); lo += 2 * width) {
      const std::size_t mid = std::min(values.size(), lo + width);
      const std::size_t hi = std::min(values.size(), lo + 2 * width);
      if (mid < hi) merge_ranges<Count>(values, temp, lo, mid, hi, stats);
    }
    if (width > values.size() / 2) break;
    width *= 2;
  }
}

template <bool Count>
static void natural_merge_sort(std::vector<Value>& values, Stats& stats) {
  if (values.size() < 2) return;

  std::vector<Value> temp(values.size());
  std::vector<std::pair<std::size_t, std::size_t>> runs;
  std::size_t lo = 0;
  while (lo < values.size()) {
    std::size_t hi = lo + 1;
    if (hi < values.size()) {
      if (lessv<Count>(values[hi], values[lo], stats)) {
        while (hi < values.size() &&
               lessv<Count>(values[hi], values[hi - 1], stats)) {
          ++hi;
        }
        for (std::size_t left = lo, right = hi - 1; left < right; ++left, --right) {
          swapv<Count>(values[left], values[right], stats);
        }
      } else {
        while (hi < values.size() &&
               !lessv<Count>(values[hi], values[hi - 1], stats)) {
          ++hi;
        }
      }
    }
    runs.emplace_back(lo, hi);
    lo = hi;
  }

  while (runs.size() > 1) {
    std::vector<std::pair<std::size_t, std::size_t>> next;
    next.reserve((runs.size() + 1) / 2);
    for (std::size_t r = 0; r < runs.size(); r += 2) {
      if (r + 1 == runs.size()) {
        next.push_back(runs[r]);
        continue;
      }
      const auto [left_lo, left_hi] = runs[r];
      const auto [right_lo, right_hi] = runs[r + 1];
      (void)right_lo;
      merge_ranges<Count>(values, temp, left_lo, left_hi, right_hi, stats);
      next.emplace_back(left_lo, right_hi);
    }
    runs.swap(next);
  }
}

template <bool Count>
static void merge_insertion_rec(std::vector<Value>& values,
                                std::vector<Value>& temp, std::size_t lo,
                                std::size_t hi, std::size_t cutoff,
                                Stats& stats) {
  if (hi - lo <= cutoff) {
    insertion_range<Count>(values, lo, hi, stats);
    return;
  }
  const std::size_t mid = lo + (hi - lo) / 2;
  merge_insertion_rec<Count>(values, temp, lo, mid, cutoff, stats);
  merge_insertion_rec<Count>(values, temp, mid, hi, cutoff, stats);
  if (!lessv<Count>(values[mid], values[mid - 1], stats)) return;
  merge_ranges<Count>(values, temp, lo, mid, hi, stats);
}

template <bool Count>
static void merge_insertion_runtime(std::vector<Value>& values, Stats& stats,
                                    std::size_t cutoff) {
  if (values.size() < 2) return;
  cutoff = std::max<std::size_t>(cutoff, 1);
  std::vector<Value> temp(values.size());
  merge_insertion_rec<Count>(values, temp, 0, values.size(), cutoff, stats);
}

template <bool Count>
static void merge_insertion_24(std::vector<Value>& values, Stats& stats) {
  merge_insertion_runtime<Count>(values, stats, 24);
}

template <bool Count>
static Value median3_value(Value x, Value y, Value z, Stats& stats) {
  const auto less = [&](Value left, Value right) {
    if constexpr (Count) ++stats.comparisons;
    return left < right;
  };
  if (less(x, y)) {
    if (less(y, z)) return y;
    return less(x, z) ? z : x;
  }
  if (less(x, z)) return x;
  return less(y, z) ? z : y;
}

template <bool Count>
static std::size_t median3_partition(std::vector<Value>& values, std::size_t lo,
                                     std::size_t hi, Stats& stats) {
  const std::size_t mid = lo + (hi - lo) / 2;
  const Value pivot =
      median3_value<Count>(values[lo], values[mid], values[hi - 1], stats);
  std::size_t i = lo;
  std::size_t j = hi - 1;
  for (;;) {
    while (i < hi && lessv<Count>(values[i], pivot, stats)) ++i;
    while (j > lo && lessv<Count>(pivot, values[j], stats)) --j;
    if (i >= j) return i;
    swapv<Count>(values[i], values[j], stats);
    ++i;
    if (j > 0) --j;
  }
}

template <bool Count>
static void quick_median3_rec(std::vector<Value>& values, std::size_t lo,
                              std::size_t hi, Stats& stats,
                              std::size_t cutoff) {
  while (hi - lo > cutoff) {
    std::size_t cut = median3_partition<Count>(values, lo, hi, stats);
    if (cut <= lo) cut = lo + 1;
    if (cut >= hi) cut = hi - 1;
    if (cut - lo < hi - cut) {
      quick_median3_rec<Count>(values, lo, cut, stats, cutoff);
      lo = cut;
    } else {
      quick_median3_rec<Count>(values, cut, hi, stats, cutoff);
      hi = cut;
    }
  }
  if (cutoff > 1) insertion_range<Count>(values, lo, hi, stats);
}

template <bool Count>
static void quick_median3_sort(std::vector<Value>& values, Stats& stats) {
  if (!values.empty()) quick_median3_rec<Count>(values, 0, values.size(), stats, 1);
}

template <bool Count>
static void quick_insertion_runtime(std::vector<Value>& values, Stats& stats,
                                    std::size_t cutoff) {
  if (values.empty()) return;
  cutoff = std::max<std::size_t>(cutoff, 1);
  quick_median3_rec<Count>(values, 0, values.size(), stats, cutoff);
}

template <bool Count>
static void quick_insertion_24(std::vector<Value>& values, Stats& stats) {
  quick_insertion_runtime<Count>(values, stats, 24);
}

template <bool Count>
static void intro_cutoff_rec(std::vector<Value>& values, std::size_t lo,
                             std::size_t hi, unsigned depth,
                             std::size_t cutoff, Stats& stats) {
  while (hi - lo > cutoff) {
    if (depth == 0) {
      heap_sort_range<Count>(values, lo, hi, stats);
      return;
    }
    --depth;
    std::size_t cut = hoare_partition<Count>(values, lo, hi, stats);
    if (cut <= lo) cut = lo + 1;
    if (cut >= hi) cut = hi - 1;
    if (cut - lo < hi - cut) {
      intro_cutoff_rec<Count>(values, lo, cut, depth, cutoff, stats);
      lo = cut;
    } else {
      intro_cutoff_rec<Count>(values, cut, hi, depth, cutoff, stats);
      hi = cut;
    }
  }
  if (cutoff > 1) insertion_range<Count>(values, lo, hi, stats);
}

template <bool Count>
static void intro_cutoff_runtime(std::vector<Value>& values, Stats& stats,
                                 std::size_t cutoff) {
  if (values.size() < 2) return;
  cutoff = std::max<std::size_t>(cutoff, 1);
  const auto bits = std::bit_width(values.size());
  const unsigned depth = static_cast<unsigned>(2 * (bits - 1));
  intro_cutoff_rec<Count>(values, 0, values.size(), depth, cutoff, stats);
}

template <bool Count>
static void dual_pivot_rec(std::vector<Value>& values, std::ptrdiff_t lo,
                           std::ptrdiff_t hi, Stats& stats) {
  if (lo >= hi) return;
  const auto at = [&](std::ptrdiff_t index) -> Value& {
    return values[static_cast<std::size_t>(index)];
  };

  if (lessv<Count>(at(hi), at(lo), stats)) swapv<Count>(at(lo), at(hi), stats);
  const Value left_pivot = at(lo);
  const Value right_pivot = at(hi);
  std::ptrdiff_t lt = lo + 1;
  std::ptrdiff_t gt = hi - 1;
  std::ptrdiff_t i = lt;

  while (i <= gt) {
    if (lessv<Count>(at(i), left_pivot, stats)) {
      swapv<Count>(at(i), at(lt), stats);
      ++lt;
      ++i;
    } else if (lessv<Count>(right_pivot, at(i), stats)) {
      while (i < gt && lessv<Count>(right_pivot, at(gt), stats)) --gt;
      swapv<Count>(at(i), at(gt), stats);
      --gt;
      if (lessv<Count>(at(i), left_pivot, stats)) {
        swapv<Count>(at(i), at(lt), stats);
        ++lt;
      }
      ++i;
    } else {
      ++i;
    }
  }

  --lt;
  ++gt;
  swapv<Count>(at(lo), at(lt), stats);
  swapv<Count>(at(hi), at(gt), stats);
  dual_pivot_rec<Count>(values, lo, lt - 1, stats);
  if (lessv<Count>(left_pivot, right_pivot, stats)) {
    dual_pivot_rec<Count>(values, lt + 1, gt - 1, stats);
  }
  dual_pivot_rec<Count>(values, gt + 1, hi, stats);
}

template <bool Count>
static void dual_pivot_sort(std::vector<Value>& values, Stats& stats) {
  if (values.size() > 1) {
    dual_pivot_rec<Count>(values, 0,
                          static_cast<std::ptrdiff_t>(values.size() - 1), stats);
  }
}

template <bool Count>
static void radix_lsd_11_sort(std::vector<Value>& values, Stats& stats) {
  if (values.size() < 2) return;

  std::vector<Value> temp(values.size());
  constexpr std::uint64_t sign = std::uint64_t{1} << 63;
  constexpr unsigned digit_bits = 11;
  constexpr std::size_t max_buckets = std::size_t{1} << digit_bits;
  std::array<std::size_t, max_buckets> count{};
  std::array<std::size_t, max_buckets> position{};

  for (unsigned shift = 0; shift < 64; shift += digit_bits) {
    const unsigned width = std::min(digit_bits, 64U - shift);
    const std::uint64_t mask = (std::uint64_t{1} << width) - 1;
    const std::size_t used = std::size_t{1} << width;
    count.fill(0);
    for (const Value value : values) {
      const std::uint64_t key = static_cast<std::uint64_t>(value) ^ sign;
      ++count[static_cast<std::size_t>((key >> shift) & mask)];
    }
    position[0] = 0;
    for (std::size_t bucket = 1; bucket < used; ++bucket) {
      position[bucket] = position[bucket - 1] + count[bucket - 1];
    }
    for (const Value value : values) {
      const std::uint64_t key = static_cast<std::uint64_t>(value) ^ sign;
      const std::size_t bucket = static_cast<std::size_t>((key >> shift) & mask);
      writev<Count>(temp[position[bucket]++], value, stats);
    }
    values.swap(temp);
  }
}

template <bool Count>
static void std_heap_sort(std::vector<Value>& values, Stats& stats) {
  const auto compare = [&](Value left, Value right) {
    if constexpr (Count) ++stats.comparisons;
    return left < right;
  };
  std::make_heap(values.begin(), values.end(), compare);
  std::sort_heap(values.begin(), values.end(), compare);
}

#define SORTLAB_EXTENDED_ALGORITHM(NAME, FAMILY, FN, QUADRATIC, COMPARISON, STABLE, IN_PLACE, ADAPTIVE, BEST, AVERAGE, WORST, AUXILIARY) \
  Algorithm{NAME, FAMILY, FN<false>, FN<true>, QUADRATIC, COMPARISON, STABLE, IN_PLACE, ADAPTIVE, BEST, AVERAGE, WORST, AUXILIARY}

inline const std::vector<Algorithm>& all_algorithms() {
  static const std::vector<Algorithm> table = [] {
    std::vector<Algorithm> result = algorithms();
    const std::vector<Algorithm> extra = {
        SORTLAB_EXTENDED_ALGORITHM(
            "binary_insertion", "insertion", binary_insertion_sort, true, true,
            "yes", "yes", "yes", "O(n log n) comparisons / O(n^2) moves",
            "O(n^2)", "O(n^2)", "O(1)"),
        SORTLAB_EXTENDED_ALGORITHM(
            "comb", "exchange", comb_sort, true, true, "no", "yes", "partial",
            "gap-dependent", "gap-dependent", "O(n^2)", "O(1)"),
        SORTLAB_EXTENDED_ALGORITHM(
            "shell_pratt", "shell", shell_pratt_sort, false, true, "no", "yes",
            "yes", "gap-dependent", "gap-dependent", "O(n log^2 n) comparisons",
            "O(log n) gaps"),
        SORTLAB_EXTENDED_ALGORITHM(
            "merge_bottom_up", "merge", merge_bottom_up_sort, false, true, "yes",
            "no", "no", "O(n log n)", "O(n log n)", "O(n log n)", "O(n)"),
        SORTLAB_EXTENDED_ALGORITHM(
            "natural_merge", "merge-adaptive", natural_merge_sort, false, true,
            "yes", "no", "runs", "O(n)", "O(n log n)", "O(n log n)", "O(n)"),
        SORTLAB_EXTENDED_ALGORITHM(
            "merge_insertion_24", "hybrid", merge_insertion_24, false, true, "yes",
            "no", "small-runs", "O(n)", "O(n log n)", "O(n log n)", "O(n)"),
        SORTLAB_EXTENDED_ALGORITHM(
            "quick_median3", "quick", quick_median3_sort, false, true, "no", "yes",
            "pivot-sampling", "O(n log n)", "O(n log n)", "O(n^2)",
            "O(log n) expected stack"),
        SORTLAB_EXTENDED_ALGORITHM(
            "quick_insertion_24", "hybrid", quick_insertion_24, false, true, "no",
            "yes", "small-partitions", "O(n log n)", "O(n log n)", "O(n^2)",
            "O(log n) expected stack"),
        SORTLAB_EXTENDED_ALGORITHM(
            "dual_pivot", "quick", dual_pivot_sort, false, true, "no", "yes",
            "duplicates/pivots", "O(n log n)", "O(n log n)", "O(n^2)",
            "O(log n) expected stack"),
        SORTLAB_EXTENDED_ALGORITHM(
            "radix_lsd_11", "distribution", radix_lsd_11_sort, false, false, "yes",
            "no", "digit-width", "O(6n)", "O(6n)", "O(6n)", "O(n + 2048)"),
        SORTLAB_EXTENDED_ALGORITHM(
            "std_heap", "library", std_heap_sort, false, true, "no", "yes",
            "implementation", "O(n log n)", "O(n log n)", "O(n log n)",
            "implementation")};
    result.insert(result.end(), extra.begin(), extra.end());
    return result;
  }();
  return table;
}

#undef SORTLAB_EXTENDED_ALGORITHM

}  // namespace sortlab
