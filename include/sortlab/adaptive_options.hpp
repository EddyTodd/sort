#pragma once

#include <cstddef>

namespace sortlab {
struct adaptive_options {
  std::size_t min_gallop = 7;
  std::size_t min_merge = 32;
};
}  // namespace sortlab
