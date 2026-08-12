#!/usr/bin/env python3
"""Evaluate a feature-based sorting portfolio on held-out benchmark trials.

The model never uses benchmark pattern labels as runtime features.  It fits one
small ridge-regression cost model per candidate algorithm on training trials,
then pays the measured feature-extraction cost when evaluating the selector on
held-out trials.
"""
from __future__ import annotations

import argparse
import csv
import math
import statistics
from collections import defaultdict
from dataclasses import dataclass
from pathlib import Path

DEFAULT_ALGORITHMS = ["intro", "natural_merge", "quick_3way", "radix_lsd_11", "std_sort"]


@dataclass(frozen=True)
class Observation:
    algorithm: str
    pattern: str
    n: int
    trial: int
    input_hash: str
    ns: float
    feature_ns: float
    inversion_rate: float
    duplicate_fraction: float
    range_bits: float


def design(row: Observation) -> list[float]:
    logn = math.log2(row.n + 1.0)
    inv = row.inversion_rate
    dup = row.duplicate_fraction
    rng = row.range_bits / 64.0
    return [1.0, logn, inv, dup, rng, logn * inv, logn * dup]


def solve(matrix: list[list[float]], vector: list[float]) -> list[float]:
    n = len(vector)
    aug = [matrix[i][:] + [vector[i]] for i in range(n)]
    for col in range(n):
        pivot = max(range(col, n), key=lambda row: abs(aug[row][col]))
        if abs(aug[pivot][col]) < 1e-12:
            aug[pivot][col] += 1e-9
        aug[col], aug[pivot] = aug[pivot], aug[col]
        scale = aug[col][col]
        for j in range(col, n + 1):
            aug[col][j] /= scale
        for row in range(n):
            if row == col:
                continue
            factor = aug[row][col]
            for j in range(col, n + 1):
                aug[row][j] -= factor * aug[col][j]
    return [aug[i][n] for i in range(n)]


def fit(rows: list[Observation], ridge: float = 1e-6) -> list[float]:
    width = len(design(rows[0]))
    xtx = [[0.0] * width for _ in range(width)]
    xty = [0.0] * width
    for row in rows:
        x = design(row)
        y = math.log(max(row.ns, 1.0))
        for i in range(width):
            xty[i] += x[i] * y
            for j in range(width):
                xtx[i][j] += x[i] * x[j]
    for i in range(width):
        if i != 0:
            xtx[i][i] += ridge
    return solve(xtx, xty)


def predict(coefficients: list[float], row: Observation) -> float:
    return sum(c * x for c, x in zip(coefficients, design(row)))


def read_rows(path: Path, candidates: set[str]) -> list[Observation]:
    required = {
        "algorithm", "pattern", "n", "trial", "input_hash", "ns", "verified",
        "feature_ns", "sample_inversion_rate", "sample_duplicate_fraction",
        "sample_range_bits",
    }
    rows: list[Observation] = []
    with path.open(newline="", encoding="utf-8") as handle:
        reader = csv.DictReader(handle)
        missing = required - set(reader.fieldnames or ())
        if missing:
            raise ValueError("missing columns: " + ", ".join(sorted(missing)))
        for raw in reader:
            if raw["algorithm"] not in candidates:
                continue
            if raw["verified"] != "1":
                raise ValueError("refusing unverified benchmark rows")
            rows.append(Observation(
                algorithm=raw["algorithm"], pattern=raw["pattern"], n=int(raw["n"]),
                trial=int(raw["trial"]), input_hash=raw["input_hash"], ns=float(raw["ns"]),
                feature_ns=float(raw["feature_ns"]),
                inversion_rate=float(raw["sample_inversion_rate"]),
                duplicate_fraction=float(raw["sample_duplicate_fraction"]),
                range_bits=float(raw["sample_range_bits"]),
            ))
    return rows


def evaluate(rows: list[Observation], algorithms: list[str]) -> dict[str, object]:
    by_instance: dict[tuple[str, int, int, str], dict[str, Observation]] = defaultdict(dict)
    for row in rows:
        by_instance[(row.pattern, row.n, row.trial, row.input_hash)][row.algorithm] = row

    complete = [group for group in by_instance.values() if all(a in group for a in algorithms)]
    training = [group for group in complete if next(iter(group.values())).trial % 3 != 2]
    heldout = [group for group in complete if next(iter(group.values())).trial % 3 == 2]
    if not training or not heldout:
        raise ValueError("need both training and held-out instances")

    models: dict[str, list[float]] = {}
    for algorithm in algorithms:
        alg_rows = [group[algorithm] for group in training]
        models[algorithm] = fit(alg_rows)

    training_medians = {
        algorithm: statistics.median(group[algorithm].ns for group in training)
        for algorithm in algorithms
    }
    best_single = min(algorithms, key=training_medians.get)

    selector_costs: list[float] = []
    single_costs: list[float] = []
    oracle_costs: list[float] = []
    choices: dict[str, int] = defaultdict(int)
    for group in heldout:
        exemplar = group[algorithms[0]]
        chosen = min(algorithms, key=lambda algorithm: predict(models[algorithm], group[algorithm]))
        choices[chosen] += 1
        selector_costs.append(group[chosen].ns + exemplar.feature_ns)
        single_costs.append(group[best_single].ns)
        oracle_costs.append(min(group[algorithm].ns for algorithm in algorithms))

    selector = statistics.median(selector_costs)
    single = statistics.median(single_costs)
    oracle = statistics.median(oracle_costs)
    return {
        "training_instances": len(training),
        "heldout_instances": len(heldout),
        "best_single": best_single,
        "heldout_best_single_median_ns": single,
        "heldout_portfolio_median_ns_including_features": selector,
        "portfolio_speedup_vs_best_single": single / selector if selector > 0 else math.nan,
        "heldout_oracle_median_ns": oracle,
        "portfolio_slowdown_vs_oracle": selector / oracle if oracle > 0 else math.nan,
        "selection_counts": dict(sorted(choices.items())),
    }


def self_test() -> int:
    rows: list[Observation] = []
    for trial in range(12):
        for n in (16, 1024):
            for algorithm in ("a", "b"):
                ns = (20 if n == 16 else 200) if algorithm == "a" else (40 if n == 16 else 80)
                rows.append(Observation(algorithm, "synthetic", n, trial, f"{n}-{trial}", ns, 1, 0.5, 0, 32))
    result = evaluate(rows, ["a", "b"])
    assert result["heldout_instances"] > 0
    assert result["heldout_portfolio_median_ns_including_features"] <= result["heldout_best_single_median_ns"]
    print("PASS: held-out feature-based portfolio evaluation")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", nargs="?", type=Path)
    parser.add_argument("--algorithms", default=",".join(DEFAULT_ALGORITHMS))
    parser.add_argument("--output", type=Path)
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if args.self_test:
        return self_test()
    if args.input is None:
        parser.error("input CSV is required")
    algorithms = [item for item in args.algorithms.split(",") if item]
    rows = read_rows(args.input, set(algorithms))
    result = evaluate(rows, algorithms)
    fields = list(result.keys())
    output = args.output.open("w", newline="", encoding="utf-8") if args.output else None
    try:
        import sys
        handle = output if output is not None else sys.stdout
        writer = csv.DictWriter(handle, fieldnames=fields)
        writer.writeheader()
        row = dict(result)
        row["selection_counts"] = ";".join(f"{k}:{v}" for k, v in result["selection_counts"].items())
        writer.writerow(row)
    finally:
        if output is not None:
            output.close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
