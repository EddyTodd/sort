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

#define SORTLAB_DECLARE_COMPARE_SORT(NAME, IMPL) \
template <std::random_access_iterator I, class Comp=std::ranges::less, class Proj=std::identity> \
requires std::sortable<I,Comp,Proj> \
void NAME(I first,I last,Comp comp={},Proj proj={}){ null_observer obs; detail::operations ops(std::move(comp),std::move(proj),obs); detail::IMPL(first,last,ops); } \
template <std::ranges::random_access_range R, class Comp=std::ranges::less, class Proj=std::identity> \
requires std::sortable<std::ranges::iterator_t<R>,Comp,Proj> \
void NAME(R&& r,Comp comp={},Proj proj={}){ NAME(std::ranges::begin(r),std::ranges::end(r),std::move(comp),std::move(proj)); }

SORTLAB_DECLARE_COMPARE_SORT(insertion_sort,insertion_impl)
SORTLAB_DECLARE_COMPARE_SORT(binary_insertion_sort,binary_insertion_impl)
SORTLAB_DECLARE_COMPARE_SORT(selection_sort,selection_impl)
SORTLAB_DECLARE_COMPARE_SORT(bubble_sort,bubble_impl)
SORTLAB_DECLARE_COMPARE_SORT(comb_sort,comb_impl)
SORTLAB_DECLARE_COMPARE_SORT(heap_sort,heap_impl)
SORTLAB_DECLARE_COMPARE_SORT(merge_sort,merge_impl)
SORTLAB_DECLARE_COMPARE_SORT(merge_bottom_up_sort,merge_bottom_up_impl)
SORTLAB_DECLARE_COMPARE_SORT(natural_merge_sort,natural_merge_impl)

#undef SORTLAB_DECLARE_COMPARE_SORT

template <std::random_access_iterator I,class Comp=std::ranges::less,class Proj=std::identity>
requires std::sortable<I,Comp,Proj>
void shell_ciura_sort(I first,I last,Comp comp={},Proj proj={}){null_observer obs;detail::operations ops(std::move(comp),std::move(proj),obs);detail::shell_with_gaps(first,last,ops,detail::ciura_gaps(static_cast<std::size_t>(last-first)));}
template <std::ranges::random_access_range R,class Comp=std::ranges::less,class Proj=std::identity>
requires std::sortable<std::ranges::iterator_t<R>,Comp,Proj>
void shell_ciura_sort(R&& r,Comp comp={},Proj proj={}){shell_ciura_sort(std::ranges::begin(r),std::ranges::end(r),std::move(comp),std::move(proj));}

template <std::random_access_iterator I,class Comp=std::ranges::less,class Proj=std::identity>
requires std::sortable<I,Comp,Proj>
void shell_pratt_sort(I first,I last,Comp comp={},Proj proj={}){null_observer obs;detail::operations ops(std::move(comp),std::move(proj),obs);detail::shell_with_gaps(first,last,ops,detail::pratt_gaps(static_cast<std::size_t>(last-first)));}
template <std::ranges::random_access_range R,class Comp=std::ranges::less,class Proj=std::identity>
requires std::sortable<std::ranges::iterator_t<R>,Comp,Proj>
void shell_pratt_sort(R&& r,Comp comp={},Proj proj={}){shell_pratt_sort(std::ranges::begin(r),std::ranges::end(r),std::move(comp),std::move(proj));}

template <std::random_access_iterator I,class Comp=std::ranges::less,class Proj=std::identity>
requires std::sortable<I,Comp,Proj>
void merge_insertion_sort(I first,I last,std::size_t cutoff=24,Comp comp={},Proj proj={}){null_observer obs;detail::operations ops(std::move(comp),std::move(proj),obs);detail::merge_insertion_impl(first,last,std::max<std::size_t>(1,cutoff),ops);}

template <std::random_access_iterator I,class Comp=std::ranges::less,class Proj=std::identity>
requires std::sortable<I,Comp,Proj> && std::copy_constructible<std::iter_value_t<I>>
void quick_hoare_sort(I first,I last,Comp comp={},Proj proj={}){null_observer obs;detail::operations ops(std::move(comp),std::move(proj),obs);detail::quick_hoare_impl(first,last,ops);}

template <std::random_access_iterator I,class Comp=std::ranges::less,class Proj=std::identity>
requires std::sortable<I,Comp,Proj> && std::copy_constructible<std::iter_value_t<I>>
void quick_3way_sort(I first,I last,Comp comp={},Proj proj={}){null_observer obs;detail::operations ops(std::move(comp),std::move(proj),obs);detail::quick3_impl(first,last,ops);}

template <std::random_access_iterator I,class Comp=std::ranges::less,class Proj=std::identity>
requires std::sortable<I,Comp,Proj> && std::copy_constructible<std::iter_value_t<I>>
void quick_median3_sort(I first,I last,Comp comp={},Proj proj={}){null_observer obs;detail::operations ops(std::move(comp),std::move(proj),obs);detail::quick_median3_impl(first,last,1,ops);}

template <std::random_access_iterator I,class Comp=std::ranges::less,class Proj=std::identity>
requires std::sortable<I,Comp,Proj> && std::copy_constructible<std::iter_value_t<I>>
void quick_insertion_sort(I first,I last,std::size_t cutoff=24,Comp comp={},Proj proj={}){null_observer obs;detail::operations ops(std::move(comp),std::move(proj),obs);detail::quick_median3_impl(first,last,std::max<std::size_t>(1,cutoff),ops);}

template <std::random_access_iterator I,class Comp=std::ranges::less,class Proj=std::identity>
requires std::sortable<I,Comp,Proj> && std::copy_constructible<std::iter_value_t<I>>
void dual_pivot_sort(I first,I last,Comp comp={},Proj proj={}){null_observer obs;detail::operations ops(std::move(comp),std::move(proj),obs);detail::dual_pivot_impl(first,last,ops);}

template <std::random_access_iterator I,class Comp=std::ranges::less,class Proj=std::identity>
requires std::sortable<I,Comp,Proj> && std::copy_constructible<std::iter_value_t<I>>
void intro_sort(I first,I last,std::size_t cutoff=24,Comp comp={},Proj proj={}){null_observer obs;detail::operations ops(std::move(comp),std::move(proj),obs);if(last-first<2)return;auto bits=std::bit_width(static_cast<std::size_t>(last-first));detail::intro_impl(first,last,static_cast<unsigned>(2*(bits-1)),std::max<std::size_t>(1,cutoff),ops);}

template <std::ranges::random_access_range R,class Comp=std::ranges::less,class Proj=std::identity>
requires std::sortable<std::ranges::iterator_t<R>,Comp,Proj>
void merge_insertion_sort(R&& r,std::size_t cutoff=24,Comp comp={},Proj proj={}){merge_insertion_sort(std::ranges::begin(r),std::ranges::end(r),cutoff,std::move(comp),std::move(proj));}
template <std::ranges::random_access_range R,class Comp=std::ranges::less,class Proj=std::identity>
requires std::sortable<std::ranges::iterator_t<R>,Comp,Proj> && std::copy_constructible<std::ranges::range_value_t<R>>
void quick_hoare_sort(R&& r,Comp comp={},Proj proj={}){quick_hoare_sort(std::ranges::begin(r),std::ranges::end(r),std::move(comp),std::move(proj));}
template <std::ranges::random_access_range R,class Comp=std::ranges::less,class Proj=std::identity>
requires std::sortable<std::ranges::iterator_t<R>,Comp,Proj> && std::copy_constructible<std::ranges::range_value_t<R>>
void quick_3way_sort(R&& r,Comp comp={},Proj proj={}){quick_3way_sort(std::ranges::begin(r),std::ranges::end(r),std::move(comp),std::move(proj));}
template <std::ranges::random_access_range R,class Comp=std::ranges::less,class Proj=std::identity>
requires std::sortable<std::ranges::iterator_t<R>,Comp,Proj> && std::copy_constructible<std::ranges::range_value_t<R>>
void quick_median3_sort(R&& r,Comp comp={},Proj proj={}){quick_median3_sort(std::ranges::begin(r),std::ranges::end(r),std::move(comp),std::move(proj));}
template <std::ranges::random_access_range R,class Comp=std::ranges::less,class Proj=std::identity>
requires std::sortable<std::ranges::iterator_t<R>,Comp,Proj> && std::copy_constructible<std::ranges::range_value_t<R>>
void quick_insertion_sort(R&& r,std::size_t cutoff=24,Comp comp={},Proj proj={}){quick_insertion_sort(std::ranges::begin(r),std::ranges::end(r),cutoff,std::move(comp),std::move(proj));}
template <std::ranges::random_access_range R,class Comp=std::ranges::less,class Proj=std::identity>
requires std::sortable<std::ranges::iterator_t<R>,Comp,Proj> && std::copy_constructible<std::ranges::range_value_t<R>>
void dual_pivot_sort(R&& r,Comp comp={},Proj proj={}){dual_pivot_sort(std::ranges::begin(r),std::ranges::end(r),std::move(comp),std::move(proj));}
template <std::ranges::random_access_range R,class Comp=std::ranges::less,class Proj=std::identity>
requires std::sortable<std::ranges::iterator_t<R>,Comp,Proj> && std::copy_constructible<std::ranges::range_value_t<R>>
void intro_sort(R&& r,std::size_t cutoff=24,Comp comp={},Proj proj={}){intro_sort(std::ranges::begin(r),std::ranges::end(r),cutoff,std::move(comp),std::move(proj));}

} // namespace sortlab

#include "sortlab/instrumented_comparison.hpp"
