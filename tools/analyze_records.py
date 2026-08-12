#!/usr/bin/env python3
"""Statistical reduction for sort_records CSV output."""
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
    ordered = sorted(values)
    if not ordered:
        raise ValueError("quantile of empty sample")
    if len(ordered) == 1:
        return float(ordered[0])
    position = (len(ordered) - 1) * q
    lo = math.floor(position)
    hi = math.ceil(position)
    if lo == hi:
        return float(ordered[lo])
    weight = position - lo
    return ordered[lo] * (1.0 - weight) + ordered[hi] * weight


def bootstrap_median_ci(values: Sequence[float], reps: int, seed: int) -> tuple[float, float]:
    if not values:
        return math.nan, math.nan
    center = float(statistics.median(values))
    if len(values) == 1 or reps == 0:
        return center, center
    rng = random.Random(seed)
    medians: list[float] = []
    for _ in range(reps):
        sample = [values[rng.randrange(len(values))] for _ in range(len(values))]
        medians.append(float(statistics.median(sample)))
    return quantile(medians, 0.025), quantile(medians, 0.975)


def sign_test_pvalue(ratios: Sequence[float]) -> float:
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
    payload_words: int
    record_bytes: int
    trial: int
    key_hash: str
    ns: float
    explicit_bytes_moved: int
    stable: bool
    verified: bool


def read_rows(paths: Iterable[Path]) -> list[Row]:
    rows: list[Row] = []
    required = {
        "algorithm", "pattern", "n", "payload_words", "record_bytes", "trial",
        "key_hash", "ns", "explicit_bytes_moved", "stable_on_trial", "verified",
    }
    for path in paths:
        with path.open(newline="", encoding="utf-8") as handle:
            reader = csv.DictReader(handle)
            missing = required - set(reader.fieldnames or ())
            if missing:
                raise ValueError(f"{path}: missing columns: {', '.join(sorted(missing))}")
            for raw in reader:
                rows.append(Row(
                    algorithm=raw["algorithm"],
                    pattern=raw["pattern"],
                    n=int(raw["n"]),
                    payload_words=int(raw["payload_words"]),
                    record_bytes=int(raw["record_bytes"]),
                    trial=int(raw["trial"]),
                    key_hash=raw["key_hash"],
                    ns=float(raw["ns"]),
                    explicit_bytes_moved=int(raw["explicit_bytes_moved"]),
                    stable=raw["stable_on_trial"] == "1",
                    verified=raw["verified"] == "1",
                ))
    if not rows:
        raise ValueError("no record benchmark rows")
    failures = [row for row in rows if not row.verified]
    if failures:
        raise ValueError(f"refusing to analyze {len(failures)} unverified rows")
    return rows


def summarize(rows: Sequence[Row], baseline: str, reps: int, seed: int) -> list[dict[str, object]]:
    grouped: dict[tuple[str, str, int, int, int], list[Row]] = defaultdict(list)
    paired: dict[tuple[str, int, int, int, str], dict[str, Row]] = defaultdict(dict)
    for row in rows:
        grouped[(row.algorithm, row.pattern, row.n, row.payload_words, row.record_bytes)].append(row)
        paired[(row.pattern, row.n, row.record_bytes, row.trial, row.key_hash)][row.algorithm] = row

    output: list[dict[str, object]] = []
    for index, ((algorithm, pattern, n, payload_words, record_bytes), group) in enumerate(sorted(grouped.items())):
        values = [row.ns for row in group]
        moved = [row.explicit_bytes_moved for row in group]
        low, high = bootstrap_median_ci(values, reps, seed + index)
        ratios: list[float] = []
        for row in group:
            base = paired[(row.pattern, row.n, row.record_bytes, row.trial, row.key_hash)].get(baseline)
            if algorithm == baseline:
                ratios.append(1.0)
            elif base is not None and row.ns > 0:
                ratios.append(base.ns / row.ns)
        speed = float(statistics.median(ratios)) if ratios else math.nan
        speed_low, speed_high = bootstrap_median_ci(ratios, reps, seed ^ (index + 0x9E3779B9)) if ratios else (math.nan, math.nan)
        wins = sum(value > 1.0 for value in ratios)
        ties = sum(value == 1.0 for value in ratios)
        output.append({
            "algorithm": algorithm,
            "pattern": pattern,
            "n": n,
            "payload_words": payload_words,
            "record_bytes": record_bytes,
            "samples": len(values),
            "median_ns": float(statistics.median(values)),
            "q1_ns": quantile(values, 0.25),
            "q3_ns": quantile(values, 0.75),
            "median_ci95_low_ns": low,
            "median_ci95_high_ns": high,
            "median_explicit_bytes_moved": float(statistics.median(moved)),
            "stable_trial_rate": sum(row.stable for row in group) / len(group),
            "median_speedup_vs_baseline": speed,
            "speedup_ci95_low": speed_low,
            "speedup_ci95_high": speed_high,
            "paired_samples": len(ratios),
            "paired_win_rate": (wins + 0.5 * ties) / len(ratios) if ratios else math.nan,
            "paired_sign_test_p": sign_test_pvalue(ratios) if ratios else math.nan,
        })
    return output


def write_summary(rows: Sequence[dict[str, object]], output) -> None:
    fields = [
        "algorithm", "pattern", "n", "payload_words", "record_bytes", "samples",
        "median_ns", "q1_ns", "q3_ns", "median_ci95_low_ns", "median_ci95_high_ns",
        "median_explicit_bytes_moved", "stable_trial_rate", "median_speedup_vs_baseline", "speedup_ci95_low", "speedup_ci95_high",
        "paired_samples", "paired_win_rate", "paired_sign_test_p",
    ]
    writer = csv.DictWriter(output, fieldnames=fields)
    writer.writeheader()
    writer.writerows(rows)


def self_test() -> int:
    rows = [
        Row("a", "few_unique", 10, 1, 24, 0, "x", 50, 120, False, True),
        Row("std_sort", "few_unique", 10, 1, 24, 0, "x", 100, 0, False, True),
        Row("a", "few_unique", 10, 1, 24, 1, "y", 100, 240, True, True),
        Row("std_sort", "few_unique", 10, 1, 24, 1, "y", 200, 0, True, True),
    ]
    result = summarize(rows, "std_sort", 100, 7)
    a = next(row for row in result if row["algorithm"] == "a")
    assert a["median_ns"] == 75.0
    assert a["median_speedup_vs_baseline"] == 2.0
    assert a["median_explicit_bytes_moved"] == 180.0
    assert a["stable_trial_rate"] == 0.5
    assert a["paired_win_rate"] == 1.0
    print("PASS: record analysis, stability aggregation, and paired inference")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("inputs", nargs="*", type=Path)
    parser.add_argument("--baseline", default="std_sort")
    parser.add_argument("--bootstrap", type=int, default=2000)
    parser.add_argument("--seed", type=int, default=20260812)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if args.self_test:
        return self_test()
    if not args.inputs:
        parser.error("at least one input CSV is required")
    if args.bootstrap < 0:
        parser.error("--bootstrap must be nonnegative")
    summary = summarize(read_rows(args.inputs), args.baseline, args.bootstrap, args.seed)
    if args.output:
        with args.output.open("w", newline="", encoding="utf-8") as handle:
            write_summary(summary, handle)
    else:
        write_summary(summary, sys.stdout)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
