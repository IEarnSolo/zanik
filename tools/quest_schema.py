"""Version-tolerant accessors for cooked QuestData exports.

Dragonwilds build 25812671 replaced ``ObjectiveTexts: TMap<FName,FText>``
with ``Objectives: TMap<FName,FQuestObjectiveData>``.  Keep that schema
boundary in one place so every generated catalog observes the same keys and
localized descriptions.
"""
from __future__ import annotations

from typing import Any


def objective_rows(properties: dict[str, Any]) -> list[dict[str, Any]]:
    """Return normalized ``Key``/``Value`` objective rows for known schemas."""
    legacy = properties.get("ObjectiveTexts")
    current = properties.get("Objectives")
    source = current if isinstance(current, list) else legacy
    if not isinstance(source, list):
        return []
    rows: list[dict[str, Any]] = []
    seen: set[str] = set()
    for item in source:
        if not isinstance(item, dict):
            continue
        key = str(item.get("Key", ""))
        value = item.get("Value")
        if isinstance(current, list) and isinstance(value, dict):
            value = value.get("Description")
        if not key or key in seen or not isinstance(value, dict):
            continue
        seen.add(key)
        rows.append({"Key": key, "Value": value})
    return rows
