#!/usr/bin/env python3
"""Inventory, build, and install Zanik's locally generated game asset bundle."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import sys
import tempfile
import time
import zipfile


SCHEMA_VERSION = 1
REQUIRED = (
    "buildings/catalog.json",
    "buildings/catalog.tsv",
    "npc-catalog.json",
    "plan-items.tsv",
    "quest-catalog.json",
    "quest-runtime-catalog.tsv",
    "quest-step-locations.json",
    "recipe-unlocks.json",
    "recipe-runtime-catalog.tsv",
)
OPTIONAL = (
    "world-map.jpg",
    "lodestone-icon.png",
    "nodestone-icon.png",
    "gravestone-icon.png",
    "Jancient-Regular.ttf",
    "skills/attack.png",
    "quests/main-quest.svg",
)
ALLOWED_TOP_LEVEL = {
    "asset-manifest.json", "buildings", "items", "npcs", "quests", "recipe-icons",
    "resources", "skills", "world-map.jpg", "lodestone-icon.png", "nodestone-icon.png",
    "gravestone-icon.png", "Jancient-Regular.ttf", "npc-catalog.json", "plan-items.tsv",
    "quest-catalog.json", "quest-runtime-catalog.tsv", "quest-step-locations.json",
    "recipe-unlocks.json", "recipe-runtime-catalog.tsv", "recipe-runtime-catalog-history.tsv",
    "resource-catalog.json", "resource-identities.tsv",
}
CONTAINER_NAMES = (
    "global.utoc", "global.ucas", "RSDragonwilds-LinuxServer.utoc",
    "RSDragonwilds-LinuxServer.ucas", "RSDragonwilds-LinuxServer.pak",
    "RSDragonwilds-Windows.utoc", "RSDragonwilds-Windows.ucas", "RSDragonwilds-Windows.pak",
)


def digest(path: Path) -> str:
    value = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            value.update(block)
    return value.hexdigest()


def game_build(game_root: Path) -> str:
    manifests: list[Path] = []
    for candidate in (game_root, *game_root.parents[:4]):
        manifests.extend(candidate.glob("appmanifest_*.acf"))
        manifests.extend(candidate.glob("steamapps/appmanifest_*.acf"))
    fallback = "unknown"
    for manifest in sorted(set(manifests)):
        contents = manifest.read_text(encoding="utf-8", errors="ignore")
        match = re.search(r'"buildid"\s+"(\d+)"', contents)
        if not match:
            continue
        install = re.search(r'"installdir"\s+"([^"]+)"', contents, re.IGNORECASE)
        if install and install.group(1).casefold() == game_root.name.casefold():
            return match.group(1)
        fallback = match.group(1) if fallback == "unknown" else fallback
    return fallback


def container_paths(game_root: Path) -> list[Path]:
    roots = (game_root / "RSDragonwilds/Content/Paks", game_root / "Content/Paks")
    found: list[Path] = []
    for root in roots:
        if root.is_dir():
            found.extend(path for name in CONTAINER_NAMES if (path := root / name).is_file())
    return sorted(set(found))


def classify(source: Path) -> tuple[list[str], list[str]]:
    missing_required = [name for name in REQUIRED if not (source / name).is_file()]
    missing_optional = [name for name in OPTIONAL if not (source / name).is_file()]
    return missing_required, missing_optional


def source_kind(paths: list[Path]) -> str:
    names = {path.name for path in paths}
    if any("LinuxServer" in name for name in names):
        return "linux-dedicated-server"
    if any("Windows" in name for name in names):
        return "windows-client"
    return "unknown"


def inventory(args: argparse.Namespace) -> int:
    root = Path(args.game_root).resolve()
    source = Path(args.generated_root).resolve() if args.generated_root else root / "zanik-generated-web"
    containers = container_paths(root)
    missing_required, missing_optional = classify(source)
    result = {
        "schemaVersion": SCHEMA_VERSION,
        "gameRoot": str(root),
        "gameBuild": game_build(root),
        "sourceKind": source_kind(containers),
        "containers": [{"name": path.name, "size": path.stat().st_size} for path in containers],
        "generatedRoot": str(source),
        "missingRequired": missing_required,
        "missingOptional": missing_optional,
        "ready": not missing_required,
    }
    output = json.dumps(result, indent=2, sort_keys=True) + "\n"
    if args.output:
        Path(args.output).write_text(output, encoding="utf-8")
    else:
        sys.stdout.write(output)
    return 0 if containers else 2


def show_build(args: argparse.Namespace) -> int:
    print(game_build(Path(args.game_root).resolve()))
    return 0


def safe_files(source: Path) -> list[Path]:
    files: list[Path] = []
    for path in source.rglob("*"):
        if not path.is_file() or path.is_symlink():
            continue
        relative = path.relative_to(source)
        if relative.parts[0] not in ALLOWED_TOP_LEVEL:
            continue
        if path.stat().st_size > 64 * 1024 * 1024:
            raise ValueError(f"asset exceeds 64 MiB limit: {relative}")
        files.append(path)
    return sorted(files, key=lambda value: value.relative_to(source).as_posix())


def manifest_for(root: Path, source: Path, files: list[Path], missing_optional: list[str]) -> dict:
    containers = container_paths(root)
    epoch = int(os.environ.get("SOURCE_DATE_EPOCH", str(int(time.time()))))
    return {
        "schemaVersion": SCHEMA_VERSION,
        "gameBuild": game_build(root),
        "sourceKind": source_kind(containers),
        "generatedAt": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime(epoch)),
        "sourceContainers": [
            {"name": path.name, "size": path.stat().st_size, "sha256": digest(path) if path.stat().st_size <= 512 * 1024 * 1024 else None}
            for path in containers
        ],
        "files": [
            {"path": path.relative_to(source).as_posix(), "size": path.stat().st_size, "sha256": digest(path)}
            for path in files if path.name != "asset-manifest.json"
        ],
        "missingRequired": [],
        "missingOptional": missing_optional,
    }


def build(args: argparse.Namespace) -> int:
    root, source, output = Path(args.game_root).resolve(), Path(args.generated_root).resolve(), Path(args.output).resolve()
    missing_required, missing_optional = classify(source)
    if missing_required:
        raise SystemExit("required generated assets are missing:\n  - " + "\n  - ".join(missing_required))
    files = safe_files(source)
    manifest = manifest_for(root, source, files, missing_optional)
    output.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(output, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        info = zipfile.ZipInfo("asset-manifest.json", date_time=(1980, 1, 1, 0, 0, 0))
        info.external_attr = 0o644 << 16
        archive.writestr(info, json.dumps(manifest, indent=2, sort_keys=True) + "\n")
        for path in files:
            if path.name == "asset-manifest.json":
                continue
            info = zipfile.ZipInfo(path.relative_to(source).as_posix(), date_time=(1980, 1, 1, 0, 0, 0))
            info.external_attr = 0o644 << 16
            archive.writestr(info, path.read_bytes())
    print(f"created {output} ({len(manifest['files'])} files)")
    return 0


def validate_member(name: str) -> None:
    path = Path(name)
    if path.is_absolute() or ".." in path.parts or not path.parts or path.parts[0] not in ALLOWED_TOP_LEVEL:
        raise ValueError(f"unsafe or unsupported bundle entry: {name}")


def install(args: argparse.Namespace) -> int:
    bundle, destination = Path(args.bundle).resolve(), Path(args.destination).resolve()
    destination.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="zanik-assets-", dir=destination) as temp_name:
        temp = Path(temp_name)
        with zipfile.ZipFile(bundle) as archive:
            for member in archive.infolist():
                validate_member(member.filename)
                if member.file_size > 64 * 1024 * 1024:
                    raise ValueError(f"bundle entry exceeds limit: {member.filename}")
            archive.extractall(temp)
        manifest_path = temp / "asset-manifest.json"
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
        if manifest.get("schemaVersion") != SCHEMA_VERSION or manifest.get("missingRequired"):
            raise ValueError("asset bundle manifest is incompatible or incomplete")
        for record in manifest.get("files", []):
            path = temp / record["path"]
            if not path.is_file() or digest(path) != record["sha256"]:
                raise ValueError(f"asset checksum mismatch: {record['path']}")
        version = str(manifest.get("gameBuild", "unknown")).replace("/", "_")
        target = destination / f"build-{version}"
        staged = destination / f".build-{version}.new"
        if staged.exists():
            shutil.rmtree(staged)
        shutil.copytree(temp, staged)
        if target.exists():
            shutil.rmtree(target)
        staged.replace(target)
        current = destination / "current"
        new_link = destination / ".current.new"
        if new_link.exists() or new_link.is_symlink():
            new_link.unlink()
        new_link.symlink_to(target.name, target_is_directory=True)
        if current.exists() and not current.is_symlink():
            legacy = destination / f".current.legacy-{int(time.time())}"
            current.replace(legacy)
        new_link.replace(current)
    print(f"activated {target}")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser()
    commands = parser.add_subparsers(dest="command", required=True)
    scan = commands.add_parser("inventory")
    scan.add_argument("--game-root", required=True)
    scan.add_argument("--generated-root")
    scan.add_argument("--output")
    scan.set_defaults(run=inventory)
    identify = commands.add_parser("show-build")
    identify.add_argument("--game-root", required=True)
    identify.set_defaults(run=show_build)
    create = commands.add_parser("build")
    create.add_argument("--game-root", required=True)
    create.add_argument("--generated-root", required=True)
    create.add_argument("--output", required=True)
    create.set_defaults(run=build)
    activate = commands.add_parser("install")
    activate.add_argument("--bundle", required=True)
    activate.add_argument("--destination", required=True)
    activate.set_defaults(run=install)
    args = parser.parse_args()
    return args.run(args)


if __name__ == "__main__":
    raise SystemExit(main())
