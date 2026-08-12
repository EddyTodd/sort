#!/usr/bin/env python3
"""Run sort_lab and capture a reproducibility manifest beside the raw CSV."""
from __future__ import annotations

import argparse
import datetime as dt
import hashlib
import json
import os
import platform
import subprocess
import sys
from pathlib import Path


def command_output(command: list[str]) -> str | None:
    try:
        return subprocess.run(command, check=True, text=True, capture_output=True).stdout.strip()
    except (OSError, subprocess.CalledProcessError):
        return None


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("binary", type=Path)
    parser.add_argument("output_dir", type=Path)
    parser.add_argument("benchmark_args", nargs=argparse.REMAINDER,
                        help="arguments passed to sort_lab; prefix with --")
    args = parser.parse_args()
    binary = args.binary.resolve()
    if not binary.exists():
        parser.error(f"binary does not exist: {binary}")
    args.output_dir.mkdir(parents=True, exist_ok=True)
    raw_path = args.output_dir / "raw.csv"
    benchmark_args = list(args.benchmark_args)
    if benchmark_args and benchmark_args[0] == "--":
        benchmark_args = benchmark_args[1:]
    command = [str(binary), *benchmark_args]
    with raw_path.open("w", encoding="utf-8", newline="") as raw:
        completed = subprocess.run(command, stdout=raw, stderr=sys.stderr)
    if completed.returncode != 0:
        return completed.returncode
    digest = hashlib.sha256(raw_path.read_bytes()).hexdigest()
    binary_env = command_output([str(binary), "--environment"])
    manifest = {
        "schema_version": 1,
        "captured_at_utc": dt.datetime.now(dt.timezone.utc).isoformat(),
        "command": command,
        "raw_csv": raw_path.name,
        "raw_csv_sha256": digest,
        "benchmark_environment": json.loads(binary_env) if binary_env else None,
        "host": {
            "platform": platform.platform(),
            "system": platform.system(),
            "release": platform.release(),
            "machine": platform.machine(),
            "processor": platform.processor(),
            "logical_cpu_count": os.cpu_count(),
            "python": platform.python_version(),
        },
        "source": {
            "git_commit": command_output(["git", "rev-parse", "HEAD"]),
            "git_status_porcelain": command_output(["git", "status", "--porcelain"]),
        },
    }
    manifest_path = args.output_dir / "manifest.json"
    manifest_path.write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(f"wrote {raw_path} ({digest})", file=sys.stderr)
    print(f"wrote {manifest_path}", file=sys.stderr)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
