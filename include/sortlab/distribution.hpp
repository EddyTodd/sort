#pragma once

#include "sortlab/detail.hpp"

#include <array>
#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <ranges>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace sortlab::detail {

template <std::integral T>
requires (!std::same_as<std::remove_cv_t<T>, bool>)
using unsigned_key_t = std::make_unsigned_t<std::remove_cv_t<T>>;

template <std::integral T>
requires (!std::same_as<std::remove_cv_t<T>, bool>)
constexpr unsigned_key_t<T> ordered_key(T value) noexcept {
  using U = unsigned_key_t<T>;
  U bits = static_cast<U>(value);
  if constexpr (std::is_signed_v<T>) bits ^= U{1} << (std::numeric_limits<U>::digits - 1);
  return bits;
}

template <std::random_access_iterator I, class Observer>
requires std::integral<std::iter_value_t<I>> && (!std::same_as<std::iter_value_t<I>, bool>)
void radix_lsd_impl(I first, I last, Observer& observer, unsigned digit_bits) {
  using T = std::iter_value_t<I>; using U = unsigned_key_t<T>;
  const std::size_t n = static_cast<std::size_t>(last - first); if (n < 2) return;
  if (digit_bits == 0 || digit_bits > 12) throw std::invalid_argument("radix digit_bits must be 1..12");
  const std::size_t max_buckets = std::size_t{1} << digit_bits;
  std::vector<std::size_t> count(max_buckets), pos(max_buckets);
  std::vector<T> temp(n);
  constexpr unsigned total_bits = std::numeric_limits<U>::digits;
  for (unsigned shift=0; shift<total_bits; shift+=digit_bits) {
    const unsigned width=std::min(digit_bits,total_bits-shift);
    const std::size_t buckets=std::size_t{1}<<width;
    const U mask=static_cast<U>(buckets-1);
    std::fill(count.begin(),count.end(),0);
    for(I it=first;it<last;++it) ++count[static_cast<std::size_t>((ordered_key(*it)>>shift)&mask)];
    pos[0]=0;for(std::size_t b=1;b<buckets;++b)pos[b]=pos[b-1]+count[b-1];
    for(I it=first;it<last;++it){auto b=static_cast<std::size_t>((ordered_key(*it)>>shift)&mask);temp[pos[b]++]=*it;observer.write();}
    I out=first;for(auto& v:temp){*out++=v;observer.write();}
  }
}

template <std::random_access_iterator I, class Observer>
requires std::integral<std::iter_value_t<I>> && (!std::same_as<std::iter_value_t<I>, bool>)
void american_flag_impl(I first, I last, Observer& observer, int shift, unsigned digit_bits) {
  using T=std::iter_value_t<I>; using U=unsigned_key_t<T>;
  const std::size_t n=static_cast<std::size_t>(last-first); if(n<2||shift<0)return;
  const std::size_t buckets=std::size_t{1}<<digit_bits; const U mask=static_cast<U>(buckets-1);
  std::vector<std::size_t> count(buckets), begin(buckets), next(buckets), end(buckets);
  for(I it=first;it<last;++it)++count[static_cast<std::size_t>((ordered_key(*it)>>shift)&mask)];
  std::size_t sum=0;for(std::size_t b=0;b<buckets;++b){begin[b]=sum;next[b]=sum;sum+=count[b];end[b]=sum;}
  for(std::size_t b=0;b<buckets;++b){
    while(next[b]<end[b]){
      I current=first+static_cast<std::iter_difference_t<I>>(next[b]);
      const auto dest=static_cast<std::size_t>((ordered_key(*current)>>shift)&mask);
      if(dest==b){++next[b];continue;}
      I target=first+static_cast<std::iter_difference_t<I>>(next[dest]++);
      std::ranges::iter_swap(current,target);observer.swap();
    }
  }
  if(shift==0)return;
  const int next_shift=shift-static_cast<int>(digit_bits);
  for(std::size_t b=0;b<buckets;++b) if(count[b]>1)
    american_flag_impl(first+static_cast<std::iter_difference_t<I>>(begin[b]),first+static_cast<std::iter_difference_t<I>>(end[b]),observer,next_shift,digit_bits);
}


template <std::random_access_iterator I, class Observer>
requires std::integral<std::iter_value_t<I>> && (!std::same_as<std::iter_value_t<I>, bool>)
void counting_impl(I first,I last,Observer& observer,std::size_t max_domain){
  using T=std::iter_value_t<I>;using U=unsigned_key_t<T>;
  if(max_domain==0)throw std::invalid_argument("counting_sort max_domain must be positive");
  if(last-first<2)return;
  U min_key=ordered_key(*first),max_key=min_key;
  for(I it=first+1;it<last;++it){U key=ordered_key(*it);min_key=std::min(min_key,key);max_key=std::max(max_key,key);}
  const U span=max_key-min_key;
  if(static_cast<std::uintmax_t>(span)>=static_cast<std::uintmax_t>(max_domain))
    throw std::length_error("counting_sort domain exceeds max_domain");
  const std::size_t domain=static_cast<std::size_t>(span)+1;
  std::vector<std::size_t> count(domain),pos(domain);
  for(I it=first;it<last;++it)++count[static_cast<std::size_t>(ordered_key(*it)-min_key)];
  pos[0]=0;for(std::size_t i=1;i<domain;++i)pos[i]=pos[i-1]+count[i-1];
  std::vector<T> temp(static_cast<std::size_t>(last-first));
  for(I it=first;it<last;++it){auto idx=static_cast<std::size_t>(ordered_key(*it)-min_key);temp[pos[idx]++]=*it;observer.write();}
  I out=first;for(auto& value:temp){*out++=std::move(value);observer.write();}
}

} // namespace sortlab::detail

namespace sortlab {

template <std::random_access_iterator I>
requires std::integral<std::iter_value_t<I>> && (!std::same_as<std::iter_value_t<I>, bool>)
void radix_lsd_sort(I first,I last,unsigned digit_bits=8){null_observer observer;detail::radix_lsd_impl(first,last,observer,digit_bits);}
template <std::ranges::random_access_range R>
requires std::integral<std::ranges::range_value_t<R>> && (!std::same_as<std::ranges::range_value_t<R>, bool>)
void radix_lsd_sort(R&& r,unsigned digit_bits=8){radix_lsd_sort(std::ranges::begin(r),std::ranges::end(r),digit_bits);}

template <std::random_access_iterator I>
requires std::integral<std::iter_value_t<I>> && (!std::same_as<std::iter_value_t<I>, bool>)
void radix_msd_sort(I first,I last,unsigned digit_bits=8){
  using U=detail::unsigned_key_t<std::iter_value_t<I>>;
  if(digit_bits==0||digit_bits>8)throw std::invalid_argument("MSD radix digit_bits must be 1..8");
  constexpr unsigned bits=std::numeric_limits<U>::digits;
  const int shift=static_cast<int>(((bits-1)/digit_bits)*digit_bits);
  null_observer observer;detail::american_flag_impl(first,last,observer,shift,digit_bits);
}
template <std::ranges::random_access_range R>
requires std::integral<std::ranges::range_value_t<R>> && (!std::same_as<std::ranges::range_value_t<R>, bool>)
void radix_msd_sort(R&& r,unsigned digit_bits=8){radix_msd_sort(std::ranges::begin(r),std::ranges::end(r),digit_bits);}

template <std::random_access_iterator I>
requires std::integral<std::iter_value_t<I>> && (!std::same_as<std::iter_value_t<I>, bool>)
void counting_sort(I first,I last,std::size_t max_domain=1u<<20){null_observer observer;detail::counting_impl(first,last,observer,max_domain);}
template <std::ranges::random_access_range R>
requires std::integral<std::ranges::range_value_t<R>> && (!std::same_as<std::ranges::range_value_t<R>, bool>)
void counting_sort(R&& r,std::size_t max_domain=1u<<20){counting_sort(std::ranges::begin(r),std::ranges::end(r),max_domain);}

namespace instrumented {
template <std::random_access_iterator I,class Observer>
requires std::integral<std::iter_value_t<I>> && (!std::same_as<std::iter_value_t<I>, bool>)
void radix_lsd_sort(I first,I last,Observer& observer,unsigned digit_bits=8){detail::radix_lsd_impl(first,last,observer,digit_bits);}
template <std::random_access_iterator I,class Observer>
requires std::integral<std::iter_value_t<I>> && (!std::same_as<std::iter_value_t<I>, bool>)
void radix_msd_sort(I first,I last,Observer& observer,unsigned digit_bits=8){using U=detail::unsigned_key_t<std::iter_value_t<I>>;constexpr unsigned bits=std::numeric_limits<U>::digits;if(digit_bits==0||digit_bits>8)throw std::invalid_argument("MSD radix digit_bits must be 1..8");int shift=static_cast<int>(((bits-1)/digit_bits)*digit_bits);detail::american_flag_impl(first,last,observer,shift,digit_bits);}
template <std::random_access_iterator I,class Observer>
requires std::integral<std::iter_value_t<I>> && (!std::same_as<std::iter_value_t<I>, bool>)
void counting_sort(I first,I last,Observer& observer,std::size_t max_domain=1u<<20){detail::counting_impl(first,last,observer,max_domain);}
} // namespace instrumented
} // namespace sortlab
