from __future__ import annotations

import json
import uuid
from dataclasses import asdict, dataclass, replace
from pathlib import Path
from urllib.parse import urlsplit, urlunsplit


def normalize_origin(value: str) -> str:
    raw = value.strip()
    if "://" not in raw:
        raw = "https://" + raw
    parsed = urlsplit(raw)
    if parsed.scheme not in {"http", "https"} or not parsed.hostname or parsed.query or parsed.fragment:
        raise ValueError("Enter a valid HTTP or HTTPS panel address")
    host = parsed.hostname.lower()
    if ":" in host:
        host = f"[{host}]"
    netloc = host + (f":{parsed.port}" if parsed.port else "")
    path = parsed.path.rstrip("/")
    return urlunsplit((parsed.scheme.lower(), netloc, path, "", ""))


def is_local_origin(origin: str) -> bool:
    host = (urlsplit(origin).hostname or "").lower()
    return host in {"localhost", "127.0.0.1", "::1"}


@dataclass
class ServerProfile:
    id: str
    name: str
    panel_origin: str
    account_id: str
    credential_target: str
    server_id: str
    server_uuid: str = ""
    server_root: str = "/home/container"
    install_mode: str = "replace-executable"
    web_base_url: str = ""
    allow_insecure_http: bool = False
    game_root: str = ""
    last_asset_manifest: str = ""

    @classmethod
    def create(cls, **values):
        return cls(id=str(uuid.uuid4()), **values)


def rename_profile(profiles: list[ServerProfile], profile_id: str, name: str) -> list[ServerProfile]:
    normalized = name.strip()
    if not normalized:
        raise ValueError("Server profile names cannot be empty")
    if not any(profile.id == profile_id for profile in profiles):
        raise ValueError("Server profile was not found")
    return [replace(profile, name=normalized) if profile.id == profile_id else replace(profile)
            for profile in profiles]


def move_profile(profiles: list[ServerProfile], profile_id: str, offset: int) -> list[ServerProfile]:
    result = [replace(profile) for profile in profiles]
    index = next((number for number, profile in enumerate(result) if profile.id == profile_id), None)
    if index is None:
        raise ValueError("Server profile was not found")
    destination = max(0, min(len(result) - 1, index + offset))
    if destination != index:
        result.insert(destination, result.pop(index))
    return result


def remove_profile(profiles: list[ServerProfile], profile_id: str) -> list[ServerProfile]:
    if not any(profile.id == profile_id for profile in profiles):
        raise ValueError("Server profile was not found")
    return [replace(profile) for profile in profiles if profile.id != profile_id]


class ProfileStore:
    def __init__(self, path: Path):
        self.path = path

    def load(self) -> list[ServerProfile]:
        if not self.path.exists():
            return []
        data = json.loads(self.path.read_text(encoding="utf-8"))
        if data.get("schemaVersion") != 1:
            raise ValueError("Unsupported profile schema")
        return [ServerProfile(**item) for item in data.get("profiles", [])]

    def save(self, profiles: list[ServerProfile]) -> None:
        self.path.parent.mkdir(parents=True, exist_ok=True)
        data = {"schemaVersion": 1, "profiles": [asdict(item) for item in profiles]}
        temporary = self.path.with_suffix(".tmp")
        temporary.write_text(json.dumps(data, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        temporary.replace(self.path)
