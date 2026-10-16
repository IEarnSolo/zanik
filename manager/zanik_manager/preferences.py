from __future__ import annotations

import json
from pathlib import Path


class PreferenceStore:
    def __init__(self, path: Path):
        self.path = path

    def load(self) -> dict:
        if not self.path.exists():
            return {"schemaVersion": 1, "lastProfileId": ""}
        try:
            value = json.loads(self.path.read_text(encoding="utf-8"))
        except (OSError, UnicodeError, json.JSONDecodeError):
            return {"schemaVersion": 1, "lastProfileId": ""}
        if value.get("schemaVersion") != 1:
            return {"schemaVersion": 1, "lastProfileId": ""}
        return {"schemaVersion": 1, "lastProfileId": str(value.get("lastProfileId", ""))}

    def save_last_profile(self, profile_id: str) -> None:
        self.path.parent.mkdir(parents=True, exist_ok=True)
        temporary = self.path.with_suffix(".tmp")
        value = {"schemaVersion": 1, "lastProfileId": profile_id}
        temporary.write_text(json.dumps(value, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        temporary.replace(self.path)


def preferred_profile_index(profiles, profile_id: str) -> int:
    for index, profile in enumerate(profiles):
        if profile.id == profile_id:
            return index
    return 0
