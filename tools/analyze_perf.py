#!/usr/bin/env python3
"""Reduce sort_perf trials into robust per-cell hardware-counter summaries."""
from __future__ import annotations

import argparse
import csv
import math
import statistics
from collections import defaultdict
from pathlib import Path


def median(values: list[float]) -> float:
    return float(statistics.median(values)) if values else math.nan


def read_rows(path: Path) -> list[dict[str, str]]:
    availability = {
        "cycles": "cycles_available",
        "instructions": "instructions_available",
        "branches": "branches_available",
        "branch_misses": "branch_misses_available",
        "cache_references": "cache_references_available",
        "cache_misses": "cache_misses_available",
    }
    required = {"algorithm", "pattern", "n", "trial", "ns", "verified"} | set(availability) | set(availability.values())
    with path.open(newline="", encoding="utf-8") as handle:
        rows = list(csv.DictReader(handle))
    if not rows:
        raise ValueError("no perf rows")
    missing = required - set(rows[0])
    if missing:
        raise ValueError("missing perf columns: " + ", ".join(sorted(missing)))
    for row in rows:
        if row["verified"] != "1":
            raise ValueError("refusing unverified perf row")
    return rows


def available_values(rows: list[dict[str, str]], metric: str) -> list[float]:
    flag = metric + "_available"
    return [float(row[metric]) for row in rows if row[flag] == "1"]


def summarize(rows: list[dict[str, str]]) -> list[dict[str, object]]:
    grouped: dict[tuple[str, str, int], list[dict[str, str]]] = defaultdict(list)
    for row in rows:
        grouped[(row["algorithm"], row["pattern"], int(row["n"]))].append(row)

    output: list[dict[str, object]] = []
    for (algorithm, pattern, n), group in sorted(grouped.items()):
        cycles = available_values(group, "cycles")
        instructions = available_values(group, "instructions")
        branches = available_values(group, "branches")
        branch_misses = available_values(group, "branch_misses")
        cache_references = available_values(group, "cache_references")
        cache_misses = available_values(group, "cache_misses")

        paired_cpi = [
            float(row["cycles"]) / float(row["instructions"])
            for row in group
            if row["cycles_available"] == "1" and row["instructions_available"] == "1"
            and float(row["instructions"]) > 0
        ]
        paired_branch_miss_rate = [
            float(row["branch_misses"]) / float(row["branches"])
            for row in group
            if row["branch_misses_available"] == "1" and row["branches_available"] == "1"
            and float(row["branches"]) > 0
        ]
        paired_cache_miss_rate = [
            float(row["cache_misses"]) / float(row["cache_references"])
            for row in group
            if row["cache_misses_available"] == "1" and row["cache_references_available"] == "1"
            and float(row["cache_references"]) > 0
        ]
        output.append({
            "algorithm": algorithm,
            "pattern": pattern,
            "n": n,
            "samples": len(group),
            "median_ns": median([float(row["ns"]) for row in group]),
            "cycles_samples": len(cycles),
            "median_cycles": median(cycles),
            "instructions_samples": len(instructions),
            "median_instructions": median(instructions),
            "median_cpi": median(paired_cpi),
            "branches_samples": len(branches),
            "median_branches": median(branches),
            "median_branch_miss_rate": median(paired_branch_miss_rate),
            "cache_references_samples": len(cache_references),
            "median_cache_references": median(cache_references),
            "median_cache_miss_rate": median(paired_cache_miss_rate),
        })
    return output


def write(rows: list[dict[str, object]], output) -> None:
    fields = [
        "algorithm", "pattern", "n", "samples", "median_ns", "cycles_samples", "median_cycles",
        "instructions_samples", "median_instructions", "median_cpi", "branches_samples", "median_branches",
        "median_branch_miss_rate", "cache_references_samples", "median_cache_references", "median_cache_miss_rate",
    ]
    writer = csv.DictWriter(output, fieldnames=fields)
    writer.writeheader()
    writer.writerows(rows)


def self_test() -> int:
    row = {
        "algorithm": "a", "pattern": "random", "n": "10", "trial": "0", "ns": "100", "verified": "1",
        "cycles_available": "1", "cycles": "200", "instructions_available": "1", "instructions": "100",
        "branches_available": "1", "branches": "20", "branch_misses_available": "1", "branch_misses": "2",
        "cache_references_available": "1", "cache_references": "10", "cache_misses_available": "1", "cache_misses": "1",
    }
    summary = summarize([row])[0]
    assert summary["median_cpi"] == 2
    assert abs(float(summary["median_branch_miss_rate"]) - 0.1) < 1e-9
    print("PASS: independent hardware-counter reduction")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", nargs="?", type=Path)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if args.self_test:
        return self_test()
    if args.input is None:
        parser.error("input required")
    rows = summarize(read_rows(args.input))
    if args.output:
        with args.output.open("w", newline="", encoding="utf-8") as handle:
            write(rows, handle)
    else:
        import sys
        write(rows, sys.stdout)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
