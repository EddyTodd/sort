#!/usr/bin/env python3
"""Tune adaptive-record gallop thresholds on training trials and evaluate held-out."""
from __future__ import annotations

import argparse
import csv
import math
import statistics
import sys
from collections import defaultdict
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable, Sequence


@dataclass(frozen=True)
class Row:
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
    stable: bool
    verified: bool


def read_rows(paths: Iterable[Path]) -> list[Row]:
    required = {
        "merge_policy", "minrun_policy", "buffer_policy", "search_policy",
        "gallop_threshold", "pattern", "n", "payload_words", "record_bytes",
        "trial", "input_hash", "ns", "stable_on_trial", "verified",
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
                    merge_policy=raw["merge_policy"],
                    minrun_policy=raw["minrun_policy"],
                    buffer_policy=raw["buffer_policy"],
                    search_policy=raw["search_policy"],
                    gallop_threshold=int(raw["gallop_threshold"]),
                    pattern=raw["pattern"], n=int(raw["n"]),
                    payload_words=int(raw["payload_words"]),
                    record_bytes=int(raw["record_bytes"]), trial=int(raw["trial"]),
                    input_hash=raw["input_hash"], ns=float(raw["ns"]),
                    stable=raw["stable_on_trial"] == "1",
                    verified=raw["verified"] == "1",
                ))
    if not rows:
        raise ValueError("no adaptive-record rows")
    invalid = [row for row in rows if not row.verified or not row.stable]
    if invalid:
        raise ValueError(
            f"refusing to tune on {len(invalid)} unverified or unstable rows")
    return rows


def tune(rows: Sequence[Row]) -> list[dict[str, object]]:
    train: dict[tuple[str, str, str, str, int, int, int], list[float]] = defaultdict(list)
    heldout_linear: dict[tuple[str, str, str, str, int, int, int, str], Row] = {}
    heldout_gallop: dict[
        tuple[str, str, str, str, int, int, int, int, str], Row
    ] = {}
    record_bytes_by_cell: dict[tuple[str, str, str, str, int, int], int] = {}

    for row in rows:
        cell = (row.merge_policy, row.minrun_policy, row.buffer_policy,
                row.pattern, row.n, row.payload_words)
        record_bytes_by_cell[cell] = row.record_bytes
        if row.trial % 3 != 2:
            if row.search_policy == "gallop":
                train[cell + (row.gallop_threshold,)].append(row.ns)
            continue
        identity = cell + (row.trial, row.input_hash)
        if row.search_policy == "linear":
            heldout_linear[identity] = row
        elif row.search_policy == "gallop":
            heldout_gallop[cell + (row.gallop_threshold, row.trial,
                                    row.input_hash)] = row

    cells = sorted({key[:-1] for key in train})
    output: list[dict[str, object]] = []
    for cell in cells:
        merge_policy, minrun_policy, buffer_policy, pattern, n, payload_words = cell
        candidates = [
            (float(statistics.median(values)), key[-1])
            for key, values in train.items() if key[:-1] == cell
        ]
        if not candidates:
            continue
        train_median, selected_threshold = min(candidates)

        ratios: list[float] = []
        gallop_ns: list[float] = []
        linear_ns: list[float] = []
        prefix = cell + (selected_threshold,)
        for key, row in heldout_gallop.items():
            if key[:7] != prefix:
                continue
            identity = cell + (row.trial, row.input_hash)
            base = heldout_linear.get(identity)
            if base is None or row.ns <= 0:
                continue
            ratios.append(base.ns / row.ns)
            gallop_ns.append(row.ns)
            linear_ns.append(base.ns)

        output.append({
            "merge_policy": merge_policy,
            "minrun_policy": minrun_policy,
            "buffer_policy": buffer_policy,
            "pattern": pattern,
            "n": n,
            "payload_words": payload_words,
            "record_bytes": record_bytes_by_cell[cell],
            "selected_gallop_threshold": selected_threshold,
            "training_selected_median_ns": train_median,
            "heldout_pairs": len(ratios),
            "heldout_gallop_median_ns": (float(statistics.median(gallop_ns))
                                         if gallop_ns else math.nan),
            "heldout_linear_median_ns": (float(statistics.median(linear_ns))
                                         if linear_ns else math.nan),
            "heldout_median_speedup_vs_linear": (float(statistics.median(ratios))
                                                  if ratios else math.nan),
            "heldout_win_rate": (sum(value > 1.0 for value in ratios) +
                                 0.5 * sum(value == 1.0 for value in ratios)) /
                                 len(ratios) if ratios else math.nan,
        })
    return output


def write(rows: Sequence[dict[str, object]], handle) -> None:
    fields = list(rows[0].keys()) if rows else ["merge_policy"]
    writer = csv.DictWriter(handle, fieldnames=fields)
    writer.writeheader()
    writer.writerows(rows)


def self_test() -> int:
    rows: list[Row] = []
    for trial in range(12):
        identity = f"h{trial}"
        rows.append(Row("powersort", "balanced", "smaller", "linear", 0,
                        "runs", 1024, 7, 72, trial, identity, 100 + trial % 2,
                        True, True))
        rows.append(Row("powersort", "balanced", "smaller", "gallop", 4,
                        "runs", 1024, 7, 72, trial, identity, 70 + trial % 2,
                        True, True))
        rows.append(Row("powersort", "balanced", "smaller", "gallop", 7,
                        "runs", 1024, 7, 72, trial, identity, 80 + trial % 2,
                        True, True))
    result = tune(rows)
    assert len(result) == 1
    row = result[0]
    assert row["selected_gallop_threshold"] == 4
    assert row["heldout_pairs"] == 4
    assert row["heldout_median_speedup_vs_linear"] > 1.3
    print("PASS: held-out adaptive-record gallop threshold tuning")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("inputs", nargs="*", type=Path)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if args.self_test:
        return self_test()
    if not args.inputs:
        parser.error("at least one input CSV is required")
    result = tune(read_rows(args.inputs))
    if args.output:
        with args.output.open("w", newline="", encoding="utf-8") as handle:
            write(result, handle)
    else:
        write(result, sys.stdout)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
