#pragma once

#include "sortlab/adaptive_options.hpp"
#include "sortlab/detail/adaptive_schedule.hpp"

#include <concepts>
#include <functional>
#include <iterator>
#include <ranges>
#include <utility>

namespace sortlab {
template <std::random_access_iterator I,class Comp=std::ranges::less,class Proj=std::identity>
requires std::sortable<I,Comp,Proj>
void timsort(I first,I last,Comp comp={},Proj proj={},adaptive_options options={}){null_observer observer;detail::operations ops(std::move(comp),std::move(proj),observer);detail::timsort_impl(first,last,ops,options);}
template <std::ranges::random_access_range R,class Comp=std::ranges::less,class Proj=std::identity>
requires std::sortable<std::ranges::iterator_t<R>,Comp,Proj>
void timsort(R&& r,Comp comp={},Proj proj={},adaptive_options options={}){timsort(std::ranges::begin(r),std::ranges::end(r),std::move(comp),std::move(proj),options);}
template <std::random_access_iterator I,class Comp=std::ranges::less,class Proj=std::identity>
requires std::sortable<I,Comp,Proj>
void powersort(I first,I last,Comp comp={},Proj proj={},adaptive_options options={}){null_observer observer;detail::operations ops(std::move(comp),std::move(proj),observer);detail::powersort_impl(first,last,ops,options);}
template <std::ranges::random_access_range R,class Comp=std::ranges::less,class Proj=std::identity>
requires std::sortable<std::ranges::iterator_t<R>,Comp,Proj>
void powersort(R&& r,Comp comp={},Proj proj={},adaptive_options options={}){powersort(std::ranges::begin(r),std::ranges::end(r),std::move(comp),std::move(proj),options);}
namespace instrumented {
template <std::random_access_iterator I,class Observer,class Comp=std::ranges::less,class Proj=std::identity>
requires std::sortable<I,Comp,Proj>
void timsort(I first,I last,Observer& observer,Comp comp={},Proj proj={},adaptive_options options={}){detail::operations ops(std::move(comp),std::move(proj),observer);detail::timsort_impl(first,last,ops,options);}
template <std::random_access_iterator I,class Observer,class Comp=std::ranges::less,class Proj=std::identity>
requires std::sortable<I,Comp,Proj>
void powersort(I first,I last,Observer& observer,Comp comp={},Proj proj={},adaptive_options options={}){detail::operations ops(std::move(comp),std::move(proj),observer);detail::powersort_impl(first,last,ops,options);}
} // namespace instrumented
}  // namespace sortlab
