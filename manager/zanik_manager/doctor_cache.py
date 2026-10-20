from __future__ import annotations

import hashlib
import json
import os
import tempfile
from pathlib import Path


SCHEMA_VERSION = 1


def _path(root: Path, profile_id: str) -> Path:
    name = hashlib.sha256(profile_id.encode("utf-8")).hexdigest()[:32]
    return root / f"{name}.json"


def load_doctor_cache(root: Path, profile_id: str) -> dict | None:
    try:
        payload = json.loads(_path(root, profile_id).read_text(encoding="utf-8-sig"))
        if (payload.get("schemaVersion") != SCHEMA_VERSION or
                payload.get("profileId") != profile_id or
                not isinstance(payload.get("result"), dict)):
            return None
        return dict(payload["result"])
    except (OSError, UnicodeError, ValueError, TypeError, json.JSONDecodeError):
        return None


def save_doctor_cache(root: Path, profile_id: str, result: dict) -> None:
    root.mkdir(parents=True, exist_ok=True)
    payload = {"schemaVersion": SCHEMA_VERSION, "profileId": profile_id, "result": result}
    descriptor, temporary = tempfile.mkstemp(prefix="doctor-", suffix=".tmp", dir=root)
    try:
        with os.fdopen(descriptor, "w", encoding="utf-8", newline="\n") as output:
            json.dump(payload, output, sort_keys=True, separators=(",", ":"))
            output.write("\n")
            output.flush()
            os.fsync(output.fileno())
        os.replace(temporary, _path(root, profile_id))
    finally:
        try:
            os.unlink(temporary)
        except FileNotFoundError:
            pass


def delete_doctor_cache(root: Path, profile_id: str) -> None:
    try:
        _path(root, profile_id).unlink()
    except FileNotFoundError:
        pass
