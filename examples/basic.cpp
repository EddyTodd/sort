#include <sortlab/sortlab.hpp>

#include <array>
#include <ranges>

namespace {

struct Record {
    int key{};
    int payload{};
};

}  // namespace

int main() {
    std::array values{5, 1, 4, 1, 3};
    sortlab::intro_sort(values);
    if (!std::ranges::is_sorted(values)) {
        return 1;
    }

    std::array records{
        Record{3, 30},
        Record{1, 10},
        Record{2, 20},
    };
    sortlab::heap_sort(records, std::ranges::less{}, &Record::key);
    return std::ranges::is_sorted(records, std::ranges::less{}, &Record::key) ? 0 : 2;
}
