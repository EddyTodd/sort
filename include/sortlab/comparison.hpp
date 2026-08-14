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

namespace sortlab {
namespace detail {

template <class Comp, class Proj, class Function>
void run_uninstrumented(Comp comp, Proj proj, Function&& function) {
    null_observer observer;
    operations ops(std::move(comp), std::move(proj), observer);
    std::forward<Function>(function)(ops);
}

}  // namespace detail

template <std::random_access_iterator I, class Comp = std::ranges::less,
          class Proj = std::identity>
requires std::sortable<I, Comp, Proj>
void insertion_sort(I first, I last, Comp comp = {}, Proj proj = {}) {
    detail::run_uninstrumented(std::move(comp), std::move(proj),
                               [&](auto& ops) { detail::insertion_impl(first, last, ops); });
}

template <std::ranges::random_access_range R, class Comp = std::ranges::less,
          class Proj = std::identity>
requires std::sortable<std::ranges::iterator_t<R>, Comp, Proj>
void insertion_sort(R&& range, Comp comp = {}, Proj proj = {}) {
    insertion_sort(std::ranges::begin(range), std::ranges::end(range), std::move(comp),
                   std::move(proj));
}

template <std::random_access_iterator I, class Comp = std::ranges::less,
          class Proj = std::identity>
requires std::sortable<I, Comp, Proj>
void binary_insertion_sort(I first, I last, Comp comp = {}, Proj proj = {}) {
    detail::run_uninstrumented(
        std::move(comp), std::move(proj),
        [&](auto& ops) { detail::binary_insertion_impl(first, last, ops); });
}

template <std::ranges::random_access_range R, class Comp = std::ranges::less,
          class Proj = std::identity>
requires std::sortable<std::ranges::iterator_t<R>, Comp, Proj>
void binary_insertion_sort(R&& range, Comp comp = {}, Proj proj = {}) {
    binary_insertion_sort(std::ranges::begin(range), std::ranges::end(range), std::move(comp),
                          std::move(proj));
}

template <std::random_access_iterator I, class Comp = std::ranges::less,
          class Proj = std::identity>
requires std::sortable<I, Comp, Proj>
void selection_sort(I first, I last, Comp comp = {}, Proj proj = {}) {
    detail::run_uninstrumented(std::move(comp), std::move(proj),
                               [&](auto& ops) { detail::selection_impl(first, last, ops); });
}

template <std::ranges::random_access_range R, class Comp = std::ranges::less,
          class Proj = std::identity>
requires std::sortable<std::ranges::iterator_t<R>, Comp, Proj>
void selection_sort(R&& range, Comp comp = {}, Proj proj = {}) {
    selection_sort(std::ranges::begin(range), std::ranges::end(range), std::move(comp),
                   std::move(proj));
}

template <std::random_access_iterator I, class Comp = std::ranges::less,
          class Proj = std::identity>
requires std::sortable<I, Comp, Proj>
void bubble_sort(I first, I last, Comp comp = {}, Proj proj = {}) {
    detail::run_uninstrumented(std::move(comp), std::move(proj),
                               [&](auto& ops) { detail::bubble_impl(first, last, ops); });
}

template <std::ranges::random_access_range R, class Comp = std::ranges::less,
          class Proj = std::identity>
requires std::sortable<std::ranges::iterator_t<R>, Comp, Proj>
void bubble_sort(R&& range, Comp comp = {}, Proj proj = {}) {
    bubble_sort(std::ranges::begin(range), std::ranges::end(range), std::move(comp),
                std::move(proj));
}

template <std::random_access_iterator I, class Comp = std::ranges::less,
          class Proj = std::identity>
requires std::sortable<I, Comp, Proj>
void comb_sort(I first, I last, Comp comp = {}, Proj proj = {}) {
    detail::run_uninstrumented(std::move(comp), std::move(proj),
                               [&](auto& ops) { detail::comb_impl(first, last, ops); });
}

template <std::ranges::random_access_range R, class Comp = std::ranges::less,
          class Proj = std::identity>
requires std::sortable<std::ranges::iterator_t<R>, Comp, Proj>
void comb_sort(R&& range, Comp comp = {}, Proj proj = {}) {
    comb_sort(std::ranges::begin(range), std::ranges::end(range), std::move(comp),
              std::move(proj));
}

template <std::random_access_iterator I, class Comp = std::ranges::less,
          class Proj = std::identity>
requires std::sortable<I, Comp, Proj>
void shell_ciura_sort(I first, I last, Comp comp = {}, Proj proj = {}) {
    detail::run_uninstrumented(std::move(comp), std::move(proj), [&](auto& ops) {
        detail::shell_with_gaps(first, last, ops,
                                detail::ciura_gaps(static_cast<std::size_t>(last - first)));
    });
}

template <std::ranges::random_access_range R, class Comp = std::ranges::less,
          class Proj = std::identity>
requires std::sortable<std::ranges::iterator_t<R>, Comp, Proj>
void shell_ciura_sort(R&& range, Comp comp = {}, Proj proj = {}) {
    shell_ciura_sort(std::ranges::begin(range), std::ranges::end(range), std::move(comp),
                     std::move(proj));
}

template <std::random_access_iterator I, class Comp = std::ranges::less,
          class Proj = std::identity>
requires std::sortable<I, Comp, Proj>
void shell_pratt_sort(I first, I last, Comp comp = {}, Proj proj = {}) {
    detail::run_uninstrumented(std::move(comp), std::move(proj), [&](auto& ops) {
        detail::shell_with_gaps(first, last, ops,
                                detail::pratt_gaps(static_cast<std::size_t>(last - first)));
    });
}

template <std::ranges::random_access_range R, class Comp = std::ranges::less,
          class Proj = std::identity>
requires std::sortable<std::ranges::iterator_t<R>, Comp, Proj>
void shell_pratt_sort(R&& range, Comp comp = {}, Proj proj = {}) {
    shell_pratt_sort(std::ranges::begin(range), std::ranges::end(range), std::move(comp),
                     std::move(proj));
}

template <std::random_access_iterator I, class Comp = std::ranges::less,
          class Proj = std::identity>
requires std::sortable<I, Comp, Proj>
void heap_sort(I first, I last, Comp comp = {}, Proj proj = {}) {
    detail::run_uninstrumented(std::move(comp), std::move(proj),
                               [&](auto& ops) { detail::heap_impl(first, last, ops); });
}

template <std::ranges::random_access_range R, class Comp = std::ranges::less,
          class Proj = std::identity>
requires std::sortable<std::ranges::iterator_t<R>, Comp, Proj>
void heap_sort(R&& range, Comp comp = {}, Proj proj = {}) {
    heap_sort(std::ranges::begin(range), std::ranges::end(range), std::move(comp),
              std::move(proj));
}

template <std::random_access_iterator I, class Comp = std::ranges::less,
          class Proj = std::identity>
requires std::sortable<I, Comp, Proj>
void merge_sort(I first, I last, Comp comp = {}, Proj proj = {}) {
    detail::run_uninstrumented(std::move(comp), std::move(proj),
                               [&](auto& ops) { detail::merge_impl(first, last, ops); });
}

template <std::ranges::random_access_range R, class Comp = std::ranges::less,
          class Proj = std::identity>
requires std::sortable<std::ranges::iterator_t<R>, Comp, Proj>
void merge_sort(R&& range, Comp comp = {}, Proj proj = {}) {
    merge_sort(std::ranges::begin(range), std::ranges::end(range), std::move(comp),
               std::move(proj));
}

template <std::random_access_iterator I, class Comp = std::ranges::less,
          class Proj = std::identity>
requires std::sortable<I, Comp, Proj>
void merge_bottom_up_sort(I first, I last, Comp comp = {}, Proj proj = {}) {
    detail::run_uninstrumented(
        std::move(comp), std::move(proj),
        [&](auto& ops) { detail::merge_bottom_up_impl(first, last, ops); });
}

template <std::ranges::random_access_range R, class Comp = std::ranges::less,
          class Proj = std::identity>
requires std::sortable<std::ranges::iterator_t<R>, Comp, Proj>
void merge_bottom_up_sort(R&& range, Comp comp = {}, Proj proj = {}) {
    merge_bottom_up_sort(std::ranges::begin(range), std::ranges::end(range), std::move(comp),
                         std::move(proj));
}

template <std::random_access_iterator I, class Comp = std::ranges::less,
          class Proj = std::identity>
requires std::sortable<I, Comp, Proj>
void natural_merge_sort(I first, I last, Comp comp = {}, Proj proj = {}) {
    detail::run_uninstrumented(
        std::move(comp), std::move(proj),
        [&](auto& ops) { detail::natural_merge_impl(first, last, ops); });
}

template <std::ranges::random_access_range R, class Comp = std::ranges::less,
          class Proj = std::identity>
requires std::sortable<std::ranges::iterator_t<R>, Comp, Proj>
void natural_merge_sort(R&& range, Comp comp = {}, Proj proj = {}) {
    natural_merge_sort(std::ranges::begin(range), std::ranges::end(range), std::move(comp),
                       std::move(proj));
}

template <std::random_access_iterator I, class Comp = std::ranges::less,
          class Proj = std::identity>
requires std::sortable<I, Comp, Proj>
void merge_insertion_sort(I first, I last, std::size_t cutoff = 24, Comp comp = {},
                          Proj proj = {}) {
    detail::run_uninstrumented(std::move(comp), std::move(proj), [&](auto& ops) {
        detail::merge_insertion_impl(first, last, std::max<std::size_t>(1, cutoff), ops);
    });
}

template <std::ranges::random_access_range R, class Comp = std::ranges::less,
          class Proj = std::identity>
requires std::sortable<std::ranges::iterator_t<R>, Comp, Proj>
void merge_insertion_sort(R&& range, std::size_t cutoff = 24, Comp comp = {}, Proj proj = {}) {
    merge_insertion_sort(std::ranges::begin(range), std::ranges::end(range), cutoff,
                         std::move(comp), std::move(proj));
}

template <std::random_access_iterator I, class Comp = std::ranges::less,
          class Proj = std::identity>
requires std::sortable<I, Comp, Proj> && std::copy_constructible<std::iter_value_t<I>>
void quick_hoare_sort(I first, I last, Comp comp = {}, Proj proj = {}) {
    detail::run_uninstrumented(
        std::move(comp), std::move(proj),
        [&](auto& ops) { detail::quick_hoare_impl(first, last, ops); });
}

template <std::ranges::random_access_range R, class Comp = std::ranges::less,
          class Proj = std::identity>
requires std::sortable<std::ranges::iterator_t<R>, Comp, Proj> &&
         std::copy_constructible<std::ranges::range_value_t<R>>
void quick_hoare_sort(R&& range, Comp comp = {}, Proj proj = {}) {
    quick_hoare_sort(std::ranges::begin(range), std::ranges::end(range), std::move(comp),
                     std::move(proj));
}

template <std::random_access_iterator I, class Comp = std::ranges::less,
          class Proj = std::identity>
requires std::sortable<I, Comp, Proj> && std::copy_constructible<std::iter_value_t<I>>
void quick_3way_sort(I first, I last, Comp comp = {}, Proj proj = {}) {
    detail::run_uninstrumented(std::move(comp), std::move(proj),
                               [&](auto& ops) { detail::quick3_impl(first, last, ops); });
}

template <std::ranges::random_access_range R, class Comp = std::ranges::less,
          class Proj = std::identity>
requires std::sortable<std::ranges::iterator_t<R>, Comp, Proj> &&
         std::copy_constructible<std::ranges::range_value_t<R>>
void quick_3way_sort(R&& range, Comp comp = {}, Proj proj = {}) {
    quick_3way_sort(std::ranges::begin(range), std::ranges::end(range), std::move(comp),
                    std::move(proj));
}

template <std::random_access_iterator I, class Comp = std::ranges::less,
          class Proj = std::identity>
requires std::sortable<I, Comp, Proj> && std::copy_constructible<std::iter_value_t<I>>
void quick_median3_sort(I first, I last, Comp comp = {}, Proj proj = {}) {
    detail::run_uninstrumented(std::move(comp), std::move(proj),
                               [&](auto& ops) { detail::quick_median3_impl(first, last, 1, ops); });
}

template <std::ranges::random_access_range R, class Comp = std::ranges::less,
          class Proj = std::identity>
requires std::sortable<std::ranges::iterator_t<R>, Comp, Proj> &&
         std::copy_constructible<std::ranges::range_value_t<R>>
void quick_median3_sort(R&& range, Comp comp = {}, Proj proj = {}) {
    quick_median3_sort(std::ranges::begin(range), std::ranges::end(range), std::move(comp),
                       std::move(proj));
}

template <std::random_access_iterator I, class Comp = std::ranges::less,
          class Proj = std::identity>
requires std::sortable<I, Comp, Proj> && std::copy_constructible<std::iter_value_t<I>>
void quick_insertion_sort(I first, I last, std::size_t cutoff = 24, Comp comp = {},
                          Proj proj = {}) {
    detail::run_uninstrumented(std::move(comp), std::move(proj), [&](auto& ops) {
        detail::quick_median3_impl(first, last, std::max<std::size_t>(1, cutoff), ops);
    });
}

template <std::ranges::random_access_range R, class Comp = std::ranges::less,
          class Proj = std::identity>
requires std::sortable<std::ranges::iterator_t<R>, Comp, Proj> &&
         std::copy_constructible<std::ranges::range_value_t<R>>
void quick_insertion_sort(R&& range, std::size_t cutoff = 24, Comp comp = {}, Proj proj = {}) {
    quick_insertion_sort(std::ranges::begin(range), std::ranges::end(range), cutoff,
                         std::move(comp), std::move(proj));
}

template <std::random_access_iterator I, class Comp = std::ranges::less,
          class Proj = std::identity>
requires std::sortable<I, Comp, Proj> && std::copy_constructible<std::iter_value_t<I>>
void dual_pivot_sort(I first, I last, Comp comp = {}, Proj proj = {}) {
    detail::run_uninstrumented(std::move(comp), std::move(proj),
                               [&](auto& ops) { detail::dual_pivot_impl(first, last, ops); });
}

template <std::ranges::random_access_range R, class Comp = std::ranges::less,
          class Proj = std::identity>
requires std::sortable<std::ranges::iterator_t<R>, Comp, Proj> &&
         std::copy_constructible<std::ranges::range_value_t<R>>
void dual_pivot_sort(R&& range, Comp comp = {}, Proj proj = {}) {
    dual_pivot_sort(std::ranges::begin(range), std::ranges::end(range), std::move(comp),
                    std::move(proj));
}

template <std::random_access_iterator I, class Comp = std::ranges::less,
          class Proj = std::identity>
requires std::sortable<I, Comp, Proj> && std::copy_constructible<std::iter_value_t<I>>
void intro_sort(I first, I last, std::size_t cutoff = 24, Comp comp = {}, Proj proj = {}) {
    detail::run_uninstrumented(std::move(comp), std::move(proj), [&](auto& ops) {
        if (last - first < 2) return;
        const auto bits = std::bit_width(static_cast<std::size_t>(last - first));
        detail::intro_impl(first, last, static_cast<unsigned>(2 * (bits - 1)),
                           std::max<std::size_t>(1, cutoff), ops);
    });
}

template <std::ranges::random_access_range R, class Comp = std::ranges::less,
          class Proj = std::identity>
requires std::sortable<std::ranges::iterator_t<R>, Comp, Proj> &&
         std::copy_constructible<std::ranges::range_value_t<R>>
void intro_sort(R&& range, std::size_t cutoff = 24, Comp comp = {}, Proj proj = {}) {
    intro_sort(std::ranges::begin(range), std::ranges::end(range), cutoff, std::move(comp),
               std::move(proj));
}

}  // namespace sortlab

#include "sortlab/instrumented_comparison.hpp"
