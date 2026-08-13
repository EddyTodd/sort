#pragma once

#include "sortlab/detail/adaptive_common.hpp"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <limits>
#include <ranges>
#include <stdexcept>
#include <utility>
#include <vector>

namespace sortlab::detail {
template <std::random_access_iterator I, class Ops>
class adaptive_merger {
 public:
  using T = std::iter_value_t<I>;
  adaptive_merger(I first, Ops& ops, std::size_t min_gallop)
      : first_(first), ops_(ops), min_gallop_(std::max<std::size_t>(1, min_gallop)) {}

  std::size_t min_gallop() const noexcept { return min_gallop_; }

  void merge(adaptive_run left, adaptive_run right) {
    if (left.base + left.len != right.base) throw std::logic_error("non-adjacent adaptive runs");
    if (left.len <= right.len) merge_lo(left, right); else merge_hi(left, right);
  }

 private:
  void update_min_gallop(bool productive) {
    const std::size_t before = min_gallop_;
    if (productive) {
      if (min_gallop_ > 1) --min_gallop_;
    } else if (min_gallop_ <= std::numeric_limits<std::size_t>::max() - 2) {
      min_gallop_ += 2;
    }
    if (before != min_gallop_) ops_.min_gallop_change(before, min_gallop_);
  }

  void merge_lo(adaptive_run left, adaptive_run right) {
    buffer_.clear();
    buffer_.reserve(left.len);
    I left_first = first_ + static_cast<std::iter_difference_t<I>>(left.base);
    for (std::size_t k = 0; k < left.len; ++k) {
      buffer_.emplace_back(std::ranges::iter_move(left_first + static_cast<std::iter_difference_t<I>>(k)));
      ops_.external_write();
    }

    auto bi = buffer_.begin();
    const auto bend = buffer_.end();
    I ri = first_ + static_cast<std::iter_difference_t<I>>(right.base);
    const I rend = ri + static_cast<std::iter_difference_t<I>>(right.len);
    I out = left_first;
    std::size_t left_wins = 0;
    std::size_t right_wins = 0;

    while (bi < bend && ri < rend) {
      if (ops_.less(*ri, *bi)) {
        ops_.assign(out++, std::ranges::iter_move(ri++));
        ++right_wins;
        left_wins = 0;
      } else {
        ops_.assign(out++, std::move(*bi++));
        ++left_wins;
        right_wins = 0;
      }
      if (bi == bend || ri == rend) break;
      if (left_wins < min_gallop_ && right_wins < min_gallop_) continue;

      // TimSort-style gallop mode: alternate both sides until neither block is
      // productive at the current threshold, adapting min_gallop across modes.
      for (;;) {
        const std::size_t threshold = min_gallop_;
        auto left_end = gallop_upper(bi, bend, *ri, ops_);
        const std::size_t left_moved = static_cast<std::size_t>(left_end - bi);
        ops_.gallop(left_moved);
        while (bi < left_end) ops_.assign(out++, std::move(*bi++));
        if (bi == bend) break;

        ops_.assign(out++, std::ranges::iter_move(ri++));
        if (ri == rend) break;

        auto right_end = gallop_lower(ri, rend, *bi, ops_);
        const std::size_t right_moved = static_cast<std::size_t>(right_end - ri);
        ops_.gallop(right_moved);
        while (ri < right_end) ops_.assign(out++, std::ranges::iter_move(ri++));
        if (ri == rend) break;

        ops_.assign(out++, std::move(*bi++));
        if (bi == bend) break;

        const bool productive = left_moved >= threshold || right_moved >= threshold;
        update_min_gallop(productive);
        if (!productive) break;
      }
      left_wins = right_wins = 0;
    }

    while (bi < bend) ops_.assign(out++, std::move(*bi++));
    // If the right side has not moved relative to out, it is already in place.
    while (ri < rend && out < ri) ops_.assign(out++, std::ranges::iter_move(ri++));
  }

  void merge_hi(adaptive_run left, adaptive_run right) {
    buffer_.clear();
    buffer_.reserve(right.len);
    I right_first = first_ + static_cast<std::iter_difference_t<I>>(right.base);
    for (std::size_t k = 0; k < right.len; ++k) {
      buffer_.emplace_back(std::ranges::iter_move(right_first + static_cast<std::iter_difference_t<I>>(k)));
      ops_.external_write();
    }

    I li = right_first;
    auto bj = buffer_.end();
    I out = right_first + static_cast<std::iter_difference_t<I>>(right.len);
    const I lbegin = first_ + static_cast<std::iter_difference_t<I>>(left.base);
    std::size_t left_wins = 0;
    std::size_t right_wins = 0;

    while (li > lbegin && bj > buffer_.begin()) {
      // Backward stable merge: right run wins ties so equal right elements occupy later positions.
      if (ops_.less(*(bj - 1), *(li - 1))) {
        ops_.assign(--out, std::ranges::iter_move(--li));
        ++left_wins;
        right_wins = 0;
      } else {
        ops_.assign(--out, std::move(*--bj));
        ++right_wins;
        left_wins = 0;
      }
      if (li == lbegin || bj == buffer_.begin()) break;
      if (left_wins < min_gallop_ && right_wins < min_gallop_) continue;

      for (;;) {
        const std::size_t threshold = min_gallop_;
        auto left_cut = gallop_upper_reverse(lbegin, li, *(bj - 1), ops_);
        const std::size_t left_moved = static_cast<std::size_t>(li - left_cut);
        ops_.gallop(left_moved);
        while (li > left_cut) ops_.assign(--out, std::ranges::iter_move(--li));
        if (li == lbegin) break;

        ops_.assign(--out, std::move(*--bj));
        if (bj == buffer_.begin()) break;

        auto right_cut = gallop_lower_reverse(buffer_.begin(), bj, *(li - 1), ops_);
        const std::size_t right_moved = static_cast<std::size_t>(bj - right_cut);
        ops_.gallop(right_moved);
        while (bj > right_cut) ops_.assign(--out, std::move(*--bj));
        if (bj == buffer_.begin()) break;

        ops_.assign(--out, std::ranges::iter_move(--li));
        if (li == lbegin) break;

        const bool productive = left_moved >= threshold || right_moved >= threshold;
        update_min_gallop(productive);
        if (!productive) break;
      }
      left_wins = right_wins = 0;
    }

    while (bj > buffer_.begin()) ops_.assign(--out, std::move(*--bj));
  }

  I first_;
  Ops& ops_;
  std::size_t min_gallop_;
  std::vector<T> buffer_;
};

}  // namespace sortlab::detail
