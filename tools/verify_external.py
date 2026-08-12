#!/usr/bin/env python3
"""Emit machine-readable provenance for materialized external baselines."""
from __future__ import annotations
import argparse, hashlib, json, subprocess, sys
from pathlib import Path
from bootstrap_external import load_manifest, inspect_checkout


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for block in iter(lambda: f.read(1 << 20), b""):
            h.update(block)
    return h.hexdigest()


def git_output(repo: Path, *args: str) -> str:
    return subprocess.run(["git", *args], cwd=repo, check=True, text=True, capture_output=True).stdout.strip()


def collect(manifest: Path, vendor: Path) -> dict:
    data = load_manifest(manifest)
    records = []
    for item in data["baselines"]:
        errors = inspect_checkout(vendor, item)
        dest = vendor / item["id"]
        record = {k: item[k] for k in ("id", "repository", "commit", "license", "license_path", "adapter")}
        record["valid"] = not errors
        record["errors"] = errors
        if not errors:
            license_file = dest / item["license_path"]
            record["license_sha256"] = sha256(license_file)
            record["tree_sha"] = git_output(dest, "rev-parse", "HEAD^{tree}")
        records.append(record)
    return {"schema_version": 1, "baselines": records, "all_valid": all(r["valid"] for r in records)}


def main() -> int:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--manifest", type=Path, default=Path("external/baselines.json"))
    p.add_argument("--vendor-dir", type=Path, default=Path("external/vendor"))
    p.add_argument("--output", type=Path)
    args = p.parse_args()
    report = collect(args.manifest, args.vendor_dir)
    text = json.dumps(report, indent=2, sort_keys=True) + "\n"
    if args.output:
        args.output.write_text(text, encoding="utf-8")
    else:
        sys.stdout.write(text)
    return 0 if report["all_valid"] else 2


if __name__ == "__main__":
    raise SystemExit(main())
