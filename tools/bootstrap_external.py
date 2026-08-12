#!/usr/bin/env python3
"""Materialize provenance-pinned external sorting baselines without hiding network I/O in CMake."""
from __future__ import annotations
import argparse, json, shutil, subprocess, sys, tempfile
from pathlib import Path


def run(cmd: list[str], cwd: Path | None = None) -> str:
    return subprocess.run(cmd, cwd=cwd, check=True, text=True, capture_output=True).stdout.strip()


def load_manifest(path: Path) -> dict:
    data = json.loads(path.read_text(encoding="utf-8"))
    if data.get("schema_version") != 1 or not isinstance(data.get("baselines"), list):
        raise ValueError("unsupported external baseline manifest")
    ids: set[str] = set()
    for item in data["baselines"]:
        required = {"id", "repository", "commit", "license", "license_path", "required_paths", "include_dir", "adapter"}
        missing = required - set(item)
        if missing:
            raise ValueError(f"{item.get('id', '<unknown>')}: missing {sorted(missing)}")
        if item["id"] in ids:
            raise ValueError(f"duplicate baseline id: {item['id']}")
        ids.add(item["id"])
        if len(item["commit"]) != 40 or any(c not in "0123456789abcdef" for c in item["commit"]):
            raise ValueError(f"{item['id']}: commit must be a full lowercase SHA-1")
    return data


def inspect_checkout(root: Path, item: dict) -> list[str]:
    dest = root / item["id"]
    errors: list[str] = []
    if not (dest / ".git").is_dir():
        return [f"{item['id']}: checkout missing"]
    try:
        head = run(["git", "rev-parse", "HEAD"], dest)
        dirty = run(["git", "status", "--porcelain"], dest)
    except (OSError, subprocess.CalledProcessError) as exc:
        return [f"{item['id']}: git inspection failed: {exc}"]
    if head != item["commit"]:
        errors.append(f"{item['id']}: HEAD {head} != pinned {item['commit']}")
    if dirty:
        errors.append(f"{item['id']}: checkout is dirty")
    for rel in item["required_paths"]:
        if not (dest / rel).exists():
            errors.append(f"{item['id']}: required path missing: {rel}")
    return errors


def materialize(root: Path, item: dict) -> None:
    dest = root / item["id"]
    if not (dest / ".git").is_dir():
        if dest.exists():
            shutil.rmtree(dest)
        run(["git", "clone", "--filter=blob:none", "--no-checkout", item["repository"], str(dest)])
    run(["git", "fetch", "--depth", "1", "origin", item["commit"]], dest)
    run(["git", "checkout", "--detach", item["commit"]], dest)


def self_test() -> int:
    with tempfile.TemporaryDirectory() as td:
        base = Path(td)
        source = base / "source"
        source.mkdir()
        run(["git", "init"], source)
        run(["git", "config", "user.email", "test@example.com"], source)
        run(["git", "config", "user.name", "Sort Lab Test"], source)
        (source / "header.hpp").write_text("// fixture\n", encoding="utf-8")
        (source / "LICENSE").write_text("fixture\n", encoding="utf-8")
        run(["git", "add", "."], source)
        run(["git", "commit", "-m", "fixture"], source)
        commit = run(["git", "rev-parse", "HEAD"], source)
        manifest = base / "manifest.json"
        manifest.write_text(json.dumps({"schema_version": 1, "baselines": [{
            "id": "fixture", "repository": str(source), "commit": commit,
            "license": "test", "license_path": "LICENSE",
            "required_paths": ["header.hpp", "LICENSE"], "include_dir": ".", "adapter": "fixture"
        }]}), encoding="utf-8")
        item = load_manifest(manifest)["baselines"][0]
        root = base / "vendor"
        root.mkdir()
        materialize(root, item)
        assert inspect_checkout(root, item) == []
        (root / "fixture" / "header.hpp").write_text("dirty\n", encoding="utf-8")
        assert any("dirty" in e for e in inspect_checkout(root, item))
    print("PASS: pinned external bootstrap and dirty-check enforcement")
    return 0


def main() -> int:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--manifest", type=Path, default=Path("external/baselines.json"))
    p.add_argument("--vendor-dir", type=Path, default=Path("external/vendor"))
    p.add_argument("--check", action="store_true", help="verify only; never access the network")
    p.add_argument("--dry-run", action="store_true")
    p.add_argument("--self-test", action="store_true")
    args = p.parse_args()
    if args.self_test:
        return self_test()
    data = load_manifest(args.manifest)
    args.vendor_dir.mkdir(parents=True, exist_ok=True)
    if args.dry_run:
        for item in data["baselines"]:
            print(f"{item['id']}: {item['repository']} @ {item['commit']}")
        return 0
    if not args.check:
        for item in data["baselines"]:
            materialize(args.vendor_dir, item)
    failures = [e for item in data["baselines"] for e in inspect_checkout(args.vendor_dir, item)]
    if failures:
        for failure in failures:
            print(f"ERROR: {failure}", file=sys.stderr)
        return 2
    print(f"PASS: {len(data['baselines'])} external baselines match pinned provenance")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
