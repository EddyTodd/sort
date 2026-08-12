#!/usr/bin/env python3
"""Select gallop thresholds on training trials and evaluate against linear merging held-out."""
from __future__ import annotations

import argparse
import csv
import math
import statistics
import sys
from collections import defaultdict
from pathlib import Path


def median(values: list[float]) -> float:
    return float(statistics.median(values)) if values else math.nan


def read_rows(path: Path) -> list[dict[str, str]]:
    with path.open(newline="", encoding="utf-8") as handle:
        rows = list(csv.DictReader(handle))
    required = {
        "treatment", "buffer_policy", "search_policy", "gallop_threshold",
        "pattern", "n", "trial", "input_hash", "ns", "verified",
    }
    if not rows or required - set(rows[0]):
        raise ValueError("missing merge-kernel benchmark columns")
    if any(row["verified"] != "1" for row in rows):
        raise ValueError("unverified merge-kernel row")
    return rows


def tune(rows: list[dict[str, str]]) -> list[dict[str, object]]:
    train: dict[tuple[str, str, int, str, int], list[float]] = defaultdict(list)
    heldout: dict[tuple[str, str, int, str, int], dict[tuple[int, str], float]] = defaultdict(dict)

    for row in rows:
        buffer_policy = row["buffer_policy"]
        pattern = row["pattern"]
        n = int(row["n"])
        trial = int(row["trial"])
        identity = (trial, row["input_hash"])
        search_policy = row["search_policy"]
        threshold = int(row["gallop_threshold"])
        elapsed = float(row["ns"])
        key = (buffer_policy, pattern, n, search_policy, threshold)
        if trial % 3 == 2:
            heldout[key][identity] = elapsed
        else:
            train[key].append(elapsed)

    cells = sorted({(row["buffer_policy"], row["pattern"], int(row["n"])) for row in rows})
    output: list[dict[str, object]] = []
    for buffer_policy, pattern, n in cells:
        candidates: list[tuple[float, int]] = []
        for (candidate_buffer, candidate_pattern, candidate_n, search_policy, threshold), values in train.items():
            if (candidate_buffer, candidate_pattern, candidate_n) != (buffer_policy, pattern, n):
                continue
            if search_policy == "gallop":
                candidates.append((median(values), threshold))

        linear_train = train.get((buffer_policy, pattern, n, "linear", 0), [])
        if not candidates or not linear_train:
            continue

        gallop_train_median, threshold = min(candidates)
        gallop = heldout.get((buffer_policy, pattern, n, "gallop", threshold), {})
        linear = heldout.get((buffer_policy, pattern, n, "linear", 0), {})
        identities = sorted(set(gallop) & set(linear))
        ratios = [linear[identity] / gallop[identity] for identity in identities if gallop[identity] > 0]
        wins = sum(gallop[identity] < linear[identity] for identity in identities)
        ties = sum(gallop[identity] == linear[identity] for identity in identities)

        output.append({
            "buffer_policy": buffer_policy,
            "pattern": pattern,
            "n": n,
            "selected_gallop_threshold": threshold,
            "selected_gallop_train_median_ns": gallop_train_median,
            "linear_train_median_ns": median(linear_train),
            "heldout_pairs": len(ratios),
            "heldout_gallop_median_ns": median([gallop[identity] for identity in identities]),
            "heldout_linear_median_ns": median([linear[identity] for identity in identities]),
            "heldout_median_paired_speedup_vs_linear": median(ratios),
            "heldout_win_rate": wins / len(identities) if identities else math.nan,
            "heldout_ties": ties,
        })
    return output


def write_rows(rows: list[dict[str, object]], handle) -> None:
    fields = list(rows[0]) if rows else ["buffer_policy"]
    writer = csv.DictWriter(handle, fieldnames=fields)
    writer.writeheader()
    writer.writerows(rows)


def self_test() -> int:
    rows: list[dict[str, str]] = []
    for trial in range(18):
        for search_policy, threshold, ns in (
            ("linear", 0, 100),
            ("gallop", 4, 85),
            ("gallop", 7, 70),
            ("gallop", 12, 90),
        ):
            rows.append({
                "treatment": f"smaller_{search_policy}_{threshold}",
                "buffer_policy": "smaller",
                "search_policy": search_policy,
                "gallop_threshold": str(threshold),
                "pattern": "run_long_short",
                "n": "4096",
                "trial": str(trial),
                "input_hash": str(trial),
                "ns": str(ns + trial % 2),
                "verified": "1",
            })
    result = tune(rows)[0]
    assert result["selected_gallop_threshold"] == 7
    assert result["heldout_median_paired_speedup_vs_linear"] > 1.3
    assert result["heldout_pairs"] == 6
    print("PASS: held-out gallop-threshold tuning")
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
        parser.error("input CSV required")
    rows = tune(read_rows(args.input))
    if args.output:
        with args.output.open("w", newline="", encoding="utf-8") as handle:
            write_rows(rows, handle)
    else:
        write_rows(rows, sys.stdout)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
