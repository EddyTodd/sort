#!/usr/bin/env python3
"""Paired reduction for the adaptive merge-kernel experiment."""
from __future__ import annotations

import argparse
import csv
import math
import random
import statistics
import sys
from collections import defaultdict
from pathlib import Path


def median(values: list[float]) -> float:
    return float(statistics.median(values)) if values else math.nan


def quartiles(values: list[float]) -> tuple[float, float]:
    if not values:
        return math.nan, math.nan
    ordered = sorted(values)
    return (
        ordered[max(0, int(0.25 * (len(ordered) - 1)))],
        ordered[min(len(ordered) - 1, int(0.75 * (len(ordered) - 1)))],
    )


def sign_test_p(wins: int, losses: int) -> float:
    n = wins + losses
    if n == 0:
        return 1.0
    k = min(wins, losses)
    lower_tail = sum(math.comb(n, i) for i in range(k + 1)) / (2**n)
    return min(1.0, 2.0 * lower_tail)


def bootstrap_interval(values: list[float], reps: int, seed: int) -> tuple[float, float]:
    if not values:
        return math.nan, math.nan
    rng = random.Random(seed)
    n = len(values)
    estimates = []
    for _ in range(reps):
        estimates.append(statistics.median(values[rng.randrange(n)] for _ in range(n)))
    estimates.sort()
    return (
        float(estimates[int(0.025 * (reps - 1))]),
        float(estimates[int(0.975 * (reps - 1))]),
    )


def read_rows(path: Path) -> list[dict[str, str]]:
    with path.open(newline="", encoding="utf-8") as handle:
        rows = list(csv.DictReader(handle))
    required = {
        "treatment", "buffer_policy", "search_policy", "gallop_threshold",
        "pattern", "n", "trial", "input_hash", "ns", "comparisons", "writes",
        "temp_bytes_peak", "temp_elements_copied", "gallop_entries",
        "gallop_elements", "verified",
    }
    if not rows or required - set(rows[0]):
        raise ValueError("missing merge-kernel benchmark columns")
    if any(row["verified"] != "1" for row in rows):
        raise ValueError("unverified merge-kernel row")
    return rows


def reduce_rows(rows: list[dict[str, str]], baseline: str = "smaller_gallop_7",
                bootstrap: int = 2000) -> list[dict[str, object]]:
    grouped: dict[tuple[str, str, int], list[dict[str, str]]] = defaultdict(list)
    identities: dict[tuple[str, int, int, str], dict[str, float]] = defaultdict(dict)

    for row in rows:
        key = (row["treatment"], row["pattern"], int(row["n"]))
        grouped[key].append(row)
        identity = (row["pattern"], int(row["n"]), int(row["trial"]), row["input_hash"])
        identities[identity][row["treatment"]] = float(row["ns"])

    output: list[dict[str, object]] = []
    for (treatment, pattern, n), group in sorted(
        grouped.items(), key=lambda item: (item[0][1], item[0][2], item[0][0])):
        elapsed = [float(row["ns"]) for row in group]
        ratios: list[float] = []
        wins = losses = ties = 0
        for row in group:
            identity = (pattern, n, int(row["trial"]), row["input_hash"])
            baseline_ns = identities[identity].get(baseline)
            if baseline_ns is None:
                continue
            current_ns = float(row["ns"])
            if current_ns > 0:
                ratios.append(baseline_ns / current_ns)
            if current_ns < baseline_ns:
                wins += 1
            elif current_ns > baseline_ns:
                losses += 1
            else:
                ties += 1

        if ratios:
            seed = 0xC0FFEE ^ n ^ sum(map(ord, treatment))
            ci_low, ci_high = bootstrap_interval(ratios, bootstrap, seed)
        else:
            ci_low, ci_high = math.nan, math.nan
        q1, q3 = quartiles(elapsed)
        first = group[0]
        output.append({
            "treatment": treatment,
            "buffer_policy": first["buffer_policy"],
            "search_policy": first["search_policy"],
            "gallop_threshold": int(first["gallop_threshold"]),
            "pattern": pattern,
            "n": n,
            "samples": len(group),
            "median_ns": median(elapsed),
            "q1_ns": q1,
            "q3_ns": q3,
            "median_comparisons": median([float(row["comparisons"]) for row in group]),
            "median_writes": median([float(row["writes"]) for row in group]),
            "median_temp_bytes_peak": median([float(row["temp_bytes_peak"]) for row in group]),
            "median_temp_elements_copied": median(
                [float(row["temp_elements_copied"]) for row in group]),
            "median_gallop_entries": median([float(row["gallop_entries"]) for row in group]),
            "median_gallop_elements": median([float(row["gallop_elements"]) for row in group]),
            "paired_samples": len(ratios),
            "median_speedup_vs_baseline": median(ratios),
            "speedup_ci_low": ci_low,
            "speedup_ci_high": ci_high,
            "win_rate": wins / len(ratios) if ratios else math.nan,
            "ties": ties,
            "sign_test_p": sign_test_p(wins, losses),
        })
    return output


def write_rows(rows: list[dict[str, object]], handle) -> None:
    fields = list(rows[0]) if rows else ["treatment"]
    writer = csv.DictWriter(handle, fieldnames=fields)
    writer.writeheader()
    writer.writerows(rows)


def self_test() -> int:
    rows: list[dict[str, str]] = []
    for trial in range(20):
        for treatment, buffer_policy, search_policy, threshold, ns in (
            ("full_linear", "full", "linear", 0, 120),
            ("smaller_linear", "smaller", "linear", 0, 100),
            ("smaller_gallop_7", "smaller", "gallop", 7, 80),
        ):
            rows.append({
                "treatment": treatment,
                "buffer_policy": buffer_policy,
                "search_policy": search_policy,
                "gallop_threshold": str(threshold),
                "pattern": "run_long_short",
                "n": "1024",
                "trial": str(trial),
                "input_hash": str(trial),
                "ns": str(ns + trial % 3),
                "comparisons": "100",
                "writes": "200",
                "temp_bytes_peak": "512",
                "temp_elements_copied": "64",
                "gallop_entries": "4",
                "gallop_elements": "20",
                "verified": "1",
            })
    result = reduce_rows(rows, bootstrap=200)
    cell = next(row for row in result if row["treatment"] == "full_linear")
    assert cell["median_speedup_vs_baseline"] < 0.8
    assert cell["paired_samples"] == 20
    print("PASS: paired adaptive merge-kernel reduction")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", nargs="?", type=Path)
    parser.add_argument("--baseline", default="smaller_gallop_7")
    parser.add_argument("--bootstrap", type=int, default=5000)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if args.self_test:
        return self_test()
    if args.input is None:
        parser.error("input CSV required")
    rows = reduce_rows(read_rows(args.input), args.baseline, args.bootstrap)
    if args.output:
        with args.output.open("w", newline="", encoding="utf-8") as handle:
            write_rows(rows, handle)
    else:
        write_rows(rows, sys.stdout)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
