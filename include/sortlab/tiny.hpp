#pragma once

#include "sortlab/detail.hpp"

#include <bit>
#include <concepts>
#include <functional>
#include <iterator>
#include <ranges>
#include <stdexcept>
#include <utility>

namespace sortlab::detail {
template <std::random_access_iterator I,class Ops>
void bitonic_merge(I first,std::size_t n,bool ascending,Ops& ops){
  if (n < 2) return;
  std::size_t k = std::bit_floor(n - 1);
  for(std::size_t i=0;i<n-k;++i){I a=first+static_cast<std::iter_difference_t<I>>(i),b=first+static_cast<std::iter_difference_t<I>>(i+k);bool out=ops.less(*b,*a);if(out==ascending)ops.swap(a,b);}
  bitonic_merge(first,k,ascending,ops);bitonic_merge(first+static_cast<std::iter_difference_t<I>>(k),n-k,ascending,ops);
}
template <std::random_access_iterator I,class Ops>
void bitonic_impl(I first,std::size_t n,bool ascending,Ops& ops){if(n<2)return;std::size_t k=n/2;bitonic_impl(first,k,!ascending,ops);bitonic_impl(first+static_cast<std::iter_difference_t<I>>(k),n-k,ascending,ops);bitonic_merge(first,n,ascending,ops);}
}

namespace sortlab {
// Generic bitonic network treatment. It accepts arbitrary n, using the standard
// greatest-power-of-two merge construction rather than sentinel padding.
template <std::random_access_iterator I,class Comp=std::ranges::less,class Proj=std::identity>
requires std::sortable<I,Comp,Proj>
void bitonic_sort(I first,I last,Comp comp={},Proj proj={}){null_observer observer;detail::operations ops(std::move(comp),std::move(proj),observer);detail::bitonic_impl(first,static_cast<std::size_t>(last-first),true,ops);}
template <std::ranges::random_access_range R,class Comp=std::ranges::less,class Proj=std::identity>
requires std::sortable<std::ranges::iterator_t<R>,Comp,Proj>
void bitonic_sort(R&& r,Comp comp={},Proj proj={}){bitonic_sort(std::ranges::begin(r),std::ranges::end(r),std::move(comp),std::move(proj));}

namespace instrumented {
template <std::random_access_iterator I,class Observer,class Comp=std::ranges::less,class Proj=std::identity>
requires std::sortable<I,Comp,Proj>
void bitonic_sort(I first,I last,Observer& observer,Comp comp={},Proj proj={}){detail::operations ops(std::move(comp),std::move(proj),observer);detail::bitonic_impl(first,static_cast<std::size_t>(last-first),true,ops);}
}
}
