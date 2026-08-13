#pragma once

#include <cstdint>

namespace sortlab {

using Value = std::int64_t;

struct Stats {
  std::uint64_t comparisons = 0;
  std::uint64_t swaps = 0;
  std::uint64_t writes = 0;
};

template <bool Count>
inline bool lessv(Value a, Value b, Stats& stats) {
  if constexpr (Count) ++stats.comparisons;
  return a < b;
}

template <bool Count>
inline void swapv(Value& a, Value& b, Stats& stats) {
  if (&a == &b) return;
  const Value tmp = a;
  a = b;
  b = tmp;
  if constexpr (Count) {
    ++stats.swaps;
    stats.writes += 2;
  }
}

template <bool Count>
inline void writev(Value& dst, Value src, Stats& stats) {
  dst = src;
  if constexpr (Count) ++stats.writes;
}

}  // namespace sortlab
