#pragma once

#include "sortlab/common.hpp"

#include <array>
#include <bit>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <vector>

namespace sortlab {

enum class TinyKernel { insertion, binary_insertion, bitonic_network };

inline const char* tiny_kernel_name(TinyKernel kernel) {
  switch (kernel) {
    case TinyKernel::insertion:
      return "insertion";
    case TinyKernel::binary_insertion:
      return "binary_insertion";
    case TinyKernel::bitonic_network:
      return "bitonic_network";
  }
  return "unknown";
}

template <bool Count>
inline void tiny_insertion_range(std::vector<Value>& values, std::size_t lo,
                                 std::size_t hi, Stats& stats) {
  for (std::size_t i = lo + 1; i < hi; ++i) {
    const Value key = values[i];
    std::size_t j = i;
    while (j > lo && lessv<Count>(key, values[j - 1], stats)) {
      writev<Count>(values[j], values[j - 1], stats);
      --j;
    }
    writev<Count>(values[j], key, stats);
  }
}

template <bool Count>
inline void tiny_binary_insertion_range(std::vector<Value>& values, std::size_t lo,
                                        std::size_t hi, Stats& stats) {
  for (std::size_t i = lo + 1; i < hi; ++i) {
    const Value value = values[i];
    std::size_t left = lo;
    std::size_t right = i;
    while (left < right) {
      const std::size_t mid = left + (right - left) / 2;
      if (lessv<Count>(value, values[mid], stats)) {
        right = mid;
      } else {
        left = mid + 1;
      }
    }
    for (std::size_t j = i; j > left; --j) {
      writev<Count>(values[j], values[j - 1], stats);
    }
    writev<Count>(values[left], value, stats);
  }
}

template <bool Count>
inline void network_compare_exchange(Value& left, Value& right, bool ascending,
                                     Stats& stats) {
  if constexpr (Count) ++stats.comparisons;
  const bool exchange = ascending ? (right < left) : (left < right);
  const Value low = exchange ? right : left;
  const Value high = exchange ? left : right;
  left = low;
  right = high;
  if constexpr (Count) stats.writes += 2;
}

template <std::size_t N, bool Count>
inline void bitonic_network(std::array<Value, N>& values, Stats& stats) {
  static_assert(N >= 2 && std::has_single_bit(N));
  for (std::size_t width = 2; width <= N; width <<= 1U) {
    for (std::size_t stride = width >> 1U; stride > 0; stride >>= 1U) {
      for (std::size_t i = 0; i < N; ++i) {
        const std::size_t partner = i ^ stride;
        if (partner <= i) continue;
        const bool ascending = (i & width) == 0;
        network_compare_exchange<Count>(values[i], values[partner], ascending, stats);
      }
    }
  }
}

template <std::size_t N, bool Count>
inline void bitonic_padded_range(std::vector<Value>& values, std::size_t lo,
                                 std::size_t hi, Stats& stats) {
  std::array<Value, N> buffer{};
  const std::size_t length = hi - lo;
  for (std::size_t i = 0; i < length; ++i) buffer[i] = values[lo + i];
  for (std::size_t i = length; i < N; ++i) {
    buffer[i] = std::numeric_limits<Value>::max();
  }
  bitonic_network<N, Count>(buffer, stats);
  for (std::size_t i = 0; i < length; ++i) {
    writev<Count>(values[lo + i], buffer[i], stats);
  }
}

template <bool Count>
inline void tiny_bitonic_range(std::vector<Value>& values, std::size_t lo,
                               std::size_t hi, Stats& stats) {
  const std::size_t length = hi - lo;
  if (length < 2) return;
  if (length <= 2) return bitonic_padded_range<2, Count>(values, lo, hi, stats);
  if (length <= 4) return bitonic_padded_range<4, Count>(values, lo, hi, stats);
  if (length <= 8) return bitonic_padded_range<8, Count>(values, lo, hi, stats);
  if (length <= 16) return bitonic_padded_range<16, Count>(values, lo, hi, stats);
  if (length <= 32) return bitonic_padded_range<32, Count>(values, lo, hi, stats);
  throw std::runtime_error("bitonic tiny kernel supports at most 32 elements");
}

template <bool Count>
inline void apply_tiny_kernel(std::vector<Value>& values, std::size_t lo,
                              std::size_t hi, TinyKernel kernel, Stats& stats) {
  switch (kernel) {
    case TinyKernel::insertion:
      tiny_insertion_range<Count>(values, lo, hi, stats);
      return;
    case TinyKernel::binary_insertion:
      tiny_binary_insertion_range<Count>(values, lo, hi, stats);
      return;
    case TinyKernel::bitonic_network:
      tiny_bitonic_range<Count>(values, lo, hi, stats);
      return;
  }
}

inline std::uint64_t bitonic_gate_count(std::size_t padded_size) {
  if (padded_size < 2 || !std::has_single_bit(padded_size) || padded_size > 32) {
    return 0;
  }
  const auto log = static_cast<std::uint64_t>(std::countr_zero(padded_size));
  return static_cast<std::uint64_t>(padded_size) * log * (log + 1) / 4;
}

inline std::size_t bitonic_padded_size(std::size_t n) {
  if (n <= 1) return n;
  const auto padded = std::bit_ceil(n);
  return padded <= 32 ? padded : 0;
}

}  // namespace sortlab
