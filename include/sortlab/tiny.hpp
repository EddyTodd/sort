#pragma once

#include "sortlab/detail.hpp"

#include <bit>
#include <concepts>
#include <functional>
#include <iterator>
#include <ranges>
#include <utility>

namespace sortlab::detail {

template <std::random_access_iterator I, class Ops>
void bitonic_merge(I first, std::size_t n, bool ascending, Ops& ops) {
    if (n < 2) {
        return;
    }

    const std::size_t split = std::bit_floor(n - 1);
    for (std::size_t i = 0; i < n - split; ++i) {
        I left = first + static_cast<std::iter_difference_t<I>>(i);
        I right = first + static_cast<std::iter_difference_t<I>>(i + split);
        const bool out_of_order = ops.less(*right, *left);
        if (out_of_order == ascending) {
            ops.swap(left, right);
        }
    }

    bitonic_merge(first, split, ascending, ops);
    bitonic_merge(first + static_cast<std::iter_difference_t<I>>(split), n - split,
                  ascending, ops);
}

template <std::random_access_iterator I, class Ops>
void bitonic_impl(I first, std::size_t n, bool ascending, Ops& ops) {
    if (n < 2) {
        return;
    }

    const std::size_t split = n / 2;
    bitonic_impl(first, split, !ascending, ops);
    bitonic_impl(first + static_cast<std::iter_difference_t<I>>(split), n - split, ascending,
                 ops);
    bitonic_merge(first, n, ascending, ops);
}

}  // namespace sortlab::detail

namespace sortlab {

// Generic bitonic-network treatment for arbitrary n. The implementation uses
// the greatest-power-of-two merge construction rather than sentinel padding.
template <std::random_access_iterator I, class Comp = std::ranges::less,
          class Proj = std::identity>
requires std::sortable<I, Comp, Proj>
void bitonic_sort(I first, I last, Comp comp = {}, Proj proj = {}) {
    null_observer observer;
    detail::operations ops(std::move(comp), std::move(proj), observer);
    detail::bitonic_impl(first, static_cast<std::size_t>(last - first), true, ops);
}

template <std::ranges::random_access_range R, class Comp = std::ranges::less,
          class Proj = std::identity>
requires std::sortable<std::ranges::iterator_t<R>, Comp, Proj>
void bitonic_sort(R&& range, Comp comp = {}, Proj proj = {}) {
    bitonic_sort(std::ranges::begin(range), std::ranges::end(range), std::move(comp),
                 std::move(proj));
}

namespace instrumented {

template <std::random_access_iterator I, class Observer, class Comp = std::ranges::less,
          class Proj = std::identity>
requires std::sortable<I, Comp, Proj>
void bitonic_sort(I first, I last, Observer& observer, Comp comp = {}, Proj proj = {}) {
    detail::operations ops(std::move(comp), std::move(proj), observer);
    detail::bitonic_impl(first, static_cast<std::size_t>(last - first), true, ops);
}

}  // namespace instrumented
}  // namespace sortlab
