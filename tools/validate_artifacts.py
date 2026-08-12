#!/usr/bin/env python3
"""Validate sort-lab raw/manifests before evidence is admitted to a claim."""
from __future__ import annotations

import argparse
import csv
import hashlib
import json
import tempfile
from dataclasses import dataclass
from pathlib import Path


@dataclass
class Finding:
    path: str
    ok: bool
    message: str


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def validate_run(run_dir: Path) -> list[Finding]:
    findings: list[Finding] = []
    manifest_path = run_dir / "manifest.json"
    if not manifest_path.exists():
        return [Finding(str(run_dir), False, "missing manifest.json")]
    try:
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        return [Finding(str(manifest_path), False, f"invalid manifest JSON: {exc}")]
    raw_name = manifest.get("raw_csv", "raw.csv")
    raw_path = run_dir / raw_name
    if not raw_path.exists():
        return [Finding(str(raw_path), False, "missing raw CSV")]
    expected = manifest.get("raw_csv_sha256")
    actual = sha256(raw_path)
    findings.append(Finding(str(raw_path), isinstance(expected, str) and expected == actual,
                            "raw SHA-256 matches manifest" if expected == actual else "raw SHA-256 mismatch"))
    if not isinstance(manifest.get("binary_sha256"), str):
        findings.append(Finding(str(manifest_path), False, "missing binary_sha256"))
    else:
        findings.append(Finding(str(manifest_path), True, "binary SHA-256 recorded"))
    source = manifest.get("source") or {}
    if source.get("git_status_porcelain") not in ("", None):
        findings.append(Finding(str(manifest_path), False, "source tree was dirty"))
    else:
        findings.append(Finding(str(manifest_path), True, "source tree clean or not recorded"))
    try:
        with raw_path.open(newline="", encoding="utf-8") as handle:
            reader = csv.DictReader(handle)
            fields = set(reader.fieldnames or [])
            rows = list(reader)
    except (OSError, csv.Error) as exc:
        findings.append(Finding(str(raw_path), False, f"cannot parse CSV: {exc}"))
        return findings
    findings.append(Finding(str(raw_path), bool(rows), f"{len(rows)} data rows" if rows else "no data rows"))
    if "verified" in fields:
        failures = sum(row.get("verified") not in {"1", "true", "True"} for row in rows)
        findings.append(Finding(str(raw_path), failures == 0,
                                "all rows verified" if failures == 0 else f"{failures} unverified rows"))
    return findings


def discover_runs(root: Path) -> list[Path]:
    return sorted({path.parent for path in root.rglob("manifest.json") if path.name == "manifest.json"})


def self_test() -> int:
    with tempfile.TemporaryDirectory() as tmp:
        run = Path(tmp) / "run"
        run.mkdir()
        raw = run / "raw.csv"
        raw.write_text("algorithm,verified\na,1\n", encoding="utf-8")
        manifest = {
            "raw_csv": "raw.csv",
            "raw_csv_sha256": sha256(raw),
            "binary_sha256": "0" * 64,
            "source": {"git_status_porcelain": ""},
        }
        (run / "manifest.json").write_text(json.dumps(manifest), encoding="utf-8")
        findings = validate_run(run)
        assert findings and all(f.ok for f in findings)
        raw.write_text("algorithm,verified\na,0\n", encoding="utf-8")
        findings = validate_run(run)
        assert any(not f.ok for f in findings)
    print("PASS: artifact hashes, cleanliness, and row verification")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("root", nargs="?", type=Path)
    parser.add_argument("--json", action="store_true", dest="as_json")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if args.self_test:
        return self_test()
    if args.root is None:
        parser.error("artifact root is required")
    runs = discover_runs(args.root)
    if not runs:
        print("no run manifests found")
        return 2
    findings = [finding for run in runs for finding in validate_run(run)]
    if args.as_json:
        print(json.dumps([f.__dict__ for f in findings], indent=2))
    else:
        for finding in findings:
            print(f"{'PASS' if finding.ok else 'FAIL'}\t{finding.path}\t{finding.message}")
    return 0 if all(f.ok for f in findings) else 1


if __name__ == "__main__":
    raise SystemExit(main())
