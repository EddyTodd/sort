#pragma once

#include "sortlab/extended_algorithms.hpp"
#include "sortlab/tiny_kernels.hpp"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <stdexcept>
#include <vector>

namespace sortlab {

inline void validate_leaf_configuration(TinyKernel kernel, std::size_t cutoff) {
  if (cutoff < 1) throw std::runtime_error("leaf cutoff must be positive");
  if (kernel == TinyKernel::bitonic_network && cutoff > 32) {
    throw std::runtime_error("bitonic leaf cutoff must not exceed 32");
  }
}

template <bool Count>
inline void merge_leaf_rec(std::vector<Value>& values, std::vector<Value>& temp,
                           std::size_t lo, std::size_t hi, std::size_t cutoff,
                           TinyKernel kernel, Stats& stats) {
  if (hi - lo <= cutoff) {
    apply_tiny_kernel<Count>(values, lo, hi, kernel, stats);
    return;
  }
  const std::size_t mid = lo + (hi - lo) / 2;
  merge_leaf_rec<Count>(values, temp, lo, mid, cutoff, kernel, stats);
  merge_leaf_rec<Count>(values, temp, mid, hi, cutoff, kernel, stats);
  if (!lessv<Count>(values[mid], values[mid - 1], stats)) return;
  merge_ranges<Count>(values, temp, lo, mid, hi, stats);
}

template <bool Count>
inline void merge_leaf_sort(std::vector<Value>& values, std::size_t cutoff,
                            TinyKernel kernel, Stats& stats) {
  validate_leaf_configuration(kernel, cutoff);
  if (values.size() < 2) return;
  std::vector<Value> temp(values.size());
  merge_leaf_rec<Count>(values, temp, 0, values.size(), cutoff, kernel, stats);
}

template <bool Count>
inline void quick_leaf_rec(std::vector<Value>& values, std::size_t lo,
                           std::size_t hi, std::size_t cutoff,
                           TinyKernel kernel, Stats& stats) {
  while (hi - lo > cutoff) {
    std::size_t cut = median3_partition<Count>(values, lo, hi, stats);
    if (cut <= lo) cut = lo + 1;
    if (cut >= hi) cut = hi - 1;
    if (cut - lo < hi - cut) {
      quick_leaf_rec<Count>(values, lo, cut, cutoff, kernel, stats);
      lo = cut;
    } else {
      quick_leaf_rec<Count>(values, cut, hi, cutoff, kernel, stats);
      hi = cut;
    }
  }
  apply_tiny_kernel<Count>(values, lo, hi, kernel, stats);
}

template <bool Count>
inline void quick_leaf_sort(std::vector<Value>& values, std::size_t cutoff,
                            TinyKernel kernel, Stats& stats) {
  validate_leaf_configuration(kernel, cutoff);
  if (values.size() < 2) return;
  quick_leaf_rec<Count>(values, 0, values.size(), cutoff, kernel, stats);
}

template <bool Count>
inline void intro_leaf_rec(std::vector<Value>& values, std::size_t lo,
                           std::size_t hi, unsigned depth,
                           std::size_t cutoff, TinyKernel kernel, Stats& stats) {
  while (hi - lo > cutoff) {
    if (depth == 0) {
      heap_sort_range<Count>(values, lo, hi, stats);
      return;
    }
    --depth;
    std::size_t cut = hoare_partition<Count>(values, lo, hi, stats);
    if (cut <= lo) cut = lo + 1;
    if (cut >= hi) cut = hi - 1;
    if (cut - lo < hi - cut) {
      intro_leaf_rec<Count>(values, lo, cut, depth, cutoff, kernel, stats);
      lo = cut;
    } else {
      intro_leaf_rec<Count>(values, cut, hi, depth, cutoff, kernel, stats);
      hi = cut;
    }
  }
  apply_tiny_kernel<Count>(values, lo, hi, kernel, stats);
}

template <bool Count>
inline void intro_leaf_sort(std::vector<Value>& values, std::size_t cutoff,
                            TinyKernel kernel, Stats& stats) {
  validate_leaf_configuration(kernel, cutoff);
  if (values.size() < 2) return;
  const unsigned depth =
      static_cast<unsigned>(2 * (std::bit_width(values.size()) - 1));
  intro_leaf_rec<Count>(values, 0, values.size(), depth, cutoff, kernel, stats);
}

}  // namespace sortlab
