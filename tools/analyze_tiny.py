#!/usr/bin/env python3
"""Reduce paired tiny-kernel benchmarks with bootstrap uncertainty."""
from __future__ import annotations

import argparse
import csv
import math
import random
import statistics
from collections import defaultdict
from pathlib import Path


def quantile(values: list[float], q: float) -> float:
    ordered = sorted(values)
    if not ordered:
        return math.nan
    if len(ordered) == 1:
        return float(ordered[0])
    position = (len(ordered) - 1) * q
    lower = int(math.floor(position))
    upper = min(lower + 1, len(ordered) - 1)
    weight = position - lower
    return float(ordered[lower] * (1 - weight) + ordered[upper] * weight)


def bootstrap_median(values: list[float], reps: int,
                     rng: random.Random) -> tuple[float, float]:
    if not values:
        return math.nan, math.nan
    medians: list[float] = []
    n = len(values)
    for _ in range(reps):
        medians.append(statistics.median(values[rng.randrange(n)] for _ in range(n)))
    return quantile(medians, 0.025), quantile(medians, 0.975)


def read(path: Path) -> list[dict[str, str]]:
    with path.open(newline="", encoding="utf-8") as handle:
        rows = list(csv.DictReader(handle))
    required = {
        "kernel", "pattern", "n", "trial", "input_hash", "ns",
        "comparisons", "writes", "verified",
    }
    if not rows or required - set(rows[0]):
        raise ValueError("missing tiny benchmark columns")
    for row in rows:
        if row["verified"] != "1":
            raise ValueError("unverified tiny benchmark row")
    return rows


def analyze(rows: list[dict[str, str]], baseline: str = "insertion",
            reps: int = 2000, seed: int = 0x7711) -> list[dict[str, object]]:
    grouped: dict[tuple[str, str, int], list[dict[str, str]]] = defaultdict(list)
    instances: dict[tuple[str, int, int, str], dict[str, float]] = defaultdict(dict)
    for row in rows:
        grouped[(row["kernel"], row["pattern"], int(row["n"]))].append(row)
        key = (row["pattern"], int(row["n"]), int(row["trial"]), row["input_hash"])
        instances[key][row["kernel"]] = float(row["ns"])

    rng = random.Random(seed)
    output: list[dict[str, object]] = []
    for (kernel, pattern, n), items in sorted(
            grouped.items(), key=lambda item: (item[0][1], item[0][2], item[0][0])):
        elapsed = [float(row["ns"]) for row in items]
        ratios: list[float] = []
        for (instance_pattern, instance_n, _, _), peers in instances.items():
            if instance_pattern != pattern or instance_n != n:
                continue
            if kernel in peers and baseline in peers and peers[kernel] > 0:
                ratios.append(peers[baseline] / peers[kernel])
        low, high = bootstrap_median(ratios, reps, rng) if ratios else (math.nan, math.nan)
        output.append({
            "kernel": kernel,
            "pattern": pattern,
            "n": n,
            "samples": len(elapsed),
            "median_ns": float(statistics.median(elapsed)),
            "median_comparisons": float(statistics.median(
                float(row["comparisons"]) for row in items)),
            "median_writes": float(statistics.median(
                float(row["writes"]) for row in items)),
            "paired_samples": len(ratios),
            "median_speedup_vs_" + baseline:
                float(statistics.median(ratios)) if ratios else math.nan,
            "speedup_ci95_low": low,
            "speedup_ci95_high": high,
            "win_rate": sum(value > 1 for value in ratios) / len(ratios)
                if ratios else math.nan,
        })
    return output


def write(rows: list[dict[str, object]], handle) -> None:
    fields = list(rows[0].keys()) if rows else ["kernel"]
    writer = csv.DictWriter(handle, fieldnames=fields)
    writer.writeheader()
    writer.writerows(rows)


def self_test() -> int:
    rows: list[dict[str, str]] = []
    for trial in range(9):
        for kernel, elapsed in (("insertion", 100), ("bitonic_network", 70)):
            rows.append({
                "kernel": kernel, "pattern": "random", "n": "8",
                "trial": str(trial), "input_hash": str(trial),
                "ns": str(elapsed + trial % 2), "comparisons": "10",
                "writes": "20", "verified": "1",
            })
    result = analyze(rows, reps=200)
    network = next(row for row in result if row["kernel"] == "bitonic_network")
    assert network["median_speedup_vs_insertion"] > 1.3
    assert network["win_rate"] == 1
    print("PASS: paired tiny-kernel bootstrap analysis")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", nargs="?", type=Path)
    parser.add_argument("--baseline", default="insertion")
    parser.add_argument("--bootstrap", type=int, default=2000)
    parser.add_argument("--seed", type=int, default=0x7711)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if args.self_test:
        return self_test()
    if args.input is None:
        parser.error("input CSV required")
    rows = analyze(read(args.input), args.baseline, args.bootstrap, args.seed)
    if args.output:
        with args.output.open("w", newline="", encoding="utf-8") as handle:
            write(rows, handle)
    else:
        import sys
        write(rows, sys.stdout)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
