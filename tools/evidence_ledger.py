#!/usr/bin/env python3
"""Render the research claim registry and attached evidence as Markdown or JSON."""
from __future__ import annotations

import argparse
import json
import tempfile
from pathlib import Path
from typing import Any

ALLOWED_STATUS = {"untested", "exploratory", "tier2-supported", "tier3-replicated", "rejected", "inconclusive"}


def load_registry(path: Path) -> dict[str, Any]:
    data = json.loads(path.read_text(encoding="utf-8"))
    if data.get("schema_version") != 1:
        raise ValueError("unsupported claim registry schema")
    claims = data.get("claims")
    if not isinstance(claims, list) or not claims:
        raise ValueError("claims must be a non-empty list")
    seen: set[str] = set()
    for claim in claims:
        cid = claim.get("id")
        if not isinstance(cid, str) or not cid:
            raise ValueError("claim id missing")
        if cid in seen:
            raise ValueError(f"duplicate claim id: {cid}")
        seen.add(cid)
        if claim.get("status") not in ALLOWED_STATUS:
            raise ValueError(f"{cid}: invalid status")
        for field in ("title", "hypothesis", "required_evidence"):
            if field not in claim:
                raise ValueError(f"{cid}: missing {field}")
    return data


def markdown(data: dict[str, Any]) -> str:
    lines = ["# Evidence ledger", "", "Generated from `claims/registry.json`. Status is evidence state, not confidence rhetoric.", ""]
    for claim in data["claims"]:
        lines += [f"## {claim['id']} — {claim['title']}", "",
                  f"**Status:** `{claim['status']}`", "", claim["hypothesis"], "",
                  "**Required evidence:**", ""]
        for item in claim["required_evidence"]:
            lines.append(f"- {item}")
        evidence = claim.get("evidence", [])
        lines += ["", "**Attached evidence:**", ""]
        if evidence:
            for item in evidence:
                lines.append(f"- `{item}`")
        else:
            lines.append("- None yet.")
        lines.append("")
    return "\n".join(lines).rstrip() + "\n"


def self_test() -> int:
    sample = {"schema_version": 1, "claims": [{
        "id": "H1", "title": "x", "hypothesis": "y", "status": "untested",
        "required_evidence": ["raw CSV"], "evidence": []}]}
    with tempfile.TemporaryDirectory() as tmp:
        path = Path(tmp) / "registry.json"
        path.write_text(json.dumps(sample), encoding="utf-8")
        loaded = load_registry(path)
        text = markdown(loaded)
        assert "## H1" in text and "None yet" in text
    print("PASS: claim registry validation and ledger rendering")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("registry", nargs="?", type=Path, default=Path("claims/registry.json"))
    parser.add_argument("--output", type=Path)
    parser.add_argument("--json", action="store_true", dest="as_json")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if args.self_test:
        return self_test()
    try:
        data = load_registry(args.registry)
    except (OSError, json.JSONDecodeError, ValueError) as exc:
        print(f"error: {exc}")
        return 2
    rendered = json.dumps(data, indent=2, sort_keys=True) + "\n" if args.as_json else markdown(data)
    if args.output:
        args.output.write_text(rendered, encoding="utf-8")
    else:
        print(rendered, end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
