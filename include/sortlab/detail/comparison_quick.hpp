#pragma once

#include "sortlab/detail/comparison_merge.hpp"

#include <concepts>
#include <iterator>

namespace sortlab::detail {

template <std::random_access_iterator I, class Ops>
requires std::copy_constructible<std::iter_value_t<I>>
I hoare_partition(I first, I last, Ops& ops) {
    using T = std::iter_value_t<I>;
    T pivot = *(first + (last - first) / 2);
    I i = first;
    I j = last - 1;
    for (;;) {
        while (i < last && ops.less(*i, pivot)) {
            ++i;
        }
        while (j > first && ops.less(pivot, *j)) {
            --j;
        }
        if (i >= j) {
            return i;
        }
        ops.swap(i, j);
        ++i;
        if (j > first) {
            --j;
        }
    }
}

template <std::random_access_iterator I, class Ops>
requires std::copy_constructible<std::iter_value_t<I>>
void quick_hoare_impl(I first, I last, Ops& ops) {
    while (last - first > 1) {
        I cut = hoare_partition(first, last, ops);
        if (cut <= first) {
            cut = first + 1;
        }
        if (cut >= last) {
            cut = last - 1;
        }
        if (cut - first < last - cut) {
            quick_hoare_impl(first, cut, ops);
            first = cut;
        } else {
            quick_hoare_impl(cut, last, ops);
            last = cut;
        }
    }
}

template <std::random_access_iterator I, class Ops>
requires std::copy_constructible<std::iter_value_t<I>>
void quick3_impl(I first, I last, Ops& ops) {
    while (last - first > 1) {
        using T = std::iter_value_t<I>;
        T pivot = *(first + (last - first) / 2);
        I lt = first;
        I it = first;
        I gt = last;
        while (it < gt) {
            if (ops.less(*it, pivot)) {
                ops.swap(lt, it);
                ++lt;
                ++it;
            } else if (ops.less(pivot, *it)) {
                --gt;
                ops.swap(it, gt);
            } else {
                ++it;
            }
        }
        if (lt - first < last - gt) {
            quick3_impl(first, lt, ops);
            first = gt;
        } else {
            quick3_impl(gt, last, ops);
            last = lt;
        }
    }
}

template <class T, class Ops>
requires std::copy_constructible<T>
const T& median3(const T& a, const T& b, const T& c, Ops& ops) {
    if (ops.less(a, b)) {
        if (ops.less(b, c)) {
            return b;
        }
        return ops.less(a, c) ? c : a;
    }
    if (ops.less(a, c)) {
        return a;
    }
    return ops.less(b, c) ? c : b;
}

template <std::random_access_iterator I, class Ops>
requires std::copy_constructible<std::iter_value_t<I>>
I median3_partition(I first, I last, Ops& ops) {
    using T = std::iter_value_t<I>;
    T pivot = median3(*first, *(first + (last - first) / 2), *(last - 1), ops);
    I i = first;
    I j = last - 1;
    for (;;) {
        while (i < last && ops.less(*i, pivot)) {
            ++i;
        }
        while (j > first && ops.less(pivot, *j)) {
            --j;
        }
        if (i >= j) {
            return i;
        }
        ops.swap(i, j);
        ++i;
        if (j > first) {
            --j;
        }
    }
}

template <std::random_access_iterator I, class Ops>
requires std::copy_constructible<std::iter_value_t<I>>
void quick_median3_impl(I first, I last, std::size_t cutoff, Ops& ops) {
    while (static_cast<std::size_t>(last - first) > cutoff) {
        I cut = median3_partition(first, last, ops);
        if (cut <= first) {
            cut = first + 1;
        }
        if (cut >= last) {
            cut = last - 1;
        }
        if (cut - first < last - cut) {
            quick_median3_impl(first, cut, cutoff, ops);
            first = cut;
        } else {
            quick_median3_impl(cut, last, cutoff, ops);
            last = cut;
        }
    }
    if (cutoff > 1) {
        insertion_impl(first, last, ops);
    }
}

template <std::random_access_iterator I, class Ops>
requires std::copy_constructible<std::iter_value_t<I>>
void dual_pivot_impl(I first, I last, Ops& ops) {
    if (last - first < 2) {
        return;
    }

    I lo = first;
    I hi = last - 1;
    if (ops.less(*hi, *lo)) {
        ops.swap(lo, hi);
    }

    using T = std::iter_value_t<I>;
    T p = *lo;
    T q = *hi;
    I lt = lo + 1;
    I gt = hi - 1;
    I it = lt;
    while (it <= gt) {
        if (ops.less(*it, p)) {
            ops.swap(it, lt);
            ++lt;
            ++it;
        } else if (ops.less(q, *it)) {
            while (it < gt && ops.less(q, *gt)) {
                --gt;
            }
            ops.swap(it, gt);
            --gt;
            if (ops.less(*it, p)) {
                ops.swap(it, lt);
                ++lt;
            }
            ++it;
        } else {
            ++it;
        }
    }

    --lt;
    ++gt;
    ops.swap(lo, lt);
    ops.swap(hi, gt);
    dual_pivot_impl(first, lt, ops);
    if (ops.less(p, q)) {
        dual_pivot_impl(lt + 1, gt, ops);
    }
    dual_pivot_impl(gt + 1, last, ops);
}

template <std::random_access_iterator I, class Ops>
requires std::copy_constructible<std::iter_value_t<I>>
void intro_impl(I first, I last, unsigned depth, std::size_t cutoff, Ops& ops) {
    while (static_cast<std::size_t>(last - first) > cutoff) {
        if (depth == 0) {
            heap_impl(first, last, ops);
            return;
        }
        --depth;
        I cut = median3_partition(first, last, ops);
        if (cut <= first) {
            cut = first + 1;
        }
        if (cut >= last) {
            cut = last - 1;
        }
        if (cut - first < last - cut) {
            intro_impl(first, cut, depth, cutoff, ops);
            first = cut;
        } else {
            intro_impl(cut, last, depth, cutoff, ops);
            last = cut;
        }
    }
    insertion_impl(first, last, ops);
}

}  // namespace sortlab::detail
