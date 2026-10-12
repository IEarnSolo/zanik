#!/usr/bin/env python3
"""Validate Zanik release identity and extract deterministic release notes."""

from __future__ import annotations

import argparse
import re
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
VERSION_RE = re.compile(r"[0-9]+\.[0-9]+\.[0-9]+")
TAG_RE = re.compile(r"v([0-9]+\.[0-9]+\.[0-9]+)")


def read_version(path: Path) -> str:
    version = path.read_text(encoding="utf-8").strip()
    if not VERSION_RE.fullmatch(version):
        raise ValueError(f"invalid VERSION value: {version!r}")
    return version


def release_notes(changelog: Path, version: str) -> str:
    text = changelog.read_text(encoding="utf-8")
    match = re.search(
        rf"(?ms)^## \[{re.escape(version)}\](?: - [^\n]+)?\n(.*?)(?=^## \[|\Z)", text
    )
    if not match:
        raise ValueError(f"CHANGELOG.md has no release section for {version}")
    notes = match.group(1).strip()
    if not notes:
        raise ValueError(f"CHANGELOG.md release section for {version} is empty")
    return notes + "\n"


def require_clean(root: Path) -> None:
    result = subprocess.run(
        ["git", "status", "--porcelain", "--untracked-files=no"],
        cwd=root,
        text=True,
        capture_output=True,
        check=True,
    )
    if result.stdout.strip():
        raise ValueError("tracked release inputs are dirty")


def validate(tag: str, version: str) -> None:
    match = TAG_RE.fullmatch(tag)
    if not match:
        raise ValueError(f"release tag must match vMAJOR.MINOR.PATCH: {tag!r}")
    if match.group(1) != version:
        raise ValueError(f"release tag {tag} does not match VERSION {version}")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--tag", required=True)
    parser.add_argument("--version-file", type=Path, default=ROOT / "VERSION")
    parser.add_argument("--changelog", type=Path, default=ROOT / "CHANGELOG.md")
    parser.add_argument("--notes-output", type=Path)
    parser.add_argument("--require-clean", action="store_true")
    args = parser.parse_args()
    try:
        version = read_version(args.version_file)
        validate(args.tag, version)
        notes = release_notes(args.changelog, version)
        if args.require_clean:
            require_clean(ROOT)
        if args.notes_output:
            args.notes_output.parent.mkdir(parents=True, exist_ok=True)
            args.notes_output.write_text(notes, encoding="utf-8")
        print(f"validated release {args.tag} for VERSION {version}")
        return 0
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        print(f"release validation failed: {error}", file=__import__("sys").stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
