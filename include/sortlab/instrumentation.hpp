#pragma once

#include <cstddef>
#include <cstdint>

namespace sortlab {

struct operation_counts {
  std::uint64_t comparisons = 0;
  std::uint64_t swaps = 0;
  std::uint64_t writes = 0;
  std::uint64_t rotations = 0;
  std::uint64_t gallop_entries = 0;
  std::uint64_t gallop_items = 0;
  std::uint64_t gallop_threshold_updates = 0;
  std::size_t last_min_gallop = 0;
};

struct null_observer {
  constexpr void comparison() noexcept {}
  constexpr void swap() noexcept {}
  constexpr void write(std::size_t = 1) noexcept {}
  constexpr void rotation(std::size_t = 1) noexcept {}
  constexpr void gallop(std::size_t = 0) noexcept {}
  constexpr void min_gallop_change(std::size_t, std::size_t) noexcept {}
};

class counting_observer {
 public:
  explicit constexpr counting_observer(operation_counts& counts) noexcept : counts_(&counts) {}

  constexpr void comparison() noexcept { ++counts_->comparisons; }
  constexpr void swap() noexcept { ++counts_->swaps; }
  constexpr void write(std::size_t n = 1) noexcept { counts_->writes += static_cast<std::uint64_t>(n); }
  constexpr void rotation(std::size_t n = 1) noexcept { counts_->rotations += static_cast<std::uint64_t>(n); }
  constexpr void gallop(std::size_t n = 0) noexcept {
    ++counts_->gallop_entries;
    counts_->gallop_items += static_cast<std::uint64_t>(n);
  }
  constexpr void min_gallop_change(std::size_t, std::size_t now) noexcept {
    ++counts_->gallop_threshold_updates;
    counts_->last_min_gallop = now;
  }

 private:
  operation_counts* counts_;
};

}  // namespace sortlab
