#pragma once

#include "sortlab/detail/comparison_quick.hpp"

#include <algorithm>
#include <bit>
#include <concepts>
#include <cstddef>
#include <functional>
#include <iterator>
#include <ranges>
#include <utility>

namespace sortlab::instrumented {
namespace detail {

template <class Observer, class Comp, class Proj, class Function>
void run(Observer& observer, Comp comp, Proj proj, Function&& function) {
    sortlab::detail::operations ops(std::move(comp), std::move(proj), observer);
    std::forward<Function>(function)(ops);
}

}  // namespace detail

template <std::random_access_iterator I, class Observer, class Comp = std::ranges::less,
          class Proj = std::identity>
requires std::sortable<I, Comp, Proj>
void insertion_sort(I first, I last, Observer& observer, Comp comp = {}, Proj proj = {}) {
    detail::run(observer, std::move(comp), std::move(proj),
                [&](auto& ops) { sortlab::detail::insertion_impl(first, last, ops); });
}

template <std::random_access_iterator I, class Observer, class Comp = std::ranges::less,
          class Proj = std::identity>
requires std::sortable<I, Comp, Proj>
void binary_insertion_sort(I first, I last, Observer& observer, Comp comp = {}, Proj proj = {}) {
    detail::run(observer, std::move(comp), std::move(proj),
                [&](auto& ops) { sortlab::detail::binary_insertion_impl(first, last, ops); });
}

template <std::random_access_iterator I, class Observer, class Comp = std::ranges::less,
          class Proj = std::identity>
requires std::sortable<I, Comp, Proj>
void selection_sort(I first, I last, Observer& observer, Comp comp = {}, Proj proj = {}) {
    detail::run(observer, std::move(comp), std::move(proj),
                [&](auto& ops) { sortlab::detail::selection_impl(first, last, ops); });
}

template <std::random_access_iterator I, class Observer, class Comp = std::ranges::less,
          class Proj = std::identity>
requires std::sortable<I, Comp, Proj>
void bubble_sort(I first, I last, Observer& observer, Comp comp = {}, Proj proj = {}) {
    detail::run(observer, std::move(comp), std::move(proj),
                [&](auto& ops) { sortlab::detail::bubble_impl(first, last, ops); });
}

template <std::random_access_iterator I, class Observer, class Comp = std::ranges::less,
          class Proj = std::identity>
requires std::sortable<I, Comp, Proj>
void comb_sort(I first, I last, Observer& observer, Comp comp = {}, Proj proj = {}) {
    detail::run(observer, std::move(comp), std::move(proj),
                [&](auto& ops) { sortlab::detail::comb_impl(first, last, ops); });
}

template <std::random_access_iterator I, class Observer, class Comp = std::ranges::less,
          class Proj = std::identity>
requires std::sortable<I, Comp, Proj>
void shell_ciura_sort(I first, I last, Observer& observer, Comp comp = {}, Proj proj = {}) {
    detail::run(observer, std::move(comp), std::move(proj), [&](auto& ops) {
        sortlab::detail::shell_with_gaps(
            first, last, ops,
            sortlab::detail::ciura_gaps(static_cast<std::size_t>(last - first)));
    });
}

template <std::random_access_iterator I, class Observer, class Comp = std::ranges::less,
          class Proj = std::identity>
requires std::sortable<I, Comp, Proj>
void shell_pratt_sort(I first, I last, Observer& observer, Comp comp = {}, Proj proj = {}) {
    detail::run(observer, std::move(comp), std::move(proj), [&](auto& ops) {
        sortlab::detail::shell_with_gaps(
            first, last, ops,
            sortlab::detail::pratt_gaps(static_cast<std::size_t>(last - first)));
    });
}

template <std::random_access_iterator I, class Observer, class Comp = std::ranges::less,
          class Proj = std::identity>
requires std::sortable<I, Comp, Proj>
void heap_sort(I first, I last, Observer& observer, Comp comp = {}, Proj proj = {}) {
    detail::run(observer, std::move(comp), std::move(proj),
                [&](auto& ops) { sortlab::detail::heap_impl(first, last, ops); });
}

template <std::random_access_iterator I, class Observer, class Comp = std::ranges::less,
          class Proj = std::identity>
requires std::sortable<I, Comp, Proj>
void merge_sort(I first, I last, Observer& observer, Comp comp = {}, Proj proj = {}) {
    detail::run(observer, std::move(comp), std::move(proj),
                [&](auto& ops) { sortlab::detail::merge_impl(first, last, ops); });
}

template <std::random_access_iterator I, class Observer, class Comp = std::ranges::less,
          class Proj = std::identity>
requires std::sortable<I, Comp, Proj>
void merge_bottom_up_sort(I first, I last, Observer& observer, Comp comp = {}, Proj proj = {}) {
    detail::run(observer, std::move(comp), std::move(proj),
                [&](auto& ops) { sortlab::detail::merge_bottom_up_impl(first, last, ops); });
}

template <std::random_access_iterator I, class Observer, class Comp = std::ranges::less,
          class Proj = std::identity>
requires std::sortable<I, Comp, Proj>
void natural_merge_sort(I first, I last, Observer& observer, Comp comp = {}, Proj proj = {}) {
    detail::run(observer, std::move(comp), std::move(proj),
                [&](auto& ops) { sortlab::detail::natural_merge_impl(first, last, ops); });
}

template <std::random_access_iterator I, class Observer, class Comp = std::ranges::less,
          class Proj = std::identity>
requires std::sortable<I, Comp, Proj>
void merge_insertion_sort(I first, I last, Observer& observer, std::size_t cutoff = 24,
                          Comp comp = {}, Proj proj = {}) {
    detail::run(observer, std::move(comp), std::move(proj), [&](auto& ops) {
        sortlab::detail::merge_insertion_impl(first, last, std::max<std::size_t>(1, cutoff), ops);
    });
}

template <std::random_access_iterator I, class Observer, class Comp = std::ranges::less,
          class Proj = std::identity>
requires std::sortable<I, Comp, Proj> && std::copy_constructible<std::iter_value_t<I>>
void quick_hoare_sort(I first, I last, Observer& observer, Comp comp = {}, Proj proj = {}) {
    detail::run(observer, std::move(comp), std::move(proj),
                [&](auto& ops) { sortlab::detail::quick_hoare_impl(first, last, ops); });
}

template <std::random_access_iterator I, class Observer, class Comp = std::ranges::less,
          class Proj = std::identity>
requires std::sortable<I, Comp, Proj> && std::copy_constructible<std::iter_value_t<I>>
void quick_3way_sort(I first, I last, Observer& observer, Comp comp = {}, Proj proj = {}) {
    detail::run(observer, std::move(comp), std::move(proj),
                [&](auto& ops) { sortlab::detail::quick3_impl(first, last, ops); });
}

template <std::random_access_iterator I, class Observer, class Comp = std::ranges::less,
          class Proj = std::identity>
requires std::sortable<I, Comp, Proj> && std::copy_constructible<std::iter_value_t<I>>
void quick_median3_sort(I first, I last, Observer& observer, Comp comp = {}, Proj proj = {}) {
    detail::run(observer, std::move(comp), std::move(proj), [&](auto& ops) {
        sortlab::detail::quick_median3_impl(first, last, 1, ops);
    });
}

template <std::random_access_iterator I, class Observer, class Comp = std::ranges::less,
          class Proj = std::identity>
requires std::sortable<I, Comp, Proj> && std::copy_constructible<std::iter_value_t<I>>
void quick_insertion_sort(I first, I last, Observer& observer, std::size_t cutoff = 24,
                          Comp comp = {}, Proj proj = {}) {
    detail::run(observer, std::move(comp), std::move(proj), [&](auto& ops) {
        sortlab::detail::quick_median3_impl(first, last, std::max<std::size_t>(1, cutoff), ops);
    });
}

template <std::random_access_iterator I, class Observer, class Comp = std::ranges::less,
          class Proj = std::identity>
requires std::sortable<I, Comp, Proj> && std::copy_constructible<std::iter_value_t<I>>
void dual_pivot_sort(I first, I last, Observer& observer, Comp comp = {}, Proj proj = {}) {
    detail::run(observer, std::move(comp), std::move(proj),
                [&](auto& ops) { sortlab::detail::dual_pivot_impl(first, last, ops); });
}

template <std::random_access_iterator I, class Observer, class Comp = std::ranges::less,
          class Proj = std::identity>
requires std::sortable<I, Comp, Proj> && std::copy_constructible<std::iter_value_t<I>>
void intro_sort(I first, I last, Observer& observer, std::size_t cutoff = 24, Comp comp = {},
                Proj proj = {}) {
    detail::run(observer, std::move(comp), std::move(proj), [&](auto& ops) {
        if (last - first < 2) return;
        const auto bits = std::bit_width(static_cast<std::size_t>(last - first));
        sortlab::detail::intro_impl(first, last, static_cast<unsigned>(2 * (bits - 1)),
                                    std::max<std::size_t>(1, cutoff), ops);
    });
}

}  // namespace sortlab::instrumented
