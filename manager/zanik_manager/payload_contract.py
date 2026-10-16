from __future__ import annotations

import argparse
import hashlib
import json
import stat
from pathlib import Path
from typing import Iterable, Mapping, Any


CONTRACT_SCHEMA_VERSION = 1
OPERATIONAL_PREFIXES = ("bin/", "lib/", "scripts/", "tools/", "config/", "compatibility/")


def is_operational_path(path: str) -> bool:
    normalized = path.replace("\\", "/").lstrip("./")
    return any(normalized.startswith(prefix) for prefix in OPERATIONAL_PREFIXES)


def _record(entry: Mapping[str, Any] | Any) -> dict:
    def value(name: str):
        return entry[name] if isinstance(entry, Mapping) else getattr(entry, name)
    path = str(value("path")).replace("\\", "/").lstrip("./")
    digest = str(value("sha256")).lower()
    if not is_operational_path(path) or len(digest) != 64 or any(character not in "0123456789abcdef" for character in digest):
        raise ValueError(f"Invalid operational payload entry: {path}")
    return {"path": path, "mode": int(value("mode")) & 0o777,
            "size": int(value("size")), "sha256": digest}


def contract_from_entries(entries: Iterable[Mapping[str, Any] | Any], release_version: str) -> dict:
    records = sorted((_record(entry) for entry in entries if is_operational_path(
        str(entry["path"] if isinstance(entry, Mapping) else getattr(entry, "path")))), key=lambda item: item["path"])
    if not records:
        raise ValueError("Server payload contains no operational files")
    if len({item["path"] for item in records}) != len(records):
        raise ValueError("Server payload contract contains duplicate paths")
    canonical = json.dumps({"schemaVersion": CONTRACT_SCHEMA_VERSION, "files": records},
                           sort_keys=True, separators=(",", ":")).encode("utf-8")
    return {"schemaVersion": CONTRACT_SCHEMA_VERSION, "releaseVersion": release_version,
            "contractSha256": hashlib.sha256(canonical).hexdigest(), "files": records}


def contract_from_directory(root: Path, release_version: str) -> dict:
    entries = []
    for path in sorted(root.rglob("*")):
        if not path.is_file():
            continue
        relative = path.relative_to(root).as_posix()
        if not is_operational_path(relative):
            continue
        if path.is_symlink():
            raise ValueError(f"Operational payload path may not be a symlink: {relative}")
        data = path.read_bytes()
        entries.append({"path": relative, "mode": stat.S_IMODE(path.stat().st_mode),
                        "size": len(data), "sha256": hashlib.sha256(data).hexdigest()})
    return contract_from_entries(entries, release_version)


def validate_contract(document: Mapping[str, Any], entries: Iterable[Mapping[str, Any] | Any]) -> dict:
    if document.get("schemaVersion") != CONTRACT_SCHEMA_VERSION:
        raise ValueError("Unsupported server payload contract schema")
    expected = contract_from_entries(entries, str(document.get("releaseVersion", "")))
    if document.get("contractSha256") != expected["contractSha256"] or document.get("files") != expected["files"]:
        raise ValueError("Server payload contract does not match the operational files")
    return expected


def compatibility_summary(state: Mapping[str, Any], manager_version: str,
                          latest_release: Mapping[str, Any] | None = None) -> str:
    return compatibility_presentation(state, manager_version, latest_release)["text"]


def compatibility_presentation(state: Mapping[str, Any], manager_version: str,
                               latest_release: Mapping[str, Any] | None = None) -> dict[str, str]:
    installed_version = str(state.get("version") or "unknown version")
    installed_contract = state.get("payloadContractSha256")
    latest_release = latest_release or {}
    latest_payload = latest_release.get("serverPayload") or {}
    if latest_release.get("updateAvailable") and installed_contract and latest_payload.get("contractSha256"):
        if installed_contract != latest_payload["contractSha256"]:
            latest_version = str(latest_release["latestVersion"])
            return {
                "text": f"Zanik {installed_version} · Update Manager to {latest_version}, then update Zanik",
                "state": "available",
                "tooltip": (f"Zanik Manager {latest_version} contains changed server-side Zanik files. "
                            "Update Manager first, then use Update / repair Zanik to install and verify that newer payload."),
            }
    compatibility = state.get("payloadCompatibility")
    if compatibility == "current":
        return {
            "text": f"Zanik {installed_version} · Up to date with Manager {manager_version}",
            "state": "current",
            "tooltip": ("The installed Zanik release may have an older version number than this Manager, but its "
                        "server-side files match the payload bundled with this Manager. No server update is required."),
        }
    if compatibility == "update-available":
        return {
            "text": f"Zanik {installed_version} · Update available in Manager {manager_version}",
            "state": "available",
            "tooltip": (f"Zanik Manager {manager_version} contains changed server-side Zanik files. Use Update / repair "
                        "Zanik to install and verify them while preserving configuration, data, and compatible assets."),
        }
    return {
        "text": f"Zanik {installed_version} · Compatibility unknown with Manager {manager_version} · Repair recommended",
        "state": "neutral",
        "tooltip": ("Manager cannot prove that this installation's server-side files match its bundled payload. "
                    "Use Update / repair Zanik to restore the verified files and record current compatibility metadata."),
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", type=Path, required=True)
    parser.add_argument("--release-version", required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    contract = contract_from_directory(args.root, args.release_version)
    args.output.write_text(json.dumps(contract, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(contract["contractSha256"])
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
