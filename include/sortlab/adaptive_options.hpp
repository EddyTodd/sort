#pragma once

#include <cstddef>

namespace sortlab {

struct AdaptiveSortOptions {
    std::size_t min_gallop = 7;
    std::size_t min_merge = 32;
};

using adaptive_options = AdaptiveSortOptions;

}  // namespace sortlab
