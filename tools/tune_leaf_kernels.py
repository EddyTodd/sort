#!/usr/bin/env python3
"""Jointly tune leaf kernel and cutoff on training trials, then evaluate held-out."""
from __future__ import annotations

import argparse
import csv
import math
import statistics
from collections import defaultdict
from pathlib import Path


def median(values: list[float]) -> float:
    return float(statistics.median(values)) if values else math.nan


def read(path: Path) -> list[tuple[str, str, int, str, int, int, str, float]]:
    with path.open(newline="", encoding="utf-8") as handle:
        rows = list(csv.DictReader(handle))
    required = {
        "family", "kernel", "cutoff", "pattern", "n", "trial",
        "input_hash", "ns", "verified",
    }
    if not rows or required - set(rows[0]):
        raise ValueError("missing leaf-hybrid benchmark columns")

    output = []
    for row in rows:
        if row["verified"] != "1":
            raise ValueError("unverified leaf-hybrid row")
        output.append((
            row["family"], row["kernel"], int(row["cutoff"]), row["pattern"],
            int(row["n"]), int(row["trial"]), row["input_hash"], float(row["ns"]),
        ))
    return output


def tune(rows: list[tuple[str, str, int, str, int, int, str, float]]) \
        -> list[dict[str, object]]:
    train: dict[tuple[str, str, int, str, int], list[float]] = defaultdict(list)
    heldout: dict[tuple[str, str, int, str, int], list[float]] = defaultdict(list)
    for family, kernel, cutoff, pattern, n, trial, input_hash, elapsed in rows:
        del input_hash
        target = heldout if trial % 3 == 2 else train
        target[(family, pattern, n, kernel, cutoff)].append(elapsed)

    cells = sorted({(family, pattern, n)
                    for family, _, _, pattern, n, _, _, _ in rows})
    output: list[dict[str, object]] = []
    for family, pattern, n in cells:
        candidates: list[tuple[float, str, int]] = []
        insertion_candidates: list[tuple[float, str, int]] = []
        for (candidate_family, candidate_pattern, candidate_n, kernel, cutoff), values in train.items():
            if (candidate_family, candidate_pattern, candidate_n) != (family, pattern, n):
                continue
            item = (median(values), kernel, cutoff)
            candidates.append(item)
            if kernel == "insertion":
                insertion_candidates.append(item)
        if not candidates or not insertion_candidates:
            continue

        train_ns, selected_kernel, selected_cutoff = min(candidates)
        insertion_train_ns, _, insertion_cutoff = min(insertion_candidates)
        selected = heldout.get(
            (family, pattern, n, selected_kernel, selected_cutoff), [])
        baseline = heldout.get(
            (family, pattern, n, "insertion", insertion_cutoff), [])
        selected_median = median(selected)
        baseline_median = median(baseline)

        output.append({
            "family": family,
            "pattern": pattern,
            "n": n,
            "selected_kernel": selected_kernel,
            "selected_cutoff": selected_cutoff,
            "train_median_ns": train_ns,
            "best_insertion_cutoff": insertion_cutoff,
            "best_insertion_train_median_ns": insertion_train_ns,
            "heldout_samples": len(selected),
            "heldout_selected_median_ns": selected_median,
            "heldout_insertion_median_ns": baseline_median,
            "heldout_speedup_vs_tuned_insertion":
                baseline_median / selected_median
                if selected and baseline and selected_median > 0 else math.nan,
            "network_selected": int(selected_kernel == "bitonic_network"),
        })
    return output


def write(rows: list[dict[str, object]], handle) -> None:
    fields = list(rows[0].keys()) if rows else ["family"]
    writer = csv.DictWriter(handle, fieldnames=fields)
    writer.writeheader()
    writer.writerows(rows)


def self_test() -> int:
    rows = []
    for trial in range(12):
        for kernel, cutoff, elapsed in (
            ("insertion", 8, 100),
            ("insertion", 16, 90),
            ("bitonic_network", 8, 70),
            ("bitonic_network", 16, 75),
            ("binary_insertion", 8, 95),
        ):
            rows.append(("quick", kernel, cutoff, "random", 1024, trial,
                         str(trial), elapsed + trial % 2))
    result = tune(rows)
    assert len(result) == 1
    assert result[0]["selected_kernel"] == "bitonic_network"
    assert result[0]["best_insertion_cutoff"] == 16
    assert result[0]["heldout_speedup_vs_tuned_insertion"] > 1.2
    print("PASS: held-out joint leaf-kernel/cutoff tuning")
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
    rows = tune(read(args.input))
    if args.output:
        with args.output.open("w", newline="", encoding="utf-8") as handle:
            write(rows, handle)
    else:
        import sys
        write(rows, sys.stdout)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
