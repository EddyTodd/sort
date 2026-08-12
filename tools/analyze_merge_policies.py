#!/usr/bin/env python3
"""Analyze paired adaptive-merge policy experiments."""
from __future__ import annotations

import argparse
import csv
import math
import random
import statistics
from collections import defaultdict
from pathlib import Path
from typing import Iterable


def median(xs: Iterable[float]) -> float:
    values = list(xs)
    return float(statistics.median(values)) if values else math.nan


def quantile(values: list[float], q: float) -> float:
    if not values:
        return math.nan
    data = sorted(values)
    pos = (len(data) - 1) * q
    lo = int(math.floor(pos))
    hi = int(math.ceil(pos))
    if lo == hi:
        return data[lo]
    return data[lo] * (hi - pos) + data[hi] * (pos - lo)


def bootstrap_median(values: list[float], reps: int, seed: int) -> tuple[float, float]:
    if not values:
        return math.nan, math.nan
    rng = random.Random(seed)
    draws = []
    for _ in range(reps):
        sample = [values[rng.randrange(len(values))] for _ in values]
        draws.append(median(sample))
    return quantile(draws, 0.025), quantile(draws, 0.975)


def sign_test_p(wins: int, losses: int) -> float:
    n = wins + losses
    if n == 0:
        return 1.0
    k = min(wins, losses)
    tail = sum(math.comb(n, i) for i in range(k + 1)) / (2 ** n)
    return min(1.0, 2.0 * tail)


def load(path: Path) -> list[dict[str, str]]:
    with path.open(newline="", encoding="utf-8") as handle:
        rows = list(csv.DictReader(handle))
    required = {
        "merge_policy", "minrun_policy", "pattern", "n", "trial", "input_hash",
        "ns", "comparisons", "writes", "natural_runs", "effective_runs",
        "scheduled_merge_cost", "max_pending_runs", "run_entropy_bits", "verified",
    }
    if not rows or required - set(rows[0]):
        raise ValueError("missing adaptive-merge benchmark columns")
    for row in rows:
        if row["verified"] != "1":
            raise ValueError("unverified adaptive-merge row")
    return rows


def analyze(rows: list[dict[str, str]], baseline_policy: str,
            baseline_minrun: str, bootstrap: int, seed: int) -> list[dict[str, object]]:
    cells: dict[tuple[str, str, str, int], list[dict[str, str]]] = defaultdict(list)
    paired: dict[tuple[str, int, int, str], dict[tuple[str, str], float]] = defaultdict(dict)
    for row in rows:
        key = (row["merge_policy"], row["minrun_policy"], row["pattern"], int(row["n"]))
        cells[key].append(row)
        pair_key = (row["pattern"], int(row["n"]), int(row["trial"]), row["input_hash"])
        paired[pair_key][(row["merge_policy"], row["minrun_policy"])] = float(row["ns"])

    output = []
    for cell_index, ((policy, minrun, pattern, n), group) in enumerate(sorted(cells.items())):
        times = [float(r["ns"]) for r in group]
        comparisons = [float(r["comparisons"]) for r in group]
        writes = [float(r["writes"]) for r in group]
        merge_costs = [float(r["scheduled_merge_cost"]) for r in group]
        entropy = [float(r["run_entropy_bits"]) for r in group]
        redundancies = [
            (float(r["scheduled_merge_cost"]) / n - float(r["run_entropy_bits"]))
            if n else math.nan for r in group
        ]
        speedups: list[float] = []
        wins = losses = ties = 0
        for (pair_pattern, pair_n, _trial, _hash), values in paired.items():
            if pair_pattern != pattern or pair_n != n:
                continue
            candidate = values.get((policy, minrun))
            base = values.get((baseline_policy, baseline_minrun))
            if candidate is None or base is None or candidate <= 0:
                continue
            speedups.append(base / candidate)
            if candidate < base:
                wins += 1
            elif candidate > base:
                losses += 1
            else:
                ties += 1
        lo, hi = bootstrap_median(speedups, bootstrap, seed + cell_index)
        output.append({
            "merge_policy": policy,
            "minrun_policy": minrun,
            "pattern": pattern,
            "n": n,
            "samples": len(group),
            "median_ns": median(times),
            "q1_ns": quantile(times, 0.25),
            "q3_ns": quantile(times, 0.75),
            "median_comparisons": median(comparisons),
            "median_writes": median(writes),
            "median_natural_runs": median(float(r["natural_runs"]) for r in group),
            "median_effective_runs": median(float(r["effective_runs"]) for r in group),
            "median_scheduled_merge_cost": median(merge_costs),
            "median_merge_cost_per_element": median([x / n for x in merge_costs]) if n else math.nan,
            "median_run_entropy_bits": median(entropy),
            "median_merge_redundancy_bits": median(x for x in redundancies if not math.isnan(x)),
            "median_max_pending_runs": median(float(r["max_pending_runs"]) for r in group),
            "paired_samples": len(speedups),
            "paired_median_speedup_vs_baseline": median(speedups),
            "speedup_ci_low": lo,
            "speedup_ci_high": hi,
            "paired_win_rate": wins / (wins + losses + ties) if speedups else math.nan,
            "paired_sign_test_p": sign_test_p(wins, losses),
        })
    return output


def write(rows: list[dict[str, object]], handle) -> None:
    fields = list(rows[0]) if rows else ["merge_policy"]
    writer = csv.DictWriter(handle, fieldnames=fields)
    writer.writeheader()
    writer.writerows(rows)


def self_test() -> int:
    rows = []
    for trial in range(9):
        for policy, minrun, ns, cost in (
            ("powersort", "balanced", 100 + trial, 300),
            ("timsort_stack", "classic", 120 + trial, 330),
            ("pairwise", "none", 150 + trial, 410),
        ):
            rows.append({
                "merge_policy": policy, "minrun_policy": minrun, "pattern": "x",
                "n": "100", "trial": str(trial), "input_hash": str(trial), "ns": str(ns),
                "comparisons": "500", "writes": "700", "natural_runs": "8",
                "effective_runs": "8", "scheduled_merge_cost": str(cost),
                "max_pending_runs": "4", "run_entropy_bits": "3", "verified": "1",
            })
    result = analyze(rows, "powersort", "balanced", 200, 17)
    by = {(r["merge_policy"], r["minrun_policy"]): r for r in result}
    assert by[("timsort_stack", "classic")]["paired_median_speedup_vs_baseline"] < 1.0
    assert by[("powersort", "balanced")]["median_merge_redundancy_bits"] == 0.0
    print("PASS: paired adaptive-merge analysis")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", nargs="?", type=Path)
    parser.add_argument("--baseline-policy", default="powersort")
    parser.add_argument("--baseline-minrun", default="balanced")
    parser.add_argument("--bootstrap", type=int, default=2000)
    parser.add_argument("--seed", type=int, default=20260812)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if args.self_test:
        return self_test()
    if args.input is None:
        parser.error("input CSV required")
    try:
        result = analyze(load(args.input), args.baseline_policy, args.baseline_minrun,
                         args.bootstrap, args.seed)
    except (OSError, ValueError) as exc:
        print(f"error: {exc}")
        return 2
    if args.output:
        with args.output.open("w", newline="", encoding="utf-8") as handle:
            write(result, handle)
    else:
        import sys
        write(result, sys.stdout)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
