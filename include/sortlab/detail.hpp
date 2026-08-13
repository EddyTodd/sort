#pragma once

#include "sortlab/instrumentation.hpp"

#include <algorithm>
#include <concepts>
#include <functional>
#include <iterator>
#include <ranges>
#include <type_traits>
#include <utility>

namespace sortlab::detail {

template <class Comp, class Proj, class Observer>
class operations {
 public:
  constexpr operations(Comp comp, Proj proj, Observer& observer)
      : comp_(std::move(comp)), proj_(std::move(proj)), observer_(&observer) {}

  template <class A, class B>
  constexpr bool less(A&& a, B&& b) {
    observer_->comparison();
    return std::invoke(comp_, std::invoke(proj_, std::forward<A>(a)),
                       std::invoke(proj_, std::forward<B>(b)));
  }

  template <std::random_access_iterator I>
  requires std::indirectly_swappable<I, I>
  constexpr void swap(I a, I b) {
    if (a == b) return;
    std::ranges::iter_swap(a, b);
    observer_->swap();
  }

  template <class I, class T>
  constexpr void assign(I dst, T&& value) {
    *dst = std::forward<T>(value);
    observer_->write();
  }

  constexpr void external_write(std::size_t n = 1) { observer_->write(n); }
  constexpr void rotation(std::size_t n) { observer_->rotation(n); }
  constexpr void gallop(std::size_t n) { observer_->gallop(n); }
  constexpr void min_gallop_change(std::size_t before, std::size_t after) { observer_->min_gallop_change(before, after); }

 private:
  Comp comp_;
  Proj proj_;
  Observer* observer_;
};

template <std::random_access_iterator I, class Ops>
constexpr void reverse_range(I first, I last, Ops& ops) {
  while (first != last) {
    --last;
    if (first == last) break;
    ops.swap(first, last);
    ++first;
  }
}

template <std::random_access_iterator I, class Ops>
constexpr I rotate_range(I first, I middle, I last, Ops& ops) {
  if (first == middle) return last;
  if (middle == last) return first;
  const auto right = last - middle;
  reverse_range(first, middle, ops);
  reverse_range(middle, last, ops);
  reverse_range(first, last, ops);
  ops.rotation(static_cast<std::size_t>(last - first));
  return first + right;
}

template <std::random_access_iterator I, class T, class Ops>
constexpr I lower_bound(I first, I last, const T& value, Ops& ops) {
  while (first < last) {
    const auto half = (last - first) / 2;
    I mid = first + half;
    if (ops.less(*mid, value)) first = mid + 1;
    else last = mid;
  }
  return first;
}

template <std::random_access_iterator I, class T, class Ops>
constexpr I upper_bound(I first, I last, const T& value, Ops& ops) {
  while (first < last) {
    const auto half = (last - first) / 2;
    I mid = first + half;
    if (!ops.less(value, *mid)) first = mid + 1;
    else last = mid;
  }
  return first;
}

}  // namespace sortlab::detail
