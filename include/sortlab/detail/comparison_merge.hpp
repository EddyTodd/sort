#pragma once

#include "sortlab/detail/comparison_elementary.hpp"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <utility>
#include <vector>

namespace sortlab::detail {
template <std::random_access_iterator I, class Ops>
void merge_ranges(I first, I mid, I last, std::vector<std::iter_value_t<I>>& buffer, Ops& ops) {
  buffer.clear();
  const auto needed = static_cast<std::size_t>(last - first);
  if (buffer.capacity() < needed) buffer.reserve(needed);
  I left = first;
  I right = mid;
  while (left < mid && right < last) {
    if (ops.less(*right, *left)) {
      buffer.emplace_back(std::ranges::iter_move(right++));
    } else {
      buffer.emplace_back(std::ranges::iter_move(left++));
    }
    ops.external_write();
  }
  while (left < mid) {
    buffer.emplace_back(std::ranges::iter_move(left++));
    ops.external_write();
  }
  while (right < last) {
    buffer.emplace_back(std::ranges::iter_move(right++));
    ops.external_write();
  }
  I out = first;
  for (auto& value : buffer) ops.assign(out++, std::move(value));
}

template <std::random_access_iterator I, class Ops>
void merge_recursive(I first, I last, std::vector<std::iter_value_t<I>>& buffer, Ops& ops) {
  if (last - first < 2) return;
  I mid = first + (last - first) / 2;
  merge_recursive(first, mid, buffer, ops);
  merge_recursive(mid, last, buffer, ops);
  if (!ops.less(*mid, *(mid - 1))) return;
  merge_ranges(first, mid, last, buffer, ops);
}

template <std::random_access_iterator I, class Ops>
void merge_impl(I first, I last, Ops& ops) {
  std::vector<std::iter_value_t<I>> buffer;
  buffer.reserve(static_cast<std::size_t>(last - first));
  merge_recursive(first, last, buffer, ops);
}

template <std::random_access_iterator I, class Ops>
void merge_bottom_up_impl(I first, I last, Ops& ops) {
  const auto n = last - first;
  if (n < 2) return;
  std::vector<std::iter_value_t<I>> buffer;
  buffer.reserve(static_cast<std::size_t>(n));
  for (std::iter_difference_t<I> width = 1; width < n;) {
    for (auto lo = std::iter_difference_t<I>{0}; lo < n; lo += 2 * width) {
      auto mid = std::min(n, lo + width);
      auto hi = std::min(n, lo + 2 * width);
      if (mid < hi && ops.less(*(first + mid), *(first + mid - 1)))
        merge_ranges(first + lo, first + mid, first + hi, buffer, ops);
    }
    if (width > n / 2) break;
    width *= 2;
  }
}

template <std::random_access_iterator I, class Ops>
void natural_merge_impl(I first, I last, Ops& ops) {
  using Run = std::pair<I,I>;
  if (last - first < 2) return;
  std::vector<std::iter_value_t<I>> buffer;
  buffer.reserve(static_cast<std::size_t>(last - first));
  std::vector<Run> runs;
  I lo = first;
  while (lo < last) {
    I hi = lo + 1;
    if (hi < last) {
      if (ops.less(*hi, *lo)) {
        ++hi;
        while (hi < last && ops.less(*hi, *(hi - 1))) ++hi;
        reverse_range(lo, hi, ops);
      } else {
        ++hi;
        while (hi < last && !ops.less(*hi, *(hi - 1))) ++hi;
      }
    }
    runs.emplace_back(lo, hi);
    lo = hi;
  }
  while (runs.size() > 1) {
    std::vector<Run> next;
    next.reserve((runs.size()+1)/2);
    for (std::size_t i=0;i<runs.size();i+=2) {
      if (i+1==runs.size()) { next.push_back(runs[i]); continue; }
      merge_ranges(runs[i].first, runs[i].second, runs[i+1].second, buffer, ops);
      next.emplace_back(runs[i].first, runs[i+1].second);
    }
    runs.swap(next);
  }
}

template <std::random_access_iterator I, class Ops>
void merge_insertion_recursive(I first, I last, std::size_t cutoff,
                               std::vector<std::iter_value_t<I>>& buffer, Ops& ops) {
  const auto n = static_cast<std::size_t>(last-first);
  if (n <= cutoff) { insertion_impl(first,last,ops); return; }
  I mid = first + (last-first)/2;
  merge_insertion_recursive(first,mid,cutoff,buffer,ops);
  merge_insertion_recursive(mid,last,cutoff,buffer,ops);
  if (ops.less(*mid,*(mid-1))) merge_ranges(first,mid,last,buffer,ops);
}

template <std::random_access_iterator I, class Ops>
void merge_insertion_impl(I first, I last, std::size_t cutoff, Ops& ops) {
  std::vector<std::iter_value_t<I>> buffer;
  buffer.reserve(static_cast<std::size_t>(last-first));
  merge_insertion_recursive(first,last,cutoff,buffer,ops);
}

}  // namespace sortlab::detail
