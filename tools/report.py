#!/usr/bin/env python3
"""Build a conservative research-status report from campaign and claim metadata."""
from __future__ import annotations

import argparse
import json
import tempfile
from pathlib import Path


def report(campaign_root: Path, registry: dict) -> str:
    manifests = sorted(campaign_root.rglob("campaign-manifest.json")) if campaign_root.exists() else []
    completed = skipped = failed = 0
    campaigns: list[tuple[str, int, int, int]] = []
    for path in manifests:
        data = json.loads(path.read_text(encoding="utf-8"))
        statuses = [run.get("status", "") for run in data.get("runs", [])]
        c = sum(s == "completed" for s in statuses)
        s = sum(s == "skipped-valid" for s in statuses)
        f = sum(x.startswith("failed:") for x in statuses)
        completed += c
        skipped += s
        failed += f
        campaigns.append((data.get("campaign_id", path.parent.name), c, s, f))
    claims = registry.get("claims", [])
    status_counts: dict[str, int] = {}
    for claim in claims:
        status = claim.get("status", "unknown")
        status_counts[status] = status_counts.get(status, 0) + 1
    lines = ["# Sorting research status", "", "This report describes evidence state; it does not infer performance conclusions from missing or exploratory data.", "",
             "## Campaign execution", "", f"Completed runs: **{completed}**  ", f"Resumed valid runs: **{skipped}**  ", f"Failed runs: **{failed}**", ""]
    if campaigns:
        lines += ["| Campaign | Completed | Resumed | Failed |", "|---|---:|---:|---:|"]
        lines += [f"| {name} | {c} | {s} | {f} |" for name, c, s, f in campaigns]
    else:
        lines.append("No campaign manifests found.")
    lines += ["", "## Claim status", "", "| Status | Claims |", "|---|---:|"]
    for status, count in sorted(status_counts.items()):
        lines.append(f"| {status} | {count} |")
    lines += ["", "See `claims/registry.json` and the generated evidence ledger for claim-specific requirements and artifact links.", ""]
    return "\n".join(lines)


def self_test() -> int:
    registry = {"claims": [{"status": "untested"}, {"status": "exploratory"}]}
    with tempfile.TemporaryDirectory() as tmp:
        text = report(Path(tmp), registry)
        assert "No campaign manifests found" in text and "untested" in text
    print("PASS: conservative campaign/claim report rendering")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--campaign-root", type=Path, default=Path("results/campaigns"))
    parser.add_argument("--registry", type=Path, default=Path("claims/registry.json"))
    parser.add_argument("--output", type=Path)
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if args.self_test:
        return self_test()
    try:
        registry = json.loads(args.registry.read_text(encoding="utf-8"))
        text = report(args.campaign_root, registry)
    except (OSError, json.JSONDecodeError) as exc:
        print(f"error: {exc}")
        return 2
    if args.output:
        args.output.write_text(text, encoding="utf-8")
    else:
        print(text, end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
