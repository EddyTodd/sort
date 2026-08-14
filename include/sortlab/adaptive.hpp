#pragma once

#include "sortlab/adaptive_options.hpp"
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
void timsort(I first, I last, Observer& observer, AdaptiveSortOptions options = {}, Comp comp = {},
             Proj proj = {}) {
    detail::adaptive_context<I, Comp, Proj, Observer> context(
        first, last, std::move(comp), std::move(proj), observer, options);
    detail::timsort_with_context(context);
}

template <std::ranges::random_access_range R, class Observer, class Comp = std::ranges::less,
          class Proj = std::identity>
requires std::sortable<std::ranges::iterator_t<R>, Comp, Proj>
void timsort(R&& range, Observer& observer, AdaptiveSortOptions options = {}, Comp comp = {},
             Proj proj = {}) {
    timsort(std::ranges::begin(range), std::ranges::end(range), observer, options,
            std::move(comp), std::move(proj));
}

template <std::random_access_iterator I, class Observer, class Comp = std::ranges::less,
          class Proj = std::identity>
requires std::sortable<I, Comp, Proj>
void powersort(I first, I last, Observer& observer, AdaptiveSortOptions options = {},
               Comp comp = {}, Proj proj = {}) {
    detail::adaptive_context<I, Comp, Proj, Observer> context(
        first, last, std::move(comp), std::move(proj), observer, options);
    detail::powersort_with_context(context);
}

template <std::ranges::random_access_range R, class Observer, class Comp = std::ranges::less,
          class Proj = std::identity>
requires std::sortable<std::ranges::iterator_t<R>, Comp, Proj>
void powersort(R&& range, Observer& observer, AdaptiveSortOptions options = {}, Comp comp = {},
               Proj proj = {}) {
    powersort(std::ranges::begin(range), std::ranges::end(range), observer, options,
              std::move(comp), std::move(proj));
}

}  // namespace instrumented

template <std::random_access_iterator I, class Comp = std::ranges::less,
          class Proj = std::identity>
requires std::sortable<I, Comp, Proj>
void timsort(I first, I last, AdaptiveSortOptions options = {}, Comp comp = {}, Proj proj = {}) {
    null_observer observer;
    instrumented::timsort(first, last, observer, options, std::move(comp), std::move(proj));
}

template <std::ranges::random_access_range R, class Comp = std::ranges::less,
          class Proj = std::identity>
requires std::sortable<std::ranges::iterator_t<R>, Comp, Proj>
void timsort(R&& range, AdaptiveSortOptions options = {}, Comp comp = {}, Proj proj = {}) {
    timsort(std::ranges::begin(range), std::ranges::end(range), options, std::move(comp),
            std::move(proj));
}

template <std::random_access_iterator I, class Comp = std::ranges::less,
          class Proj = std::identity>
requires std::sortable<I, Comp, Proj>
void powersort(I first, I last, AdaptiveSortOptions options = {}, Comp comp = {}, Proj proj = {}) {
    null_observer observer;
    instrumented::powersort(first, last, observer, options, std::move(comp), std::move(proj));
}

template <std::ranges::random_access_range R, class Comp = std::ranges::less,
          class Proj = std::identity>
requires std::sortable<std::ranges::iterator_t<R>, Comp, Proj>
void powersort(R&& range, AdaptiveSortOptions options = {}, Comp comp = {}, Proj proj = {}) {
    powersort(std::ranges::begin(range), std::ranges::end(range), options, std::move(comp),
              std::move(proj));
}

}  // namespace sortlab
