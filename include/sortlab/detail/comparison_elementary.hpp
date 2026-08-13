#pragma once

#include "sortlab/detail.hpp"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <iterator>
#include <ranges>
#include <utility>
#include <vector>

namespace sortlab::detail {

template <std::random_access_iterator I, class Ops>
constexpr void insertion_impl(I first, I last, Ops& ops) {
  if (last - first < 2) return;
  for (I it = first + 1; it < last; ++it) {
    std::iter_value_t<I> value = std::ranges::iter_move(it);
    I pos = it;
    while (pos > first && ops.less(value, *(pos - 1))) {
      ops.assign(pos, std::ranges::iter_move(pos - 1));
      --pos;
    }
    ops.assign(pos, std::move(value));
  }
}

template <std::random_access_iterator I, class Ops>
constexpr void binary_insertion_impl(I first, I last, Ops& ops) {
  if (last - first < 2) return;
  for (I it = first + 1; it < last; ++it) {
    std::iter_value_t<I> value = std::ranges::iter_move(it);
    I lo = first;
    I hi = it;
    while (lo < hi) {
      I mid = lo + (hi - lo) / 2;
      if (ops.less(value, *mid)) hi = mid;
      else lo = mid + 1;
    }
    for (I pos = it; pos > lo; --pos) ops.assign(pos, std::ranges::iter_move(pos - 1));
    ops.assign(lo, std::move(value));
  }
}

template <std::random_access_iterator I, class Ops>
constexpr void selection_impl(I first, I last, Ops& ops) {
  for (I it = first; it < last; ++it) {
    I min_it = it;
    for (I scan = it + 1; scan < last; ++scan) if (ops.less(*scan, *min_it)) min_it = scan;
    ops.swap(it, min_it);
  }
}

template <std::random_access_iterator I, class Ops>
constexpr void bubble_impl(I first, I last, Ops& ops) {
  for (I end = last; end - first > 1; --end) {
    bool changed = false;
    for (I it = first + 1; it < end; ++it) {
      if (ops.less(*it, *(it - 1))) { ops.swap(it, it - 1); changed = true; }
    }
    if (!changed) break;
  }
}

template <std::random_access_iterator I, class Ops>
constexpr void comb_impl(I first, I last, Ops& ops) {
  std::size_t gap = static_cast<std::size_t>(last - first);
  bool changed = true;
  while (gap > 1 || changed) {
    gap = (gap * 10) / 13;
    if (gap == 0) gap = 1;
    changed = false;
    const auto d = static_cast<std::iter_difference_t<I>>(gap);
    for (I it = first; it + d < last; ++it) {
      if (ops.less(*(it + d), *it)) { ops.swap(it, it + d); changed = true; }
    }
  }
}

inline std::vector<std::size_t> ciura_gaps(std::size_t n) {
  static constexpr std::size_t base[] = {1,4,10,23,57,132,301,701,1750};
  std::vector<std::size_t> gaps;
  for (auto gap : base) if (gap < n) gaps.push_back(gap);
  std::size_t gap = 1750;
  while (gap < n / 2) {
    const auto next = gap + gap + gap / 4;
    if (next <= gap) break;
    gap = next;
    if (gap < n) gaps.push_back(gap);
  }
  std::ranges::sort(gaps, std::greater<>());
  return gaps;
}

inline std::vector<std::size_t> pratt_gaps(std::size_t n) {
  std::vector<std::size_t> gaps;
  for (std::size_t p2 = 1; p2 < n;) {
    for (std::size_t g = p2; g < n;) {
      gaps.push_back(g);
      if (g > (n - 1) / 3) break;
      g *= 3;
    }
    if (p2 > (n - 1) / 2) break;
    p2 *= 2;
  }
  std::ranges::sort(gaps);
  gaps.erase(std::unique(gaps.begin(), gaps.end()), gaps.end());
  std::ranges::reverse(gaps);
  return gaps;
}

template <std::random_access_iterator I, class Ops>
constexpr void shell_with_gaps(I first, I last, Ops& ops, const std::vector<std::size_t>& gaps) {
  for (auto gap_size : gaps) {
    const auto gap = static_cast<std::iter_difference_t<I>>(gap_size);
    for (I it = first + gap; it < last; ++it) {
      std::iter_value_t<I> value = std::ranges::iter_move(it);
      I pos = it;
      while (pos - first >= gap && ops.less(value, *(pos - gap))) {
        ops.assign(pos, std::ranges::iter_move(pos - gap));
        pos -= gap;
      }
      ops.assign(pos, std::move(value));
    }
  }
}

template <std::random_access_iterator I, class Ops>
constexpr void sift_down(I first, std::size_t root, std::size_t n, Ops& ops) {
  for (;;) {
    std::size_t child = root * 2 + 1;
    if (child >= n) return;
    if (child + 1 < n && ops.less(*(first + static_cast<std::iter_difference_t<I>>(child)),
                                  *(first + static_cast<std::iter_difference_t<I>>(child + 1)))) ++child;
    if (!ops.less(*(first + static_cast<std::iter_difference_t<I>>(root)),
                  *(first + static_cast<std::iter_difference_t<I>>(child)))) return;
    ops.swap(first + static_cast<std::iter_difference_t<I>>(root),
             first + static_cast<std::iter_difference_t<I>>(child));
    root = child;
  }
}

template <std::random_access_iterator I, class Ops>
constexpr void heap_impl(I first, I last, Ops& ops) {
  const std::size_t n = static_cast<std::size_t>(last - first);
  for (std::size_t i = n / 2; i > 0; --i) sift_down(first, i - 1, n, ops);
  for (std::size_t remaining = n; remaining > 1; --remaining) {
    ops.swap(first, first + static_cast<std::iter_difference_t<I>>(remaining - 1));
    sift_down(first, 0, remaining - 1, ops);
  }
}

}  // namespace sortlab::detail
