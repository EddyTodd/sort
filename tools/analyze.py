#!/usr/bin/env python3
"""Robust, dependency-free statistical reduction for sort_lab CSV results."""
from __future__ import annotations

import argparse
import csv
import math
import random
import statistics
import sys
from collections import defaultdict
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable, Sequence


def quantile(values: Sequence[float], q: float) -> float:
    if not values:
        raise ValueError("quantile of empty sample")
    ordered = sorted(values)
    if len(ordered) == 1:
        return float(ordered[0])
    pos = (len(ordered) - 1) * q
    lo = math.floor(pos)
    hi = math.ceil(pos)
    if lo == hi:
        return float(ordered[lo])
    weight = pos - lo
    return ordered[lo] * (1.0 - weight) + ordered[hi] * weight


def mad(values: Sequence[float]) -> float:
    center = statistics.median(values)
    return statistics.median(abs(x - center) for x in values)


def bootstrap_ci(values: Sequence[float], reps: int, seed: int, alpha: float = 0.05) -> tuple[float, float]:
    if not values:
        return math.nan, math.nan
    if len(values) == 1 or reps == 0:
        value = float(statistics.median(values))
        return value, value
    rng = random.Random(seed)
    n = len(values)
    medians = []
    for _ in range(reps):
        sample = [values[rng.randrange(n)] for _ in range(n)]
        medians.append(float(statistics.median(sample)))
    return quantile(medians, alpha / 2), quantile(medians, 1 - alpha / 2)


def sign_test_pvalue(ratios: Sequence[float]) -> float:
    """Exact two-sided paired sign test; ties are excluded from the binomial count."""
    wins = sum(value > 1.0 for value in ratios)
    losses = sum(value < 1.0 for value in ratios)
    n = wins + losses
    if n == 0:
        return 1.0
    tail = min(wins, losses)
    probability = sum(math.comb(n, k) for k in range(tail + 1)) / (2 ** n)
    return min(1.0, 2.0 * probability)


@dataclass(frozen=True)
class Row:
    algorithm: str
    pattern: str
    n: int
    trial: int
    input_hash: str
    ns: float
    verified: bool


def read_rows(paths: Iterable[Path]) -> list[Row]:
    rows: list[Row] = []
    required = {"algorithm", "pattern", "n", "trial", "input_hash", "ns", "verified"}
    for path in paths:
        with path.open(newline="", encoding="utf-8") as handle:
            reader = csv.DictReader(handle)
            missing = required - set(reader.fieldnames or ())
            if missing:
                raise ValueError(f"{path}: missing columns: {', '.join(sorted(missing))}")
            for raw in reader:
                rows.append(Row(
                    algorithm=raw["algorithm"], pattern=raw["pattern"], n=int(raw["n"]),
                    trial=int(raw["trial"]), input_hash=raw["input_hash"], ns=float(raw["ns"]),
                    verified=raw["verified"] == "1"))
    if not rows:
        raise ValueError("no benchmark rows")
    failures = [row for row in rows if not row.verified]
    if failures:
        raise ValueError(f"refusing to analyze {len(failures)} unverified rows")
    return rows


def summarize(rows: Sequence[Row], baseline: str, reps: int, seed: int) -> list[dict[str, object]]:
    grouped: dict[tuple[str, str, int], list[Row]] = defaultdict(list)
    by_trial: dict[tuple[str, int, int, str], dict[str, Row]] = defaultdict(dict)
    for row in rows:
        grouped[(row.algorithm, row.pattern, row.n)].append(row)
        by_trial[(row.pattern, row.n, row.trial, row.input_hash)][row.algorithm] = row

    output: list[dict[str, object]] = []
    for group_index, ((algorithm, pattern, n), group) in enumerate(sorted(grouped.items())):
        values = [row.ns for row in group]
        center = float(statistics.median(values))
        low, high = bootstrap_ci(values, reps, seed + group_index)
        ratios: list[float] = []
        if algorithm != baseline:
            for row in group:
                peers = by_trial[(row.pattern, row.n, row.trial, row.input_hash)]
                base = peers.get(baseline)
                if base is not None and row.ns > 0:
                    ratios.append(base.ns / row.ns)
        else:
            ratios = [1.0 for _ in group]
        speed_center = float(statistics.median(ratios)) if ratios else math.nan
        speed_low, speed_high = bootstrap_ci(ratios, reps, seed ^ (group_index + 0x9E3779B9)) if ratios else (math.nan, math.nan)
        wins = sum(value > 1.0 for value in ratios)
        ties = sum(value == 1.0 for value in ratios)
        output.append({
            "algorithm": algorithm,
            "pattern": pattern,
            "n": n,
            "samples": len(values),
            "median_ns": center,
            "q1_ns": quantile(values, 0.25),
            "q3_ns": quantile(values, 0.75),
            "mad_ns": mad(values),
            "median_ci95_low_ns": low,
            "median_ci95_high_ns": high,
            "median_speedup_vs_baseline": speed_center,
            "speedup_ci95_low": speed_low,
            "speedup_ci95_high": speed_high,
            "paired_samples": len(ratios),
            "paired_win_rate": (wins + 0.5 * ties) / len(ratios) if ratios else math.nan,
            "paired_sign_test_p": sign_test_pvalue(ratios) if ratios else math.nan,
        })
    return output


def write_summary(rows: Sequence[dict[str, object]], output) -> None:
    fields = [
        "algorithm", "pattern", "n", "samples", "median_ns", "q1_ns", "q3_ns", "mad_ns",
        "median_ci95_low_ns", "median_ci95_high_ns", "median_speedup_vs_baseline",
        "speedup_ci95_low", "speedup_ci95_high", "paired_samples", "paired_win_rate", "paired_sign_test_p",
    ]
    writer = csv.DictWriter(output, fieldnames=fields)
    writer.writeheader()
    for row in rows:
        writer.writerow(row)


def self_test() -> int:
    assert quantile([1, 2, 3, 4], 0.5) == 2.5
    assert mad([1, 2, 3]) == 1
    assert bootstrap_ci([7], 100, 1) == (7.0, 7.0)
    assert sign_test_pvalue([2.0, 2.0, 2.0]) == 0.25
    rows = [
        Row("a", "random", 10, 0, "x", 50, True), Row("std_sort", "random", 10, 0, "x", 100, True),
        Row("a", "random", 10, 1, "y", 100, True), Row("std_sort", "random", 10, 1, "y", 200, True),
    ]
    summary = summarize(rows, "std_sort", 100, 3)
    a = next(row for row in summary if row["algorithm"] == "a")
    assert a["median_ns"] == 75.0
    assert a["median_speedup_vs_baseline"] == 2.0
    assert a["paired_samples"] == 2
    assert a["paired_win_rate"] == 1.0
    print("PASS: analysis statistics, paired-baseline matching, and paired sign test")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("inputs", nargs="*", type=Path, help="raw sort_lab CSV files")
    parser.add_argument("--baseline", default="std_sort", help="paired speedup baseline")
    parser.add_argument("--bootstrap", type=int, default=2000, help="bootstrap replicates (0 disables resampling)")
    parser.add_argument("--seed", type=int, default=20260811, help="analysis RNG seed")
    parser.add_argument("--output", type=Path, help="summary CSV (default: stdout)")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if args.self_test:
        return self_test()
    if not args.inputs:
        parser.error("at least one input CSV is required")
    if args.bootstrap < 0:
        parser.error("--bootstrap must be nonnegative")
    rows = read_rows(args.inputs)
    summary = summarize(rows, args.baseline, args.bootstrap, args.seed)
    if args.output:
        with args.output.open("w", newline="", encoding="utf-8") as handle:
            write_summary(summary, handle)
    else:
        write_summary(summary, sys.stdout)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
