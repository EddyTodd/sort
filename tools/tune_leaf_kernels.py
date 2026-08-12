#!/usr/bin/env python3
"""Jointly tune leaf kernel and cutoff on training trials, then evaluate held-out."""
from __future__ import annotations

import argparse
import csv
import math
import statistics
from collections import defaultdict
from pathlib import Path

Config = tuple[str, str, int]
Instance = tuple[str, str, int, int, str]


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
    heldout_instances: dict[Instance, dict[Config, float]] = defaultdict(dict)

    for family, kernel, cutoff, pattern, n, trial, input_hash, elapsed in rows:
        if trial % 3 == 2:
            heldout_instances[(family, pattern, n, trial, input_hash)][
                (family, kernel, cutoff)] = elapsed
        else:
            train[(family, pattern, n, kernel, cutoff)].append(elapsed)

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
        selected_config = (family, selected_kernel, selected_cutoff)
        baseline_config = (family, "insertion", insertion_cutoff)

        selected_values: list[float] = []
        baseline_values: list[float] = []
        paired_speedups: list[float] = []
        for (instance_family, instance_pattern, instance_n, _, _), peers in heldout_instances.items():
            if (instance_family, instance_pattern, instance_n) != (family, pattern, n):
                continue
            if selected_config not in peers or baseline_config not in peers:
                continue
            selected_ns = peers[selected_config]
            baseline_ns = peers[baseline_config]
            selected_values.append(selected_ns)
            baseline_values.append(baseline_ns)
            if selected_ns > 0:
                paired_speedups.append(baseline_ns / selected_ns)

        output.append({
            "family": family,
            "pattern": pattern,
            "n": n,
            "selected_kernel": selected_kernel,
            "selected_cutoff": selected_cutoff,
            "train_median_ns": train_ns,
            "best_insertion_cutoff": insertion_cutoff,
            "best_insertion_train_median_ns": insertion_train_ns,
            "heldout_samples": len(paired_speedups),
            "heldout_selected_median_ns": median(selected_values),
            "heldout_insertion_median_ns": median(baseline_values),
            "heldout_paired_median_speedup_vs_tuned_insertion": median(paired_speedups),
            "heldout_win_rate_vs_tuned_insertion":
                sum(value > 1 for value in paired_speedups) / len(paired_speedups)
                if paired_speedups else math.nan,
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
    assert result[0]["heldout_paired_median_speedup_vs_tuned_insertion"] > 1.2
    assert result[0]["heldout_win_rate_vs_tuned_insertion"] == 1
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
