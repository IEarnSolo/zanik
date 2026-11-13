#!/usr/bin/env python3
"""Fail-closed preflight for private WSL server and client research copies."""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import sys
from pathlib import Path, PurePosixPath

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
from manager.zanik_manager.game_updates import PublicSteamBuildChecker

CLIENT_APP_ID = "1374490"
SERVER_APP_ID = "4019830"


def manifest_build(path: Path) -> str:
    try:
        raw = path.read_bytes()
    except OSError as error:
        raise RuntimeError(f"Steam manifest is unavailable: {path.name}") from error
    match = re.search(rb'"buildid"\s*"([0-9]+)"', raw)
    if not match:
        raise RuntimeError(f"Steam manifest has no valid build ID: {path.name}")
    return match.group(1).decode("ascii")


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def safe_relative(raw: str) -> Path:
    value = PurePosixPath(raw.replace("\\", "/"))
    if value.is_absolute() or not value.parts or ".." in value.parts:
        raise ValueError(f"research input path must be relative: {raw}")
    return Path(*value.parts)


def atomic_write(path: Path, value: dict) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_name(path.name + ".tmp")
    temporary.write_text(json.dumps(value, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    os.replace(temporary, path)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--wsl-client-root", type=Path, required=True)
    parser.add_argument("--windows-client-root", type=Path, required=True)
    parser.add_argument("--wsl-server-executable", type=Path, required=True)
    parser.add_argument("--expected-server-sha256", required=True)
    parser.add_argument("--research-input", action="append", default=[])
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--latest-server-build")
    parser.add_argument("--latest-client-build")
    args = parser.parse_args()

    expected_hash = args.expected_server_sha256.lower()
    if len(expected_hash) != 64 or any(character not in "0123456789abcdef" for character in expected_hash):
        raise ValueError("expected server SHA-256 is invalid")
    latest_server = args.latest_server_build or PublicSteamBuildChecker(
        app_id=SERVER_APP_ID, description="dedicated-server").latest_build()
    latest_client = args.latest_client_build or PublicSteamBuildChecker(
        app_id=CLIENT_APP_ID, description="Windows-client").latest_build()
    if not latest_server.isdigit() or not latest_client.isdigit():
        raise RuntimeError("fresh Steam metadata returned an invalid build")

    wsl_manifest = args.wsl_client_root / "steamapps" / f"appmanifest_{CLIENT_APP_ID}.acf"
    windows_manifest = args.windows_client_root.parent.parent / f"appmanifest_{CLIENT_APP_ID}.acf"
    if not windows_manifest.is_file():
        windows_manifest = args.windows_client_root / "steamapps" / f"appmanifest_{CLIENT_APP_ID}.acf"
    wsl_client_build = manifest_build(wsl_manifest)
    windows_client_build = manifest_build(windows_manifest)
    server_hash = sha256_file(args.wsl_server_executable)
    server_directory_match = args.wsl_server_executable.parent.name == f"build-{latest_server}"

    inputs = []
    mismatches = []
    for raw in args.research_input:
        relative = safe_relative(raw)
        source = args.windows_client_root / relative
        staged = args.wsl_client_root / relative
        if not source.is_file() or not staged.is_file():
            mismatches.append({"path": relative.as_posix(), "reason": "missing"})
            continue
        source_hash, staged_hash = sha256_file(source), sha256_file(staged)
        inputs.append({"path": relative.as_posix(), "sha256": staged_hash})
        if source_hash != staged_hash:
            mismatches.append({"path": relative.as_posix(), "reason": "sha256-mismatch"})

    checks = {
        "serverBuildDirectory": server_directory_match,
        "serverExecutableSha256": server_hash == expected_hash,
        "windowsClientCurrent": windows_client_build == latest_client,
        "wslClientCurrent": wsl_client_build == latest_client,
        "researchInputsMatch": not mismatches,
    }
    result = {
        "schemaVersion": 1, "compatible": all(checks.values()), "checks": checks,
        "builds": {"latestServer": latest_server, "latestClient": latest_client,
                   "windowsClient": windows_client_build, "wslClient": wsl_client_build},
        "serverExecutableSha256": server_hash, "researchInputs": inputs, "mismatches": mismatches,
    }
    atomic_write(args.output, result)
    print(json.dumps(result, sort_keys=True))
    return 0 if result["compatible"] else 3


if __name__ == "__main__":
    raise SystemExit(main())
