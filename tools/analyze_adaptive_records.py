#!/usr/bin/env python3
"""Paired statistical reduction for adaptive-record sorting experiments."""
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


def bootstrap_median_ci(values: Sequence[float], reps: int,
                        seed: int) -> tuple[float, float]:
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
    treatment: str
    merge_policy: str
    minrun_policy: str
    buffer_policy: str
    search_policy: str
    gallop_threshold: int
    pattern: str
    n: int
    payload_words: int
    record_bytes: int
    trial: int
    input_hash: str
    ns: float
    comparisons: int
    explicit_moves: int
    explicit_bytes_moved: int
    temp_bytes_peak: int
    temp_bytes_copied: int
    gallop_entries: int
    gallop_records: int
    stable: bool
    verified: bool


def read_rows(paths: Iterable[Path]) -> list[Row]:
    required = {
        "treatment", "merge_policy", "minrun_policy", "buffer_policy",
        "search_policy", "gallop_threshold", "pattern", "n", "payload_words",
        "record_bytes", "trial", "input_hash", "ns", "comparisons",
        "explicit_record_moves", "explicit_bytes_moved",
        "temp_bytes_requested_peak", "temp_bytes_copied", "gallop_entries",
        "gallop_records", "stable_on_trial", "verified",
    }
    rows: list[Row] = []
    for path in paths:
        with path.open(newline="", encoding="utf-8") as handle:
            reader = csv.DictReader(handle)
            missing = required - set(reader.fieldnames or ())
            if missing:
                raise ValueError(
                    f"{path}: missing columns: {', '.join(sorted(missing))}")
            for raw in reader:
                rows.append(Row(
                    treatment=raw["treatment"],
                    merge_policy=raw["merge_policy"],
                    minrun_policy=raw["minrun_policy"],
                    buffer_policy=raw["buffer_policy"],
                    search_policy=raw["search_policy"],
                    gallop_threshold=int(raw["gallop_threshold"]),
                    pattern=raw["pattern"],
                    n=int(raw["n"]),
                    payload_words=int(raw["payload_words"]),
                    record_bytes=int(raw["record_bytes"]),
                    trial=int(raw["trial"]),
                    input_hash=raw["input_hash"],
                    ns=float(raw["ns"]),
                    comparisons=int(raw["comparisons"]),
                    explicit_moves=int(raw["explicit_record_moves"]),
                    explicit_bytes_moved=int(raw["explicit_bytes_moved"]),
                    temp_bytes_peak=int(raw["temp_bytes_requested_peak"]),
                    temp_bytes_copied=int(raw["temp_bytes_copied"]),
                    gallop_entries=int(raw["gallop_entries"]),
                    gallop_records=int(raw["gallop_records"]),
                    stable=raw["stable_on_trial"] == "1",
                    verified=raw["verified"] == "1",
                ))
    if not rows:
        raise ValueError("no adaptive-record benchmark rows")
    invalid = [row for row in rows if not row.verified or not row.stable]
    if invalid:
        raise ValueError(
            f"refusing to analyze {len(invalid)} unverified or unstable rows")
    return rows


def summarize(rows: Sequence[Row], baseline: str, reps: int,
              seed: int) -> list[dict[str, object]]:
    grouped: dict[tuple[str, str, int, int, int], list[Row]] = defaultdict(list)
    paired: dict[tuple[str, int, int, int, str], dict[str, Row]] = defaultdict(dict)
    for row in rows:
        grouped[(row.treatment, row.pattern, row.n, row.payload_words,
                 row.record_bytes)].append(row)
        paired[(row.pattern, row.n, row.payload_words, row.trial,
                row.input_hash)][row.treatment] = row

    output: list[dict[str, object]] = []
    for index, (key, group) in enumerate(sorted(grouped.items())):
        treatment, pattern, n, payload_words, record_bytes = key
        first = group[0]
        elapsed = [row.ns for row in group]
        ratios: list[float] = []
        for row in group:
            base = paired[(row.pattern, row.n, row.payload_words, row.trial,
                           row.input_hash)].get(baseline)
            if treatment == baseline:
                ratios.append(1.0)
            elif base is not None and row.ns > 0:
                ratios.append(base.ns / row.ns)
        speed = float(statistics.median(ratios)) if ratios else math.nan
        low, high = bootstrap_median_ci(
            ratios, reps, seed ^ (index + 0x9E3779B9)) if ratios else (math.nan, math.nan)
        wins = sum(value > 1.0 for value in ratios)
        ties = sum(value == 1.0 for value in ratios)
        output.append({
            "treatment": treatment,
            "merge_policy": first.merge_policy,
            "minrun_policy": first.minrun_policy,
            "buffer_policy": first.buffer_policy,
            "search_policy": first.search_policy,
            "gallop_threshold": first.gallop_threshold,
            "pattern": pattern,
            "n": n,
            "payload_words": payload_words,
            "record_bytes": record_bytes,
            "samples": len(group),
            "median_ns": float(statistics.median(elapsed)),
            "q1_ns": quantile(elapsed, 0.25),
            "q3_ns": quantile(elapsed, 0.75),
            "median_comparisons": float(statistics.median(
                row.comparisons for row in group)),
            "median_explicit_record_moves": float(statistics.median(
                row.explicit_moves for row in group)),
            "median_explicit_bytes_moved": float(statistics.median(
                row.explicit_bytes_moved for row in group)),
            "median_temp_bytes_peak": float(statistics.median(
                row.temp_bytes_peak for row in group)),
            "median_temp_bytes_copied": float(statistics.median(
                row.temp_bytes_copied for row in group)),
            "median_gallop_entries": float(statistics.median(
                row.gallop_entries for row in group)),
            "median_gallop_records": float(statistics.median(
                row.gallop_records for row in group)),
            "stable_trial_rate": sum(row.stable for row in group) / len(group),
            "median_speedup_vs_baseline": speed,
            "speedup_ci95_low": low,
            "speedup_ci95_high": high,
            "paired_samples": len(ratios),
            "paired_win_rate": ((wins + 0.5 * ties) / len(ratios)
                                if ratios else math.nan),
            "paired_sign_test_p": sign_test_pvalue(ratios) if ratios else math.nan,
        })
    return output


def write(rows: Sequence[dict[str, object]], handle) -> None:
    fields = list(rows[0].keys()) if rows else ["treatment"]
    writer = csv.DictWriter(handle, fieldnames=fields)
    writer.writeheader()
    writer.writerows(rows)


def self_test() -> int:
    rows = [
        Row("candidate", "powersort", "balanced", "smaller", "linear", 0,
            "few_unique", 64, 7, 72, 0, "x", 50, 100, 80, 5760, 1024,
            2048, 0, 0, True, True),
        Row("baseline", "powersort", "balanced", "full", "linear", 0,
            "few_unique", 64, 7, 72, 0, "x", 100, 120, 120, 8640, 2048,
            4096, 0, 0, True, True),
        Row("candidate", "powersort", "balanced", "smaller", "linear", 0,
            "few_unique", 64, 7, 72, 1, "y", 75, 101, 82, 5904, 1024,
            2100, 0, 0, True, True),
        Row("baseline", "powersort", "balanced", "full", "linear", 0,
            "few_unique", 64, 7, 72, 1, "y", 150, 121, 121, 8712, 2048,
            4100, 0, 0, True, True),
    ]
    result = summarize(rows, "baseline", 100, 7)
    candidate = next(row for row in result if row["treatment"] == "candidate")
    assert candidate["median_ns"] == 62.5
    assert candidate["median_speedup_vs_baseline"] == 2.0
    assert candidate["paired_samples"] == 2
    assert candidate["stable_trial_rate"] == 1.0
    assert candidate["median_temp_bytes_peak"] == 1024.0
    print("PASS: adaptive-record paired analysis and stability gate")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("inputs", nargs="*", type=Path)
    parser.add_argument("--baseline", required=False,
                        default="powersort_balanced_full_linear")
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
    rows = summarize(read_rows(args.inputs), args.baseline, args.bootstrap, args.seed)
    if args.output:
        with args.output.open("w", newline="", encoding="utf-8") as handle:
            write(rows, handle)
    else:
        write(rows, sys.stdout)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
