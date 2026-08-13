#pragma once

#include "sortlab/common.hpp"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace sortlab {

struct InputFeatures {
  std::size_t samples = 0;
  double inversion_rate = 0.0;
  double duplicate_fraction = 0.0;
  unsigned range_bits = 0;
};

inline InputFeatures sample_features(const std::vector<Value>& values) {
  constexpr std::size_t max_samples = 32;
  InputFeatures out;
  if (values.empty()) return out;

  out.samples = std::min(max_samples, values.size());
  std::vector<Value> sample;
  sample.reserve(out.samples);
  if (out.samples == 1) {
    sample.push_back(values.front());
  } else {
    for (std::size_t i = 0; i < out.samples; ++i) {
      const std::size_t index = i * (values.size() - 1) / (out.samples - 1);
      sample.push_back(values[index]);
    }
  }

  std::size_t inversions = 0;
  for (std::size_t i = 1; i < sample.size(); ++i) {
    if (sample[i] < sample[i - 1]) ++inversions;
  }
  if (sample.size() > 1) {
    out.inversion_rate = static_cast<double>(inversions) /
                         static_cast<double>(sample.size() - 1);
  }

  std::size_t unique = 0;
  for (std::size_t i = 0; i < sample.size(); ++i) {
    bool seen = false;
    for (std::size_t j = 0; j < i; ++j) {
      if (sample[i] == sample[j]) {
        seen = true;
        break;
      }
    }
    if (!seen) ++unique;
  }
  out.duplicate_fraction = 1.0 - static_cast<double>(unique) /
                                     static_cast<double>(sample.size());

  constexpr std::uint64_t sign = std::uint64_t{1} << 63;
  std::uint64_t minimum = static_cast<std::uint64_t>(sample.front()) ^ sign;
  std::uint64_t maximum = minimum;
  for (const Value value : sample) {
    const std::uint64_t ordered = static_cast<std::uint64_t>(value) ^ sign;
    minimum = std::min(minimum, ordered);
    maximum = std::max(maximum, ordered);
  }
  out.range_bits = std::bit_width(maximum - minimum);
  return out;
}

}  // namespace sortlab
