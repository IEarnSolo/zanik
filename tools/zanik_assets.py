#!/usr/bin/env python3
"""Inventory, build, and install Zanik's locally generated game asset bundle."""

from __future__ import annotations

import argparse
from collections import defaultdict
import hashlib
import json
import math
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile
import time
import zipfile


SCHEMA_VERSION = 2
BUNDLE_SET_SCHEMA_VERSION = 1
EXTRACTION_RECIPE_VERSION = 4
DEFAULT_PART_BYTES = 90 * 1024 * 1024
MAX_ENTRY_BYTES = 64 * 1024 * 1024
WORLD_MAP_DIMENSIONS = (4096, 4096)
LEGACY_BUILDING_PLACEHOLDER_SHA256 = "04608a9130fcdd8b385f426a95b11f3b1e8fdac39b3bec166c05f65721433c09"
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
    "resource-catalog.json",
    "resource-identities.tsv",
    "npc-identities.tsv",
    "world-map.jpg",
    "lodestone-icon.png",
    "nodestone-icon.png",
    "gravestone-icon.png",
    "Jancient-Regular.ttf",
    "skills/attack.png",
    "quests/main-quest.svg",
)
OPTIONAL = (
    "quests/side-quest.svg",
)
OPTIONAL_GROUPS = (
    "items/*.png",
    "npcs/*.png",
    "resources/*.png",
    "buildings/*.png",
)
CATALOG_MINIMUMS = {
    "buildings/catalog.json": {"pieces": 100},
    "npc-catalog.json": {"types": 10, "nodes": 10},
    "quest-catalog.json": {"quests": 10},
    "quest-step-locations.json": {"quests": 10},
    "recipe-unlocks.json": {"entries": 100},
    "resource-catalog.json": {"types": 5, "nodes": 10},
}
ALLOWED_TOP_LEVEL = {
    "asset-manifest.json", "buildings", "items", "npcs", "quests", "recipe-icons",
    "resources", "skills", "world-map.jpg", "lodestone-icon.png", "nodestone-icon.png",
    "gravestone-icon.png", "Jancient-Regular.ttf", "npc-catalog.json", "plan-items.tsv",
    "quest-catalog.json", "quest-runtime-catalog.tsv", "quest-step-locations.json",
    "recipe-unlocks.json", "recipe-runtime-catalog.tsv", "recipe-runtime-catalog-history.tsv",
    "resource-catalog.json", "resource-identities.tsv", "npc-identities.tsv",
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


def tool_versions_from_file(path: Path) -> dict[str, str]:
    """Load normalized tool versions without passing JSON through a shell."""
    value = json.loads(path.read_text(encoding="utf-8"))
    tools = value.get("tools") if isinstance(value, dict) else None
    if not isinstance(tools, dict) or not tools:
        raise ValueError("tool versions file has no tools object")
    versions: dict[str, str] = {}
    for name, record in sorted(tools.items()):
        version = record.get("version") if isinstance(record, dict) else None
        if not isinstance(version, str) or not version.strip():
            raise ValueError(f"tool versions file has no version for {name}")
        versions[name] = version.strip()
    return versions


def clean_relative(value: str) -> str:
    return value.replace("\\", "/").lstrip("/")


def zip_member_is_link(member: zipfile.ZipInfo) -> bool:
    return ((member.external_attr >> 16) & 0o170000) == 0o120000


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
    missing_optional.extend(pattern for pattern in OPTIONAL_GROUPS if not any(source.glob(pattern)))
    return missing_required, missing_optional


def jpeg_dimensions(path: Path) -> tuple[int, int]:
    """Read JPEG dimensions without adding an image-library runtime dependency."""
    start_of_frame = {0xC0, 0xC1, 0xC2, 0xC3, 0xC5, 0xC6, 0xC7, 0xC9, 0xCA, 0xCB, 0xCD, 0xCE, 0xCF}
    with path.open("rb") as stream:
        if stream.read(2) != b"\xff\xd8":
            raise ValueError("missing JPEG start marker")
        while True:
            value = stream.read(1)
            if not value:
                break
            if value != b"\xff":
                continue
            while value == b"\xff":
                value = stream.read(1)
            if not value:
                break
            marker = value[0]
            if marker in {0x01, *range(0xD0, 0xDA)}:
                continue
            size_bytes = stream.read(2)
            if len(size_bytes) != 2:
                break
            size = int.from_bytes(size_bytes, "big")
            if size < 2:
                raise ValueError("invalid JPEG segment length")
            if marker in start_of_frame:
                frame = stream.read(5)
                if len(frame) != 5:
                    break
                height = int.from_bytes(frame[1:3], "big")
                width = int.from_bytes(frame[3:5], "big")
                if not width or not height:
                    raise ValueError("invalid JPEG dimensions")
                return width, height
            stream.seek(size - 2, os.SEEK_CUR)
    raise ValueError("JPEG has no supported frame header")


def png_dimensions(path: Path) -> tuple[int, int]:
    value = path.read_bytes()[:24]
    if len(value) != 24 or value[:8] != b"\x89PNG\r\n\x1a\n" or value[12:16] != b"IHDR":
        raise ValueError("missing PNG signature or IHDR")
    width, height = int.from_bytes(value[16:20], "big"), int.from_bytes(value[20:24], "big")
    if not width or not height:
        raise ValueError("invalid PNG dimensions")
    return width, height


def validate_generated(source: Path) -> dict:
    missing_required, missing_optional = classify(source)
    errors: list[str] = []
    warnings: list[str] = []
    counts: dict[str, int] = {}
    building_icon_sources: dict[str, int] = {}
    recipe_route_coverage: dict[str, int] = {}
    if not source.is_dir():
        errors.append(f"generated asset root does not exist: {source}")
    for relative in REQUIRED:
        path = source / relative
        if not path.is_file():
            continue
        if path.is_symlink() or path.stat().st_size == 0:
            errors.append(f"required asset is empty or a symlink: {relative}")
            continue
        try:
            if path.suffix == ".json":
                value = json.loads(path.read_text(encoding="utf-8"))
                if not isinstance(value, (dict, list)):
                    errors.append(f"required JSON has an unsupported root value: {relative}")
                if isinstance(value, dict):
                    if relative == "buildings/catalog.json":
                        building_icon_sources = value.get("iconSources", {})
                        if value.get("counts", {}).get("pieces") == 822 and building_icon_sources != {
                            "authored-texture": 753,
                            "embedded-build-ui-thumbnail": 68,
                            "procedural-thumbnail": 1,
                        }:
                            errors.append(f"building icon source coverage changed: {building_icon_sources}")
                        pieces = value.get("pieces", [])
                        if isinstance(pieces, list) and len(pieces) == 822:
                            seen_icons: set[str] = set()
                            for piece in pieces:
                                icon = str(piece.get("icon", "")) if isinstance(piece, dict) else ""
                                if not icon or Path(icon).name != icon or icon in seen_icons:
                                    errors.append(f"building catalog has an invalid or duplicate icon reference: {icon!r}")
                                    continue
                                seen_icons.add(icon)
                                icon_path = source / "buildings" / icon
                                if not icon_path.is_file() or icon_path.is_symlink():
                                    errors.append(f"building icon is missing: {icon}")
                                    continue
                                try:
                                    dimensions = png_dimensions(icon_path)
                                    if dimensions[0] < 256 or dimensions[1] < 256:
                                        errors.append(f"building icon is undersized: {icon} ({dimensions[0]}x{dimensions[1]})")
                                    if digest(icon_path) == LEGACY_BUILDING_PLACEHOLDER_SHA256:
                                        errors.append(f"building icon is the removed legacy placeholder: {icon}")
                                except (OSError, ValueError) as error:
                                    errors.append(f"building icon is not a valid PNG: {icon}: {error}")
                    if relative == "recipe-unlocks.json":
                        recipe_route_coverage = {
                            "entries": len(value.get("entries", [])),
                            "unknownTriggers": int(value.get("counts", {}).get("unknownTriggers", 0)),
                        }
                        if "Available from the start" in path.read_text(encoding="utf-8"):
                            errors.append("recipe catalog contains an unsupported automatic-availability claim")
                    for key in ("pieces", "types", "nodes", "quests", "entries"):
                        rows = value.get(key)
                        if isinstance(rows, list):
                            counts[f"{relative}:{key}"] = len(rows)
                            identifiers = [row.get("id") for row in rows if isinstance(row, dict) and row.get("id") is not None]
                            if len(identifiers) != len(set(identifiers)):
                                errors.append(f"catalog contains duplicate stable identifiers: {relative}:{key}")
                            if key == "nodes":
                                for row in rows:
                                    if not isinstance(row, dict) or any(not isinstance(row.get(axis), (int, float)) or not math.isfinite(row[axis]) or not 0 <= row[axis] <= 1 for axis in ("x", "y")):
                                        errors.append(f"catalog contains an invalid map coordinate: {relative}:{key}")
                                        break
                    for key, minimum in CATALOG_MINIMUMS.get(relative, {}).items():
                        rows = value.get(key)
                        if not isinstance(rows, list) or len(rows) < minimum:
                            actual = len(rows) if isinstance(rows, list) else "missing"
                            errors.append(f"catalog coverage is below the reviewed minimum: {relative}:{key}={actual} < {minimum}")
            elif path.suffix == ".tsv":
                first = path.open(encoding="utf-8", errors="strict").readline().rstrip("\r\n")
                if not first or ("\t" not in first and not first.startswith("#")):
                    errors.append(f"required TSV has no header or records: {relative}")
        except (OSError, UnicodeError, json.JSONDecodeError) as error:
            errors.append(f"required asset is invalid: {relative}: {error}")
    if missing_required:
        errors.extend(f"required asset is missing: {name}" for name in missing_required)
    world_map = source / "world-map.jpg"
    if world_map.is_file() and not world_map.is_symlink() and world_map.stat().st_size:
        try:
            dimensions = jpeg_dimensions(world_map)
            if dimensions != WORLD_MAP_DIMENSIONS:
                errors.append(
                    f"world map has unexpected dimensions: {dimensions[0]}x{dimensions[1]} "
                    f"(expected {WORLD_MAP_DIMENSIONS[0]}x{WORLD_MAP_DIMENSIONS[1]})"
                )
        except (OSError, ValueError) as error:
            errors.append(f"world map is not a valid JPEG: {error}")
    asset_references: set[str] = set()
    absolute_pattern = re.compile(r"(?:[A-Za-z]:[\\/]|/(?:home|Users|tmp)/)")
    for path in safe_files(source) if source.is_dir() else []:
        relative = path.relative_to(source).as_posix()
        if path.suffix.lower() not in {".json", ".tsv", ".html", ".js", ".css"}:
            continue
        try:
            contents = path.read_text(encoding="utf-8")
        except (OSError, UnicodeError):
            continue
        if absolute_pattern.search(contents):
            errors.append(f"generated file contains a machine-local absolute path: {relative}")
        asset_references.update(clean_relative(match) for match in re.findall(r"/assets/([A-Za-z0-9_./-]+)", contents))
    missing_references = sorted(reference for reference in asset_references if not (source / reference).is_file())
    errors.extend(f"web asset reference is unresolved: {reference}" for reference in missing_references)
    return {
        "valid": not errors,
        "missingRequired": missing_required,
        "missingOptional": missing_optional,
        "errors": errors,
        "warnings": warnings,
        "catalogCounts": counts,
        "assetReferences": len(asset_references),
        "buildingIconSources": building_icon_sources,
        "recipeRouteCoverage": recipe_route_coverage,
    }


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
    validation = validate_generated(source)
    result = {
        "schemaVersion": SCHEMA_VERSION,
        "gameRoot": str(root),
        "gameBuild": game_build(root),
        "sourceKind": source_kind(containers),
        "containers": [{"name": path.name, "size": path.stat().st_size} for path in containers],
        "generatedRoot": str(source),
        "missingRequired": validation["missingRequired"],
        "missingOptional": validation["missingOptional"],
        "validationErrors": validation["errors"],
        "ready": validation["valid"],
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


def container_fingerprint(path: Path) -> dict:
    size = path.stat().st_size
    record = {"name": path.name, "size": size}
    if size <= 512 * 1024 * 1024:
        record["sha256"] = digest(path)
    else:
        value = hashlib.sha256()
        with path.open("rb") as stream:
            value.update(stream.read(1024 * 1024))
            stream.seek(max(0, size - 1024 * 1024))
            value.update(stream.read(1024 * 1024))
        value.update(str(size).encode("ascii"))
        record["sampledSha256"] = value.hexdigest()
    return record


def manifest_for(root: Path, source: Path, files: list[Path], validation: dict,
                 mapping_path: Path | None = None, tool_versions: dict | None = None,
                 recipe_version: int = EXTRACTION_RECIPE_VERSION, profile: dict | None = None) -> dict:
    containers = container_paths(root)
    epoch = int(os.environ.get("SOURCE_DATE_EPOCH", str(int(time.time()))))
    expected = (profile or {}).get("catalogProfile", {}).get("catalogCounts", {})
    actual = validation.get("catalogCounts", {})
    drift = [
        {"catalog": key, "expected": value, "actual": actual.get(key)}
        for key, value in sorted(expected.items()) if actual.get(key) != value
    ]
    warnings = list(validation.get("warnings", []))
    warnings.extend(
        f"unreviewed catalog drift: {item['catalog']} expected {item['expected']}, got {item['actual']}"
        for item in drift
    )
    executable = root / "RSDragonwilds/Binaries/Win64/RSDragonwilds-Win64-Shipping.exe"
    categories: dict[str, int] = defaultdict(int)
    for path in files:
        relative = path.relative_to(source)
        categories[relative.parts[0] if len(relative.parts) > 1 else "core"] += 1
    return {
        "schemaVersion": SCHEMA_VERSION,
        "gameBuild": game_build(root),
        "sourceKind": source_kind(containers),
        "generatedAt": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime(epoch)),
        "sourceContainers": [container_fingerprint(path) for path in containers],
        "sourceExecutable": ({"name": executable.name, "size": executable.stat().st_size,
                              "sha256": digest(executable)} if executable.is_file() else None),
        "mappingSha256": digest(mapping_path) if mapping_path and mapping_path.is_file() else None,
        "toolVersions": tool_versions or {},
        "extractionRecipeVersion": recipe_version,
        "extractionStage": "validated-and-packaged",
        "cleanupWarnings": [],
        "compatibility": "compatible" if validation["valid"] and not drift else "incompatible",
        "compatibilityProfile": {
            "version": (profile or {}).get("catalogProfile", {}).get("version"),
            "expectedCatalogCounts": expected,
            "drift": drift,
        },
        "catalogCounts": validation.get("catalogCounts", {}),
        "buildingIconSources": validation.get("buildingIconSources", {}),
        "recipeRouteCoverage": validation.get("recipeRouteCoverage", {}),
        "assetReferenceCount": validation.get("assetReferences", 0),
        "generatedFileCount": len([path for path in files if path.name != "asset-manifest.json"]),
        "fileCountsByCategory": dict(sorted(categories.items())),
        "files": [
            {"path": path.relative_to(source).as_posix(), "size": path.stat().st_size, "sha256": digest(path)}
            for path in files if path.name != "asset-manifest.json"
        ],
        "missingRequired": [],
        "missingOptional": validation["missingOptional"],
        "warnings": warnings,
    }


def build(args: argparse.Namespace) -> int:
    root, source, output = Path(args.game_root).resolve(), Path(args.generated_root).resolve(), Path(args.output).resolve()
    validation = validate_generated(source)
    if not validation["valid"]:
        raise SystemExit("generated assets are invalid:\n  - " + "\n  - ".join(validation["errors"]))
    files = safe_files(source)
    if getattr(args, "tool_versions_file", None):
        tool_versions = tool_versions_from_file(Path(args.tool_versions_file).resolve())
    else:
        tool_versions = json.loads(args.tool_versions) if getattr(args, "tool_versions", None) else {}
    mapping = Path(args.mapping).resolve() if getattr(args, "mapping", None) else None
    profile = json.loads(Path(args.profile).read_text(encoding="utf-8")) if getattr(args, "profile", None) else None
    manifest = manifest_for(root, source, files, validation, mapping, tool_versions,
                            getattr(args, "recipe_version", 1), profile)
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


def _clear_readonly(function, target, error) -> None:
    try:
        os.chmod(target, 0o700)
        function(target)
    except FileNotFoundError:
        return


def cleanup_work(args: argparse.Namespace) -> int:
    """Delete only a marked Zanik asset workspace, tolerating Windows races."""
    root = Path(args.work_root).resolve()
    marker = root / ".zanik-asset-work.json"
    if not marker.is_file():
        raise SystemExit(f"refusing to clean an unmarked asset workspace: {root}")
    try:
        metadata = json.loads(marker.read_text(encoding="utf-8-sig"))
    except (OSError, json.JSONDecodeError) as error:
        raise SystemExit(f"refusing to clean an invalid asset workspace: {error}") from error
    if metadata.get("schemaVersion") != 1 or metadata.get("owner") != "Zanik asset extractor":
        raise SystemExit(f"refusing to clean an unowned asset workspace: {root}")
    attempts = max(1, int(args.attempts))
    last_error: OSError | None = None
    for attempt in range(attempts):
        try:
            shutil.rmtree(root, onerror=_clear_readonly)
            if args.pending_journal:
                Path(args.pending_journal).unlink(missing_ok=True)
            print(json.dumps({"cleaned": True, "path": str(root)}, sort_keys=True))
            return 0
        except FileNotFoundError:
            if args.pending_journal:
                Path(args.pending_journal).unlink(missing_ok=True)
            print(json.dumps({"cleaned": True, "path": str(root)}, sort_keys=True))
            return 0
        except OSError as error:
            last_error = error
            if attempt + 1 < attempts:
                time.sleep(min(0.25 * (2 ** attempt), 4.0))
    if args.pending_journal:
        journal = Path(args.pending_journal).resolve()
        journal.parent.mkdir(parents=True, exist_ok=True)
        temporary = journal.with_suffix(journal.suffix + ".new")
        temporary.write_text(json.dumps({
            "schemaVersion": 1,
            "workRoot": str(root),
            "error": str(last_error),
            "createdAt": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
        }, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        temporary.replace(journal)
    print(json.dumps({"cleaned": False, "path": str(root), "warning": str(last_error)}, sort_keys=True))
    return 3


def validate_member(name: str) -> None:
    path = Path(name)
    if path.is_absolute() or ".." in path.parts or not path.parts or path.parts[0] not in ALLOWED_TOP_LEVEL:
        raise ValueError(f"unsafe or unsupported bundle entry: {name}")


def extract_archive(archive_path: Path, destination: Path, seen: set[str] | None = None) -> set[str]:
    seen = seen if seen is not None else set()
    with zipfile.ZipFile(archive_path) as archive:
        for member in archive.infolist():
            validate_member(member.filename)
            if zip_member_is_link(member):
                raise ValueError(f"bundle contains a symbolic link: {member.filename}")
            if member.file_size > MAX_ENTRY_BYTES:
                raise ValueError(f"bundle entry exceeds limit: {member.filename}")
            normalized = Path(member.filename).as_posix().rstrip("/")
            if not normalized:
                continue
            if normalized in seen:
                raise ValueError(f"bundle set contains a duplicate path: {normalized}")
            seen.add(normalized)
        archive.extractall(destination)
    return seen


def validate_manifest_tree(root: Path, require_compatible: bool = False) -> dict:
    manifest_path = root / "asset-manifest.json"
    if not manifest_path.is_file():
        raise ValueError("asset bundle has no asset-manifest.json")
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    if manifest.get("schemaVersion") != SCHEMA_VERSION or manifest.get("missingRequired"):
        raise ValueError("asset bundle manifest is incompatible or incomplete")
    if require_compatible and manifest.get("compatibility") != "compatible":
        raise ValueError("asset bundle is marked incompatible by its reviewed catalog profile")
    records = manifest.get("files")
    if not isinstance(records, list) or not records:
        raise ValueError("asset bundle manifest has no file records")
    seen: set[str] = set()
    for record in records:
        relative = record.get("path", "") if isinstance(record, dict) else ""
        validate_member(relative)
        if relative in seen:
            raise ValueError(f"duplicate asset manifest record: {relative}")
        seen.add(relative)
        path = root / relative
        if not path.is_file() or path.is_symlink() or digest(path) != record.get("sha256"):
            raise ValueError(f"asset checksum mismatch: {relative}")
    validation = validate_generated(root)
    if not validation["valid"]:
        raise ValueError("asset bundle contents are invalid: " + "; ".join(validation["errors"]))
    return manifest


def validate(args: argparse.Namespace) -> int:
    if args.bundle:
        bundle = Path(args.bundle).resolve()
        with tempfile.TemporaryDirectory(prefix="zanik-assets-validate-") as temp_name:
            temp = Path(temp_name)
            extract_archive(bundle, temp)
            manifest = validate_manifest_tree(temp)
    else:
        generated = Path(args.generated_root).resolve()
        result = validate_generated(generated)
        if not result["valid"]:
            raise ValueError("generated assets are invalid: " + "; ".join(result["errors"]))
        manifest = result
    print(json.dumps(manifest, indent=2, sort_keys=True))
    return 0


def extract(args: argparse.Namespace) -> int:
    root = Path(args.game_root).resolve()
    output = Path(args.output).resolve()
    containers = container_paths(root)
    detected = source_kind(containers)
    requested = args.source_kind
    if requested == "server" and detected != "linux-dedicated-server":
        raise ValueError("server source requested but Linux dedicated-server containers were not found")
    if requested == "client" and detected != "windows-client":
        raise ValueError("client source requested but Windows client containers were not found")
    if not containers:
        raise ValueError("no supported game containers were found")
    if detected == "linux-dedicated-server":
        result = {
            "sourceKind": detected,
            "gameBuild": game_build(root),
            "ready": False,
            "missingRequired": list(REQUIRED),
            "message": "the Linux dedicated server is inventory-only and cannot produce the complete Zanik web asset bundle; use a licensed game client",
        }
        print(json.dumps(result, indent=2, sort_keys=True))
        return 4
    if not args.tool_path:
        result = {
            "sourceKind": detected,
            "gameBuild": game_build(root),
            "ready": False,
            "missingRequired": list(REQUIRED),
            "message": "inventory completed; extraction requires the platform asset converter",
        }
        print(json.dumps(result, indent=2, sort_keys=True))
        return 3
    tool = Path(args.tool_path).resolve()
    if not tool.is_file():
        raise ValueError(f"asset converter does not exist: {tool}")
    output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="zanik-extract-", dir=output.parent) as temp_name:
        staged = Path(temp_name) / "generated"
        command = [str(tool), "--game-root", str(root), "--output", str(staged), "--source-kind", detected]
        if args.usmap_path:
            command.extend(("--usmap-path", str(Path(args.usmap_path).resolve())))
        completed = subprocess.run(command, check=False)
        if completed.returncode:
            raise RuntimeError(f"asset converter failed with exit code {completed.returncode}")
        validation = validate_generated(staged)
        if not validation["valid"]:
            raise ValueError("converter output is incomplete: " + "; ".join(validation["errors"]))
        replacement = output.with_name(output.name + ".new")
        if replacement.exists():
            shutil.rmtree(replacement)
        shutil.copytree(staged, replacement)
        previous = output.with_name(output.name + ".previous")
        if output.exists():
            if previous.exists():
                shutil.rmtree(previous)
            output.replace(previous)
        replacement.replace(output)
    print(json.dumps({"sourceKind": detected, "gameBuild": game_build(root), "output": str(output), "ready": True}, sort_keys=True))
    return 0


def activate_tree(temp: Path, destination: Path) -> Path:
    manifest = validate_manifest_tree(temp, require_compatible=True)
    version = str(manifest.get("gameBuild", "unknown")).replace("/", "_")
    manifest_hash = digest(temp / "asset-manifest.json")[:12]
    target = destination / f"build-{version}-{manifest_hash}"
    staged = destination / f".build-{version}-{manifest_hash}.new"
    if staged.exists():
        shutil.rmtree(staged)
    shutil.copytree(temp, staged)
    if target.exists():
        shutil.rmtree(staged)
    else:
        staged.replace(target)
    current = destination / "current"
    new_link = destination / ".current.new"
    if new_link.exists() or new_link.is_symlink():
        new_link.unlink()
    try:
        new_link.symlink_to(target.name, target_is_directory=True)
    except OSError as error:
        if os.name != "nt" or getattr(error, "winerror", None) != 1314:
            raise
        # Ordinary Windows accounts cannot create symbolic links unless
        # Developer Mode is enabled. A fully staged directory rename retains
        # the same atomic activation and rollback behavior for local tests.
        shutil.copytree(target, new_link)
    if current.exists() and not current.is_symlink():
        legacy = destination / f".current.legacy-{int(time.time())}"
        current.replace(legacy)
    new_link.replace(current)
    return target


def install(args: argparse.Namespace) -> int:
    bundle, destination = Path(args.bundle).resolve(), Path(args.destination).resolve()
    destination.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="zanik-assets-", dir=destination) as temp_name:
        temp = Path(temp_name)
        extract_archive(bundle, temp)
        target = activate_tree(temp, destination)
    print(f"activated {target}")
    return 0


def write_part(output: Path, source: zipfile.ZipFile, members: list[zipfile.ZipInfo]) -> None:
    with zipfile.ZipFile(output, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        for member in members:
            info = zipfile.ZipInfo(member.filename, date_time=(1980, 1, 1, 0, 0, 0))
            info.external_attr = 0o644 << 16
            archive.writestr(info, source.read(member))


def bundle_set(args: argparse.Namespace) -> int:
    bundle = Path(args.bundle).resolve()
    requested_output = Path(args.output_dir).resolve()
    output = requested_output
    staging: Path | None = None
    if args.replace:
        requested_output.parent.mkdir(parents=True, exist_ok=True)
        staging = Path(tempfile.mkdtemp(prefix=requested_output.name + ".new-", dir=requested_output.parent))
        output = staging
    maximum = int(args.max_part_bytes)
    if maximum < 1024 * 1024:
        raise ValueError("maximum part size must be at least 1 MiB")
    if output.exists() and any(output.iterdir()):
        raise ValueError(f"bundle-set output directory is not empty: {output}")
    output.mkdir(parents=True, exist_ok=True)
    records: list[dict] = []
    with zipfile.ZipFile(bundle) as source:
        members = [member for member in source.infolist() if not member.is_dir()]
        groups: dict[str, list[zipfile.ZipInfo]] = defaultdict(list)
        for member in members:
            validate_member(member.filename)
            top = Path(member.filename).parts[0]
            group = "core" if top in REQUIRED or top in {"asset-manifest.json", "skills", "quests"} else top.replace(".", "-")
            groups[group].append(member)
        sequence = 0
        for group in sorted(groups, key=lambda value: (value != "core", value)):
            pending = sorted(groups[group], key=lambda member: member.filename)
            while pending:
                sequence += 1
                selected: list[zipfile.ZipInfo] = []
                raw = 0
                while pending and (not selected or raw + pending[0].file_size <= maximum * 8 // 10):
                    item = pending.pop(0)
                    selected.append(item)
                    raw += item.file_size
                filename = f"part-{sequence:02d}-{group}.zip"
                path = output / filename
                write_part(path, source, selected)
                if path.stat().st_size > maximum and len(selected) > 1:
                    pending = selected[len(selected) // 2:] + pending
                    selected = selected[:len(selected) // 2]
                    write_part(path, source, selected)
                if path.stat().st_size > maximum:
                    raise ValueError(f"one logical asset part exceeds the configured limit: {filename}")
                records.append({"name": filename, "size": path.stat().st_size, "sha256": digest(path),
                                "files": [member.filename for member in selected]})
    with zipfile.ZipFile(bundle) as source:
        manifest_hash = hashlib.sha256(source.read("asset-manifest.json")).hexdigest()
    index = {"schemaVersion": BUNDLE_SET_SCHEMA_VERSION, "assetManifestSha256": manifest_hash,
             "sourceBundleSha256": digest(bundle), "parts": records,
             "fileCount": sum(len(record["files"]) for record in records)}
    (output / "bundle-index.json").write_text(json.dumps(index, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    if staging is not None:
        previous = requested_output.with_name(requested_output.name + ".previous")
        if previous.exists():
            shutil.rmtree(previous, onerror=_clear_readonly)
        if requested_output.exists():
            requested_output.replace(previous)
        try:
            staging.replace(requested_output)
        except Exception:
            if previous.exists() and not requested_output.exists():
                previous.replace(requested_output)
            raise
        if previous.exists():
            shutil.rmtree(previous, onerror=_clear_readonly)
        output = requested_output
    print(json.dumps({"output": str(output), "parts": len(records), "files": index["fileCount"]}, sort_keys=True))
    return 0


def install_set(args: argparse.Namespace) -> int:
    index_path = Path(args.index).resolve()
    index = json.loads(index_path.read_text(encoding="utf-8"))
    if index.get("schemaVersion") != BUNDLE_SET_SCHEMA_VERSION or not isinstance(index.get("parts"), list):
        raise ValueError("unsupported bundle-set index")
    destination = Path(args.destination).resolve()
    destination.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="zanik-assets-set-", dir=destination) as temp_name:
        temp = Path(temp_name)
        seen: set[str] = set()
        expected_names: set[str] = set()
        for record in index["parts"]:
            name = record.get("name", "")
            if Path(name).name != name or not name.endswith(".zip") or name in expected_names:
                raise ValueError(f"unsafe or duplicate bundle-set part: {name}")
            expected_names.add(name)
            part = index_path.parent / name
            if not part.is_file() or part.stat().st_size != record.get("size") or digest(part) != record.get("sha256"):
                raise ValueError(f"bundle-set part is missing or corrupt: {name}")
            before = set(seen)
            extract_archive(part, temp, seen)
            actual = sorted(seen - before)
            if actual != sorted(record.get("files", [])):
                raise ValueError(f"bundle-set part contents do not match its index: {name}")
        extra = {path.name for path in index_path.parent.glob("part-*.zip")} - expected_names
        if extra:
            raise ValueError("bundle-set directory contains unexpected parts: " + ", ".join(sorted(extra)))
        if len(seen) != index.get("fileCount"):
            raise ValueError("bundle-set file count does not match its index")
        if digest(temp / "asset-manifest.json") != index.get("assetManifestSha256"):
            raise ValueError("bundle-set asset manifest does not match its index")
        target = activate_tree(temp, destination)
    print(f"activated {target}")
    return 0


def prepare_mappings(args: argparse.Namespace) -> int:
    sys.path.insert(0, str(Path(__file__).resolve().parent))
    from windows_mapping import prepare
    result = prepare(Path(args.game_root), Path(args.ue4ss_root), Path(args.output), Path(args.work_root), args.timeout,
                     args.preserve_existing_loader,
                     Path(args.recovery_root) if args.recovery_root else None)
    print(json.dumps(result, indent=2, sort_keys=True))
    return 0


def recover_mappings(args: argparse.Namespace) -> int:
    sys.path.insert(0, str(Path(__file__).resolve().parent))
    from windows_mapping import recover
    restored = recover(Path(args.journal))
    print(json.dumps({"restored": restored}, sort_keys=True))
    return 0


def mapping_preflight(args: argparse.Namespace) -> int:
    sys.path.insert(0, str(Path(__file__).resolve().parent))
    from windows_mapping import mapping_status
    result = mapping_status(Path(args.game_root), Path(args.recovery_root) if args.recovery_root else None)
    print(json.dumps(result, indent=2, sort_keys=True))
    return 0


def main() -> int:
    parser = argparse.ArgumentParser()
    commands = parser.add_subparsers(dest="command", required=True)
    scan = commands.add_parser("inventory")
    scan.add_argument("--game-root", required=True)
    scan.add_argument("--generated-root")
    scan.add_argument("--output")
    scan.set_defaults(run=inventory)
    extraction = commands.add_parser("extract")
    extraction.add_argument("--game-root", required=True)
    extraction.add_argument("--output", required=True)
    extraction.add_argument("--source-kind", choices=("auto", "server", "client"), default="auto")
    extraction.add_argument("--tool-path")
    extraction.add_argument("--usmap-path")
    extraction.set_defaults(run=extract)
    mappings = commands.add_parser("prepare-mappings")
    mappings.add_argument("--game-root", required=True)
    mappings.add_argument("--ue4ss-root", required=True)
    mappings.add_argument("--output", required=True)
    mappings.add_argument("--work-root", required=True)
    mappings.add_argument("--timeout", type=int, default=600)
    mappings.add_argument("--preserve-existing-loader", action="store_true")
    mappings.add_argument("--recovery-root")
    mappings.set_defaults(run=prepare_mappings)
    mapping_check = commands.add_parser("mapping-status")
    mapping_check.add_argument("--game-root", required=True)
    mapping_check.add_argument("--recovery-root")
    mapping_check.set_defaults(run=mapping_preflight)
    recovery = commands.add_parser("recover-mappings")
    recovery.add_argument("--journal", required=True)
    recovery.set_defaults(run=recover_mappings)
    identify = commands.add_parser("show-build")
    identify.add_argument("--game-root", required=True)
    identify.set_defaults(run=show_build)
    create = commands.add_parser("build")
    create.add_argument("--game-root", required=True)
    create.add_argument("--generated-root", required=True)
    create.add_argument("--output", required=True)
    create.add_argument("--mapping")
    create.add_argument("--tool-versions")
    create.add_argument("--tool-versions-file")
    create.add_argument("--recipe-version", type=int, default=EXTRACTION_RECIPE_VERSION)
    create.add_argument("--profile")
    create.set_defaults(run=build)
    bundle = commands.add_parser("bundle")
    bundle.add_argument("--game-root", required=True)
    bundle.add_argument("--generated-root", required=True)
    bundle.add_argument("--output", required=True)
    bundle.add_argument("--mapping")
    bundle.add_argument("--tool-versions")
    bundle.add_argument("--tool-versions-file")
    bundle.add_argument("--recipe-version", type=int, default=EXTRACTION_RECIPE_VERSION)
    bundle.add_argument("--profile")
    bundle.set_defaults(run=build)
    parts = commands.add_parser("bundle-set")
    parts.add_argument("--bundle", required=True)
    parts.add_argument("--output-dir", required=True)
    parts.add_argument("--max-part-bytes", type=int, default=DEFAULT_PART_BYTES)
    parts.add_argument("--replace", action="store_true")
    parts.set_defaults(run=bundle_set)
    check = commands.add_parser("validate")
    source = check.add_mutually_exclusive_group(required=True)
    source.add_argument("--generated-root")
    source.add_argument("--bundle")
    check.set_defaults(run=validate)
    activate = commands.add_parser("install")
    activate.add_argument("--bundle", required=True)
    activate.add_argument("--destination", required=True)
    activate.set_defaults(run=install)
    install_bundle = commands.add_parser("install-bundle")
    install_bundle.add_argument("--bundle", required=True)
    install_bundle.add_argument("--destination", required=True)
    install_bundle.set_defaults(run=install)
    install_bundle_set = commands.add_parser("install-bundle-set")
    install_bundle_set.add_argument("--index", required=True)
    install_bundle_set.add_argument("--destination", required=True)
    install_bundle_set.set_defaults(run=install_set)
    cleanup = commands.add_parser("cleanup-work")
    cleanup.add_argument("--work-root", required=True)
    cleanup.add_argument("--pending-journal")
    cleanup.add_argument("--attempts", type=int, default=8)
    cleanup.set_defaults(run=cleanup_work)
    args = parser.parse_args()
    return args.run(args)


if __name__ == "__main__":
    raise SystemExit(main())
