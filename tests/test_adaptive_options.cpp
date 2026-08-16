#include <sortlab/sortlab.hpp>

#include <algorithm>
#include <cstddef>
#include <functional>
#include <iostream>
#include <limits>
#include <ranges>
#include <stdexcept>
#include <vector>

int main() {
    std::vector<int> values = {5, 4, 3, 2, 1, 0};
    sortlab::AdaptiveSortOptions options;
    options.min_merge = std::numeric_limits<std::size_t>::max();
    options.min_gallop = 0;

    sortlab::timsort(values, std::ranges::less{}, std::identity{}, options);
    if (!std::ranges::is_sorted(values)) {
        std::cerr << "FAIL: extreme adaptive options did not sort\n";
        return 1;
    }

    bool rejected = false;
    try {
        (void)sortlab::detail::timsort_minrun(64, 0);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    if (!rejected) {
        std::cerr << "FAIL: zero min_merge was not rejected\n";
        return 1;
    }

    std::cout << "PASS: adaptive option overflow/validation contract\n";
    return 0;
}
