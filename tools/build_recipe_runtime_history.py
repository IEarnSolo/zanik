#!/usr/bin/env python3
"""Merge private runtime recipe catalogs into the additive-compatibility history."""
from __future__ import annotations

import argparse
import re
from pathlib import Path


def read_catalog(path: Path) -> tuple[str, list[str]]:
    lines = path.read_text(encoding="utf-8").splitlines()
    match = re.search(r"fingerprint=([0-9a-f]{64})", lines[0] if lines else "")
    ids = sorted({line.split("\t", 1)[0] for line in lines[1:] if line})
    if not match or not ids or any(len(recipe_id) != 16 for recipe_id in ids):
        raise ValueError(f"Invalid runtime recipe catalog: {path}")
    return match.group(1), ids


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("output", type=Path)
    parser.add_argument("catalog", type=Path, nargs="+")
    args = parser.parse_args()
    catalogs = dict(read_catalog(path) for path in args.catalog)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text("# schema=1\n" + "".join(
        f"C\t{fingerprint}\t{','.join(recipe_ids)}\n"
        for fingerprint, recipe_ids in sorted(catalogs.items())), encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
