#pragma once

#include "sortlab/common.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace sortlab {

template <std::size_t Words>
struct Payload {
  std::array<std::uint64_t, Words> words{};
  bool operator==(const Payload&) const = default;
};

template <>
struct Payload<0> {
  bool operator==(const Payload&) const = default;
};

template <std::size_t Words>
struct Record {
  Value key{};
  std::uint64_t ordinal{};
  [[no_unique_address]] Payload<Words> payload{};
  bool operator==(const Record&) const = default;
};

struct RecordStats {
  std::uint64_t comparisons = 0;
  std::uint64_t swaps = 0;
  std::uint64_t explicit_moves = 0;
};

template <bool Count, std::size_t Words>
inline bool record_less(const Record<Words>& a, const Record<Words>& b, RecordStats& stats) {
  if constexpr (Count) ++stats.comparisons;
  return a.key < b.key;
}

template <bool Count, std::size_t Words>
inline void record_write(Record<Words>& dst, const Record<Words>& src, RecordStats& stats) {
  dst = src;
  if constexpr (Count) ++stats.explicit_moves;
}

template <bool Count, std::size_t Words>
inline void record_swap(Record<Words>& a, Record<Words>& b, RecordStats& stats) {
  if (&a == &b) return;
  std::swap(a, b);
  if constexpr (Count) {
    ++stats.swaps;
    stats.explicit_moves += 3;
  }
}

template <bool Count, std::size_t Words>
static void record_insertion_sort(std::vector<Record<Words>>& a, RecordStats& stats) {
  for (std::size_t i = 1; i < a.size(); ++i) {
    const auto value = a[i];
    std::size_t j = i;
    while (j > 0 && record_less<Count>(value, a[j - 1], stats)) {
      record_write<Count>(a[j], a[j - 1], stats);
      --j;
    }
    record_write<Count>(a[j], value, stats);
  }
}

template <bool Count, std::size_t Words>
static void record_sift_down(std::vector<Record<Words>>& a, std::size_t base, std::size_t root,
                             std::size_t n, RecordStats& stats) {
  for (;;) {
    std::size_t child = root * 2 + 1;
    if (child >= n) return;
    if (child + 1 < n && record_less<Count>(a[base + child], a[base + child + 1], stats)) ++child;
    if (!record_less<Count>(a[base + root], a[base + child], stats)) return;
    record_swap<Count>(a[base + root], a[base + child], stats);
    root = child;
  }
}

template <bool Count, std::size_t Words>
static void record_heap_sort_range(std::vector<Record<Words>>& a, std::size_t lo, std::size_t hi,
                                   RecordStats& stats) {
  const std::size_t n = hi - lo;
  for (std::size_t i = n / 2; i > 0; --i) record_sift_down<Count>(a, lo, i - 1, n, stats);
  for (std::size_t remaining = n; remaining > 1; --remaining) {
    record_swap<Count>(a[lo], a[lo + remaining - 1], stats);
    record_sift_down<Count>(a, lo, 0, remaining - 1, stats);
  }
}

template <bool Count, std::size_t Words>
static void record_heap_sort(std::vector<Record<Words>>& a, RecordStats& stats) {
  record_heap_sort_range<Count>(a, 0, a.size(), stats);
}

template <bool Count, std::size_t Words>
static void record_merge_rec(std::vector<Record<Words>>& a, std::vector<Record<Words>>& tmp,
                             std::size_t lo, std::size_t hi, RecordStats& stats) {
  if (hi - lo < 2) return;
  const std::size_t mid = lo + (hi - lo) / 2;
  record_merge_rec<Count>(a, tmp, lo, mid, stats);
  record_merge_rec<Count>(a, tmp, mid, hi, stats);
  std::size_t i = lo;
  std::size_t j = mid;
  std::size_t k = lo;
  while (i < mid && j < hi) {
    if (record_less<Count>(a[j], a[i], stats)) record_write<Count>(tmp[k++], a[j++], stats);
    else record_write<Count>(tmp[k++], a[i++], stats);
  }
  while (i < mid) record_write<Count>(tmp[k++], a[i++], stats);
  while (j < hi) record_write<Count>(tmp[k++], a[j++], stats);
  for (k = lo; k < hi; ++k) record_write<Count>(a[k], tmp[k], stats);
}

template <bool Count, std::size_t Words>
static void record_merge_sort(std::vector<Record<Words>>& a, RecordStats& stats) {
  std::vector<Record<Words>> tmp(a.size());
  record_merge_rec<Count>(a, tmp, 0, a.size(), stats);
}

template <bool Count, std::size_t Words>
static std::size_t record_hoare_partition(std::vector<Record<Words>>& a, std::size_t lo,
                                          std::size_t hi, RecordStats& stats) {
  const auto pivot = a[lo + (hi - lo) / 2];
  std::size_t i = lo;
  std::size_t j = hi - 1;
  for (;;) {
    while (i < hi && record_less<Count>(a[i], pivot, stats)) ++i;
    while (j > lo && record_less<Count>(pivot, a[j], stats)) --j;
    if (i >= j) return i;
    record_swap<Count>(a[i], a[j], stats);
    ++i;
    if (j > 0) --j;
  }
}

template <bool Count, std::size_t Words>
static void record_quick_rec(std::vector<Record<Words>>& a, std::size_t lo, std::size_t hi,
                             RecordStats& stats) {
  while (hi - lo > 1) {
    std::size_t cut = record_hoare_partition<Count>(a, lo, hi, stats);
    if (cut <= lo) cut = lo + 1;
    if (cut >= hi) cut = hi - 1;
    if (cut - lo < hi - cut) {
      record_quick_rec<Count>(a, lo, cut, stats);
      lo = cut;
    } else {
      record_quick_rec<Count>(a, cut, hi, stats);
      hi = cut;
    }
  }
}

template <bool Count, std::size_t Words>
static void record_quick_sort(std::vector<Record<Words>>& a, RecordStats& stats) {
  if (!a.empty()) record_quick_rec<Count>(a, 0, a.size(), stats);
}

template <bool Count, std::size_t Words>
static void record_quick3_rec(std::vector<Record<Words>>& a, std::size_t lo, std::size_t hi,
                              RecordStats& stats) {
  while (hi - lo > 1) {
    const auto pivot = a[lo + (hi - lo) / 2];
    std::size_t lt = lo;
    std::size_t i = lo;
    std::size_t gt = hi;
    while (i < gt) {
      if (record_less<Count>(a[i], pivot, stats)) {
        record_swap<Count>(a[lt++], a[i++], stats);
      } else if (record_less<Count>(pivot, a[i], stats)) {
        record_swap<Count>(a[i], a[--gt], stats);
      } else {
        ++i;
      }
    }
    if (lt - lo < hi - gt) {
      record_quick3_rec<Count>(a, lo, lt, stats);
      lo = gt;
    } else {
      record_quick3_rec<Count>(a, gt, hi, stats);
      hi = lt;
    }
  }
}

template <bool Count, std::size_t Words>
static void record_quick3_sort(std::vector<Record<Words>>& a, RecordStats& stats) {
  record_quick3_rec<Count>(a, 0, a.size(), stats);
}

template <bool Count, std::size_t Words>
static void record_insertion_range(std::vector<Record<Words>>& a, std::size_t lo, std::size_t hi,
                                   RecordStats& stats) {
  for (std::size_t i = lo + 1; i < hi; ++i) {
    const auto value = a[i];
    std::size_t j = i;
    while (j > lo && record_less<Count>(value, a[j - 1], stats)) {
      record_write<Count>(a[j], a[j - 1], stats);
      --j;
    }
    record_write<Count>(a[j], value, stats);
  }
}

template <bool Count, std::size_t Words>
static void record_intro_rec(std::vector<Record<Words>>& a, std::size_t lo, std::size_t hi,
                             unsigned depth_limit, RecordStats& stats) {
  constexpr std::size_t insertion_cutoff = 24;
  while (hi - lo > insertion_cutoff) {
    if (depth_limit == 0) {
      record_heap_sort_range<Count>(a, lo, hi, stats);
      return;
    }
    --depth_limit;
    std::size_t cut = record_hoare_partition<Count>(a, lo, hi, stats);
    if (cut <= lo) cut = lo + 1;
    if (cut >= hi) cut = hi - 1;
    if (cut - lo < hi - cut) {
      record_intro_rec<Count>(a, lo, cut, depth_limit, stats);
      lo = cut;
    } else {
      record_intro_rec<Count>(a, cut, hi, depth_limit, stats);
      hi = cut;
    }
  }
  record_insertion_range<Count>(a, lo, hi, stats);
}

template <bool Count, std::size_t Words>
static void record_intro_sort(std::vector<Record<Words>>& a, RecordStats& stats) {
  if (a.size() < 2) return;
  const auto bits = std::bit_width(a.size());
  const unsigned depth = static_cast<unsigned>(2 * (bits - 1));
  record_intro_rec<Count>(a, 0, a.size(), depth, stats);
}

template <bool Count, std::size_t Words>
static void record_radix_lsd_sort(std::vector<Record<Words>>& a, RecordStats& stats) {
  if (a.size() < 2) return;
  std::vector<Record<Words>> tmp(a.size());
  constexpr std::uint64_t sign = std::uint64_t{1} << 63;
  for (unsigned pass = 0; pass < 8; ++pass) {
    std::size_t count[256]{};
    const unsigned shift = pass * 8;
    for (const auto& value : a) {
      const auto key = static_cast<std::uint64_t>(value.key) ^ sign;
      ++count[(key >> shift) & 0xffU];
    }
    std::size_t position[256]{};
    for (std::size_t i = 1; i < 256; ++i) position[i] = position[i - 1] + count[i - 1];
    for (const auto& value : a) {
      const auto key = static_cast<std::uint64_t>(value.key) ^ sign;
      const auto bucket = static_cast<std::size_t>((key >> shift) & 0xffU);
      record_write<Count>(tmp[position[bucket]++], value, stats);
    }
    a.swap(tmp);
  }
}

template <bool Count, std::size_t Words>
static void record_std_sort(std::vector<Record<Words>>& a, RecordStats& stats) {
  std::sort(a.begin(), a.end(), [&](const auto& x, const auto& y) {
    if constexpr (Count) ++stats.comparisons;
    return x.key < y.key;
  });
}

template <bool Count, std::size_t Words>
static void record_std_stable_sort(std::vector<Record<Words>>& a, RecordStats& stats) {
  std::stable_sort(a.begin(), a.end(), [&](const auto& x, const auto& y) {
    if constexpr (Count) ++stats.comparisons;
    return x.key < y.key;
  });
}

template <std::size_t Words>
using RecordSortFn = void (*)(std::vector<Record<Words>>&, RecordStats&);

template <std::size_t Words>
struct RecordAlgorithm {
  std::string_view name;
  std::string_view family;
  RecordSortFn<Words> timed;
  RecordSortFn<Words> instrumented;
  bool quadratic;
  std::string_view stable_guarantee;
  std::string_view auxiliary;
};

#define RECORD_ALGORITHM(NAME, FAMILY, FN, QUAD, STABLE, AUX) \
  RecordAlgorithm<Words>{NAME, FAMILY, FN<false, Words>, FN<true, Words>, QUAD, STABLE, AUX}

template <std::size_t Words>
static const std::vector<RecordAlgorithm<Words>>& record_algorithms() {
  static const std::vector<RecordAlgorithm<Words>> table = {
      RECORD_ALGORITHM("insertion", "insertion", record_insertion_sort, true, "yes", "O(1)"),
      RECORD_ALGORITHM("heap", "heap", record_heap_sort, false, "no", "O(1)"),
      RECORD_ALGORITHM("merge", "merge", record_merge_sort, false, "yes", "O(n records)"),
      RECORD_ALGORITHM("quick_hoare", "quick", record_quick_sort, false, "no", "O(log n) expected stack"),
      RECORD_ALGORITHM("quick_3way", "quick", record_quick3_sort, false, "no", "O(log n) expected stack"),
      RECORD_ALGORITHM("intro", "hybrid", record_intro_sort, false, "no", "O(log n) stack"),
      RECORD_ALGORITHM("radix_lsd", "distribution", record_radix_lsd_sort, false, "yes", "O(n records + 256)"),
      RECORD_ALGORITHM("std_sort", "library", record_std_sort, false, "not guaranteed", "implementation-defined"),
      RECORD_ALGORITHM("std_stable_sort", "library", record_std_stable_sort, false, "yes", "implementation-defined")};
  return table;
}

#undef RECORD_ALGORITHM

}  // namespace sortlab
