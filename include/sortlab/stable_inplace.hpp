#pragma once

#include "sortlab/detail.hpp"

#include <concepts>
#include <functional>
#include <iterator>
#include <ranges>
#include <utility>

namespace sortlab::detail {

template <std::random_access_iterator I, class Ops>
void stable_inplace_merge(I first, I middle, I last, Ops& ops) {
  if (first == middle || middle == last) return;
  if (last - first == 2) {
    if (ops.less(*middle, *first)) ops.swap(first, middle);
    return;
  }

  I first_cut;
  I second_cut;
  const auto left_n = middle - first;
  const auto right_n = last - middle;

  if (left_n > right_n) {
    first_cut = first + left_n / 2;
    second_cut = detail::lower_bound(middle, last, *first_cut, ops);
  } else {
    second_cut = middle + right_n / 2;
    first_cut = detail::upper_bound(first, middle, *second_cut, ops);
  }

  I new_middle = detail::rotate_range(first_cut, middle, second_cut, ops);
  stable_inplace_merge(first, first_cut, new_middle, ops);
  stable_inplace_merge(new_middle, second_cut, last, ops);
}

template <std::random_access_iterator I, class Ops>
void stable_inplace_merge_sort_impl(I first, I last, Ops& ops) {
  if (last - first < 2) return;
  I middle = first + (last - first) / 2;
  stable_inplace_merge_sort_impl(first, middle, ops);
  stable_inplace_merge_sort_impl(middle, last, ops);
  if (!ops.less(*middle, *(middle - 1))) return;
  stable_inplace_merge(first, middle, last, ops);
}

}  // namespace sortlab::detail

namespace sortlab {

template <std::random_access_iterator I, class Comp = std::ranges::less,
          class Proj = std::identity>
requires std::sortable<I, Comp, Proj>
void stable_inplace_merge_sort(I first, I last, Comp comp = {}, Proj proj = {}) {
  null_observer observer;
  detail::operations ops(std::move(comp), std::move(proj), observer);
  detail::stable_inplace_merge_sort_impl(first, last, ops);
}

template <std::ranges::random_access_range R, class Comp = std::ranges::less,
          class Proj = std::identity>
requires std::sortable<std::ranges::iterator_t<R>, Comp, Proj>
void stable_inplace_merge_sort(R&& range, Comp comp = {}, Proj proj = {}) {
  stable_inplace_merge_sort(std::ranges::begin(range), std::ranges::end(range),
                            std::move(comp), std::move(proj));
}

namespace instrumented {
template <std::random_access_iterator I, class Observer,
          class Comp = std::ranges::less, class Proj = std::identity>
requires std::sortable<I, Comp, Proj>
void stable_inplace_merge_sort(I first, I last, Observer& observer,
                               Comp comp = {}, Proj proj = {}) {
  detail::operations ops(std::move(comp), std::move(proj), observer);
  detail::stable_inplace_merge_sort_impl(first, last, ops);
}
}  // namespace instrumented

}  // namespace sortlab
