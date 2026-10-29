#!/usr/bin/env python3
"""Windows-client extraction orchestrator owned by Zanik."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
from pathlib import PurePosixPath
import shutil
import subprocess
import sys
import tempfile
import re
import time
import zipfile

try:
    from asset_filesystem import extended_path, read_json as read_json_file
except ModuleNotFoundError:  # Imported as tools.* by repository tests.
    from tools.asset_filesystem import extended_path, read_json as read_json_file


RECIPE_VERSION = 13
GAME_EXE = Path("RSDragonwilds/Binaries/Win64/RSDragonwilds-Win64-Shipping.exe")
EXPORT_CHECKPOINT = "export-checkpoint.json"
INCREMENTAL_STATE = "incremental-state.json"
INCREMENTAL_STATE_SCHEMA_VERSION = 2
GENERATION_STATE = "generation-state.json"
GENERATION_GRAPH_VERSION = 1
MAP_ICON_TARGETS = {
    "package:RSDragonwilds/Content/Art/UI/Icons/T_LodestoneMapIcon",
    "basename:T_Icon_Clipboard",
    "basename:T_Icon_ObjectiveArrow",
    # Retained for the exact reviewed recipe-7 -> recipe-8 removal. The
    # fragment-crystal texture is no longer used for nodestone map markers.
    "package:RSDragonwilds/Content/Art/UI/NavIcons/T_DR_Icon_Compass_FragmentCrystal",
    "basename:T_Icon_Gravestone",
}
GENERATION_GROUPS = (
    "buildings", "plan-items", "npc", "resources", "quests", "item-artwork",
    "npc-artwork", "resource-artwork", "spell-and-perk-artwork", "skills", "map-icons", "world-map", "font",
    "quest-badges", "recipes", "quest-destinations",
)
REVIEWED_LEGACY_GENERATION_RECIPES = {
    "fee0d17d8c0d51276ec4d9f62939fd1dade779905f5e403f174d819325b5f412",  # recipe 6
    "bdf48eadbc433e4680f1cb2485b75accd2169498d5277616828992a94bf566a7",  # recipe 7
    "6956f42170b02bf55190521492f867e2800dfcc5666806dda8023022ca0f2459",  # recipe 8
    "7a36e2a1feeab1530b5235f814167722c11bce85f39f37c1c0050aa2828b78b5",  # recipe 9
    "b472ea5a3d7e02a4b0abbb5487188f5b310e7dc0a205e0377b020cb755551012",  # recipe 10
    "0b9898ed11bc9a8afc00c0c3e05370bbbd8e022af8dfff32e4510ef6e8610bad",  # recipe 11
    "ae31d34ea86b57ccad911eb872c5e4b92c0f475396e927715ede1778faf5874d",  # recipe 12
    "a8a4ae9a1078cb5c505b32fa4c0821a8295a5d13b58d5dc57db73a3c3d645f22",  # recipe 13
}
RETIRED_FRAGMENT_CRYSTAL_TARGET = \
    "package:RSDragonwilds/Content/Art/UI/NavIcons/T_DR_Icon_Compass_FragmentCrystal"
# These reviewed recipe fixtures were added explicitly and were proven absent
# from the retained catalog reference inventory. This also upgrades recipe-10
# schema-1 workspaces without making users rescan 17,000+ JSON exports.
REVIEWED_STANDALONE_TEXTURE_TARGETS = {
    "basename:T_Icon_Clipboard",
    "basename:T_Icon_ObjectiveArrow",
    RETIRED_FRAGMENT_CRYSTAL_TARGET,
}
GROUP_OUTPUTS = {
    "buildings": ("buildings",), "plan-items": ("plan-items.tsv",),
    "npc": ("npc-catalog.json", "npc-identities.tsv"),
    "resources": ("resource-catalog.json", "resource-identities.tsv"),
    "quests": ("quest-catalog.json", "quest-runtime-catalog.tsv"),
    "item-artwork": ("items",), "npc-artwork": ("npcs",),
    "resource-artwork": ("resources",), "spell-and-perk-artwork": ("spells-and-perks",), "skills": ("skills",),
    "map-icons": ("lodestone-icon.png", "nodestone-icon.png", "gravestone-icon.png",
                  "quests/clipboard-icon.png", "quests/objective-arrow-icon.png"),
    "world-map": ("world-map.jpg",), "font": ("Jancient-Regular.ttf",),
    "quest-badges": ("quests/main-quest.svg", "quests/side-quest.svg"),
    "recipes": ("recipe-unlocks.json", "recipe-runtime-catalog.tsv", "recipe-runtime-catalog-history.tsv"),
    "quest-destinations": ("quest-step-locations.json",),
}


def sha256(path: Path) -> str:
    value = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            value.update(block)
    return value.hexdigest()


def read_operational_json(path: Path) -> dict:
    value = read_json_file(path)
    if not isinstance(value, dict):
        raise ValueError(f"operational JSON must contain an object: {path}")
    return value


def write_json_atomic(path: Path, value: dict) -> None:
    temporary = path.with_suffix(path.suffix + ".new")
    temporary.write_text(json.dumps(value, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    temporary.replace(path)


def emit_progress(message: str, *, detail: str = "", phase: str = "extract",
                  overall: int | None = None, current: int | None = None,
                  total: int | None = None, log: bool = True) -> None:
    payload: dict[str, object] = {"phase": phase, "message": message,
                                 "headline": message, "detail": detail, "log": log}
    if overall is not None: payload["overallPercent"] = overall
    if current is not None: payload["taskCurrent"] = current
    if total is not None: payload["taskTotal"] = total
    print("ZANIK_EVENT " + json.dumps(payload, separators=(",", ":")), flush=True)


def remove_tree(path: Path) -> None:
    def handle_error(function, target, error):
        if isinstance(error[1], FileNotFoundError):
            return
        raise error[1]
    for attempt in range(20):
        try:
            shutil.rmtree(extended_path(path), onerror=handle_error)
            return
        except FileNotFoundError:
            return
        except OSError:
            if attempt == 19:
                raise
            time.sleep(0.5)


def _safe_zip_name(value: str) -> str:
    normalized = value.replace("\\", "/")
    path = PurePosixPath(normalized)
    if path.is_absolute() or ".." in path.parts or not path.parts:
        raise RuntimeError(f"unsafe cached bundle path: {value}")
    return path.as_posix()


def seed_validated_baseline(bundle: Path, destination: Path, expected_recipe_sha: str,
                            expected_executable_sha: str) -> dict:
    """Materialize a previously validated public tree without trusting ZIP metadata."""
    if destination.exists(): remove_tree(destination)
    destination.mkdir(parents=True)
    with zipfile.ZipFile(bundle) as archive:
        members = [item for item in archive.infolist() if not item.is_dir()]
        names = {_safe_zip_name(item.filename): item for item in members}
        if len(names) != len(members):
            raise RuntimeError("cached baseline contains duplicate ZIP paths")
        if any((item.external_attr >> 16) & 0o170000 == 0o120000 for item in members):
            raise RuntimeError("cached baseline contains a symbolic link")
        if "asset-manifest.json" not in names:
            raise RuntimeError("cached baseline has no asset manifest")
        manifest = json.loads(archive.read(names["asset-manifest.json"]))
        executable = manifest.get("sourceExecutable") or {}
        if (manifest.get("schemaVersion") != 2 or manifest.get("compatibility") != "compatible"
                or manifest.get("extractionStage") != "validated-and-packaged"
                or manifest.get("extractionRecipeSha256") != expected_recipe_sha
                or executable.get("sha256") != expected_executable_sha):
            raise RuntimeError("cached baseline does not match the retained extraction state")
        expected = {record.get("path"): record for record in manifest.get("files", [])
                    if isinstance(record, dict)}
        if not expected or set(expected) != (set(names) - {"asset-manifest.json"}):
            raise RuntimeError("cached baseline manifest does not describe the complete ZIP")
        for relative, record in expected.items():
            safe = _safe_zip_name(relative)
            data = archive.read(names[safe])
            if len(data) != record.get("size") or hashlib.sha256(data).hexdigest() != record.get("sha256"):
                raise RuntimeError(f"cached baseline failed validation for {safe}")
            target = destination / Path(*PurePosixPath(safe).parts)
            target.parent.mkdir(parents=True, exist_ok=True); target.write_bytes(data)
    emit_progress("Reusing validated generated assets", detail=f"Verified {len(expected):,} cached files")
    return manifest


def _link_or_copy(source: Path | str, destination: Path | str) -> str:
    """Clone a private cache file without duplicating its bytes when possible."""
    source = Path(source); destination = Path(destination)
    destination.parent.mkdir(parents=True, exist_ok=True)
    try:
        os.link(extended_path(source), extended_path(destination))
    except OSError:
        shutil.copy2(extended_path(source), extended_path(destination))
    return str(destination)


def seed_validated_private_tree(bundle: Path, source: Path, destination: Path,
                                expected_recipe_sha: str,
                                expected_executable_sha: str) -> dict:
    """Clone a retained tree after proving it matches the cached bundle manifest."""
    metadata_path = bundle.parent / "cache.json"
    if not source.is_dir() or not metadata_path.is_file():
        raise RuntimeError("no authenticated retained generated tree is available")
    metadata = read_operational_json(metadata_path)
    with zipfile.ZipFile(bundle) as archive:
        members = [item for item in archive.infolist() if not item.is_dir()]
        names = {_safe_zip_name(item.filename): item for item in members}
        if len(names) != len(members) or "asset-manifest.json" not in names:
            raise RuntimeError("cached baseline has an invalid ZIP file set")
        manifest = json.loads(archive.read(names["asset-manifest.json"]))
    canonical = json.dumps(manifest, sort_keys=True, separators=(",", ":"))
    if hashlib.sha256(canonical.encode()).hexdigest() != metadata.get("manifestHash"):
        raise RuntimeError("cached baseline manifest no longer matches its cache record")
    executable = manifest.get("sourceExecutable") or {}
    if (manifest.get("schemaVersion") != 2 or manifest.get("compatibility") != "compatible"
            or manifest.get("extractionStage") != "validated-and-packaged"
            or manifest.get("extractionRecipeSha256") != expected_recipe_sha
            or executable.get("sha256") != expected_executable_sha):
        raise RuntimeError("cached baseline does not match the retained extraction state")
    records = {record.get("path"): record for record in manifest.get("files", [])
               if isinstance(record, dict) and isinstance(record.get("path"), str)}
    actual = {path.relative_to(source).as_posix(): path for path in source.rglob("*")
              if path.is_file() and not path.is_symlink()}
    if not records or set(records) != set(actual):
        raise RuntimeError("retained generated tree does not match the cached bundle file set")
    for relative, record in records.items():
        path = actual[relative]
        if path.stat().st_size != record.get("size") or sha256(path) != record.get("sha256"):
            raise RuntimeError(f"retained generated tree failed validation for {relative}")
    if destination.exists(): remove_tree(destination)
    shutil.copytree(source, destination, copy_function=_link_or_copy)
    emit_progress("Reusing validated generated assets",
                  detail=f"Verified and linked {len(records):,} retained files")
    return manifest


def generation_contracts(scripts: Path, repo: Path, recipe: dict) -> dict[str, str]:
    """Fingerprint generator code and authored inputs for independent output groups."""
    shared = scripts / "generate_web_assets.py"
    sources: dict[str, list[Path]] = {
        "buildings": [scripts / "generate_building_catalog.py"],
        "plan-items": [scripts / "generate_plan_item_catalog.py"],
        "npc": [scripts / "generate_world_catalogs.py"],
        "resources": [scripts / "generate_world_catalogs.py"],
        "quests": [scripts / "generate_quest_catalog.py", repo / "deploy/quest-overrides.json"],
        "item-artwork": [shared], "npc-artwork": [shared], "resource-artwork": [shared],
        "spell-and-perk-artwork": [shared, scripts / "generate_activity_unlock_catalog.py"],
        "skills": [shared],
        "map-icons": [shared, repo / "asset-tools/artwork/nodestone-icon-map.png"],
        "world-map": [shared], "font": [shared],
        "quest-badges": [shared],
        "recipes": [scripts / "generate_recipe_unlock_catalog.py", repo / "asset-tools/catalog-rules.json",
                    repo / "asset-tools/recipe-runtime-catalog-history.tsv"],
        "quest-destinations": [scripts / "analyze_quest_steps.py", repo / "deploy/quest-step-overrides.json"],
    }
    snapshot = recipe_snapshot(recipe)
    catalog_counts = (recipe.get("catalogProfile") or {}).get("catalogCounts", {})
    count_prefixes = {
        "buildings": ("buildings/",), "npc": ("npc-catalog.",),
        "resources": ("resource-catalog.",), "quests": ("quest-catalog.",),
        "recipes": ("recipe-unlocks.",), "quest-destinations": ("quest-step-locations.",),
    }
    result = {}
    for group, paths in sources.items():
        digest = hashlib.sha256(group.encode())
        for path in paths:
            digest.update(path.as_posix().encode())
            digest.update(path.read_bytes() if path.is_file() else b"<missing>")
        if group in count_prefixes:
            reviewed = {key: value for key, value in catalog_counts.items()
                        if key.startswith(count_prefixes[group])}
            digest.update(json.dumps({"jsonTargets": snapshot.get("jsonTargets", []),
                                      "reviewedCounts": reviewed}, sort_keys=True).encode())
        elif group == "map-icons":
            digest.update(json.dumps(sorted(set(snapshot.get("requiredTextureAssets", [])) & MAP_ICON_TARGETS)).encode())
        elif group == "font":
            digest.update(str(snapshot.get("fontFilter", "")).encode())
        result[group] = digest.hexdigest()
    return result


def selective_generation_groups(previous_recipe: dict, recipe: dict, prior_state: dict | None,
                                 contracts: dict[str, str], added_textures: list[str],
                                 removed_textures: list[str], legacy_reviewed: bool) -> set[str]:
    """Return dirty groups; uncertainty deliberately expands rather than narrows work."""
    if prior_state and prior_state.get("schemaVersion") == 1 \
            and prior_state.get("graphVersion") == GENERATION_GRAPH_VERSION:
        recorded = prior_state.get("contracts", {})
        dirty = {group for group in GENERATION_GROUPS if recorded.get(group) != contracts.get(group)}
    elif legacy_reviewed:
        dirty = set()
    else:
        return set(GENERATION_GROUPS)
    comparison_fields = ("schemaVersion", "engine", "jsonFilter", "fontFilter",
                         "requiredTextureAssets", "catalogProfile", "jsonTargets")
    old = {key: previous_recipe.get(key) for key in comparison_fields}
    new = recipe_snapshot(recipe)
    normalize_target = lambda value: value if value.startswith(("package:", "basename:")) else f"basename:{value}"
    old_required = {normalize_target(value) for value in (old.pop("requiredTextureAssets", []) or [])}
    new_required = {normalize_target(value) for value in (new.pop("requiredTextureAssets", []) or [])}
    old.pop("catalogProfile", None); new.pop("catalogProfile", None)
    current_sha = sha256(Path(recipe["_path"])) if recipe.get("_path") else ""
    # Reviewed recipe-only migrations must not manufacture catalog
    # invalidations. Recipe 6 -> 7 added the old fragment-crystal target;
    # recipe 7 -> 8 removes it in favor of bundled Zanik-authored artwork.
    if legacy_reviewed and current_sha in REVIEWED_LEGACY_GENERATION_RECIPES:
        old.pop("schemaVersion", None); new.pop("schemaVersion", None)
    reviewed_map_only_migration = (
        legacy_reviewed
        and current_sha in REVIEWED_LEGACY_GENERATION_RECIPES
        and old == new
        and bool(old_required ^ new_required)
        and old_required ^ new_required <= MAP_ICON_TARGETS
    )
    if reviewed_map_only_migration:
        # generate_web_assets.py owns several independent groups, so changing
        # its map-icon branch changes their code contract mechanically. This
        # exact reviewed migration affects only map-icons; accept the previous
        # validated bytes for every unrelated group and record the new hashes.
        dirty.intersection_update({"map-icons"})
    prior_recipe_version = previous_recipe.get("schemaVersion")
    reviewed_spell_and_perk_migration = (
        legacy_reviewed
        and current_sha == "a8a4ae9a1078cb5c505b32fa4c0821a8295a5d13b58d5dc57db73a3c3d645f22"
        and prior_recipe_version in {11, 12}
        and recipe.get("schemaVersion") == 13
        and old == new
        and old_required == new_required
        and not added_textures
        and not removed_textures
    )
    if old != new:
        dirty.update({"buildings", "plan-items", "npc", "resources", "quests", "recipes",
                      "quest-destinations", "item-artwork", "npc-artwork", "resource-artwork", "spell-and-perk-artwork"})
    texture_changes = set(added_textures) | set(removed_textures) | (old_required ^ new_required)
    if texture_changes:
        if texture_changes <= MAP_ICON_TARGETS:
            dirty.add("map-icons")
        else:
            dirty.update({"buildings", "plan-items", "item-artwork", "npc-artwork",
                          "resource-artwork", "spell-and-perk-artwork", "skills", "map-icons", "world-map"})
    dependencies = {"item-artwork": {"plan-items"}, "npc-artwork": {"npc"},
                    "resource-artwork": {"resources"},
                    "recipes": {"buildings", "plan-items", "item-artwork"},
                    "quest-destinations": {"quests", "npc"}}
    changed = True
    while changed:
        changed = False
        for group, parents in dependencies.items():
            if parents & dirty and group not in dirty: dirty.add(group); changed = True
    if reviewed_spell_and_perk_migration:
        # Recipe 13 uses the final hyphenated folder. Recipe 11 also needs the
        # private NPC identity update that first appeared with recipe 12.
        migration_groups = {"spell-and-perk-artwork"}
        if prior_recipe_version == 11:
            migration_groups.add("npc")
        dirty.intersection_update(migration_groups)
        dirty.update(migration_groups)
    return dirty


def remove_group_outputs(web: Path, groups: set[str]) -> None:
    for group in groups:
        for relative in GROUP_OUTPUTS[group]:
            target = web / relative
            if target.is_dir(): remove_tree(target)
            elif target.exists(): target.unlink()


def output_digest(web: Path, group: str) -> str:
    value = hashlib.sha256()
    found = 0
    for relative in GROUP_OUTPUTS[group]:
        target = web / relative
        paths = sorted((path for path in target.rglob("*") if path.is_file()), key=lambda path: path.as_posix()) \
            if target.is_dir() else ([target] if target.is_file() else [])
        for path in paths:
            name = path.relative_to(web).as_posix()
            value.update(name.encode()); value.update(sha256(path).encode()); found += 1
    if not found: raise RuntimeError(f"generated output group is empty: {group}")
    return value.hexdigest()


def validate_reviewed_counts(actual: dict[str, int], expected: dict[str, int]) -> None:
    """Require the reviewed raw-export floor without rejecting a richer mapping.

    Raw UAS coverage can increase when a newly generated mapping resolves more
    packages from the same cooked build. Exact public catalog counts are checked
    after generation, where additions and removals are meaningful.
    """
    drift = [f"{key} expected at least {value}, got {actual.get(key)}"
             for key, value in expected.items()
             if not isinstance(actual.get(key), int) or actual[key] < value]
    if drift:
        raise RuntimeError("reviewed export coverage decreased: " + "; ".join(drift))


def validate_catalog_export(json_root: Path, expected_counts: dict[str, int] | None = None) -> None:
    from generate_building_catalog import REMOVED_RUNTIME_PIECES, asset_key
    try:
        catalogue_path = next(json_root.rglob("DA_BuildPieceCatalogue_Default.json"))
    except StopIteration as error:
        raise RuntimeError("targeted export omitted the building catalogue") from error
    records = read_json_file(catalogue_path)
    if not isinstance(records, list) or not records:
        raise RuntimeError("targeted export produced a malformed building catalogue")
    required: set[str] = set()
    for page in records[0].get("Properties", {}).get("Pages", []):
        for section in page.get("Collection", []):
            for reference in section.get("Collection", []):
                key = asset_key(reference.get("AssetPathName", ""))
                if key and key not in REMOVED_RUNTIME_PIECES:
                    required.add(key)
    found: set[str] = set()
    cells = 0
    json_files = 0
    scan_started = time.monotonic()
    for path in json_root.rglob("*.json"):
        json_files += 1
        if json_files == 1 or json_files % 1000 == 0:
            emit_progress("Checking targeted catalog coverage",
                          detail=(f"Scanned {json_files:,} exported JSON files · "
                                  f"{int(time.monotonic() - scan_started)}s elapsed"),
                          current=json_files, log=False)
        normalized = path.as_posix()
        if "L_World/_Generated_/" in normalized:
            cells += 1
        if path.stem not in required:
            continue
        try:
            values = read_json_file(path)
        except (OSError, UnicodeError, json.JSONDecodeError) as error:
            raise RuntimeError(f"cannot read exported JSON {path.name}: {error}") from error
        for value in values if isinstance(values, list) else []:
            if value.get("Type") == "BuildingPieceData" and value.get("Name") in required:
                found.add(value["Name"])
    missing = sorted(required - found)
    if missing:
        raise RuntimeError(f"targeted export omitted {len(missing)} building records; first: {missing[0]}")
    if cells < 1100:
        raise RuntimeError(f"targeted export contains only {cells} world-partition cells; expected at least 1100")
    if expected_counts:
        actual = {"jsonFiles": json_files, "worldPartitionCells": cells}
        validate_reviewed_counts(actual, expected_counts)


def run(command: list[str], label: str, *, quiet: bool = False) -> None:
    print(f"[Zanik] {label}", flush=True)
    emit_progress(label, detail="Starting this step", log=False)
    started = time.monotonic()
    completed = subprocess.Popen(
        command,
        stdout=subprocess.DEVNULL if quiet else None,
        stderr=subprocess.STDOUT if quiet else None,
    )
    output = None
    if "-o" in command:
        try: output = Path(command[command.index("-o") + 1])
        except (ValueError, IndexError): output = None
    while completed.poll() is None:
        elapsed = int(time.monotonic() - started)
        discovered = 0
        if output and output.is_dir():
            try: discovered = sum(1 for path in output.rglob("*") if path.is_file())
            except OSError: discovered = 0
        is_uas_scan = any(Path(str(part)).name.casefold() == "uas.exe" for part in command[:1])
        detail = (("Scanning cooked containers · " if is_uas_scan else "") +
                  f"{elapsed // 60}:{elapsed % 60:02d} elapsed")
        if discovered: detail += f" · {discovered:,} output files currently present"
        emit_progress(label, detail=detail, current=discovered or None, log=False)
        time.sleep(2)
    if completed.returncode:
        raise RuntimeError(f"{label} failed with exit code {completed.returncode}")
    emit_progress(label, detail=f"Completed in {int(time.monotonic() - started)} seconds", log=False)


def update_stage(path: Path | None, stage: str) -> None:
    if path is None:
        return
    value = read_operational_json(path)
    value["stage"] = stage
    value["updatedAt"] = time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime())
    write_json_atomic(path, value)


def locate(root: Path, name: str) -> Path:
    matches = list(root.rglob(name))
    if len(matches) != 1:
        raise RuntimeError(f"expected one {name} in {root}, found {len(matches)}")
    return matches[0]


def cooked_package_path(value: str) -> str | None:
    """Translate an Unreal soft-object reference to its exact cooked package path."""
    match = re.search(r"(/[^'\"]+?)(?:\.[^/'\"]+)?(?:'|$)", value)
    if not match:
        return None
    package = match.group(1)
    if package.startswith("/Game/"):
        return "RSDragonwilds/Content/" + package[len("/Game/"):]
    parts = package.strip("/").split("/", 1)
    if len(parts) == 2:
        return f"RSDragonwilds/Plugins/GameFeatures/{parts[0]}/Content/{parts[1]}"
    return None


def referenced_textures(json_root: Path, required: list[str]) -> list[str]:
    targets = {name if name.startswith(("package:", "basename:")) else f"basename:{name}"
               for name in required}
    def web_texture_package(reference: object) -> str:
        package = cooked_package_path(str(reference or ""))
        normalized = package.replace("\\", "/").casefold()
        leaf = Path(package).name.casefold()
        if (leaf.startswith("t_") or "/generatedicons/" in normalized
                or "/proceduralthumbnails/" in normalized):
            return package
        return ""
    def visit(value: object, key: str = "") -> None:
        if isinstance(value, dict):
            for field in ("Icon", "DisplayIcon", "Image"):
                display = value.get(field)
                if not isinstance(display, dict):
                    continue
                reference = display.get("AssetPathName") or display.get("ObjectPath")
                package = web_texture_package(reference)
                if package:
                    targets.add("package:" + package)
            for child_key, child in value.items(): visit(child, str(child_key))
        elif isinstance(value, list):
            for child in value: visit(child, key)
        elif isinstance(value, str) and key in {"AssetPathName", "ObjectPath", "ObjectName"}:
            match = re.search(r"/(T_(?:Icon|Journal|Skill|Tag|WorldMap)[^/.]*)\.", value, re.I)
            if match:
                package = cooked_package_path(value)
                targets.add("package:" + package if package else "basename:" + match.group(1))
    for path in json_root.rglob("*.json"):
        if path.name == ".uas-manifest.json": continue
        try: visit(read_json_file(path))
        except (OSError, UnicodeError, json.JSONDecodeError) as error:
            raise RuntimeError(f"cannot read exported JSON {path.name}: {error}") from error
    package_basenames = {
        Path(target.split(":", 1)[1]).name.casefold()
        for target in targets if target.startswith("package:")
    }
    targets = {
        target for target in targets
        if not (target.startswith("basename:") and target.split(":", 1)[1].casefold() in package_basenames)
    }
    return sorted(targets, key=str.casefold)


def filter_chunks(targets: list[str], maximum: int = 28000) -> list[str]:
    chunks: list[list[str]] = [[]]
    length = 20
    for target in targets:
        kind, value = target.split(":", 1)
        encoded = ("^" + re.escape(value) if kind == "package"
                   else r"(?:^|/)" + re.escape(value))
        if chunks[-1] and length + len(encoded) + 1 > maximum:
            chunks.append([]); length = 20
        chunks[-1].append(encoded); length += len(encoded) + 1
    return ["(?:" + "|".join(chunk) + r")\.uasset$" for chunk in chunks if chunk]


def _recipe_history(recipe: dict) -> list[dict]:
    history_path = Path(recipe["_path"]).with_name("extraction-recipe-history.json")
    if not history_path.is_file():
        return []
    history = read_operational_json(history_path)
    records = history.get("recipes", [])
    return records if isinstance(records, list) else []


def recipe_snapshot(recipe: dict) -> dict:
    """Retain only Zanik-authored fields that determine extraction work."""
    snapshot = {key: recipe.get(key) for key in (
        "schemaVersion", "engine", "jsonFilter", "fontFilter", "requiredTextureAssets", "catalogProfile")}
    recipe_sha = sha256(Path(recipe["_path"]))
    for record in _recipe_history(recipe):
        if record.get("sha256") == recipe_sha and isinstance(record.get("jsonTargets"), list):
            snapshot["jsonTargets"] = record["jsonTargets"]
            break
    return snapshot


def normalized_required_texture_targets(recipe: dict) -> set[str]:
    return {
        value if value.startswith(("package:", "basename:")) else f"basename:{value}"
        for value in recipe.get("requiredTextureAssets", [])
    }


def incremental_texture_targets(old_targets: list[str], previous_recipe: dict,
                                recipe: dict, json_root: Path,
                                standalone_targets: list[str] | None = None) -> list[str]:
    """Avoid fully parsing every retained JSON file for narrow recipe changes.

    Unchanged catalog selectors preserve every previously discovered reference.
    Explicit additions can be unioned directly. For removals, a cheap byte search
    proves that each retired basename is absent from retained JSON. Any possible
    reference falls back to the complete semantic scan, preserving correctness.
    """
    old_required = normalized_required_texture_targets(previous_recipe)
    new_required = normalized_required_texture_targets(recipe)
    if previous_recipe.get("jsonTargets") == recipe_snapshot(recipe).get("jsonTargets"):
        removed = old_required - new_required
        if not removed:
            return sorted(set(old_targets) | (new_required - old_required), key=str.casefold)
        known_standalone = set(standalone_targets or ()) | REVIEWED_STANDALONE_TEXTURE_TARGETS
        uncertain = removed - known_standalone
        if not uncertain:
            return sorted((set(old_targets) - removed) | (new_required - old_required), key=str.casefold)
        needles = {
            target: Path(target.split(":", 1)[1]).name.casefold().encode("utf-8")
            for target in uncertain
        }
        possible_references: set[str] = set()
        for path in json_root.rglob("*.json"):
            if path.name == ".uas-manifest.json":
                continue
            try:
                payload = path.read_bytes().lower()
            except OSError as error:
                raise RuntimeError(f"cannot inspect exported JSON {path.name}: {error}") from error
            possible_references.update(target for target, needle in needles.items() if needle in payload)
            if possible_references == uncertain:
                break
        if not possible_references:
            return sorted((set(old_targets) - removed) | (new_required - old_required), key=str.casefold)
    return referenced_textures(json_root, recipe["requiredTextureAssets"])


def load_prior_incremental_state(temporary: Path, recipe: dict,
                                 stage_journal: Path | None) -> dict | None:
    state_path = temporary / INCREMENTAL_STATE
    if state_path.is_file():
        value = read_operational_json(state_path)
        if value.get("schemaVersion") in {1, INCREMENTAL_STATE_SCHEMA_VERSION} \
                and isinstance(value.get("recipe"), dict):
            if value.get("schemaVersion") == 1:
                old_required = normalized_required_texture_targets(value["recipe"])
                value["standaloneTextureTargets"] = sorted(
                    old_required & REVIEWED_STANDALONE_TEXTURE_TARGETS, key=str.casefold)
            return value
    # Upgrade completed caches produced before incremental-state.json existed.
    journal = read_operational_json(stage_journal) if stage_journal and stage_journal.is_file() else {}
    previous = journal.get("previousFingerprints", {})
    previous_sha = previous.get("recipeSha256")
    current_sha = sha256(Path(recipe["_path"]))
    old_recipe = recipe_snapshot(recipe) if previous_sha in {None, current_sha} else None
    if old_recipe is None:
        for record in _recipe_history(recipe):
            if record.get("sha256") == previous_sha and isinstance(record.get("recipe"), dict):
                old_recipe = dict(record["recipe"])
                if isinstance(record.get("jsonTargets"), list):
                    old_recipe["jsonTargets"] = record["jsonTargets"]
                break
    json_root = temporary / "json"
    if old_recipe is None or not json_root.is_dir():
        return None
    texture_root = temporary / "textures"
    targets = texture_target_inventory(texture_root) if texture_root.is_dir() else referenced_textures(
        json_root, old_recipe.get("requiredTextureAssets", []))
    old_required = normalized_required_texture_targets(old_recipe)
    return {"schemaVersion": INCREMENTAL_STATE_SCHEMA_VERSION,
            "recipeSha256": previous_sha or current_sha,
            "recipe": old_recipe,
            "textureTargets": targets,
            "standaloneTextureTargets": sorted(
                old_required & REVIEWED_STANDALONE_TEXTURE_TARGETS, key=str.casefold)}


def write_incremental_state(temporary: Path, recipe: dict, targets: list[str],
                            stage_journal: Path | None,
                            standalone_targets: list[str] | None = None) -> None:
    fingerprints = read_operational_json(stage_journal).get("fingerprints", {}) if stage_journal else {}
    write_json_atomic(temporary / INCREMENTAL_STATE, {
        "schemaVersion": INCREMENTAL_STATE_SCHEMA_VERSION,
        "recipeSha256": sha256(Path(recipe["_path"])),
        "recipe": recipe_snapshot(recipe),
        "textureTargets": targets,
        "standaloneTextureTargets": sorted(set(standalone_targets or ()), key=str.casefold),
        "fingerprints": fingerprints,
    })


def texture_matches_target(image: Path, texture_root: Path, target: str) -> bool:
    parts = image.relative_to(texture_root).parts
    if len(parts) < 2 or not parts[0].startswith("batch-"):
        return False
    kind, value = target.split(":", 1)
    return (Path(*parts[1:]).with_suffix("").as_posix().casefold() == value.casefold()
            if kind == "package" else image.stem.casefold() == value.casefold())


def texture_target_inventory(texture_root: Path) -> list[str]:
    targets: set[str] = set()
    for image in texture_root.rglob("*.png"):
        parts = image.relative_to(texture_root).parts
        if len(parts) < 2 or not parts[0].startswith("batch-"):
            continue
        if len(parts) == 2:
            targets.add("basename:" + image.stem)
        else:
            targets.add("package:" + Path(*parts[1:]).with_suffix("").as_posix())
    return sorted(targets, key=str.casefold)


def _json_export_name(path: Path, json_root: Path) -> str:
    relative = path.relative_to(json_root).with_suffix("").as_posix()
    lowered = relative.casefold()
    marker = "/content/"
    if marker in lowered:
        offset = lowered.index(marker) + len(marker) - 1
        prefix = relative[:offset]
        if "/plugins/gamefeatures/" in prefix.casefold():
            match = re.search(r"/plugins/gamefeatures/([^/]+)", prefix.replace("\\", "/"), re.I)
            if not match:
                raise RuntimeError(f"cannot derive GameFeatures package path from {relative}")
            plugin = match.group(1)
            return f"/GameFeatures/{plugin}/Content/{relative[offset + 1:]}"
        return "/" + relative[offset + 1:]
    return "/" + relative


def json_matches_target(path: Path, json_root: Path, target: str) -> bool:
    return re.search(target, _json_export_name(path, json_root), re.I) is not None


def apply_json_delta(common: list[str], temporary: Path, json_root: Path,
                     old_targets: list[str], new_targets: list[str]) -> dict[str, list[str]]:
    removed = sorted(set(old_targets) - set(new_targets), key=str.casefold)
    added = sorted(set(new_targets) - set(old_targets), key=str.casefold)
    removed_files = 0
    if removed:
        for exported in list(json_root.rglob("*.json")):
            if exported.name == ".uas-manifest.json":
                continue
            if (any(json_matches_target(exported, json_root, target) for target in removed)
                    and not any(json_matches_target(exported, json_root, target) for target in new_targets)):
                exported.unlink()
                removed_files += 1
    if removed:
        print(f"[Zanik] Removed {removed_files} cached catalog file(s) for {len(removed)} retired selector(s)", flush=True)
    if not added:
        print("[Zanik] Catalog selectors unchanged; retained JSON verified and UnrealAssetScout catalog pass skipped", flush=True)
        return {"added": [], "removed": removed}
    for target in added:
        identity = hashlib.sha256(target.encode("utf-8")).hexdigest()[:12]
        delta = temporary / f"json-delta-{identity}"
        if delta.exists():
            remove_tree(delta)
        run(common + ["-f", target, "export", "Json", "-b", "-o", str(delta), "-c", "-r",
                      "-l", str(temporary / f"json-delta-{identity}.log")],
            "Exporting one newly required catalog selector", quiet=True)
        _uas_manifest(delta / ".uas-manifest.json", "json")
        copied = 0
        for source in delta.rglob("*.json"):
            if source.name == ".uas-manifest.json":
                continue
            destination = json_root / source.relative_to(delta)
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(source, destination)
            copied += 1
        if copied == 0:
            raise RuntimeError(f"new catalog selector exported no JSON files: {target}")
        remove_tree(delta)
    print(f"[Zanik] Exported {len(added)} newly required catalog selector(s)", flush=True)
    return {"added": added, "removed": removed}


def apply_texture_delta(common: list[str], temporary: Path, texture_root: Path,
                        old_targets: list[str], new_targets: list[str]) -> dict[str, list[str]]:
    removed = sorted(set(old_targets) - set(new_targets), key=str.casefold)
    added = sorted(set(new_targets) - set(old_targets), key=str.casefold)
    if removed:
        for image in list(texture_root.rglob("*.png")):
            if (any(texture_matches_target(image, texture_root, target) for target in removed)
                    and not any(texture_matches_target(image, texture_root, target) for target in new_targets)):
                image.unlink()
        for batch in list(texture_root.iterdir()):
            if (batch.is_dir() and batch.name.startswith("batch-")
                    and not any(batch.rglob("*.png"))):
                remove_tree(batch)
        print(f"[Zanik] Removed {len(removed)} retired texture target(s) from the retained cache", flush=True)
    if not added:
        print("[Zanik] No new texture exports are required; UnrealAssetScout texture passes skipped", flush=True)
        return {"added": [], "removed": removed}
    additions = missing_texture_targets(texture_root, added)
    if not additions:
        print("[Zanik] No new texture exports are required; UnrealAssetScout texture passes skipped", flush=True)
        return {"added": [], "removed": removed}
    for expression in filter_chunks(additions):
        identity = hashlib.sha256(expression.encode("utf-8")).hexdigest()[:12]
        batch = texture_root / f"batch-delta-{identity}"
        run(common + ["-f", expression, "export", "Textures", "-o", str(batch), "-c", "-r",
                      "-l", str(temporary / f"texture-delta-{identity}.log")],
            f"Exporting {len(additions)} newly required web texture target(s)", quiet=True)
    return {"added": additions, "removed": removed}


def missing_texture_targets(texture_root: Path, targets: list[str]) -> list[str]:
    packages: set[str] = set()
    basenames: set[str] = set()
    for image in texture_root.rglob("*.png"):
        parts = image.relative_to(texture_root).parts
        if len(parts) < 2 or not parts[0].startswith("batch-"):
            continue
        packages.add(Path(*parts[1:]).with_suffix("").as_posix().casefold())
        basenames.add(image.stem.casefold())
    missing = []
    for target in targets:
        kind, value = target.split(":", 1)
        inventory = packages if kind == "package" else basenames
        if value.casefold() not in inventory:
            missing.append(target)
    return missing


def _uas_manifest(path: Path, mode: str) -> dict:
    if not path.is_file():
        raise RuntimeError(f"completed export is missing UnrealAssetScout manifest: {path}")
    try:
        value = json.loads(path.read_text(encoding="utf-8-sig"))
    except (OSError, UnicodeError, json.JSONDecodeError) as error:
        raise RuntimeError(f"completed export has an invalid UnrealAssetScout manifest: {path}") from error
    if not isinstance(value, dict) or value.get("schema") != 2 or value.get("mode") != mode:
        raise RuntimeError(f"completed export has an unexpected UnrealAssetScout manifest: {path}")
    return value


def verified_export_checkpoint(temporary: Path, recipe: dict, reviewed_counts: dict[str, int] | None,
                               mapping: Path, stage_journal: Path | None,
                               texture_targets: list[str] | None = None) -> dict:
    if not mapping.is_file():
        raise RuntimeError(f"resume workspace is missing its generated mapping: {mapping}")
    json_root = temporary / "json"; texture_root = temporary / "textures"; font_root = temporary / "font"
    if not json_root.is_dir() or not texture_root.is_dir() or not font_root.is_dir():
        raise RuntimeError("resume workspace does not contain completed JSON, texture, and font exports")
    validate_catalog_export(json_root, reviewed_counts)
    json_manifest = json_root / ".uas-manifest.json"
    _uas_manifest(json_manifest, "json")
    targets = texture_targets if texture_targets is not None else referenced_textures(
        json_root, recipe["requiredTextureAssets"])
    actual_batches = sorted(path.name for path in texture_root.iterdir()
                            if path.is_dir() and path.name.startswith("batch-"))
    if not actual_batches:
        raise RuntimeError("completed texture export contains no verified batches")
    batches = []
    texture_pngs = 0
    for name in actual_batches:
        batch = texture_root / name
        manifest = batch / ".uas-manifest.json"
        _uas_manifest(manifest, "textures")
        images = list(batch.rglob("*.png"))
        if not images:
            raise RuntimeError(f"completed texture export batch is empty: {name}")
        texture_pngs += len(images)
        batches.append({"name": name, "manifestSha256": sha256(manifest), "pngFiles": len(images)})
    missing_targets = missing_texture_targets(texture_root, targets)
    if missing_targets:
        preview = ", ".join(missing_targets[:10])
        suffix = f" (+{len(missing_targets) - 10} more)" if len(missing_targets) > 10 else ""
        raise RuntimeError(f"completed texture exports are missing {len(missing_targets)} target(s): {preview}{suffix}")
    font_manifest = font_root / ".uas-manifest.json"
    _uas_manifest(font_manifest, "raw")
    fonts = list(font_root.rglob("Jancient.ufont"))
    if len(fonts) != 1 or not fonts[0].is_file():
        raise RuntimeError(f"completed font export must contain exactly one Jancient.ufont; found {len(fonts)}")
    json_files = sum(1 for _ in json_root.rglob("*.json"))
    world_cells = sum(1 for path in json_root.rglob("*.json") if "L_World/_Generated_/" in path.as_posix())
    journal_fingerprints = read_operational_json(stage_journal).get("fingerprints", {}) if stage_journal else {}
    return {
        "schemaVersion": 1,
        "stage": "exports",
        "jsonFiles": json_files,
        "worldPartitionCells": world_cells,
        "json": {"manifestSha256": sha256(json_manifest)},
        "textureTargets": len(targets),
        "textureTargetInventorySha256": hashlib.sha256(
            json.dumps(targets, separators=(",", ":")).encode("utf-8")).hexdigest(),
        "texturePngs": texture_pngs,
        "textureBatches": batches,
        "font": {"manifestSha256": sha256(font_manifest), "path": "Jancient.ufont"},
        "mappingSha256": sha256(mapping),
        "recipeSha256": sha256(Path(recipe["_path"])),
        "fingerprints": journal_fingerprints,
    }


def recover_or_create_export_checkpoint(temporary: Path, recipe: dict,
                                        reviewed_counts: dict[str, int] | None,
                                        mapping: Path, stage_journal: Path | None) -> dict:
    checkpoint_path = temporary / EXPORT_CHECKPOINT
    actual = verified_export_checkpoint(temporary, recipe, reviewed_counts, mapping, stage_journal)
    if checkpoint_path.is_file():
        recorded = read_operational_json(checkpoint_path)
        if recorded != actual:
            raise RuntimeError("retained export checkpoint no longer matches the verified export workspace")
    else:
        write_json_atomic(checkpoint_path, actual)
    return actual


def complete_remaining_exports(common: list[str], temporary: Path, recipe: dict,
                               json_root: Path, texture_root: Path, font_root: Path,
                               *, incremental: bool = False) -> None:
    """Finish texture/font exports after a verified retained JSON-only stage."""
    targets = referenced_textures(json_root, recipe["requiredTextureAssets"])
    if not targets:
        raise RuntimeError("no web texture references were discovered in the targeted JSON export")
    if texture_root.exists() and not incremental:
        remove_tree(texture_root)
    if font_root.exists() and not incremental:
        remove_tree(font_root)
    chunks = filter_chunks(targets)
    expected_batches = {f"batch-{index:02d}" for index in range(1, len(chunks) + 1)}
    for index, expression in enumerate(chunks, 1):
        batch = texture_root / f"batch-{index:02d}"
        rebuild = [] if incremental else ["-r"]
        run(common + ["-f", expression, "export", "Textures", "-o", str(batch), "-c", *rebuild,
                      "-l", str(temporary / f"texture-export-{index:02d}.log")],
            f"Exporting targeted web textures ({index}/{len(chunks)})", quiet=True)
    if incremental and texture_root.is_dir():
        for stale in (path for path in texture_root.iterdir()
                      if path.is_dir() and path.name.startswith("batch-") and path.name not in expected_batches):
            remove_tree(stale)
    rebuild = [] if incremental else ["-r"]
    run(common + ["-f", recipe["fontFilter"], "export", "Raw", "-o", str(font_root), "-c", *rebuild,
                  "-l", str(temporary / "font-export.log")], "Exporting the web font", quiet=True)


def validate_font_export(font_root: Path) -> None:
    _uas_manifest(font_root / ".uas-manifest.json", "raw")
    fonts = list(font_root.rglob("Jancient.ufont"))
    if len(fonts) != 1 or not fonts[0].is_file():
        raise RuntimeError(f"completed font export must contain exactly one Jancient.ufont; found {len(fonts)}")


def reset_partial_generation(temporary: Path) -> None:
    """Remove reproducible output while preserving expensive verified exports."""
    for name in ("web", "internal"):
        path = temporary / name
        if path.exists():
            remove_tree(path)
    (temporary / "internal").mkdir(parents=True, exist_ok=True)


def extract(args: argparse.Namespace) -> int:
    game = args.game_root.resolve(); output = args.output.resolve(); tools = args.tool_root.resolve()
    paks = game / "RSDragonwilds/Content/Paks"; executable = game / GAME_EXE
    if not paks.is_dir() or not executable.is_file():
        raise ValueError("the selected directory is not a complete Windows Dragonwilds client")
    recipe = json.loads(args.recipe.read_text(encoding="utf-8"))
    recipe["_path"] = str(args.recipe.resolve())
    if recipe.get("schemaVersion") != RECIPE_VERSION:
        raise ValueError("unsupported extraction recipe")
    from zanik_assets import game_build
    catalog_profile = recipe.get("catalogProfile", {})
    reviewed_counts = (catalog_profile.get("exportCounts", {})
                       if str(game_build(game)) == str(catalog_profile.get("clientBuild")) else None)
    uas = locate(tools, "uas.exe")
    mapping = args.usmap_path.resolve()
    if not mapping.is_file(): raise ValueError(f"mapping file does not exist: {mapping}")
    work_parent = args.work_root.resolve() if args.work_root else output.parent
    work_parent.mkdir(parents=True, exist_ok=True)
    if os.name == "nt" and len(str(work_parent)) > 70:
        raise ValueError("the intermediate work root is too deep for cooked asset paths; use a short path such as $env:TEMP\\zw-assets")
    if args.resume_work or args.incremental_work:
        temporary = (args.resume_work or args.incremental_work).resolve()
    elif args.workspace:
        temporary = args.workspace.resolve()
        if temporary.exists() and any(temporary.iterdir()):
            raise ValueError(f"new extraction workspace is not empty: {temporary}")
        temporary.mkdir(parents=True, exist_ok=True)
    else:
        temporary = Path(tempfile.mkdtemp(prefix="zx-", dir=work_parent))
    keep = bool(args.keep_work or args.resume_work or args.incremental_work or args.workspace)
    stage_journal = args.stage_journal.resolve() if args.stage_journal else None
    try:
        json_root = temporary / "json"; texture_root = temporary / "textures"; font_root = temporary / "font"; web = temporary / "web"; internal = temporary / "internal"
        internal.mkdir(parents=True, exist_ok=True)
        common = [str(uas), "-p", str(paks), "-g", recipe["engine"], "-u", str(mapping)]
        previous_recipe: dict = {}; prior: dict | None = None; selective = False
        texture_delta = {"added": [], "removed": []}
        if args.resume_work:
            print("[Zanik] Validating retained extraction work", flush=True)
            checkpoint_path = temporary / EXPORT_CHECKPOINT
            if not checkpoint_path.is_file() and json_root.is_dir():
                validate_catalog_export(json_root, reviewed_counts)
                _uas_manifest(json_root / ".uas-manifest.json", "json")
                print("[Zanik] Retained JSON export verified; continuing with artwork and font exports", flush=True)
                complete_remaining_exports(common, temporary, recipe, json_root, texture_root, font_root)
            recover_or_create_export_checkpoint(temporary, recipe, reviewed_counts, mapping, stage_journal)
            write_incremental_state(temporary, recipe,
                                    referenced_textures(json_root, recipe["requiredTextureAssets"]),
                                    stage_journal)
            update_stage(stage_journal, "exports")
            print("[Zanik] Retained exports verified; UnrealAssetScout extraction skipped", flush=True)
        else:
            incremental = bool(args.incremental_work)
            prior = load_prior_incremental_state(temporary, recipe, stage_journal) if incremental else None
            previous_recipe = prior.get("recipe", {}) if prior else {}
            selective = bool(prior and not args.game_changed
                             and previous_recipe.get("engine") == recipe.get("engine"))
            last_export_error: Exception | None = None
            old_json_targets = previous_recipe.get("jsonTargets")
            new_json_targets = recipe_snapshot(recipe).get("jsonTargets")
            structural_json = (selective and isinstance(old_json_targets, list)
                               and isinstance(new_json_targets, list) and json_root.is_dir())
            if structural_json:
                apply_json_delta(common, temporary, json_root, old_json_targets, new_json_targets)
                validate_catalog_export(json_root, reviewed_counts)
                _uas_manifest(json_root / ".uas-manifest.json", "json")
            elif selective and previous_recipe.get("jsonFilter") == recipe.get("jsonFilter"):
                validate_catalog_export(json_root, reviewed_counts)
                _uas_manifest(json_root / ".uas-manifest.json", "json")
                print("[Zanik] Catalog extraction filter unchanged; retained JSON verified and UnrealAssetScout catalog pass skipped", flush=True)
            else:
                for attempt in range(1, 4):
                    if json_root.exists() and not incremental: remove_tree(json_root)
                    rebuild = [] if incremental else ["-r"]
                    run(common + ["-f", recipe["jsonFilter"], "export", "Json", "-b", "-o", str(json_root), "-c", *rebuild, "-l", str(temporary / f"json-export-{attempt}.log")], f"{'Updating' if incremental else 'Exporting'} targeted catalog data (attempt {attempt}/3)", quiet=True)
                    try:
                        validate_catalog_export(json_root, reviewed_counts)
                        last_export_error = None
                        break
                    except (OSError, ValueError, RuntimeError, json.JSONDecodeError) as error:
                        last_export_error = error
                        print(f"[Zanik] Catalog export attempt {attempt} was incomplete: {error}", flush=True)
            if last_export_error is not None:
                raise RuntimeError(f"targeted catalog export remained incomplete after three attempts: {last_export_error}")
            old_targets = prior.get("textureTargets", []) if prior else []
            old_required = normalized_required_texture_targets(previous_recipe)
            new_required = normalized_required_texture_targets(recipe)
            old_standalone = set(prior.get("standaloneTextureTargets", [])) if prior else set()
            target_inputs_unchanged = (structural_json and old_json_targets == new_json_targets
                                       and previous_recipe.get("requiredTextureAssets")
                                       == recipe.get("requiredTextureAssets"))
            new_targets = old_targets if target_inputs_unchanged else incremental_texture_targets(
                old_targets, previous_recipe, recipe, json_root, sorted(old_standalone))
            new_standalone = ((old_standalone & new_required)
                              | ((new_required - old_required) - set(old_targets)))
            if selective and isinstance(old_targets, list) and texture_root.is_dir():
                texture_delta = apply_texture_delta(common, temporary, texture_root, old_targets, new_targets)
                if previous_recipe.get("fontFilter") == recipe.get("fontFilter"):
                    validate_font_export(font_root)
                    print("[Zanik] Web-font filter unchanged; retained font verified and UnrealAssetScout font pass skipped", flush=True)
                else:
                    run(common + ["-f", recipe["fontFilter"], "export", "Raw", "-o", str(font_root), "-c",
                                  "-l", str(temporary / "font-export.log")],
                        "Updating the web font", quiet=True)
            else:
                if incremental and not selective:
                    print("[Zanik] A game/tool change or legacy cache requires broad package discovery", flush=True)
                complete_remaining_exports(common, temporary, recipe, json_root, texture_root, font_root,
                                           incremental=incremental)
            if incremental:
                write_json_atomic(temporary / EXPORT_CHECKPOINT,
                                  verified_export_checkpoint(temporary, recipe, reviewed_counts,
                                                             mapping, stage_journal, new_targets))
            else:
                recover_or_create_export_checkpoint(temporary, recipe, reviewed_counts, mapping, stage_journal)
            write_incremental_state(temporary, recipe, new_targets, stage_journal,
                                    sorted(new_standalone))
            update_stage(stage_journal, "exports")
        scripts = Path(__file__).resolve().parent; repo = scripts.parent; py = sys.executable
        contracts = generation_contracts(scripts, repo, recipe)
        prior_generation_path = temporary / GENERATION_STATE
        prior_generation = read_operational_json(prior_generation_path) if prior_generation_path.is_file() else None
        reset_partial_generation(temporary)
        dirty = set(GENERATION_GROUPS)
        if args.incremental_work and selective and args.baseline_bundle:
            previous_sha = str((prior or {}).get("recipeSha256") or "")
            previous_executable = str((prior or {}).get("fingerprints", {}).get("gameExecutableSha256") or "")
            try:
                seed_validated_private_tree(
                    args.baseline_bundle.resolve(), output, web, previous_sha, previous_executable)
            except (OSError, ValueError, RuntimeError, zipfile.BadZipFile, json.JSONDecodeError) as error:
                emit_progress("Retained generated tree unavailable",
                              detail=f"Falling back to the validated prior bundle: {error}")
                seed_validated_baseline(args.baseline_bundle.resolve(), web,
                                        previous_sha, previous_executable)
            dirty = selective_generation_groups(
                previous_recipe, recipe, prior_generation, contracts,
                texture_delta["added"], texture_delta["removed"],
                previous_sha in REVIEWED_LEGACY_GENERATION_RECIPES)
            remove_group_outputs(web, dirty)
            for legacy_name in ("unlocks", "spells and perks"):
                legacy_unlocks = web / legacy_name
                if legacy_unlocks.is_dir():
                    remove_tree(legacy_unlocks)
            reused = len(GENERATION_GROUPS) - len(dirty)
            emit_progress("Planning generated website update",
                          detail=f"Reusing {reused} validated output group(s); rebuilding {len(dirty)}",
                          current=reused, total=len(GENERATION_GROUPS))
        update_stage(stage_journal, "generating")
        if "buildings" in dirty:
            run([py, str(scripts / "generate_building_catalog.py"), str(json_root), str(texture_root), str(web / "buildings"), "--catalogue-root", str(json_root)], "Generating building catalog")
        if "plan-items" in dirty:
            run([py, str(scripts / "generate_plan_item_catalog.py"), "--assets", str(json_root), "--icons", str(texture_root), "--web", str(web)], "Generating plan-item catalog")
        cells = next((path for path in json_root.rglob("_Generated_") if path.is_dir() and "L_World" in path.as_posix()), None)
        if cells is None: raise RuntimeError("the targeted export contains no L_World partition cells")
        if "npc" in dirty:
            run([py, str(scripts / "generate_world_catalogs.py"), "npc", "--cells-root", str(cells), "--spawn-root", str(json_root), "--character-root", str(json_root), "--placed-root", str(json_root), "--public-output", str(web / "npc-catalog.json"), "--identities-output", str(web / "npc-identities.tsv"), "--exclusions-output", str(internal / "npc-exclusions.tsv")], "Generating NPC catalog")
        if "resources" in dirty:
            run([py, str(scripts / "generate_world_catalogs.py"), "resource", "--cells-root", str(cells), "--blueprint-root", str(json_root), "--item-root", str(json_root), "--public-output", str(web / "resource-catalog.json"), "--identities-output", str(web / "resource-identities.tsv")], "Generating resource catalog")
        deploy = scripts.parent / "deploy"
        if "quests" in dirty:
            run([py, str(scripts / "generate_quest_catalog.py"), "--source", str(json_root), "--public", str(web / "quest-catalog.json"), "--runtime", str(web / "quest-runtime-catalog.tsv"), "--overrides", str(deploy / "quest-overrides.json")], "Generating quest catalog")
        artwork_groups = {"item-artwork": "items", "npc-artwork": "npc", "resource-artwork": "resources",
                          "spell-and-perk-artwork": "spells-and-perks", "skills": "skills", "map-icons": "map-icons", "world-map": "world-map",
                          "font": "font", "quest-badges": "quest-badges"}
        selected_artwork = [value for group, value in artwork_groups.items() if group in dirty]
        if selected_artwork:
            artwork_command = [py, str(scripts / "generate_web_assets.py"), "--json-root", str(json_root),
                               "--texture-root", str(texture_root), "--font-root", str(font_root),
                               "--web-root", str(web)]
            for group in selected_artwork: artwork_command.extend(["--group", group])
            run(artwork_command, "Resolving " + ", ".join(selected_artwork).replace("-", " "))
        history_seed = repo / "asset-tools/recipe-runtime-catalog-history.tsv"
        if not history_seed.is_file():
            history_seed = repo / "web/recipe-runtime-catalog-history.tsv"
        recipe_command = [py, str(scripts / "generate_recipe_unlock_catalog.py"), "--assets", str(json_root), "--building-assets", str(json_root), "--building-catalog", str(web / "buildings/catalog.json"), "--usmap", str(mapping), "--game-exe", str(executable), "--rules", str(repo / "asset-tools/catalog-rules.json"), "--output", str(internal / "recipe-unlocks-audit.json"), "--docs-dir", str(internal / "docs"), "--web-output", str(web / "recipe-unlocks.json"), "--runtime-web-output", str(web / "recipe-runtime-catalog.tsv"), "--runtime-history-output", str(web / "recipe-runtime-catalog-history.tsv")]
        if history_seed.is_file():
            recipe_command.extend(["--runtime-history-seed", str(history_seed)])
        if "recipes" in dirty:
            run(recipe_command, "Generating recipe catalog")
        if "quest-destinations" in dirty:
            run([py, str(scripts / "analyze_quest_steps.py"), "--source", str(json_root), "--quest-catalog", str(web / "quest-catalog.json"), "--quest-runtime-catalog", str(web / "quest-runtime-catalog.tsv"), "--npc-catalog", str(web / "npc-catalog.json"), "--npc-identities", str(web / "npc-identities.tsv"), "--overrides", str(deploy / "quest-step-overrides.json"), "--analysis-output", str(internal / "quest-step-analysis.json"), "--public-output", str(web / "quest-step-locations.json"), "--coverage-output", str(internal / "quest-step-location-coverage.md")], "Generating quest destination catalog")
        asset_tool = scripts / "zanik_assets.py"
        run([py, str(asset_tool), "validate", "--generated-root", str(web)], "Validating generated asset tree")
        prior_digests = prior_generation.get("outputDigests", {}) if isinstance(prior_generation, dict) else {}
        output_digests = {
            group: (output_digest(web, group) if group in dirty else prior_digests.get(group) or output_digest(web, group))
            for group in GENERATION_GROUPS
        }
        generation_state = {"schemaVersion": 1, "graphVersion": GENERATION_GRAPH_VERSION,
                            "recipeSha256": sha256(Path(recipe["_path"])), "contracts": contracts,
                            "outputDigests": output_digests}
        write_json_atomic(prior_generation_path, generation_state)
        replacement = output.with_name(output.name + ".new")
        if replacement.exists(): remove_tree(replacement)
        shutil.copytree(web, replacement, copy_function=_link_or_copy)
        previous = output.with_name(output.name + ".previous")
        if output.exists():
            if previous.exists(): remove_tree(previous)
            output.replace(previous)
        replacement.replace(output)
        update_stage(stage_journal, "generated")
        print(json.dumps({"output": str(output), "work": str(temporary) if keep else None, "ready": True}, sort_keys=True))
        return 0
    finally:
        if not keep and temporary.exists():
            try:
                remove_tree(temporary)
            except OSError as error:
                print(f"[Zanik] WARNING: temporary exports could not be removed: {temporary} ({error})", file=sys.stderr)
                print("[Zanik] These files are game-derived and must not be redistributed; remove them after filesystem activity settles.", file=sys.stderr)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--game-root", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--tool-root", type=Path, required=True)
    parser.add_argument("--usmap-path", type=Path, required=True)
    parser.add_argument("--recipe", type=Path, required=True)
    parser.add_argument("--work-root", type=Path)
    parser.add_argument("--keep-work", action="store_true")
    parser.add_argument("--resume-work", type=Path, help=argparse.SUPPRESS)
    parser.add_argument("--incremental-work", type=Path, help=argparse.SUPPRESS)
    parser.add_argument("--game-changed", action="store_true", help=argparse.SUPPRESS)
    parser.add_argument("--workspace", type=Path, help=argparse.SUPPRESS)
    parser.add_argument("--stage-journal", type=Path, help=argparse.SUPPRESS)
    parser.add_argument("--baseline-bundle", type=Path, help=argparse.SUPPRESS)
    return extract(parser.parse_args())


if __name__ == "__main__":
    raise SystemExit(main())
