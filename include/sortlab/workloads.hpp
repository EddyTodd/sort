#pragma once

#include "sortlab/common.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <numeric>
#include <random>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace sortlab {
static std::uint64_t fnv1a(std::string_view text) {
  std::uint64_t hash = 14695981039346656037ULL;
  for (const unsigned char c : text) {
    hash ^= c;
    hash *= 1099511628211ULL;
  }
  return hash;
}

static std::uint64_t splitmix64(std::uint64_t x) {
  x += 0x9E3779B97F4A7C15ULL;
  x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ULL;
  x = (x ^ (x >> 27)) * 0x94D049BB133111EBULL;
  return x ^ (x >> 31);
}

static std::uint64_t trial_seed(std::uint64_t seed, std::string_view pattern,
                                std::size_t n, std::uint64_t trial) {
  std::uint64_t x = seed ^ fnv1a(pattern);
  x = splitmix64(x ^ static_cast<std::uint64_t>(n));
  return splitmix64(x ^ trial);
}

static std::uint64_t input_hash(const std::vector<Value>& values) {
  std::uint64_t hash = 14695981039346656037ULL;
  for (const auto value : values) {
    std::uint64_t bits = static_cast<std::uint64_t>(value);
    for (int byte = 0; byte < 8; ++byte) {
      hash ^= bits & 0xffU;
      hash *= 1099511628211ULL;
      bits >>= 8;
    }
  }
  return hash;
}

static std::vector<Value> base_random(std::size_t n, std::uint64_t seed) {
  std::mt19937_64 rng(seed);
  std::vector<Value> out(n);
  for (auto& value : out) {
    value = static_cast<Value>(rng() % 2000000001ULL) - 1000000000LL;
  }
  return out;
}

static std::vector<Value> make_data(std::string_view pattern, std::size_t n, std::uint64_t seed) {
  std::mt19937_64 rng(seed);
  auto a = base_random(n, seed);
  if (pattern == "random") {
    return a;
  }
  if (pattern == "sorted") {
    std::sort(a.begin(), a.end());
    return a;
  }
  if (pattern == "reversed") {
    std::sort(a.begin(), a.end(), std::greater<>());
    return a;
  }
  if (pattern == "few_unique") {
    for (auto& value : a) value = static_cast<Value>(rng() % 8ULL);
    return a;
  }
  if (pattern == "all_equal") {
    std::fill(a.begin(), a.end(), 7);
    return a;
  }
  if (pattern == "nearly_sorted") {
    std::sort(a.begin(), a.end());
    if (n > 1) {
      const std::size_t swaps = std::max<std::size_t>(1, n / 100);
      for (std::size_t k = 0; k < swaps; ++k) {
        const auto i = static_cast<std::size_t>(rng() % n);
        const auto j = static_cast<std::size_t>(rng() % n);
        std::swap(a[i], a[j]);
      }
    }
    return a;
  }
  if (pattern == "organ_pipe") {
    for (std::size_t i = 0; i < n; ++i) {
      const std::size_t mirror = std::min(i, n == 0 ? 0 : n - 1 - i);
      a[i] = static_cast<Value>(mirror);
    }
    return a;
  }
  if (pattern == "sawtooth") {
    for (std::size_t i = 0; i < n; ++i) a[i] = static_cast<Value>(i % 32);
    return a;
  }
  if (pattern == "runs") {
    constexpr std::size_t run = 32;
    for (std::size_t base = 0; base < n; base += run) {
      const auto end = std::min(n, base + run);
      std::sort(a.begin() + static_cast<std::ptrdiff_t>(base),
                a.begin() + static_cast<std::ptrdiff_t>(end));
    }
    return a;
  }
  throw std::runtime_error("unknown pattern: " + std::string(pattern));
}

static const std::vector<std::string>& all_patterns() {
  static const std::vector<std::string> patterns = {
      "random", "sorted", "reversed", "few_unique", "all_equal",
      "nearly_sorted", "organ_pipe", "sawtooth", "runs"};
  return patterns;
}

static bool verify(const std::vector<Value>& input, const std::vector<Value>& output) {
  auto expected = input;
  std::sort(expected.begin(), expected.end());
  return output == expected;
}

static std::vector<std::string> split_csv(std::string_view text) {
  std::vector<std::string> out;
  std::size_t pos = 0;
  while (pos <= text.size()) {
    const auto end = text.find(',', pos);
    const auto token = text.substr(pos, end == std::string_view::npos ? text.size() - pos : end - pos);
    if (token.empty()) throw std::runtime_error("empty list item");
    out.emplace_back(token);
    if (end == std::string_view::npos) break;
    pos = end + 1;
  }
  return out;
}

static std::vector<std::size_t> parse_sizes(std::string_view text) {
  std::vector<std::size_t> out;
  for (const auto& token : split_csv(text)) out.push_back(static_cast<std::size_t>(std::stoull(token)));
  return out;
}

static std::vector<std::string> selected_patterns(const std::vector<std::string>& names) {
  if (names.empty()) return all_patterns();
  for (const auto& name : names) {
    if (std::find(all_patterns().begin(), all_patterns().end(), name) == all_patterns().end()) {
      throw std::runtime_error("unknown pattern: " + name);
    }
  }
  return names;
}

static void deterministic_shuffle(std::vector<std::size_t>& values, std::uint64_t seed) {
  std::mt19937_64 rng(seed);
  for (std::size_t i = values.size(); i > 1; --i) {
    const auto j = static_cast<std::size_t>(rng() % i);
    std::swap(values[i - 1], values[j]);
  }
}

}  // namespace sortlab
