#pragma once

#include "sortlab/adaptive_options.hpp"
#include "sortlab/comparison.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <ranges>
#include <stdexcept>
#include <utility>
#include <vector>

namespace sortlab::detail {
struct adaptive_run {
  std::size_t base = 0;
  std::size_t len = 0;
  int power = 0;
};

inline std::size_t timsort_minrun(std::size_t n, std::size_t min_merge = 32) {
  if (n == 0) return 0;
  if (min_merge == 0) throw std::invalid_argument("min_merge must be positive");
  std::size_t r = 0;
  while (min_merge <= n / 2) {
    r |= n & 1U;
    n >>= 1U;
  }
  return n + r;
}

inline int powerloop(std::size_t s1, std::size_t n1, std::size_t n2, std::size_t n) {
  if (n == 0 || n1 == 0 || n2 == 0 || s1 > n || n1 > n - s1 ||
      n2 > n - s1 - n1)
    throw std::logic_error("invalid Powersort run geometry");
  if (n > std::numeric_limits<std::size_t>::max() / 2)
    throw std::length_error("Powersort input too large");
  int result = 0;
  std::size_t a = 2 * s1 + n1;
  std::size_t b = a + n1 + n2;
  for (;;) {
    ++result;
    if (a >= n) { a -= n; b -= n; }
    else if (b >= n) break;
    a <<= 1U; b <<= 1U;
  }
  return result;
}

template <std::random_access_iterator I, class Ops>
std::size_t count_run(I first, I last, std::size_t base, Ops& ops) {
  const std::size_t n = static_cast<std::size_t>(last - first);
  if (base >= n) return 0;
  if (base + 1 >= n) return 1;
  I lo = first + static_cast<std::iter_difference_t<I>>(base);
  I hi = lo + 2;
  if (ops.less(*(lo + 1), *lo)) {
    while (hi < last && ops.less(*hi, *(hi - 1))) ++hi;
    reverse_range(lo, hi, ops); // strict descending only: equal keys are never reversed
  } else {
    while (hi < last && !ops.less(*hi, *(hi - 1))) ++hi;
  }
  return static_cast<std::size_t>(hi - lo);
}

template <std::random_access_iterator I, class Ops>
void binary_extend(I first, std::size_t lo, std::size_t sorted_hi, std::size_t hi, Ops& ops) {
  I begin = first + static_cast<std::iter_difference_t<I>>(lo);
  for (std::size_t index = sorted_hi; index < hi; ++index) {
    I it = first + static_cast<std::iter_difference_t<I>>(index);
    std::iter_value_t<I> value = std::ranges::iter_move(it);
    I left = begin;
    I right = it;
    while (left < right) {
      I mid = left + (right - left) / 2;
      if (ops.less(value, *mid)) right = mid;
      else left = mid + 1;
    }
    for (I pos = it; pos > left; --pos) ops.assign(pos, std::ranges::iter_move(pos - 1));
    ops.assign(left, std::move(value));
  }
}

template <std::random_access_iterator I, class T, class Ops>
I gallop_lower(I first, I last, const T& value, Ops& ops) {
  if (first == last || !ops.less(*first, value)) return first;
  auto length = last - first;
  decltype(length) ofs = 1;
  I prev = first;
  while (ofs < length) {
    I probe = first + ofs;
    if (!ops.less(*probe, value)) return detail::lower_bound(prev + 1, probe + 1, value, ops);
    prev = probe;
    if (ofs > length / 2) break;
    ofs *= 2;
  }
  return detail::lower_bound(prev + 1, last, value, ops);
}

template <std::random_access_iterator I, class T, class Ops>
I gallop_upper(I first, I last, const T& value, Ops& ops) {
  if (first == last || ops.less(value, *first)) return first;
  auto length = last - first;
  decltype(length) ofs = 1;
  I prev = first;
  while (ofs < length) {
    I probe = first + ofs;
    if (ops.less(value, *probe)) return detail::upper_bound(prev + 1, probe + 1, value, ops);
    prev = probe;
    if (ofs > length / 2) break;
    ofs *= 2;
  }
  return detail::upper_bound(prev + 1, last, value, ops);
}

template <std::random_access_iterator I, class T, class Ops>
I gallop_upper_reverse(I first, I last, const T& value, Ops& ops) {
  if (first == last) return last;
  if (!ops.less(value, *(last - 1))) return last;
  I search_lo = first;
  I search_hi = last;
  auto length = last - first;
  decltype(length) distance = 1;
  while (distance < length) {
    I probe = last - 1 - distance;
    if (ops.less(value, *probe)) {
      search_hi = probe + 1;
      if (distance > length / 2) break;
      distance *= 2;
    } else {
      search_lo = probe + 1;
      break;
    }
  }
  return detail::upper_bound(search_lo, search_hi, value, ops);
}

template <std::random_access_iterator I, class T, class Ops>
I gallop_lower_reverse(I first, I last, const T& value, Ops& ops) {
  if (first == last) return last;
  if (ops.less(*(last - 1), value)) return last;
  I search_lo = first;
  I search_hi = last;
  auto length = last - first;
  decltype(length) distance = 1;
  while (distance < length) {
    I probe = last - 1 - distance;
    if (!ops.less(*probe, value)) {
      search_hi = probe + 1;
      if (distance > length / 2) break;
      distance *= 2;
    } else {
      search_lo = probe + 1;
      break;
    }
  }
  return detail::lower_bound(search_lo, search_hi, value, ops);
}

}  // namespace sortlab::detail
