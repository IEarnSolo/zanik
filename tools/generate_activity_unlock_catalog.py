#!/usr/bin/env python3
"""Generate canonical perk/spell activity metadata from cooked perk assets."""

from __future__ import annotations

import argparse
import hashlib
import json
import re
from pathlib import Path


OBJECT_NAME = re.compile(r"'([^']+)'$")


def clean(value: object) -> str:
    return str(value or "").replace("\t", " ").replace("\r", " ").replace("\n", " ").strip()


def display_text(value: object) -> str:
    if not isinstance(value, dict):
        return ""
    return clean(value.get("LocalizedString") or value.get("SourceString"))


def object_name(value: object) -> str:
    if not isinstance(value, dict):
        return ""
    raw = clean(value.get("ObjectName"))
    match = OBJECT_NAME.search(raw)
    return match.group(1).rsplit(".", 1)[-1] if match else ""


def asset_name(value: object) -> str:
    if not isinstance(value, dict):
        return ""
    return clean(value.get("AssetPathName")).rsplit("/", 1)[-1].split(".", 1)[0]


def generate(source: Path) -> list[tuple[str, str, str, str, str, str]]:
    entries: dict[str, tuple[str, str, str, str, str, str]] = {}
    for path in sorted(source.rglob("*.json")):
        if path.name == ".uas-manifest.json":
            continue
        try:
            exports = json.loads(path.read_text(encoding="utf-8-sig"))
        except (OSError, UnicodeDecodeError, json.JSONDecodeError):
            continue
        if not isinstance(exports, list):
            continue
        spell_links: dict[str, str] = {}
        for export in exports:
            if not isinstance(export, dict) or export.get("Type") != "SkillPerkModule_Spell":
                continue
            spell = object_name((export.get("Properties") or {}).get("SpellData"))
            perk = object_name(export.get("Outer"))
            if perk and spell:
                spell_links[perk] = spell
        for export in exports:
            if not isinstance(export, dict) or export.get("Type") != "SkillPerkData":
                continue
            key = clean(export.get("Name"))
            properties = export.get("Properties") or {}
            name = display_text(properties.get("PerkName"))
            texture = asset_name(properties.get("SkillPerkImage"))
            if not key or not name or not texture or key.startswith("Default__"):
                continue
            spell = spell_links.get(key)
            kind = "spell" if spell else "perk"
            canonical = spell or key
            icon = f"/assets/spells-and-perks/{key}.png"
            row = (key, kind, canonical, name, icon, texture)
            prior = entries.get(key)
            if prior is not None and prior != row:
                raise ValueError(f"conflicting unlock metadata for {key}")
            entries[key] = row
    return [entries[key] for key in sorted(entries)]


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    entries = generate(args.source)
    if not entries:
        raise SystemExit("No perk or spell unlocks found")
    body = "".join("\t".join(entry) + "\n" for entry in entries)
    fingerprint = hashlib.sha256(body.encode()).hexdigest()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(
        f"# schema=1 count={len(entries)} fingerprint={fingerprint}\n" + body,
        encoding="utf-8",
    )
    print(f"wrote {len(entries)} canonical perk/spell unlocks to {args.output}")


if __name__ == "__main__":
    main()
