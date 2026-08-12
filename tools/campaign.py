#!/usr/bin/env python3
"""Validate, plan, and execute reproducible sort-lab benchmark campaigns."""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import subprocess
import sys
import tempfile
from dataclasses import dataclass
from pathlib import Path
from typing import Any
import platform

SCHEMA_VERSION = 1


class CampaignError(RuntimeError):
    pass


@dataclass(frozen=True)
class Run:
    experiment_id: str
    repetition: int
    binary: str
    args: tuple[str, ...]
    platforms: tuple[str, ...]
    analyses: tuple[dict[str, Any], ...]

    @property
    def run_id(self) -> str:
        return f"{self.experiment_id}/rep-{self.repetition:03d}"


def canonical_json(value: Any) -> bytes:
    return (json.dumps(value, sort_keys=True, separators=(",", ":")) + "\n").encode()


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def load_spec(path: Path) -> dict[str, Any]:
    try:
        spec = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise CampaignError(f"cannot read campaign spec {path}: {exc}") from exc
    validate_spec(spec)
    return spec


def validate_spec(spec: dict[str, Any]) -> None:
    if not isinstance(spec, dict):
        raise CampaignError("campaign spec must be a JSON object")
    if spec.get("schema_version") != SCHEMA_VERSION:
        raise CampaignError(f"unsupported campaign schema_version: {spec.get('schema_version')!r}")
    campaign_id = spec.get("campaign_id")
    if not isinstance(campaign_id, str) or not campaign_id or "/" in campaign_id:
        raise CampaignError("campaign_id must be a non-empty path-safe string")
    experiments = spec.get("experiments")
    if not isinstance(experiments, list) or not experiments:
        raise CampaignError("experiments must be a non-empty list")
    seen: set[str] = set()
    for exp in experiments:
        if not isinstance(exp, dict):
            raise CampaignError("each experiment must be an object")
        exp_id = exp.get("id")
        if not isinstance(exp_id, str) or not exp_id or "/" in exp_id:
            raise CampaignError("experiment id must be a non-empty path-safe string")
        if exp_id in seen:
            raise CampaignError(f"duplicate experiment id: {exp_id}")
        seen.add(exp_id)
        if not isinstance(exp.get("binary"), str) or not exp["binary"]:
            raise CampaignError(f"{exp_id}: binary must be a non-empty string")
        args = exp.get("args", [])
        if not isinstance(args, list) or not all(isinstance(x, str) for x in args):
            raise CampaignError(f"{exp_id}: args must be a list of strings")
        reps = exp.get("repetitions", 1)
        if not isinstance(reps, int) or reps < 1:
            raise CampaignError(f"{exp_id}: repetitions must be a positive integer")
        purpose = exp.get("purpose")
        if purpose is not None and not isinstance(purpose, str):
            raise CampaignError(f"{exp_id}: purpose must be a string")
        platforms = exp.get("platforms", [])
        if not isinstance(platforms, list) or not all(isinstance(x, str) for x in platforms):
            raise CampaignError(f"{exp_id}: platforms must be a list of strings")
        analyses = exp.get("analyses", [])
        if not isinstance(analyses, list):
            raise CampaignError(f"{exp_id}: analyses must be a list")
        for analysis in analyses:
            if not isinstance(analysis, dict) or not isinstance(analysis.get("tool"), str):
                raise CampaignError(f"{exp_id}: each analysis needs a tool")
            if not isinstance(analysis.get("args", []), list) or not all(isinstance(x, str) for x in analysis.get("args", [])):
                raise CampaignError(f"{exp_id}: analysis args must be strings")


def plan_runs(spec: dict[str, Any]) -> list[Run]:
    runs: list[Run] = []
    for exp in spec["experiments"]:
        for repetition in range(1, exp.get("repetitions", 1) + 1):
            runs.append(Run(exp["id"], repetition, exp["binary"], tuple(exp.get("args", [])),
                            tuple(exp.get("platforms", [])), tuple(exp.get("analyses", []))))
    return runs


def raw_manifest_valid(run_dir: Path) -> bool:
    manifest_path = run_dir / "manifest.json"
    raw_path = run_dir / "raw.csv"
    if not manifest_path.exists() or not raw_path.exists():
        return False
    try:
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError):
        return False
    expected = manifest.get("raw_csv_sha256")
    if not isinstance(expected, str):
        return False
    return hashlib.sha256(raw_path.read_bytes()).hexdigest() == expected


def execute(spec_path: Path, spec: dict[str, Any], binary_dir: Path, output_root: Path,
            cpu: int | None, settle_ms: int, resume: bool, dry_run: bool) -> int:
    if settle_ms < 0:
        raise CampaignError("settle-ms must be nonnegative")
    campaign_dir = output_root / spec["campaign_id"]
    campaign_dir.mkdir(parents=True, exist_ok=True)
    spec_hash = sha256_bytes(canonical_json(spec))
    runs = plan_runs(spec)
    state: dict[str, Any] = {
        "schema_version": 1,
        "campaign_id": spec["campaign_id"],
        "campaign_spec": str(spec_path),
        "campaign_spec_sha256": spec_hash,
        "runs": [],
    }
    wrapper = Path(__file__).resolve().with_name("run_experiment.py")
    for run in runs:
        run_dir = campaign_dir / run.experiment_id / f"rep-{run.repetition:03d}"
        binary = (binary_dir / run.binary).resolve()
        status = "planned"
        current_platform = platform.system().lower()
        if run.platforms and current_platform not in {x.lower() for x in run.platforms}:
            status = "skipped-platform"
        elif resume and raw_manifest_valid(run_dir):
            status = "skipped-valid"
        elif dry_run:
            status = "dry-run"
        else:
            if not binary.exists():
                raise CampaignError(f"missing benchmark binary: {binary}")
            command = [sys.executable, str(wrapper)]
            if cpu is not None:
                command += ["--cpu", str(cpu)]
            if settle_ms:
                command += ["--settle-ms", str(settle_ms)]
            command += [str(binary), str(run_dir), "--", *run.args]
            completed = subprocess.run(command)
            if completed.returncode != 0:
                status = f"failed:{completed.returncode}"
                state["runs"].append({"run_id": run.run_id, "status": status})
                (campaign_dir / "campaign-manifest.json").write_text(
                    json.dumps(state, indent=2, sort_keys=True) + "\n", encoding="utf-8")
                return completed.returncode
            if not raw_manifest_valid(run_dir):
                raise CampaignError(f"run completed without valid artifact hashes: {run.run_id}")
            status = "completed"
        if status in {"completed", "skipped-valid"} and run.analyses:
            for analysis in run.analyses:
                tool = Path(__file__).resolve().with_name(analysis["tool"])
                mapping = {"raw": str(run_dir / "raw.csv"), "run_dir": str(run_dir)}
                analysis_args = [arg.format(**mapping) for arg in analysis.get("args", [])]
                result = subprocess.run([sys.executable, str(tool), *analysis_args])
                if result.returncode != 0:
                    status = f"analysis-failed:{analysis['tool']}:{result.returncode}"
                    break
        state["runs"].append({
            "run_id": run.run_id,
            "experiment_id": run.experiment_id,
            "repetition": run.repetition,
            "binary": run.binary,
            "args": list(run.args),
            "platforms": list(run.platforms),
            "status": status,
        })
    (campaign_dir / "campaign-manifest.json").write_text(
        json.dumps(state, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    return 0


def self_test() -> int:
    good = {
        "schema_version": 1,
        "campaign_id": "test",
        "experiments": [
            {"id": "scalar", "binary": "sort_lab", "args": ["--trials", "3"], "repetitions": 2}
        ],
    }
    validate_spec(good)
    runs = plan_runs(good)
    assert [r.run_id for r in runs] == ["scalar/rep-001", "scalar/rep-002"]
    bad = dict(good)
    bad["experiments"] = [good["experiments"][0], good["experiments"][0]]
    try:
        validate_spec(bad)
    except CampaignError:
        pass
    else:
        raise AssertionError("duplicate experiment ids must fail")
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        raw = root / "raw.csv"
        raw.write_text("verified\n1\n", encoding="utf-8")
        digest = hashlib.sha256(raw.read_bytes()).hexdigest()
        (root / "manifest.json").write_text(json.dumps({"raw_csv_sha256": digest}), encoding="utf-8")
        assert raw_manifest_valid(root)
        raw.write_text("verified\n0\n", encoding="utf-8")
        assert not raw_manifest_valid(root)
    print("PASS: campaign validation, planning, resume integrity")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("spec", nargs="?", type=Path)
    parser.add_argument("--binary-dir", type=Path, default=Path("build"))
    parser.add_argument("--output-root", type=Path, default=Path("results/campaigns"))
    parser.add_argument("--cpu", type=int)
    parser.add_argument("--settle-ms", type=int, default=0)
    parser.add_argument("--resume", action="store_true")
    parser.add_argument("--dry-run", action="store_true")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if args.self_test:
        return self_test()
    if args.spec is None:
        parser.error("campaign spec is required")
    try:
        spec = load_spec(args.spec)
        return execute(args.spec, spec, args.binary_dir, args.output_root,
                       args.cpu, args.settle_ms, args.resume, args.dry_run)
    except CampaignError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
