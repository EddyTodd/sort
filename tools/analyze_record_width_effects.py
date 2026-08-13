#!/usr/bin/env python3
"""Measure payload-width effects for adaptive-record treatments on paired key inputs."""
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
    treatment: str
    pattern: str
    n: int
    payload_words: int
    record_bytes: int
    trial: int
    key_hash: str
    ns: float
    explicit_moves: int
    explicit_bytes_moved: int
    temp_bytes_peak: int
    temp_bytes_copied: int
    stable: bool
    verified: bool


def read_rows(paths: Iterable[Path]) -> list[Row]:
    required = {
        "treatment", "pattern", "n", "payload_words", "record_bytes", "trial",
        "key_hash", "ns", "explicit_record_moves", "explicit_bytes_moved",
        "temp_bytes_requested_peak", "temp_bytes_copied", "stable_on_trial",
        "verified",
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
                    treatment=raw["treatment"], pattern=raw["pattern"],
                    n=int(raw["n"]), payload_words=int(raw["payload_words"]),
                    record_bytes=int(raw["record_bytes"]), trial=int(raw["trial"]),
                    key_hash=raw["key_hash"], ns=float(raw["ns"]),
                    explicit_moves=int(raw["explicit_record_moves"]),
                    explicit_bytes_moved=int(raw["explicit_bytes_moved"]),
                    temp_bytes_peak=int(raw["temp_bytes_requested_peak"]),
                    temp_bytes_copied=int(raw["temp_bytes_copied"]),
                    stable=raw["stable_on_trial"] == "1",
                    verified=raw["verified"] == "1",
                ))
    if not rows:
        raise ValueError("no adaptive-record rows")
    invalid = [row for row in rows if not row.verified or not row.stable]
    if invalid:
        raise ValueError(
            f"refusing to analyze {len(invalid)} unverified or unstable rows")
    return rows


def safe_ratio(numerator: float, denominator: float) -> float:
    if denominator == 0:
        return 1.0 if numerator == 0 else math.inf
    return numerator / denominator


def summarize(rows: Sequence[Row], baseline_words: int) -> list[dict[str, object]]:
    paired: dict[tuple[str, str, int, int, str], dict[int, Row]] = defaultdict(dict)
    for row in rows:
        paired[(row.treatment, row.pattern, row.n, row.trial,
                row.key_hash)][row.payload_words] = row

    grouped: dict[tuple[str, str, int, int, int], list[tuple[Row, Row]]] = defaultdict(list)
    for widths in paired.values():
        base = widths.get(baseline_words)
        if base is None:
            continue
        for words, row in widths.items():
            grouped[(row.treatment, row.pattern, row.n, words,
                     row.record_bytes)].append((base, row))

    output: list[dict[str, object]] = []
    for (treatment, pattern, n, words, record_bytes), pairs in sorted(grouped.items()):
        time_ratios = [safe_ratio(row.ns, base.ns) for base, row in pairs]
        move_ratios = [safe_ratio(row.explicit_moves, base.explicit_moves)
                       for base, row in pairs]
        byte_ratios = [safe_ratio(row.explicit_bytes_moved,
                                  base.explicit_bytes_moved)
                       for base, row in pairs]
        temp_peak_ratios = [safe_ratio(row.temp_bytes_peak, base.temp_bytes_peak)
                            for base, row in pairs]
        temp_copy_ratios = [safe_ratio(row.temp_bytes_copied,
                            base.temp_bytes_copied) for base, row in pairs]
        output.append({
            "treatment": treatment,
            "pattern": pattern,
            "n": n,
            "baseline_payload_words": baseline_words,
            "payload_words": words,
            "record_bytes": record_bytes,
            "paired_samples": len(pairs),
            "median_time_slowdown_vs_width0": float(statistics.median(time_ratios)),
            "median_record_move_ratio_vs_width0": float(statistics.median(move_ratios)),
            "median_explicit_byte_ratio_vs_width0": float(statistics.median(byte_ratios)),
            "median_temp_peak_byte_ratio_vs_width0": float(statistics.median(temp_peak_ratios)),
            "median_temp_copied_byte_ratio_vs_width0": float(statistics.median(temp_copy_ratios)),
        })
    return output


def write(rows: Sequence[dict[str, object]], handle) -> None:
    fields = list(rows[0].keys()) if rows else ["treatment"]
    writer = csv.DictWriter(handle, fieldnames=fields)
    writer.writeheader()
    writer.writerows(rows)


def self_test() -> int:
    rows = [
        Row("x", "random", 100, 0, 16, 0, "a", 100, 10, 160, 80, 160,
            True, True),
        Row("x", "random", 100, 7, 72, 0, "a", 250, 10, 720, 360, 720,
            True, True),
        Row("x", "random", 100, 0, 16, 1, "b", 120, 12, 192, 96, 192,
            True, True),
        Row("x", "random", 100, 7, 72, 1, "b", 300, 12, 864, 432, 864,
            True, True),
    ]
    result = summarize(rows, 0)
    wide = next(row for row in result if row["payload_words"] == 7)
    assert wide["paired_samples"] == 2
    assert wide["median_time_slowdown_vs_width0"] == 2.5
    assert wide["median_record_move_ratio_vs_width0"] == 1.0
    assert wide["median_explicit_byte_ratio_vs_width0"] == 4.5
    print("PASS: paired adaptive-record payload-width effects")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("inputs", nargs="*", type=Path)
    parser.add_argument("--baseline-payload-words", type=int, default=0)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if args.self_test:
        return self_test()
    if not args.inputs:
        parser.error("at least one input CSV is required")
    result = summarize(read_rows(args.inputs), args.baseline_payload_words)
    if args.output:
        with args.output.open("w", newline="", encoding="utf-8") as handle:
            write(result, handle)
    else:
        write(result, sys.stdout)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
