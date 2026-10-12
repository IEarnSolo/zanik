#!/usr/bin/env python3
"""Refresh public output-type tags without re-extracting or rebuilding assets."""
from __future__ import annotations

import hashlib
import json
import sys
from pathlib import Path

from generate_recipe_unlock_catalog import (  # type: ignore[attr-defined]
    INTERNAL_STATION_ROWS,
    PUBLIC_HIDDEN_STATION_SECTIONS,
    STATION_DISPLAY_NAMES,
    STATION_PRIORITY,
    public_recipe_types,
)


def main() -> int:
    root = Path(__file__).resolve().parents[1]
    reference = json.loads((root / "reference/recipe-unlocks.json").read_text(encoding="utf-8"))
    web_path = root / "web/recipe-unlocks.json"
    web = json.loads(web_path.read_text(encoding="utf-8"))
    by_id = {
        hashlib.sha256(f"{entry['kind']}:{entry['persistentId']}:{entry['internalName']}".encode()).hexdigest()[:16]: entry
        for entry in reference["recipes"]
    }
    # Older generated catalogs mixed journal grouping metadata into the live
    # station memberships.  Keep that evidence, but prefer cooked station and
    # station-upgrade tables whenever they exist.
    for source in by_id.values():
        candidates = list(source.get("stationCandidates", []))
        live = [candidate for candidate in candidates
                if candidate.get("table") != "JournalEntryRecipeData"]
        journal = [candidate for candidate in candidates
                   if candidate.get("table") == "JournalEntryRecipeData"]
        if live and journal:
            source["stationCandidates"] = live
            metadata_rows = {
                (candidate.get("station", ""), candidate.get("table", ""),
                 candidate.get("property", ""), candidate.get("assetPath", ""))
                for candidate in source.get("stationMetadataCandidates", []) + journal
            }
            source["stationMetadataCandidates"] = [
                {"station": row[0], "table": row[1], "property": row[2], "assetPath": row[3]}
                for row in sorted(metadata_rows)
            ]
            source["station"] = max(
                live,
                key=lambda candidate: (STATION_PRIORITY.get(candidate.get("station", ""), 30),
                                       candidate.get("station", "")),
            )["station"]
    for entry in web["entries"]:
        source = by_id[entry["id"]]
        entry["types"] = public_recipe_types(source)
        if source.get("kind") == "crafting":
            sections = sorted({
                STATION_DISPLAY_NAMES.get(str(candidate.get("station", "")),
                                          str(candidate.get("station", "")))
                for candidate in source.get("stationCandidates", [])
                if candidate.get("station") and candidate.get("station") not in INTERNAL_STATION_ROWS
            } - PUBLIC_HIDDEN_STATION_SECTIONS, key=str.casefold)
            if sections:
                entry["sections"] = sections
                entry["section"] = STATION_DISPLAY_NAMES.get(str(source.get("station", "")),
                                                               str(source.get("station", "")))
    (root / "reference/recipe-unlocks.json").write_text(
        json.dumps(reference, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    web_path.write_text(json.dumps(web, ensure_ascii=False, separators=(",", ":")) + "\n", encoding="utf-8")
    if "--audit" in sys.argv:
        groups: dict[tuple[str, str, str], set[str]] = {}
        for entry in web["entries"]:
            methods = groups.setdefault((entry["kind"], entry["name"], entry.get("icon", "")), set())
            methods.update(entry.get("types", []))
            if entry["kind"] == "building":
                methods.add("building-pieces")
            for route in entry.get("routes", []):
                for step in route.get("steps", []):
                    kind = str(step.get("kind", "")).casefold()
                    if kind == "obtain_item" and entry["kind"] != "building": methods.add("items")
                    elif "quest" in kind: methods.add("quests")
                    elif "vestige" in kind:
                        methods.add("vestiges")
                        if entry["kind"] != "building": methods.add("items")
                    elif kind == "interact_recipe_dispenser" and "dragonkin effigy" in str(step.get("text", "")).casefold(): methods.add("effigies")
                    elif "consume_unlock" in kind and entry["kind"] == "building": methods.add("plans")
                    elif "skill" in kind or "perk" in kind: methods.add("skills")
                    elif "entitlement" in kind: methods.add("entitlements")
        other = sorted(name for (_, name, _), methods in groups.items() if not methods)
        print(f"Other ({len(other)}):")
        print("\n".join(other))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
