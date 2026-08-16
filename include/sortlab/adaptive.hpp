#pragma once

#include "sortlab/adaptive_options.hpp"
#include "sortlab/detail.hpp"
#include "sortlab/detail/adaptive_schedule.hpp"
#include "sortlab/instrumentation.hpp"

#include <concepts>
#include <functional>
#include <iterator>
#include <ranges>
#include <utility>

namespace sortlab {
namespace instrumented {

template <std::random_access_iterator I, class Observer, class Comp = std::ranges::less,
          class Proj = std::identity>
requires std::sortable<I, Comp, Proj>
void timsort(I first, I last, Observer& observer, Comp comp = {}, Proj proj = {},
             AdaptiveSortOptions options = {}) {
    detail::operations<Comp, Proj, Observer> operations(std::move(comp), std::move(proj),
                                                        observer);
    detail::timsort_impl(first, last, operations, options);
}

template <std::ranges::random_access_range R, class Observer, class Comp = std::ranges::less,
          class Proj = std::identity>
requires std::sortable<std::ranges::iterator_t<R>, Comp, Proj>
void timsort(R&& range, Observer& observer, Comp comp = {}, Proj proj = {},
             AdaptiveSortOptions options = {}) {
    timsort(std::ranges::begin(range), std::ranges::end(range), observer, std::move(comp),
            std::move(proj), options);
}

template <std::random_access_iterator I, class Observer, class Comp = std::ranges::less,
          class Proj = std::identity>
requires std::sortable<I, Comp, Proj>
void powersort(I first, I last, Observer& observer, Comp comp = {}, Proj proj = {},
               AdaptiveSortOptions options = {}) {
    detail::operations<Comp, Proj, Observer> operations(std::move(comp), std::move(proj),
                                                        observer);
    detail::powersort_impl(first, last, operations, options);
}

template <std::ranges::random_access_range R, class Observer, class Comp = std::ranges::less,
          class Proj = std::identity>
requires std::sortable<std::ranges::iterator_t<R>, Comp, Proj>
void powersort(R&& range, Observer& observer, Comp comp = {}, Proj proj = {},
               AdaptiveSortOptions options = {}) {
    powersort(std::ranges::begin(range), std::ranges::end(range), observer, std::move(comp),
              std::move(proj), options);
}

}  // namespace instrumented

template <std::random_access_iterator I, class Comp = std::ranges::less,
          class Proj = std::identity>
requires std::sortable<I, Comp, Proj>
void timsort(I first, I last, Comp comp = {}, Proj proj = {}, AdaptiveSortOptions options = {}) {
    null_observer observer;
    instrumented::timsort(first, last, observer, std::move(comp), std::move(proj), options);
}

template <std::ranges::random_access_range R, class Comp = std::ranges::less,
          class Proj = std::identity>
requires std::sortable<std::ranges::iterator_t<R>, Comp, Proj>
void timsort(R&& range, Comp comp = {}, Proj proj = {}, AdaptiveSortOptions options = {}) {
    timsort(std::ranges::begin(range), std::ranges::end(range), std::move(comp), std::move(proj),
            options);
}

template <std::random_access_iterator I, class Comp = std::ranges::less,
          class Proj = std::identity>
requires std::sortable<I, Comp, Proj>
void powersort(I first, I last, Comp comp = {}, Proj proj = {},
               AdaptiveSortOptions options = {}) {
    null_observer observer;
    instrumented::powersort(first, last, observer, std::move(comp), std::move(proj), options);
}

template <std::ranges::random_access_range R, class Comp = std::ranges::less,
          class Proj = std::identity>
requires std::sortable<std::ranges::iterator_t<R>, Comp, Proj>
void powersort(R&& range, Comp comp = {}, Proj proj = {}, AdaptiveSortOptions options = {}) {
    powersort(std::ranges::begin(range), std::ranges::end(range), std::move(comp),
              std::move(proj), options);
}

}  // namespace sortlab
