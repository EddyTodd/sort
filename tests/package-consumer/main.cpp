#include <sortlab/sortlab.hpp>

#include <array>

int main() {
    std::array<int, 5> values{5, 1, 4, 1, 3};
    sortlab::intro_sort(values);
    const std::array<int, 5> expected{1, 1, 3, 4, 5};
    return values == expected ? 0 : 1;
}
