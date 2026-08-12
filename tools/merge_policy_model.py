#!/usr/bin/env python3
"""Compare merge schedules against the exact optimal alphabetic merge cost."""
from __future__ import annotations

import argparse
import csv
import json
import math
from dataclasses import dataclass
from typing import Iterable


@dataclass
class Run:
    base: int
    length: int
    power: int = 0


def merge_cost_pairwise(lengths: list[int]) -> int:
    runs = lengths[:]
    cost = 0
    while len(runs) > 1:
        nxt = []
        for i in range(0, len(runs), 2):
            if i + 1 == len(runs):
                nxt.append(runs[i])
            else:
                merged = runs[i] + runs[i + 1]
                cost += merged
                nxt.append(merged)
        runs = nxt
    return cost


def merge_at(stack: list[Run], i: int) -> int:
    left, right = stack[i], stack[i + 1]
    merged = Run(left.base, left.length + right.length, left.power)
    stack[i:i + 2] = [merged]
    return merged.length


def merge_cost_timsort(lengths: list[int]) -> int:
    stack: list[Run] = []
    base = 0
    cost = 0
    for length in lengths:
        stack.append(Run(base, length))
        base += length
        while len(stack) > 1:
            n = len(stack) - 2
            first = n > 0 and stack[n - 1].length <= stack[n].length + stack[n + 1].length
            second = n > 1 and stack[n - 2].length <= stack[n - 1].length + stack[n].length
            if first or second:
                if stack[n - 1].length < stack[n + 1].length:
                    n -= 1
            elif stack[n].length > stack[n + 1].length:
                break
            cost += merge_at(stack, n)
    while len(stack) > 1:
        n = len(stack) - 2
        if n > 0 and stack[n - 1].length < stack[n + 1].length:
            n -= 1
        cost += merge_at(stack, n)
    return cost


def powerloop(s1: int, n1: int, n2: int, n: int) -> int:
    result = 0
    a = 2 * s1 + n1
    b = a + n1 + n2
    while True:
        result += 1
        if a >= n:
            a -= n
            b -= n
        elif b >= n:
            return result
        a <<= 1
        b <<= 1


def merge_cost_powersort(lengths: list[int]) -> int:
    total = sum(lengths)
    stack: list[Run] = []
    base = 0
    cost = 0
    for length in lengths:
        incoming = Run(base, length)
        if stack:
            top = stack[-1]
            power = powerloop(top.base, top.length, incoming.length, total)
            while len(stack) > 1 and stack[-2].power > power:
                cost += merge_at(stack, len(stack) - 2)
            stack[-1].power = power
        stack.append(incoming)
        base += length
    while len(stack) > 1:
        n = len(stack) - 2
        if n > 0 and stack[n - 1].length < stack[n + 1].length:
            n -= 1
        cost += merge_at(stack, n)
    return cost


def exact_optimal_alphabetic_cost(lengths: list[int]) -> int:
    r = len(lengths)
    if r <= 1:
        return 0
    if r > 64:
        raise ValueError("exact dynamic program is capped at 64 runs")
    prefix = [0]
    for length in lengths:
        prefix.append(prefix[-1] + length)
    dp = [[0] * r for _ in range(r)]
    for width in range(2, r + 1):
        for i in range(r - width + 1):
            j = i + width - 1
            weight = prefix[j + 1] - prefix[i]
            dp[i][j] = min(dp[i][k] + dp[k + 1][j] + weight for k in range(i, j))
    return dp[0][r - 1]


def entropy_bits(lengths: Iterable[int]) -> float:
    values = list(lengths)
    total = sum(values)
    if not total:
        return 0.0
    result = 0.0
    for length in values:
        p = length / total
        result -= p * math.log2(p)
    return result


def evaluate(lengths: list[int]) -> list[dict[str, object]]:
    if not lengths or any(x <= 0 for x in lengths):
        raise ValueError("run lengths must be positive")
    optimum = exact_optimal_alphabetic_cost(lengths)
    total = sum(lengths)
    entropy = entropy_bits(lengths)
    rows = []
    for policy, cost in (
        ("pairwise", merge_cost_pairwise(lengths)),
        ("timsort_stack", merge_cost_timsort(lengths)),
        ("powersort", merge_cost_powersort(lengths)),
    ):
        rows.append({
            "policy": policy,
            "runs": len(lengths),
            "n": total,
            "merge_cost": cost,
            "optimal_alphabetic_cost": optimum,
            "cost_ratio_to_optimal": cost / optimum if optimum else 1.0,
            "cost_per_element": cost / total,
            "run_entropy_bits": entropy,
            "redundancy_bits_per_element": cost / total - entropy,
        })
    return rows


def self_test() -> int:
    assert exact_optimal_alphabetic_cost([1, 1, 1]) == 5
    rows = {r["policy"]: r for r in evaluate([10, 1, 1])}
    assert rows["pairwise"]["merge_cost"] == 23
    assert rows["powersort"]["merge_cost"] == 14
    assert rows["powersort"]["optimal_alphabetic_cost"] == 14
    assert merge_cost_timsort([1, 1, 1, 1]) >= 8
    print("PASS: exact alphabetic merge-cost model and policy simulators")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--lengths", help="comma-separated positive run lengths")
    parser.add_argument("--suite", help="JSON file containing named run-length sequences")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if args.self_test:
        return self_test()
    if bool(args.lengths) == bool(args.suite):
        parser.error("provide exactly one of --lengths or --suite")
    try:
        if args.lengths:
            rows = evaluate([int(x) for x in args.lengths.split(",")])
        else:
            data = json.loads(__import__("pathlib").Path(args.suite).read_text(encoding="utf-8"))
            if data.get("schema_version") != 1 or not isinstance(data.get("sequences"), list):
                raise ValueError("unsupported merge-policy sequence suite")
            rows = []
            for sequence in data["sequences"]:
                name = sequence["id"]
                lengths = [int(x) for x in sequence["lengths"]]
                for row in evaluate(lengths):
                    rows.append({"sequence": name, **row})
    except (OSError, json.JSONDecodeError, KeyError, ValueError) as exc:
        print(f"error: {exc}")
        return 2
    writer = csv.DictWriter(__import__("sys").stdout, fieldnames=list(rows[0]))
    writer.writeheader()
    writer.writerows(rows)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
