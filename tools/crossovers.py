#!/usr/bin/env python3
"""Locate empirical size brackets where paired speedup crosses 1.0."""
from __future__ import annotations

import argparse
import csv
import math
import sys
from collections import defaultdict
from pathlib import Path


def state(low: float, high: float) -> str:
    if low > 1.0:
        return "faster"
    if high < 1.0:
        return "slower"
    return "uncertain"


def estimate_crossing(n0: int, speed0: float, n1: int, speed1: float) -> float:
    if n0 <= 0 or n1 <= 0 or speed0 <= 0 or speed1 <= 0 or speed0 == speed1:
        return math.nan
    x0, x1 = math.log(n0), math.log(n1)
    y0, y1 = math.log(speed0), math.log(speed1)
    if y0 == y1:
        return math.nan
    x = x0 + (0.0 - y0) * (x1 - x0) / (y1 - y0)
    return math.exp(x)


def read_summary(path: Path) -> list[dict[str, str]]:
    with path.open(newline="", encoding="utf-8") as handle:
        rows = list(csv.DictReader(handle))
    required = {"algorithm", "pattern", "n", "median_speedup_vs_baseline", "speedup_ci95_low", "speedup_ci95_high"}
    if not rows:
        raise ValueError("empty summary")
    missing = required - set(rows[0])
    if missing:
        raise ValueError(f"missing columns: {', '.join(sorted(missing))}")
    return rows


def find_crossovers(rows: list[dict[str, str]]) -> list[dict[str, object]]:
    groups: dict[tuple[str, str, str], list[dict[str, str]]] = defaultdict(list)
    for row in rows:
        record_bytes = row.get("record_bytes", "0") or "0"
        groups[(row["algorithm"], row["pattern"], record_bytes)].append(row)

    output: list[dict[str, object]] = []
    for (algorithm, pattern, record_bytes), group in sorted(groups.items()):
        ordered = sorted(group, key=lambda row: int(row["n"]))
        for left, right in zip(ordered, ordered[1:]):
            s0 = float(left["median_speedup_vs_baseline"])
            s1 = float(right["median_speedup_vs_baseline"])
            if not (math.isfinite(s0) and math.isfinite(s1)):
                continue
            if (s0 - 1.0) * (s1 - 1.0) >= 0:
                continue
            left_state = state(float(left["speedup_ci95_low"]), float(left["speedup_ci95_high"]))
            right_state = state(float(right["speedup_ci95_low"]), float(right["speedup_ci95_high"]))
            support = "decisive" if {left_state, right_state} == {"faster", "slower"} else "exploratory"
            output.append({
                "algorithm": algorithm,
                "pattern": pattern,
                "record_bytes": int(record_bytes),
                "n_low": int(left["n"]),
                "n_high": int(right["n"]),
                "speedup_low_n": s0,
                "speedup_high_n": s1,
                "estimated_crossover_n": estimate_crossing(int(left["n"]), s0, int(right["n"]), s1),
                "left_state": left_state,
                "right_state": right_state,
                "support": support,
            })
    return output


def write_rows(rows: list[dict[str, object]], output) -> None:
    fields = [
        "algorithm", "pattern", "record_bytes", "n_low", "n_high", "speedup_low_n",
        "speedup_high_n", "estimated_crossover_n", "left_state", "right_state", "support",
    ]
    writer = csv.DictWriter(output, fieldnames=fields)
    writer.writeheader()
    writer.writerows(rows)


def self_test() -> int:
    rows = [
        {"algorithm": "a", "pattern": "random", "n": "100", "median_speedup_vs_baseline": "0.8",
         "speedup_ci95_low": "0.7", "speedup_ci95_high": "0.9"},
        {"algorithm": "a", "pattern": "random", "n": "1000", "median_speedup_vs_baseline": "1.25",
         "speedup_ci95_low": "1.1", "speedup_ci95_high": "1.4"},
    ]
    found = find_crossovers(rows)
    assert len(found) == 1
    assert found[0]["support"] == "decisive"
    assert 100 < found[0]["estimated_crossover_n"] < 1000
    print("PASS: crossover bracketing and log-scale interpolation")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("summary", nargs="?", type=Path)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if args.self_test:
        return self_test()
    if args.summary is None:
        parser.error("summary CSV is required")
    found = find_crossovers(read_summary(args.summary))
    if args.output:
        with args.output.open("w", newline="", encoding="utf-8") as handle:
            write_rows(found, handle)
    else:
        write_rows(found, sys.stdout)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
