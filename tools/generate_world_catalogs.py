#!/usr/bin/env python3
"""Generate NPC and resource map catalogs from targeted cooked JSON exports."""

from __future__ import annotations

import argparse
from collections import Counter
from datetime import datetime, timezone
import hashlib
import json
import math
from pathlib import Path
import re
from typing import Any, Iterable

try:
    from asset_filesystem import read_json
except ModuleNotFoundError:  # Imported as tools.* by repository tests.
    from tools.asset_filesystem import read_json

MAP_X = -11075.0
MAP_Y = -117685.0
MAP_SIZE = 420000.0
RESOURCE_NAMES = {
    "BP_Spawner_AnimaInfusedBark": "Anima-Infused Bark",
    "BP_Spawner_BittercapMushroom": "Bittercap Mushroom",
    "BP_Spawner_HollowBark": "Hollow Bark",
    "BP_Spawner_OakBranch": "Oak Branch",
    "BP_Spawner_SwampWeedPlant": "Swamp Weed",
    "BP_Spawner_Swamp_Tar": "Swamp Tar",
    "BP_RedberryBush": "Redberries",
    "BP_CadavaBush": "Cadavaberries",
    "BP_DwellberryBush": "Dwellberries",
    "BP_ThornyRoot_Irit": "Irit",
    "BP_Plant_Explosive_CorpseCotton": "Corpse Cotton",
    "BP_Weed_Desert": "Desert Weed",
    "BP_Cactus_Barrel": "Barrel Cactus",
    "BP_Cactus_Bunny": "Bunny Cactus",
    "BP_Cactus_Pipe": "Fibrous Pipe Cactus",
}


def generated_at() -> str:
    import os
    epoch = int(os.environ.get("SOURCE_DATE_EPOCH", "0"))
    return datetime.fromtimestamp(epoch, timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")


def files(root: Path) -> Iterable[Path]:
    return (path for path in root.rglob("*.json") if path.name != ".uas-manifest.json")


def load(path: Path) -> list[dict[str, Any]] | None:
    try:
        value = read_json(path)
        return value if isinstance(value, list) else None
    except (OSError, UnicodeError, json.JSONDecodeError) as error:
        raise RuntimeError(f"cannot read exported JSON {path.name}: {error}") from error


def props(value: dict[str, Any] | None) -> dict[str, Any]:
    return (value or {}).get("Properties") or {}


def ref_index(value: Any) -> int:
    match = re.search(r"\.(\d+)$", str((value or {}).get("ObjectPath", ""))) if isinstance(value, dict) else None
    return int(match.group(1)) if match else -1


def ref_base(value: Any) -> str:
    if not isinstance(value, dict):
        return ""
    raw = str(value.get("AssetPathName") or value.get("ObjectName") or value.get("ObjectPath") or "")
    quoted = re.search(r"'([^']+)'", raw)
    raw = quoted.group(1) if quoted else raw
    raw = raw.rsplit(":", 1)[-1]
    match = re.search(r"/([^/.]+)(?:\.\d+|\.[^/.]+)?$", raw)
    return match.group(1) if match else raw.removesuffix("_C")


def slug(value: str) -> str:
    return re.sub(r"[^a-z0-9]+", "-", value.lower()).strip("-")


def display_name(value: str) -> str:
    if value in RESOURCE_NAMES:
        return RESOURCE_NAMES[value]
    value = re.sub(r"^(?:BP_Spawner_|BP_AI_|BP_SpawnPoint_|BP_)", "", value)
    value = re.sub(r"_0[1-9]$", "", value).replace("_Character", "").replace("_", " ")
    value = re.sub(r"(?<=[a-z])(?=[A-Z])", " ", value)
    return " ".join(word.capitalize() for word in value.split())


def canonical_npc_name(ai_class: str, authored: str) -> str:
    rules = (
        (r"DragonLesserGreen_Blightscale", "Blightscale the Lesser Green Dragon"),
        (r"DragonLesserGreen", "Green Dragon"), (r"DragonLesserBlue", "Blue Dragon"),
        (r"DragonLesserRed", "Red Dragon"), (r"DragonLesserBlack", "Black Dragon"),
        (r"MediumBeast_Spinning", "Garou Berserker"),
        (r"MagicBeast_BirdKing", "Garou Mage (Bird King)"),
        (r"LesserDemon_Character_02", "Lesser Demon"),
        (r"Melee2HBlackKnight_Flaming", "Black Knight Crusader"),
        (r"MeleeBlackKnight_Flaming", "Blazing Black Knight"),
    )
    return next((name for pattern, name in rules if re.search(pattern, ai_class, re.I)), authored)


def canonical_icon(ai_class: str, name: str, journal_by_name: dict[str, dict[str, str]]) -> str:
    if name.lower() in journal_by_name:
        return journal_by_name[name.lower()]["iconAsset"]
    rules = ((r"DragonLesserGreen_Blightscale", "green dragon"), (r"Kebbit_Character_02", "kebbit"),
             (r"MagicBeast_BirdKing", "garou druid"), (r"Melee2HBlackKnight_Flaming", "elite black knight"),
             (r"MeleeBlackKnight_Flaming", "black knight warrior"))
    match = next((label for pattern, label in rules if re.search(pattern, ai_class, re.I)), "")
    return journal_by_name.get(match, {}).get("iconAsset", "")


def text(value: Any) -> str:
    if isinstance(value, str):
        return value
    if not isinstance(value, dict):
        return ""
    return str(value.get("LocalizedString") or value.get("SourceString") or "")


def world_location(exports: list[dict[str, Any]], component: dict[str, Any] | None, depth: int = 0) -> dict[str, float] | None:
    if not component or depth > 12:
        return None
    local = props(component).get("RelativeLocation") or {"X": 0, "Y": 0, "Z": 0}
    try:
        point = {axis: float(local.get(axis, 0)) for axis in "XYZ"}
    except (TypeError, ValueError):
        return None
    parent_index = ref_index(props(component).get("AttachParent"))
    if parent_index < 0 or parent_index >= len(exports):
        return point
    parent = world_location(exports, exports[parent_index], depth + 1)
    return {axis: point[axis] + parent[axis] for axis in "XYZ"} if parent else None


def map_point(point: dict[str, float]) -> tuple[float, float] | None:
    x, y = (point["X"] - MAP_X) / MAP_SIZE, (point["Y"] - MAP_Y) / MAP_SIZE
    return (round(x, 8), round(y, 8)) if 0 <= x <= 1 and 0 <= y <= 1 else None


def duration_seconds(value: Any) -> float:
    value = value if isinstance(value, dict) else {}
    return sum(float(value.get(key, 0) or 0) * scale for key, scale in
               (("Days", 86400), ("Hours", 3600), ("Minutes", 60), ("Seconds", 1)))


def npc_catalog(args: argparse.Namespace) -> int:
    character_files = list(files(args.character_root))
    journal_by_ai: dict[str, dict[str, str]] = {}
    journal_by_name: dict[str, dict[str, str]] = {}
    for path in character_files:
        if not re.search(r"/UI/JournalData/Entries/(?:World/(?:Factions|Fauna)|Knowledge/People)/", path.as_posix(), re.I):
            continue
        exports = load(path)
        p = props(exports[0]) if exports else {}
        name, icon, ai = text(p.get("DisplayName")), ref_base(p.get("Image")), ref_base((p.get("UnlockCondition") or {}).get("AIData"))
        if name and icon:
            record = {"name": name, "iconAsset": icon}
            journal_by_name[name.lower()] = record
            if ai:
                journal_by_ai[ai] = record
    characters: dict[str, dict[str, str]] = {}
    for path in character_files:
        exports = load(path)
        if not exports:
            continue
        klass = next((item for item in exports if item.get("Type") == "BlueprintGeneratedClass"), None)
        cdo = next((item for item in exports if item.get("Name") == f"Default__{(klass or {}).get('Name', '')}"), None)
        if not klass or not cdo:
            continue
        key = str(klass.get("Name", "")).removesuffix("_C")
        ai = ref_base(props(cdo).get("AIDataClass"))
        journal = journal_by_ai.get(ai, {})
        characters[key] = {"name": journal.get("name") or text(props(cdo).get("AIName")) or display_name(key),
                           "iconAsset": journal.get("iconAsset", "")}
    spawn_raw: dict[str, dict[str, Any]] = {}
    for path in files(args.spawn_root):
        exports = load(path)
        if not exports:
            continue
        klass = next((item for item in exports if item.get("Type") == "BlueprintGeneratedClass"), None)
        cdo = next((item for item in exports if item.get("Name") == f"Default__{(klass or {}).get('Name', '')}"), None)
        if not klass or not cdo:
            continue
        parent = ref_base(klass.get("SuperStruct") or klass.get("Super"))
        ai = ref_base(props(cdo).get("AIClass"))
        spawn_raw[str(klass.get("Name", "")).removesuffix("_C")] = {"parent": parent, "ai": ai, "properties": props(cdo)}
    resolved: dict[str, dict[str, Any]] = {}
    def resolve(name: str, seen: set[str] | None = None) -> dict[str, Any] | None:
        if name in resolved:
            return resolved[name]
        seen = set() if seen is None else seen
        if name in seen or name not in spawn_raw:
            return None
        seen.add(name); raw = spawn_raw[name]
        parent = {} if raw["parent"] == "AISpawnPoint" else resolve(raw["parent"], seen)
        if raw["parent"] != "AISpawnPoint" and not parent:
            return None
        ai = raw["ai"] or parent.get("ai", "")
        if not ai:
            return None
        character = characters.get(ai, {})
        name_value = canonical_npc_name(ai, character.get("name") or display_name(ai or name))
        marker = f"{name} {ai} {name_value}".lower()
        passive = bool(re.search(r"chicken|cow|sheep|ram|deer|kebbit|chinchompa|falcon|magpie|raven|goat|vulture", marker))
        quest = bool(re.search(r"quest|postie|wise old|npc", marker))
        p = raw["properties"]
        should = p.get("bShouldRespawn", parent.get("should", True)) is not False
        duration = duration_seconds(p["RespawnDuration"]) if "RespawnDuration" in p else parent.get("respawn", 0)
        result = {"ai": ai, "name": name_value, "iconAsset": character.get("iconAsset") or canonical_icon(ai, name_value, journal_by_name),
                  "category": "quest" if quest else "non-hostile" if passive else "hostile",
                  "hostile": not quest and not passive, "should": should, "respawn": duration if should else 0}
        resolved[name] = result
        return result
    for name in spawn_raw:
        resolve(name)
    placed_raw: dict[str, dict[str, Any]] = {}
    for path in files(args.placed_root):
        exports = load(path)
        if not exports:
            continue
        klass = next((item for item in exports if item.get("Type") == "BlueprintGeneratedClass"), None)
        class_name = str((klass or {}).get("Name", "")).removesuffix("_C")
        if not klass or not class_name.startswith("BP_NPC_"):
            continue
        parent = ref_base(klass.get("Super") or klass.get("SuperStruct"))
        participant = next((item for item in exports if item.get("Type") == "DomConversationParticipant" and text(props(item).get("DisplayName"))), None)
        mesh = next((item for item in exports if item.get("Type") == "SkeletalMeshComponent" and item.get("Name") == "CharacterMesh0"
                     and (props(item).get("SkeletalMesh") or props(item).get("SkinnedAsset"))), None)
        placed_raw[class_name] = {"parent": parent, "name": text(props(participant).get("DisplayName")), "mesh": bool(mesh)}
    placed_resolved: dict[str, dict[str, Any]] = {}
    def resolve_placed(name: str, seen: set[str] | None = None) -> dict[str, Any] | None:
        if name in placed_resolved:
            return placed_resolved[name]
        seen = set() if seen is None else seen
        if name in seen or name not in placed_raw:
            return None
        seen.add(name); raw = placed_raw[name]; parent = resolve_placed(raw["parent"], seen)
        valid = raw["mesh"] or bool(parent and parent["valid"])
        display = raw["name"] or (parent or {}).get("name") or display_name(name.removeprefix("BP_NPC_"))
        record = {"valid": valid, "ai": name, "name": display,
                  "iconAsset": journal_by_name.get(display.lower(), {}).get("iconAsset") or (parent or {}).get("iconAsset", ""),
                  "category": "quest", "hostile": False, "should": False, "respawn": 0}
        placed_resolved[name] = record
        return record
    for name in placed_raw:
        resolve_placed(name)
    nodes: list[dict[str, Any]] = []; identities: list[dict[str, Any]] = []; exclusions: list[tuple[str, str, str, str]] = []
    types: dict[str, dict[str, Any]] = {}; rejected = scanned = 0
    world_sources = list(files(args.cells_root))
    world_sources.extend(path for path in files(args.placed_root)
                         if path.name == "L_World.json" and path not in world_sources)
    for path in world_sources:
        exports = load(path)
        if not exports:
            rejected += 1; continue
        scanned += 1
        try:
            relative = path.relative_to(args.cells_root).as_posix()
        except ValueError:
            relative = path.relative_to(args.placed_root).as_posix()
        world = bool(re.search(r"/Maps/World/L_World(?:/|\.json$)", path.as_posix(), re.I))
        for actor in exports:
            spawn_type = str(actor.get("Type", "")).removesuffix("_C")
            meta = resolved.get(spawn_type)
            if not meta and placed_resolved.get(spawn_type, {}).get("valid"):
                meta = placed_resolved[spawn_type]
            if not meta:
                continue
            reason = "" if world else "non-overworld-map"
            if re.search(r"Dungeon|Developers|Test", relative + " " + str((actor.get("Outer") or {}).get("ObjectPath", "")), re.I):
                reason = "dungeon-instance-or-test"
            index = ref_index(props(actor).get("RootComponent"))
            location = world_location(exports, exports[index] if 0 <= index < len(exports) else None)
            point = map_point(location) if location else None
            if not point:
                reason = reason or "invalid-transform"
            state = str(actor.get("Name") or props(actor).get("Guid") or (actor.get("ActorInstanceGuid") or {}).get("ActorGuid") or "")
            if not state:
                reason = reason or "missing-persistent-identity"
            if reason:
                exclusions.append((state, spawn_type, reason, relative)); continue
            type_id = slug(meta["name"]); stable = props(actor).get("Guid") or (actor.get("ActorInstanceGuid") or {}).get("ActorGuid") or state
            opaque = hashlib.sha256(f"{relative}:{stable}".encode()).hexdigest()[:20]
            actor_props = props(actor); should = actor_props.get("bShouldRespawn", meta["should"]) is not False
            respawn = duration_seconds(actor_props["RespawnDuration"]) if "RespawnDuration" in actor_props else meta["respawn"]
            respawn = respawn if should else 0
            node = {"id": opaque, "type": type_id, "x": point[0], "y": point[1],
                    "worldX": round(location["X"], 3), "worldY": round(location["Y"], 3), "worldZ": round(location["Z"], 3)}
            if respawn > 0: node["respawnGameSeconds"] = respawn
            nodes.append(node)
            identities.append({"stateKey": state, "id": opaque, "typeId": type_id, "spawnType": spawn_type,
                               "aiClass": meta["ai"], "respawn": respawn, "location": location, "source": relative})
            if type_id not in types:
                types[type_id] = {"id": type_id, "name": meta["name"], "category": meta["category"], "hostile": meta["hostile"], "count": 0}
                if meta["iconAsset"]: types[type_id]["icon"] = f"/assets/npcs/{meta['iconAsset']}.png"
            types[type_id]["count"] += 1
    nodes.sort(key=lambda item: (item["type"], item["id"])); identities.sort(key=lambda item: (item["typeId"], item["id"]))
    type_list = sorted(types.values(), key=lambda item: item["name"])
    fingerprint = hashlib.sha256(json.dumps({"typeList": type_list, "nodes": nodes}, separators=(",", ":"), sort_keys=True).encode()).hexdigest()
    catalog = {"version": f"npcs-{fingerprint[:12]}", "contentFingerprint": fingerprint, "mapVersion": "world-map-1.0.0.2",
               "generatedAt": generated_at(), "types": type_list, "nodes": nodes,
               "diagnostics": {"files": scanned, "nodes": len(nodes), "types": len(type_list), "rejected": rejected, "excluded": len(exclusions)}}
    args.public_output.parent.mkdir(parents=True, exist_ok=True)
    args.identities_output.parent.mkdir(parents=True, exist_ok=True)
    args.exclusions_output.parent.mkdir(parents=True, exist_ok=True)
    args.public_output.write_text(json.dumps(catalog, separators=(",", ":")) + "\n", encoding="utf-8")
    header = f"# zanik-npc-identities\t1\t{len(identities)}\t{fingerprint}\tstateKey\topaqueId\ttypeId\tspawnType\taiClass\trespawnGameSeconds\tworldX\tworldY\tworldZ\tsourceFile"
    rows = ["\t".join(map(str, (item["stateKey"], item["id"], item["typeId"], item["spawnType"], item["aiClass"], item["respawn"], item["location"]["X"], item["location"]["Y"], item["location"]["Z"], item["source"]))) for item in identities]
    args.identities_output.write_text(header + "\n" + "\n".join(rows) + "\n", encoding="utf-8")
    args.exclusions_output.write_text("# stateKey\tspawnType\treason\tsourceFile\n" + "\n".join("\t".join(row) for row in sorted(exclusions)) + "\n", encoding="utf-8")
    print(json.dumps(catalog["diagnostics"], sort_keys=True)); return 0


def resource_catalog(args: argparse.Namespace) -> int:
    metadata = {key: {"name": value, "iconAsset": "", "respawn": ""} for key, value in RESOURCE_NAMES.items()}
    items: dict[str, Path] = {path.stem: path for path in files(args.item_root)}
    packages: dict[str, list[dict[str, Any]]] = {}
    blueprint_records: list[tuple[Path, list[dict[str, Any]]]] = []
    for path in files(args.blueprint_root):
        exports = load(path)
        if not exports: continue
        blueprint_records.append((path, exports))
        package = next((item.get("Package") for item in exports if isinstance(item.get("Package"), str)), None)
        if package: packages[package] = exports
    def inherited(component: dict[str, Any] | None, seen: set[str] | None = None) -> str:
        if not component: return ""
        direct = (((props(component).get("SpawnSettings") or {}).get("ResourceRespawnSettingsRowHandle") or {}).get("RowName") or "")
        if direct: return str(direct)
        reference = str(((component.get("Template") or {}).get("ObjectPath") or "")); seen = set() if seen is None else seen
        if not reference or reference in seen: return ""
        seen.add(reference); match = re.match(r"^(.*)\.(\d+)$", reference)
        if not match: return ""
        exports = packages.get(match.group(1), []); index = int(match.group(2))
        parent = exports[index] if index < len(exports) else next((item for item in exports if item.get("ObjectPath") == reference), None)
        return inherited(parent, seen)
    for _, exports in blueprint_records:
        cdo = next((item for item in exports if str(item.get("Name", "")).startswith("Default__BP_")), None)
        if not cdo: continue
        raw = str(cdo.get("Name", "")).removeprefix("Default__").removesuffix("_C")
        item_key = ref_base(props(cdo).get("ItemData")); item_exports = load(items[item_key]) if item_key in items else None
        item = item_exports[0] if item_exports else {}
        respawn = next((entry for entry in exports if entry.get("Type") == "ResourceRespawnComponent" and entry.get("Name") == "RespawnComponent"), None)
        metadata[raw] = {"name": text(props(item).get("Name")) or display_name(raw), "iconAsset": ref_base(props(item).get("Icon")), "respawn": inherited(respawn)}
    nodes: list[dict[str, Any]] = []; private: list[dict[str, Any]] = []; types: dict[str, dict[str, Any]] = {}; rejected = 0; scanned = 0
    for path in files(args.cells_root):
        exports = load(path)
        if not exports: rejected += 1; continue
        scanned += 1
        def add(raw: str, stable: str, state: str, location: dict[str, float]) -> None:
            nonlocal rejected
            point = map_point(location)
            if not point: rejected += 1; return
            meta = metadata.get(raw, {"name": display_name(raw), "iconAsset": "", "respawn": ""})
            type_id = slug(meta["name"]); profile = meta["respawn"] or "RUNTIME_DEFAULT_24H"
            match = re.search(r"_(\d+(?:\.\d+)?)H$", profile); hours = float(match.group(1)) if match else 24.0
            opaque = hashlib.sha256(stable.encode()).hexdigest()[:20]
            nodes.append({"id": opaque, "type": type_id, "x": point[0], "y": point[1], "worldX": round(location["X"], 3),
                          "worldY": round(location["Y"], 3), "worldZ": round(location["Z"], 3), "respawnGameHours": hours,
                          **({"respawnSource": "runtime-default"} if not meta["respawn"] else {})})
            types[type_id] = {"id": type_id, "name": meta["name"], "icon": f"/assets/resources/{type_id}.png", "aliases": []}
            private.append({"state": state, "id": opaque, "type": type_id, "profile": profile, "icon": meta["iconAsset"], "location": location, "source": path.relative_to(args.cells_root).as_posix()})
        for actor in exports:
            actor_path = str(props(actor).get("ActorClass", {}).get("ObjectPath", ""))
            if not re.search(r"/(?:Spawners/(?:GatherableResources|HarvestableResources)|ActionableResources/(?:BP_Weed_Desert|BP_Cactus_Barrel|BP_Cactus_Bunny|BP_Cactus_Pipe))[/\.]", actor_path): continue
            raw = ref_base(props(actor).get("ActorClass")); origin_index = ref_index(props(actor).get("AttachParent")); origin = world_location(exports, exports[origin_index] if 0 <= origin_index < len(exports) else None)
            if not origin: rejected += 1; continue
            for index, instance in enumerate(actor.get("PerInstanceSMData") or []):
                offset = ((instance or {}).get("TransformData") or {}).get("Translation") or {}
                try: location = {axis: origin[axis] + float(offset.get(axis, 0)) for axis in "XYZ"}
                except (TypeError, ValueError): rejected += 1; continue
                guid = str(props(actor).get("Guid") or actor.get("Name") or path.name); add(raw, f"{guid}:{index}", f"{guid.upper()}:{index}", location)
        for actor in exports:
            match = re.match(r"^(BP_(?:Spawner_.+|.+Bush|ThornyRoot_Irit|Plant_Explosive_CorpseCotton))_C$", str(actor.get("Type", "")))
            if not match or not (props(actor).get("RespawnComponent") or props(actor).get("ResourceRespawn")): continue
            index = ref_index(props(actor).get("RootComponent") or props(actor).get("Root") or props(actor).get("DefaultSceneRoot")); location = world_location(exports, exports[index] if 0 <= index < len(exports) else None)
            if not location: rejected += 1; continue
            raw = match.group(1); guid = actor.get("Name") if raw in {"BP_ThornyRoot_Irit", "BP_Plant_Explosive_CorpseCotton"} else (actor.get("ActorInstanceGuid") or {}).get("ActorGuid") or actor.get("Name")
            add(raw, str(guid), str(actor.get("Name") or guid), location)
    nodes.sort(key=lambda item: (item["type"], item["id"])); counts = Counter(item["type"] for item in nodes)
    type_list = sorted(({**value, "count": counts[key]} for key, value in types.items()), key=lambda item: item["name"])
    fingerprint = hashlib.sha256(json.dumps(nodes, separators=(",", ":"), sort_keys=True).encode()).hexdigest()
    catalog = {"version": f"resources-{fingerprint[:12]}", "contentFingerprint": fingerprint, "mapVersion": "world-map-1.0.0.2", "generatedAt": generated_at(),
               "types": type_list, "nodes": nodes, "diagnostics": {"files": scanned, "nodes": len(nodes), "types": len(type_list), "rejected": rejected}}
    args.public_output.parent.mkdir(parents=True, exist_ok=True); args.identities_output.parent.mkdir(parents=True, exist_ok=True)
    args.public_output.write_text(json.dumps(catalog, separators=(",", ":")) + "\n", encoding="utf-8")
    lines = ["# stateKey\topaqueId\ttypeId\trespawnProfile\ticonAsset\tworldX\tworldY\tworldZ\tsourceFile"]
    lines += ["\t".join(map(str, (item["state"], item["id"], item["type"], item["profile"], item["icon"], item["location"]["X"], item["location"]["Y"], item["location"]["Z"], item["source"]))) for item in private]
    args.identities_output.write_text("\n".join(lines) + "\n", encoding="utf-8"); print(json.dumps(catalog["diagnostics"], sort_keys=True)); return 0


def main() -> int:
    parser = argparse.ArgumentParser(); commands = parser.add_subparsers(dest="command", required=True)
    npc = commands.add_parser("npc")
    for name in ("cells_root", "spawn_root", "character_root", "placed_root", "public_output", "identities_output", "exclusions_output"):
        npc.add_argument("--" + name.replace("_", "-"), type=Path, required=True)
    npc.set_defaults(run=npc_catalog)
    resource = commands.add_parser("resource")
    for name in ("cells_root", "blueprint_root", "item_root", "public_output", "identities_output"):
        resource.add_argument("--" + name.replace("_", "-"), type=Path, required=True)
    resource.set_defaults(run=resource_catalog)
    args = parser.parse_args()
    return args.run(args)


if __name__ == "__main__":
    raise SystemExit(main())
