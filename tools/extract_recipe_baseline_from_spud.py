#!/usr/bin/env python3
"""Extract one character's recipe/building baseline from a SPUD world save.

This tool is deliberately read-only. Dragonwilds stores cached character state
as JSON strings inside the binary SPUD container; we locate and decode those
documents without rewriting or normalising the save.
"""

from __future__ import annotations

import argparse
import hashlib
import json
from datetime import datetime, timezone
from pathlib import Path
from typing import Any


def embedded_json_documents(data: bytes) -> list[dict[str, Any]]:
    documents: list[dict[str, Any]] = []
    marker = b'"Version"'
    cursor = 0
    decoder = json.JSONDecoder()
    while True:
        marker_at = data.find(marker, cursor)
        if marker_at < 0:
            break
        start = data.rfind(b"{", 0, marker_at)
        cursor = marker_at + len(marker)
        if start < 0:
            continue
        # Cached state is UTF-8 JSON followed by binary SPUD fields. Invalid
        # trailing bytes are harmless because raw_decode stops at the JSON end.
        candidate = data[start:].decode("utf-8", "ignore")
        try:
            value, _ = decoder.raw_decode(candidate)
        except json.JSONDecodeError:
            continue
        if isinstance(value, dict) and isinstance(value.get("meta_data"), dict):
            documents.append(value)
    return documents


def locate(value: Any, key: str) -> list[Any]:
    found: list[Any] = []
    if isinstance(value, dict):
        for current_key, child in value.items():
            if current_key == key:
                found.append(child)
            found.extend(locate(child, key))
    elif isinstance(value, list):
        for child in value:
            found.extend(locate(child, key))
    return found


def flatten_ids(values: list[Any]) -> list[str]:
    result: set[str] = set()
    for value in values:
        if isinstance(value, dict):
            result.update(str(key) for key, enabled in value.items() if enabled is not False)
        elif isinstance(value, list):
            for item in value:
                if isinstance(item, str):
                    result.add(item)
                elif isinstance(item, dict):
                    for field in ("Id", "ID", "Guid", "GUID", "PersistentId", "PersistenceID"):
                        if item.get(field):
                            result.add(str(item[field]))
                            break
    return sorted(result)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("save", type=Path)
    parser.add_argument("--character-name", required=True)
    parser.add_argument("--character-guid", help="Required identity guard when known")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()

    raw = args.save.read_bytes()
    states = embedded_json_documents(raw)
    matches = [state for state in states if state.get("meta_data", {}).get("char_name") == args.character_name]
    if len(matches) != 1:
        raise SystemExit(f"expected exactly one {args.character_name!r} state; found {len(matches)}")
    state = matches[0]
    actual_guid = str(state.get("meta_data", {}).get("char_guid", ""))
    if args.character_guid and actual_guid.casefold() != args.character_guid.casefold():
        raise SystemExit(f"identity guard failed: expected {args.character_guid}, found {actual_guid}")

    result = {
        "schemaVersion": 1,
        "capturedAtUtc": datetime.now(timezone.utc).isoformat(),
        "source": {"file": args.save.name, "sha256": hashlib.sha256(raw).hexdigest()},
        "character": {"name": args.character_name, "persistentId": actual_guid},
        "recipesUnlocked": flatten_ids(locate(state, "RecipesUnlocked")),
        "buildingsUnlocked": flatten_ids(locate(state, "BuildingsUnlocked")),
        "recipesNew": flatten_ids(locate(state, "RecipesNew")),
        "buildingPiecesNew": flatten_ids(locate(state, "BuildingPiecesNew")),
    }
    rendered = json.dumps(result, indent=2, sort_keys=True) + "\n"
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(rendered, encoding="utf-8")
    else:
        print(rendered, end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
