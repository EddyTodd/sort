#pragma once

#include "sortlab/adaptive_options.hpp"
#include "sortlab/detail/adaptive_merger.hpp"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <vector>

namespace sortlab::detail {

template <std::random_access_iterator I, class Ops>
std::vector<adaptive_run> discover_runs(I first, I last, std::size_t minrun, Ops& ops) {
    std::vector<adaptive_run> runs;
    const std::size_t n = static_cast<std::size_t>(last - first);
    std::size_t base = 0;

    while (base < n) {
        const std::size_t natural = count_run(first, last, base, ops);
        const std::size_t target = std::min(n - base, std::max(natural, minrun));
        if (target > natural) {
            binary_extend(first, base, base + natural, base + target, ops);
        }
        runs.push_back({base, target, 0});
        base += target;
    }

    return runs;
}

template <std::random_access_iterator I, class Ops>
void merge_at(std::vector<adaptive_run>& stack, std::size_t index,
              adaptive_merger<I, Ops>& merger) {
    const auto left = stack[index];
    const auto right = stack[index + 1];
    merger.merge(left, right);
    stack[index] = {left.base, left.len + right.len, left.power};
    stack.erase(stack.begin() + static_cast<std::ptrdiff_t>(index + 1));
}

template <std::random_access_iterator I, class Ops>
void timsort_impl(I first, I last, Ops& ops, AdaptiveSortOptions options) {
    const std::size_t n = static_cast<std::size_t>(last - first);
    if (n < 2) {
        return;
    }

    const std::size_t minrun =
        timsort_minrun(n, std::max<std::size_t>(16, options.min_merge));
    const auto runs = discover_runs(first, last, minrun, ops);
    adaptive_merger<I, Ops> merger(first, ops, options.min_gallop);
    std::vector<adaptive_run> stack;

    for (const auto run : runs) {
        stack.push_back(run);
        while (stack.size() > 1) {
            std::size_t k = stack.size() - 2;
            const bool a =
                k > 0 && stack[k - 1].len <= stack[k].len + stack[k + 1].len;
            const bool b =
                k > 1 && stack[k - 2].len <= stack[k - 1].len + stack[k].len;

            if (a || b) {
                if (stack[k - 1].len < stack[k + 1].len) {
                    --k;
                }
            } else if (stack[k].len > stack[k + 1].len) {
                break;
            }

            merge_at(stack, k, merger);
        }
    }

    while (stack.size() > 1) {
        std::size_t k = stack.size() - 2;
        if (k > 0 && stack[k - 1].len < stack[k + 1].len) {
            --k;
        }
        merge_at(stack, k, merger);
    }
}

template <std::random_access_iterator I, class Ops>
void powersort_impl(I first, I last, Ops& ops, AdaptiveSortOptions options) {
    const std::size_t n = static_cast<std::size_t>(last - first);
    if (n < 2) {
        return;
    }

    const std::size_t minrun =
        timsort_minrun(n, std::max<std::size_t>(16, options.min_merge));
    const auto runs = discover_runs(first, last, minrun, ops);
    adaptive_merger<I, Ops> merger(first, ops, options.min_gallop);
    std::vector<adaptive_run> stack;

    for (auto incoming : runs) {
        if (!stack.empty()) {
            const auto top = stack.back();
            const int power = powerloop(top.base, top.len, incoming.len, n);
            while (stack.size() > 1 && stack[stack.size() - 2].power > power) {
                merge_at(stack, stack.size() - 2, merger);
            }
            stack.back().power = power;
        }
        stack.push_back(incoming);
    }

    while (stack.size() > 1) {
        merge_at(stack, stack.size() - 2, merger);
    }
}

}  // namespace sortlab::detail
