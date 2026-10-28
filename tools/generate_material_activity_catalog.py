#!/usr/bin/env python3
"""Generate the authoritative first-pickup material catalogue from cooked assets."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path


def clean(value: object) -> str:
    return str(value or "").replace("\t", " ").replace("\r", " ").replace("\n", " ").strip()


def display_name(properties: dict, key: str) -> str:
    value = properties.get("Name")
    if isinstance(value, dict):
        for field in ("LocalizedString", "SourceString"):
            if clean(value.get(field)):
                return clean(value[field])
    return key.replace("ITEM_", "").replace("_", " ")


def generate(source: Path) -> list[tuple[str, str, str]]:
    entries: dict[str, tuple[str, str, str]] = {}
    for path in sorted(source.rglob("*.json")):
        try:
            exports = json.loads(path.read_text(encoding="utf-8-sig"))
        except (OSError, UnicodeDecodeError, json.JSONDecodeError):
            continue
        if not isinstance(exports, list):
            continue
        for export in exports:
            if not isinstance(export, dict):
                continue
            properties = export.get("Properties")
            key = clean(export.get("Name"))
            if (not isinstance(properties, dict) or
                    properties.get("bShowNotificationOnFirstPickup") is not True or
                    not key or key.startswith("Default__")):
                continue
            entries[key] = (key, display_name(properties, key), f"/assets/items/{key}.png")
    return [entries[key] for key in sorted(entries)]


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    entries = generate(args.source)
    if not entries:
        raise SystemExit("No first-pickup materials found")
    body = "".join("\t".join(entry) + "\n" for entry in entries)
    fingerprint = hashlib.sha256(body.encode()).hexdigest()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(
        f"# schema=1 count={len(entries)} fingerprint={fingerprint}\n" + body,
        encoding="utf-8",
    )
    print(f"wrote {len(entries)} first-pickup materials to {args.output}")


if __name__ == "__main__":
    main()
