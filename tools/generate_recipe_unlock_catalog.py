#!/usr/bin/env python3
"""Build the auditable Dragonwilds recipe/building unlock catalogue.

The script consumes JSON exported from the cooked game with UnrealAssetScout,
the save-editor GUID catalogues, and Zanik's authoritative building catalogue.
It never writes to the game or a server.  Unknown active entries are retained
with status ``unresolved`` and make validation fail unless explicitly allowed.
"""

from __future__ import annotations

import argparse
import copy
import hashlib
import json
import math
import re
import shutil
import subprocess
from difflib import SequenceMatcher
from collections import Counter, defaultdict
from datetime import datetime, timezone
from pathlib import Path
from typing import Any, Iterable


TRIGGER_LABELS = {
    "ItemsPickedUp": "obtain_item",
    "ActorsInteractedWith": "open_station_ui",
    "SkillLevel": "skill_or_perk",
    "Entitlement": "entitlement",
    "AgilityCourseMedal": "skill_or_perk",
    "AgilityCourseMedalsUnlocked": "skill_or_perk",
}

# Cooked station catalog membership is the authoritative source for where a
# recipe is made.  A recipe can intentionally be present at more than one
# station; in that case the public catalogue uses the most capable station.
# The numbers are only a deterministic overlap tie-breaker--they never assign
# a station that does not actually contain the recipe.
STATION_PRIORITY = {
    "CraftingTable": 10,
    "Campfire": 10,
    "Spinning Wheel": 20,
    "Tannery": 20,
    "Furnace": 20,
    "RangeStone": 25,
    "Grill": 30,
    "RangeBronze": 35,
    "AdvancedSpinningWheel": 40,
    "AdvancedTannery": 40,
    "AdvancedSmelter": 40,
    "GarouMasonsBench": 50,
}

STATION_DISPLAY_NAMES = {
    "CraftingTable": "Crafting Table",
    "RuneAltar": "Rune Altar",
    "GarouMasonsBench": "Garou Mason's Bench",
    "PotteryWheel": "Pottery Wheel",
    "RangeBronze": "Cooking Range",
    "RangeStone": "Stone Cooking Range",
    "JewelersBench": "Jeweller's Bench",
    "BrewingCauldron": "Brewing Cauldron",
    "ToolsMeleeBench": "Blacksmith's Bench",
    "MysticForge": "Mystic Forge",
    "FletchingBenchv2": "Fletching Bench",
    "ArmourBench": "Armour Bench",
    "Campfire": "Campfire",
    "Kiln": "Kiln",
    "Grinder": "Grindstone",
    "Loom": "Loom",
    "Sawmill": "Sawmill",
    "Spinning Wheel": "Spinning Wheel",
    "Tannery": "Tannery",
    "Furnace": "Furnace",
    "Air Altar": "Air Altar",
    "Fire Altar": "Fire Altar",
    "Grill": "Grill",
    "Stonecutter": "Stonecutter",
    "AdvancedSmelter": "Blast Furnace",
    "FermentationBarrel": "Brewing Barrel",
    "AdvancedTannery": "Advanced Tannery",
    "AdvancedSpinningWheel": "Advanced Spinning Wheel",
}

INTERNAL_STATION_ROWS = {
    "OnlyCatalyst", "TestNone", "TestOnlyCatAndFuel", "TestOnlyFuel",
    "TestOnlyResource", "TestOnlyResourceAndCatalyst",
    "TestOnlyResourceAndFuel", "TestResCatAndFuel",
}

PUBLIC_HIDDEN_STATION_SECTIONS = {"Air Altar", "Fire Altar"}


def public_recipe_types(entry: dict[str, Any]) -> list[str]:
    """Classify outputs without exposing the internal evidence used to do it."""
    if entry["kind"] == "building":
        return []
    internal = str(entry.get("internalName", "")).casefold()
    display = str(entry.get("displayName", "")).casefold()
    outputs = " ".join(
        f"{output.get('internalName', '')} {output.get('assetPath', '')}"
        for output in entry.get("output", [])
    ).casefold()
    if internal in {"recipe_weapon_club_wood", "recipe_crafting_glass_owl", "recipe_masterworks_horn_mouthpiece"}:
        return []
    if "/equipment/cape/" in outputs or "_cape_" in internal:
        return ["items", "capes"]
    if "/equipment/held/shield/" in outputs or "recipe_shield_" in internal or "recipe_weapon_shield_" in internal:
        return ["items", "armour"]
    if "recipe_armour_" in internal or any(part in outputs for part in
                                            ("/equipment/head/", "/equipment/body/", "/equipment/legs/")):
        return ["items", "armour"]
    if "recipe_ammo_" in internal or "/equipment/ammo/" in outputs or (
            "rune" in display and "recipe_process_altar_" in internal):
        return ["items", "arrows-runes"]
    if "recipe_trinket_" in internal or "/equipment/jewellery/" in outputs or "/equipment/jewlery/" in outputs:
        return ["items", "jewellery"]
    if "recipe_consumable_potion_" in internal or display.endswith(" potion") or display == "weapon poison":
        return ["items", "potions"]
    if "recipe_consumable_tea_" in internal:
        return ["items", "drink"]
    if "recipe_consumable_" in internal or "/consumables/food/" in outputs or "/items/fishes/" in outputs:
        return ["items", "food"]
    if ("wateringcan" in internal or "recipe_tool_bucket_compost" in internal or
            "recipe_tool_secateurs" in internal or "spade" in internal):
        return ["items", "farming"]
    if ("recipe_fishbait_" in internal or "recipe_tool_fishingrod_" in internal or
            "recipe_tool_fishingnet_" in internal):
        return ["items", "fishing"]
    if "recipe_torch" in internal or "/equipment/held/torch/" in outputs:
        return ["items"]
    if display == "shadow sword":
        return ["items", "weapons"]
    if ("recipe_weapon_" in internal or "recipe_vestige_weapon_" in internal or
            ("recipe_masterworks_" in internal and "/equipment/held/" in outputs) or
            "/equipment/held/" in outputs):
        return ["items", "weapons"]
    return ["items"]

STATION_ACTOR_ROWS = {
    "BP_Station_Loom": "Loom",
    "BP_Station_Tanner": "AdvancedTannery",
    "BP_Station_Smelter": "Furnace",
    "BP_Crafting_WeaponBench": "ToolsMeleeBench",
}


def read_json(path: Path) -> Any:
    return json.loads(path.read_text(encoding="utf-8-sig"))


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for block in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def ref_name(value: Any) -> str:
    if not isinstance(value, dict):
        return ""
    asset_name = str(value.get("AssetPathName", ""))
    if asset_name:
        return asset_name.rsplit(".", 1)[-1].removesuffix("_C")
    object_name = str(value.get("ObjectName", ""))
    match = re.search(r"'([^']+)'", object_name)
    return (match.group(1) if match else object_name).removesuffix("_C")


def ref_path(value: Any) -> str:
    if not isinstance(value, dict):
        return ""
    asset_path = str(value.get("AssetPathName", ""))
    if asset_path:
        return asset_path.rsplit(".", 1)[0]
    path = str(value.get("ObjectPath", ""))
    return path.rsplit(".", 1)[0] if path else ""


def text(value: Any) -> str:
    if isinstance(value, dict):
        return str(value.get("LocalizedString") or value.get("SourceString") or value.get("CultureInvariantString") or "")
    return str(value or "")


def humanize_identifier(value: str) -> str:
    value = re.sub(r"^(?:BP_|V_|DungeonActor_|DungeonExit_)", "", value)
    value = re.sub(r"_C$", "", value)
    value = value.replace("_", " ")
    return re.sub(r"(?<=[a-z0-9])(?=[A-Z])", " ", value).strip()


def placed_actor_location(actor: dict[str, Any], records: list[dict[str, Any]]) -> dict[str, float] | None:
    """Resolve a cooked placed actor's root-component world location."""
    actor_name = str(actor.get("Name", ""))
    for component in records:
        props = component.get("Properties") or {}
        location = props.get("RelativeLocation")
        outer_name = str((component.get("Outer") or {}).get("ObjectName", ""))
        if actor_name and actor_name in outer_name and isinstance(location, dict):
            return {axis: float(location.get(axis, 0.0) or 0.0) for axis in ("X", "Y", "Z")}
    return None


def point_in_polygon(x: float, y: float, polygon: list[tuple[float, float]]) -> bool:
    inside = False
    previous = polygon[-1]
    for current in polygon:
        x1, y1 = previous
        x2, y2 = current
        if (y1 > y) != (y2 > y):
            crossing_x = (x2 - x1) * (y - y1) / (y2 - y1) + x1
            if x < crossing_x:
                inside = not inside
        previous = current
    return inside


def extract_dungeon_regions(records_by_file: dict[Path, list[dict[str, Any]]]) -> tuple[dict[str, dict[str, Any]], list[dict[str, Any]]]:
    """Map named dungeon entrances into the game's cooked map-region splines."""
    world_pair = next((item for item in records_by_file.items() if item[0].name == "L_World.json"), None)
    if not world_pair:
        return {}, []
    world_path, records = world_pair
    regions: list[dict[str, Any]] = []
    for actor in records:
        if actor.get("Type") != "BP_MapRegion_C":
            continue
        props = actor.get("Properties") or {}
        actor_name = str(actor.get("Name", ""))
        component = next((candidate for candidate in records if
                          candidate.get("Type") == "SplineComponent" and
                          actor_name in str((candidate.get("Outer") or {}).get("ObjectName", ""))), None)
        points = (((component or {}).get("Properties") or {}).get("SplineCurves") or {}).get("position", {}).get("Points", [])
        polygon = [(float(point["OutVal"]["X"]), float(point["OutVal"]["Y"]))
                   for point in points if isinstance(point.get("OutVal"), dict)]
        if len(polygon) < 3:
            continue
        region_name = text(props.get("RegionName"))
        region_tag = str((props.get("MapRegionTag") or {}).get("TagName", ""))
        parent = "Fellhollow" if region_tag == "Location.Region.Fellhollow" else ""
        regions.append({
            "name": region_name,
            "parent": parent,
            "displayName": f"{region_name} — {parent}" if parent and region_name != parent else region_name,
            "regionId": props.get("RegionID"),
            "regionTag": region_tag,
            "polygon": polygon,
            "evidenceActor": actor.get("ActorLabel", ""),
            "assetPath": "/Game/Maps/World/L_World",
        })

    def normalized(value: str) -> str:
        return re.sub(r"[^a-z0-9]", "", value.casefold())

    region_labels: list[tuple[dict[str, Any], dict[str, float], str]] = []
    for actor in records:
        if actor.get("Type") != "BP_RegionLabelActor_C":
            continue
        label = str(actor.get("ActorLabel", "")).removeprefix("RegionLabel_")
        location = placed_actor_location(actor, records)
        if not location:
            continue
        label_key = normalized(label)
        matches = [region for region in regions
                   if label_key in normalized(region["name"]) or normalized(region["name"]) in label_key]
        if matches:
            region_labels.append((matches[0], location, str(actor.get("ActorLabel", ""))))

    result: dict[str, dict[str, Any]] = {}
    for entrance_path, entrance_records in records_by_file.items():
        for actor in entrance_records:
            label = str(actor.get("ActorLabel", ""))
            match = re.match(r"DungeonEntrance_([A-Za-z0-9_]+)", label)
            if not match:
                continue
            location = placed_actor_location(actor, entrance_records)
            if not location:
                continue
            containing = [region for region in regions if point_in_polygon(location["X"], location["Y"], region["polygon"])]
            actor_tags = set((actor.get("Properties") or {}).get("Tags", []))
            boundary_conflicts = ("CTX_Fellhollow" in actor_tags and len(containing) == 1 and
                                  containing[0].get("parent") != "Fellhollow")
            eligible_labels = [item for item in region_labels
                               if not boundary_conflicts or item[0].get("parent") == "Fellhollow"]
            if len(containing) == 1 and not boundary_conflicts:
                region = {key: value for key, value in containing[0].items() if key != "polygon"}
                nearest_region = containing[0]
                nearest_location = location
                nearest_label = containing[0]["evidenceActor"]
            elif eligible_labels:
                nearest_region, nearest_location, nearest_label = min(
                    eligible_labels,
                    key=lambda item: math.hypot(location["X"] - item[1]["X"], location["Y"] - item[1]["Y"]),
                )
                region = {key: value for key, value in nearest_region.items() if key != "polygon"}
            else:
                continue
            if region:
                region["entranceActor"] = label
                region["entranceCoordinates"] = location
                region["entranceAssetPath"] = str(entrance_path)
                if boundary_conflicts:
                    region["regionLabelActor"] = nearest_label
                    region["regionLabelDistance"] = round(math.hypot(
                        location["X"] - nearest_location["X"], location["Y"] - nearest_location["Y"]), 2)
                    region["boundaryRegionAtEntrance"] = containing[0]["displayName"]
                    region["validationNote"] = "Nearest same-world-region cooked map label is used because entrance context and map-boundary placement disagree."
                result[humanize_identifier(match.group(1))] = region
    return result, regions


def extract_recipe_dispenser_locations(
    asset_records: list[tuple[Path, dict[str, Any]]]
) -> dict[str, list[dict[str, Any]]]:
    """Tie recipe dispenser classes to cooked dungeon/overworld placements."""
    records_by_file: dict[Path, list[dict[str, Any]]] = defaultdict(list)
    for path, record in asset_records:
        records_by_file[path].append(record)
    dungeon_regions, world_regions = extract_dungeon_regions(records_by_file)

    anchors: list[tuple[str, dict[str, float], Path, str]] = []
    for path, records in records_by_file.items():
        for record in records:
            label = str(record.get("ActorLabel", ""))
            match = re.match(r"Dungeon(?:Actor|Exit)_([A-Za-z0-9_]+)", label)
            if not match:
                continue
            location = placed_actor_location(record, records)
            if location:
                anchors.append((humanize_identifier(match.group(1)), location, path, label))

    result: dict[str, list[dict[str, Any]]] = defaultdict(list)
    for path, records in records_by_file.items():
        normalized_path = str(path).replace("\\", "/")
        if "/Maps/World/L_World/_Generated_/" not in normalized_path:
            continue
        labels = [str(record.get("ActorLabel", "")) for record in records if record.get("ActorLabel")]
        colocated_name = ""
        colocated_evidence = ""
        for label in labels:
            match = re.search(r"FinalChest_([A-Za-z0-9]+)|DungeonExit_([A-Za-z0-9_]+)", label)
            if match:
                colocated_name = humanize_identifier(match.group(1) or match.group(2))
                if match.group(1) and not colocated_name.endswith("Kara"):
                    colocated_name += " Kara"
                colocated_evidence = label
                break
        for record in records:
            record_type = str(record.get("Type", ""))
            if not (record_type.startswith("BP_VaultRecipeDispenser_") and record_type.endswith("_C")):
                continue
            location = placed_actor_location(record, records)
            place_name = colocated_name
            place_kind = "dungeon" if place_name else ""
            place_evidence = colocated_evidence
            distance = 0.0
            if not place_name and location and anchors:
                nearest = min(anchors, key=lambda anchor: math.hypot(
                    location["X"] - anchor[1]["X"], location["Y"] - anchor[1]["Y"]))
                distance = math.hypot(location["X"] - nearest[1]["X"], location["Y"] - nearest[1]["Y"])
                if distance <= 30000:
                    place_name, place_kind, place_evidence = nearest[0], "dungeon", nearest[3]
            landmarks = [label for label in labels if label.endswith("_PostBox")]
            if place_name.startswith("Spectral Room") and landmarks:
                preferred = next((label for label in landmarks if "SilverthornKeep" in label), landmarks[0])
                landmark_name = humanize_identifier(preferred.removesuffix("_PostBox")).replace("FH ", "Fellhollow — ")
                place_name = re.sub(r"\s+\d+$", "", place_name) + f" ({landmark_name})"
                place_evidence += "; " + preferred
            if not place_name:
                if landmarks:
                    preferred = next((label for label in landmarks if "SilverthornKeep" in label), landmarks[0])
                    place_name = humanize_identifier(preferred.removesuffix("_PostBox"))
                    place_name = place_name.replace("FH ", "Fellhollow — ")
                    place_kind, place_evidence = "overworld", preferred
            placement = {
                "name": place_name or "unknown world placement",
                "kind": place_kind or "unknown",
                "worldPartitionCell": path.stem,
                "assetPath": "/Game/Maps/World/L_World/_Generated_/" + path.stem,
                "evidenceActor": place_evidence,
            }
            region = dungeon_regions.get(place_name)
            if not region and location:
                containing = [candidate for candidate in world_regions
                              if point_in_polygon(location["X"], location["Y"], candidate["polygon"])]
                if len(containing) == 1:
                    region = {key: value for key, value in containing[0].items() if key != "polygon"}
            if not region and landmarks:
                preferred_landmark = next((label for label in landmarks if "SilverthornKeep" in label), landmarks[0])
                landmark_words = humanize_identifier(preferred_landmark.removesuffix("_PostBox")).replace("FH ", "")
                matching = [candidate for candidate in world_regions if candidate["name"] in landmark_words]
                if len(matching) == 1:
                    region = {key: value for key, value in matching[0].items() if key != "polygon"}
            if region:
                placement["region"] = region
            if location:
                placement["coordinates"] = location
            if distance:
                placement["anchorDistance"] = round(distance, 2)
            result[record_type.removesuffix("_C")].append(placement)
    return result


def extract_quest_reward_contexts(
    asset_records: list[tuple[Path, dict[str, Any]]]
) -> dict[tuple[Path, str], dict[str, Any]]:
    """Resolve quest reward nodes to localized quests and graph milestones."""
    records_by_file: dict[Path, list[dict[str, Any]]] = defaultdict(list)
    quest_data: dict[str, dict[str, Any]] = {}
    npc_assets: list[dict[str, str]] = []
    for path, record in asset_records:
        records_by_file[path].append(record)
        record_name = str(record.get("Name", ""))
        if record_name.startswith("BP_NPC_") and record.get("Package"):
            npc_assets.append({
                "internalName": record_name.removesuffix("_C"),
                "assetPath": str(record.get("Package", "")),
                "searchKey": re.sub(r"[^a-z0-9]", "", record_name.casefold()),
            })
        if record.get("Type") == "QuestData":
            props = record.get("Properties") or {}
            objectives = {
                str(item.get("Key", "")): text(item.get("Value"))
                for item in props.get("ObjectiveTexts", []) if isinstance(item, dict)
            }
            quest_data[str(record.get("Name", ""))] = {
                "internalName": record.get("Name", ""),
                "displayName": text(props.get("QuestName")) or record.get("Name", ""),
                "assetPath": record.get("Package", ""),
                "region": str(props.get("QuestRegion", "")).split("::")[-1],
                "hidden": bool(props.get("bHideInQuestList")),
                "objectives": objectives,
            }

    contexts: dict[tuple[Path, str], dict[str, Any]] = {}
    reward_types = {"QFT_GiveCraftingRecipe_C", "QFT_UnlockBuildingPieces_C"}
    for path, records in records_by_file.items():
        rewards = [record for record in records if record.get("Type") in reward_types]
        if not rewards:
            continue
        guid_to_record = {
            str((record.get("Properties") or {}).get("Compiled_NodeGUID")): record
            for record in records if (record.get("Properties") or {}).get("Compiled_NodeGUID")
        }
        reverse: dict[str, list[dict[str, Any]]] = defaultdict(list)
        for record in records:
            props = record.get("Properties") or {}
            for target_guid in props.get("OutputConnections", []):
                reverse[str(target_guid)].append(record)

        flow_quest_refs = {
            ref_name((node.get("Properties") or {}).get("QuestData") or
                     (node.get("Properties") or {}).get("Quest Data"))
            for node in records
        } - {""}
        def flow_identity(value: str) -> str:
            return re.sub(r"(?:qf|quest)|beat\d+|_", "", value.casefold())

        flow_key = flow_identity(path.stem)
        ranked_flow_quests = sorted(
            ((SequenceMatcher(None, flow_key, flow_identity(candidate)).ratio(), candidate)
             for candidate in flow_quest_refs),
            reverse=True,
        )
        primary_flow_quest = ranked_flow_quests[0][1] if ranked_flow_quests and ranked_flow_quests[0][0] >= 0.55 else ""

        def connected(start: dict[str, Any], forward: bool, limit: int = 16) -> list[tuple[int, dict[str, Any]]]:
            queue: list[tuple[int, dict[str, Any]]] = [(0, start)]
            seen: set[str] = set()
            found: list[tuple[int, dict[str, Any]]] = []
            while queue:
                distance, current = queue.pop(0)
                guid = str((current.get("Properties") or {}).get("Compiled_NodeGUID", ""))
                if guid in seen or distance > limit:
                    continue
                seen.add(guid)
                if distance:
                    found.append((distance, current))
                if forward:
                    neighbors = [guid_to_record[target] for target in (current.get("Properties") or {}).get("OutputConnections", [])
                                 if str(target) in guid_to_record]
                else:
                    neighbors = reverse.get(guid, [])
                queue.extend((distance + 1, neighbor) for neighbor in neighbors)
            return found

        for reward in rewards:
            forward_nodes = connected(reward, True)
            previous_nodes = connected(reward, False)
            quest_ref = primary_flow_quest
            completion: tuple[int, dict[str, Any]] | None = next(
                ((distance, node) for distance, node in forward_nodes if node.get("Type") == "QFT_CompleteQuest_C"), None)
            if completion:
                completion_quest = ref_name((completion[1].get("Properties") or {}).get("QuestData"))
                if not quest_ref:
                    quest_ref = completion_quest
                elif completion_quest and completion_quest != quest_ref:
                    completion = None
            if not quest_ref:
                # A later, unrelated branch can reference another quest in a shared
                # conversation database. Only ancestry can attribute a pre-completion
                # reward to a quest safely.
                candidates = previous_nodes
                candidates.sort(key=lambda item: item[0])
                for _, node in candidates:
                    props = node.get("Properties") or {}
                    quest_ref = ref_name(props.get("QuestData") or props.get("Quest Data"))
                    if quest_ref:
                        break
            direct_parent = ref_name((reward.get("Properties") or {}).get("ParentNode"))
            if not quest_ref and "ConversationEntryPointNode" not in direct_parent:
                # Some dialogue rewards update their quest immediately after the
                # grant (for example the Goblin Chef branch of Black Knight's
                # Fortress). A direct conversation-entry reward is instead a
                # standalone interaction, even if it sets a quest compatibility
                # flag afterward (the Masterwork Statues case).
                for _, node in sorted(forward_nodes, key=lambda item: item[0]):
                    props = node.get("Properties") or {}
                    quest_ref = ref_name(props.get("QuestData") or props.get("Quest Data"))
                    if quest_ref:
                        break
            quest = quest_data.get(quest_ref, {
                "internalName": quest_ref,
                "displayName": humanize_identifier(quest_ref.removeprefix("Quest_")),
                "assetPath": "",
                "region": "",
                "hidden": False,
                "objectives": {},
            })
            if completion:
                milestone_type = "quest_completion"
                milestone = "Quest completion"
                qualifier = "hidden quest/event " if quest.get("hidden") else ""
                action = f"Complete {qualifier}{str(quest['displayName']).removeprefix('[PH] ').strip()}"
            else:
                objective_node = next((node for _, node in sorted(previous_nodes, key=lambda item: item[0])
                                       if node.get("Type") == "QFT_SetQuestObjective_C"), None)
                objective_key = str(((objective_node or {}).get("Properties") or {}).get("TextName", ""))
                objective = quest.get("objectives", {}).get(objective_key, "")
                prompt_node = next((node for _, node in sorted(previous_nodes, key=lambda item: item[0])
                                    if node.get("Type") == "QFT_PromptMessage_C" and text((node.get("Properties") or {}).get("Message"))), None)
                prompt_props = (prompt_node or {}).get("Properties") or {}
                prompt = text(prompt_props.get("Message"))
                participant_tag = str((prompt_props.get("SpeakerTag") or {}).get("TagName", ""))
                participant_parts = participant_tag.split(".")
                participant_index = next((index for index, part in enumerate(participant_parts)
                                          if part.casefold() == "participant"), -1)
                participant_name = (participant_parts[participant_index + 1]
                                    if 0 <= participant_index < len(participant_parts) - 1
                                    else participant_parts[-1])
                npc = None
                if prompt and participant_name and participant_name.casefold() not in {"player", "narrator"}:
                    display_name = humanize_identifier(participant_name)
                    speaker_keys = {
                        re.sub(r"[^a-z0-9]", "", part.casefold())
                        for part in participant_parts[participant_index + 1:] if part
                    }
                    candidates = [asset for asset in npc_assets
                                  if any(key and key in asset["searchKey"] for key in speaker_keys)]
                    npc = {
                        "displayName": display_name,
                        "participantTag": participant_tag,
                        "assetCandidates": [
                            {"internalName": asset["internalName"], "assetPath": asset["assetPath"]}
                            for asset in sorted(candidates, key=lambda item: (item["internalName"], item["assetPath"]))
                        ],
                        "evidenceNode": (prompt_node or {}).get("Name", ""),
                    }
                state_node = next((node for _, node in sorted(previous_nodes, key=lambda item: item[0])
                                   if node.get("Type") in {"QFT_SetQuestBool_C", "QFT_ModifyQuestInt_C"}), None)
                state_props = (state_node or {}).get("Properties") or {}
                state_name = state_props.get("BoolName") or state_props.get("IntName") or ""
                state_milestone = (f'Quest state "{humanize_identifier(str(state_name))}" is set'
                                   if state_name else "")
                entry_node = next((node for _, node in sorted(previous_nodes, key=lambda item: item[0])
                                   if node.get("Type") == "ConversationEntryPointNode"), None)
                entry_tag = (((entry_node or {}).get("Properties") or {}).get("EntryTag") or {}).get("TagName", "")
                entry_name = str(entry_tag).split(".")[-1].removesuffix("Entry")
                entry_milestone = f"Interact with {humanize_identifier(entry_name)}" if entry_name else ""
                milestone_type = "quest_progress" if quest_ref else "quest_flow_event"
                milestone = objective or prompt or state_milestone or entry_milestone or f"Activate reward node {reward.get('Name', '')}"
                if quest_ref:
                    quest_name = str(quest['displayName']).removeprefix('[PH] ').strip()
                    if npc:
                        action = f"Progress {quest_name}: Talk to {npc['displayName']} — {milestone}"
                    else:
                        action = f"Progress {quest_name}: {milestone}"
                else:
                    event_name = humanize_identifier(path.stem.removeprefix("QF_"))
                    if npc:
                        action = f"Trigger {event_name} event: Talk to {npc['displayName']} — {milestone}"
                    else:
                        action = f"Trigger {event_name} event: {milestone}"
            context = {
                "action": action,
                "quest": {key: value for key, value in quest.items() if key != "objectives"},
                "milestone": {"type": milestone_type, "description": milestone},
                "graph": {
                    "flowAsset": str(path),
                    "rewardNode": reward.get("Name", ""),
                    "rewardNodeGuid": (reward.get("Properties") or {}).get("Compiled_NodeGUID", ""),
                    "completionNode": completion[1].get("Name", "") if completion else "",
                    "completionDistance": completion[0] if completion else None,
                },
            }
            if not completion and npc:
                context["npc"] = npc
            contexts[(path, str(reward.get("Name", "")))] = context
    return contexts


def refs(value: Any) -> list[dict[str, str]]:
    if isinstance(value, dict) and ("ObjectName" in value or "ObjectPath" in value or "AssetPathName" in value):
        return [{"internalName": ref_name(value), "assetPath": ref_path(value)}]
    if isinstance(value, list):
        result: list[dict[str, str]] = []
        for item in value:
            result.extend(refs(item))
        return result
    if isinstance(value, dict):
        result = []
        for item in value.values():
            result.extend(refs(item))
        return result
    return []


def iter_records(root: Path) -> Iterable[tuple[Path, dict[str, Any]]]:
    for path in sorted(root.rglob("*.json")):
        if path.name == ".uas-manifest.json":
            continue
        try:
            data = read_json(path)
        except (OSError, json.JSONDecodeError):
            continue
        if isinstance(data, list):
            for record in data:
                if isinstance(record, dict):
                    yield path, record


def operand(value: dict[str, Any]) -> dict[str, Any]:
    unlock_type = str(value.get("UnlockType", "")).split("::")[-1]
    result: dict[str, Any] = {
        "type": TRIGGER_LABELS.get(unlock_type, unlock_type or "unknown"),
        "gameType": unlock_type,
        "condition": str(value.get("Condition", "")).split("::")[-1],
        "numberOfMatchesRequired": int(value.get("NumberOfMatchesRequired", 0) or 0),
    }
    targets = refs(value.get("Items", [])) + refs(value.get("Actors", []))
    skill = value.get("Skill")
    entitlement = value.get("Entitlement")
    if skill:
        targets += refs(skill)
        result["requiredSkillLevel"] = int(value.get("RequiredSkillLevel", 0) or 0)
    if entitlement:
        targets += refs(entitlement)
    course = value.get("AgilityCourseUnlockRequirements") or {}
    targets += refs(course.get("CourseDataAssets", []))
    if targets:
        result["targets"] = targets
    medal = str(course.get("MinimumRequiredMedal", "")).split("::")[-1]
    if course.get("CourseDataAssets") and medal:
        result["minimumMedal"] = medal
    return result


def progression_trigger(
    asset: str, row_name: str, row: dict[str, Any], actor_display_names: dict[str, str]
) -> dict[str, Any]:
    query = row.get("UnlockQuery") or {}
    parts = [operand(query.get("FirstOperand") or {})]
    if query.get("bUseSecondOperand"):
        parts.append(operand(query.get("SecondOperand") or {}))
    types = {part["type"] for part in parts}
    trigger_type = next(iter(types)) if len(types) == 1 else "compound"
    action = str(row.get("UnlockQueryString") or row_name)
    for part in parts:
        if part.get("type") != "open_station_ui":
            continue
        for target in part.get("targets", []):
            internal_name = str(target.get("internalName", ""))
            display_name = actor_display_names.get(internal_name, "")
            if not display_name:
                continue
            target["displayName"] = display_name
            action = re.sub(rf"(?<![A-Za-z0-9_]){re.escape(internal_name)}(?:_C)?(?![A-Za-z0-9_])",
                            display_name, action)
    return {
        "type": trigger_type,
        "action": action,
        "source": {"assetPath": asset, "row": row_name},
        "query": {
            "operator": str(query.get("Operator", "")).split("::")[-1] or "AND",
            "operands": parts,
        },
        "evidence": "static:cooked_progression_row",
        "confidence": "proven",
    }


def direct_trigger(kind: str, action: str, path: str, record: dict[str, Any], field: str) -> dict[str, Any]:
    return {
        "type": kind,
        "action": action,
        "source": {
            "assetPath": record.get("Package", ""),
            "internalName": record.get("Name", ""),
            "property": field,
        },
        "evidence": f"static:{path}",
        "confidence": "proven",
    }


def _operand_semantics(path: dict[str, Any]) -> tuple[str, str, frozenset[str]] | None:
    """Return simple one-operand set semantics for safe redundancy checks."""
    query = path.get("query") or {}
    operands = query.get("operands") or []
    if len(operands) != 1:
        return None
    operand_value = operands[0]
    targets = frozenset(target.get("internalName", "") for target in operand_value.get("targets", []) if target.get("internalName"))
    condition = operand_value.get("condition", "")
    if not targets or condition not in {"AnyMatch", "AllMatch"}:
        return None
    # ANY and ALL are identical for a singleton target.
    if len(targets) == 1:
        condition = "AllMatch"
    return str(operand_value.get("gameType") or operand_value.get("type")), condition, targets


def _dominates(easier: tuple[str, str, frozenset[str]], harder: tuple[str, str, frozenset[str]]) -> bool:
    """True when every state satisfying harder also satisfies easier."""
    easy_type, easy_condition, easy_targets = easier
    hard_type, hard_condition, hard_targets = harder
    if easy_type != hard_type:
        return False
    if easy_condition == "AllMatch" and hard_condition == "AllMatch":
        return easy_targets <= hard_targets
    if easy_condition == "AnyMatch" and hard_condition == "AnyMatch":
        return hard_targets <= easy_targets
    if easy_condition == "AnyMatch" and hard_condition == "AllMatch":
        return bool(easy_targets & hard_targets)
    if easy_condition == "AllMatch" and hard_condition == "AnyMatch":
        return len(easy_targets) == 1 and hard_targets == easy_targets
    return False


def canonicalize_unlock_paths(paths: list[dict[str, Any]]) -> tuple[list[dict[str, Any]], list[dict[str, Any]]]:
    """Merge equivalent progression paths and suppress strictly dominated ones.

    Suppressed paths remain attached to the recipe for audit/update purposes.
    """
    canonical: list[dict[str, Any]] = []
    redundant: list[dict[str, Any]] = []
    by_semantics: dict[tuple[str, str, frozenset[str]], dict[str, Any]] = {}
    for original in paths:
        path = copy.deepcopy(original)
        semantics = _operand_semantics(path)
        if semantics is None:
            canonical.append(path)
            continue
        duplicate = by_semantics.get(semantics)
        if duplicate is not None:
            duplicate.setdefault("sources", [duplicate["source"]])
            if path.get("source") not in duplicate["sources"]:
                duplicate["sources"].append(path["source"])
            duplicate["evidence"] = "static:multiple_cooked_progression_rows"
            continue
        path["sources"] = [path["source"]]
        by_semantics[semantics] = path
        canonical.append(path)

    simple = [(path, _operand_semantics(path)) for path in canonical]
    suppressed_ids: set[int] = set()
    for harder, hard_semantics in simple:
        if hard_semantics is None:
            continue
        for easier, easy_semantics in simple:
            if easier is harder or easy_semantics is None:
                continue
            if _dominates(easy_semantics, hard_semantics) and not _dominates(hard_semantics, easy_semantics):
                harder["redundantBecause"] = f"The effective path '{easier['action']}' always unlocks this entry no later."
                redundant.append(harder)
                suppressed_ids.add(id(harder))
                break
    return [path for path in canonical if id(path) not in suppressed_ids], redundant


def md(value: Any) -> str:
    return str(value or "").replace("|", "\\|").replace("\n", " ").strip()


def render_docs(result: dict[str, Any], docs_dir: Path) -> None:
    docs_dir.mkdir(parents=True, exist_ok=True)
    recipes = result["recipes"]
    groups: dict[str, list[dict[str, Any]]] = defaultdict(list)
    for entry in recipes:
        primary = entry["unlockPaths"][0]["type"] if entry["unlockPaths"] else "unresolved"
        groups[primary].append(entry)
    lines = [
        "# Recipe unlock catalog", "",
        f"Generated for the stamped game build in `reference/recipe-unlocks.json`. "
        f"This list contains **{result['counts']['totalActive']}** active crafting/building recipes.", "",
        "Each row states the exact recorded action. Compound rows retain their full Boolean query in the JSON dataset.", "",
    ]
    order = ["default", "obtain_item", "open_station_ui", "compound", "consume_vestige",
             "consume_unlock_item", "interact_recipe_dispenser", "complete_quest", "skill_or_perk",
             "entitlement", "unresolved"]
    for kind in order + sorted(set(groups) - set(order)):
        entries = sorted(groups.get(kind, []), key=lambda item: (item["kind"], item["displayName"].casefold(), item["internalName"]))
        if not entries:
            continue
        lines += [f"## {kind.replace('_', ' ').title()} ({len(entries)})", "",
                  "| Type | Recipe | Internal name | Persistent ID | Required action |", "|---|---|---|---|---|"]
        for entry in entries:
            actions = " **OR** ".join(path["action"] for path in entry["unlockPaths"]) or "UNRESOLVED — not classified as unlocked"
            lines.append(f"| {md(entry['kind'])} | {md(entry['displayName'])} | `{md(entry['internalName'])}` | `{md(entry['persistentId'])}` | {md(actions)} |")
        lines.append("")
    (docs_dir / "recipe-unlocks.md").write_text("\n".join(lines) + "\n", encoding="utf-8")

    excluded = ["# Recipe unlock exclusions", "",
                "Assets omitted from the active list are retained here; none are silently discarded.", "",
                "| Type | Internal name | Persistent ID | Reason | Asset |", "|---|---|---|---|---|"]
    for entry in result["exclusions"]:
        excluded.append(f"| {md(entry.get('kind'))} | `{md(entry.get('internalName'))}` | `{md(entry.get('persistentId'))}` | {md(entry.get('reason'))} | `{md(entry.get('assetPath'))}` |")
    (docs_dir / "recipe-unlock-exclusions.md").write_text("\n".join(excluded) + "\n", encoding="utf-8")


def render_web_catalog(
    result: dict[str, Any], asset_records: list[tuple[Path, dict[str, Any]]], output_path: Path,
    runtime_output_path: Path | None = None, runtime_history_path: Path | None = None,
    runtime_history_imports: list[Path] | None = None,
) -> dict[str, int]:
    """Emit a compact, display-only dependency graph for the public website."""
    recipes = result["recipes"]
    labels: dict[str, str] = {}
    buildable_to_piece: dict[str, str] = {}
    station_rows: dict[str, str] = {}
    # Labels must be collected before station rows: asset export order is not
    # guaranteed to put BuildingPieceData ahead of either station DataTable.
    for _, record in asset_records:
        props = record.get("Properties") or {}
        name = str(record.get("Name", ""))
        label = text(props.get("Name")) or text(props.get("DisplayName"))
        if name and label:
            labels[name] = label
        actor = ref_name(props.get("BuildableActor"))
        if record.get("Type") == "BuildingPieceData" and actor:
            buildable_to_piece[actor] = name
    for _, record in asset_records:
        name = str(record.get("Name", ""))
        if record.get("Type") == "DataTable" and name in {
            "DT_CraftingStationsDataTable", "DT_ProcessingStationDataTable"
        }:
            for row_name, row in (record.get("Rows") or {}).items():
                if not isinstance(row, dict):
                    continue
                piece = ref_name(row.get("StationBuildingPieceData"))
                override = text(row.get("DisplayNameOverride"))
                row_key = str(row_name)
                station_rows[row_key] = (STATION_DISPLAY_NAMES.get(row_key) or override or
                                         labels.get(piece, "") or humanize_identifier(row_key))

    def public_id(entry: dict[str, Any]) -> str:
        seed = f"{entry['kind']}:{entry['persistentId']}:{entry['internalName']}".encode()
        return hashlib.sha256(seed).hexdigest()[:16]

    ids = {entry["internalName"]: public_id(entry) for entry in recipes}
    output_producers: dict[str, list[str]] = defaultdict(list)
    for entry in recipes:
        for produced in entry.get("output", []):
            internal = produced.get("internalName", "")
            if internal:
                output_producers[internal].append(ids[entry["internalName"]])
                labels.setdefault(internal, entry["displayName"])
    actor_unlocks: dict[str, list[str]] = defaultdict(list)
    for actor, piece in buildable_to_piece.items():
        if piece in ids:
            actor_unlocks[actor].append(ids[piece])

    icon_root = output_path.parent / "recipe-icons"
    icon_root.mkdir(parents=True, exist_ok=True)
    used_icons: set[str] = set()

    def public_icon(internal: str) -> str:
        icon_internal = buildable_to_piece.get(internal, internal)
        source = next((candidate for candidate in (
            output_path.parent / "items" / f"{icon_internal}.png",
            output_path.parent / "buildings" / f"{icon_internal}.png",
        ) if candidate.is_file()), None)
        if source is None:
            return ""
        filename = hashlib.sha256(f"icon:{icon_internal}".encode()).hexdigest()[:16] + ".png"
        destination = icon_root / filename
        if not destination.exists() or destination.stat().st_size != source.stat().st_size:
            shutil.copyfile(source, destination)
        used_icons.add(filename)
        return f"/assets/recipe-icons/{filename}"

    def target_label(target: dict[str, Any]) -> str:
        internal = str(target.get("internalName", ""))
        return str(target.get("displayName") or labels.get(internal) or humanize_identifier(
            re.sub(r"^(?:ITEM|BP|DA|BUILDPIECE|ED|SKILL)_", "", internal).removesuffix("_C")
        ))

    def friendly_action(path: dict[str, Any]) -> str:
        action = str(path.get("action", "Complete the recorded unlock requirement")).strip()
        action = re.sub(r"\bSKILL_([A-Za-z0-9_]+)",
                        lambda match: humanize_identifier(match.group(1)), action)
        action = action.replace("deluxe_edition", "Deluxe Edition")
        if re.search(r"(?:ALL|ANY)\s+(?:ITEMS|ACTORS|\[)|SKILL LEVEL", action):
            return "Satisfy the recorded progression requirement"
        return action

    def web_route(path: dict[str, Any], current: dict[str, Any]) -> dict[str, Any] | None:
        query = path.get("query") or {}
        operands = query.get("operands") or []
        steps: list[dict[str, Any]] = []
        for operand_value in operands:
            kind = str(operand_value.get("type", ""))
            condition = str(operand_value.get("condition", "AllMatch"))
            targets = []
            for target in operand_value.get("targets", []):
                internal = str(target.get("internalName", ""))
                dependencies = (actor_unlocks.get(internal, []) if kind == "open_station_ui"
                                else output_producers.get(internal, []))
                targets.append({"name": target_label(target), "icon": public_icon(internal),
                                "recipes": sorted(set(dependencies))})
            names = [target["name"] for target in targets]
            if kind == "obtain_item" and not names:
                continue
            if kind == "obtain_item" and names:
                if condition == "AnyMatch" and len(names) > 1:
                    description = "Obtain any one of " + ", ".join(names[:-1]) + " or " + names[-1]
                else:
                    description = "Obtain " + (", ".join(names[:-1]) + " and " + names[-1] if len(names) > 1 else names[0])
            elif kind == "open_station_ui" and names:
                verb = "Open " if len(names) == 1 else "Open each of "
                description = verb + (", ".join(names[:-1]) + " and " + names[-1] if len(names) > 1 else names[0])
            elif kind == "entitlement" and names:
                description = "Own " + (", ".join(names[:-1]) + " and " + names[-1] if len(names) > 1 else names[0]) + " entitlement"
            elif kind == "SkillLevelReached":
                skill_names = [target_label(target) for target in operand_value.get("targets", [])
                               if str(target.get("internalName", "")).startswith("SKILL_")]
                skill = skill_names[0] if skill_names else "the required skill"
                description = f"Reach {skill} level {operand_value.get('requiredSkillLevel', '?')}"
                targets = []
            else:
                description = friendly_action(path)
            steps.append({
                "kind": kind or str(path.get("type", "action")),
                "mode": "any" if condition == "AnyMatch" else "all",
                "text": description.rstrip("."),
                "targets": targets,
            })
        if not steps:
            action = friendly_action(path)
            if path.get("type") == "default":
                action = "Available from the start"
            steps.append({"kind": str(path.get("type", "action")), "mode": "all",
                          "text": action.rstrip("."), "targets": []})
        return {
            "mode": "or" if str(query.get("operator", "AND")).upper() == "OR" else "and",
            "steps": steps,
        }

    entries = []
    for entry in recipes:
        station = "Building Pieces" if entry["kind"] == "building" else str(entry.get("station") or "Other Crafting")
        station = station_rows.get(station) or STATION_DISPLAY_NAMES.get(station, station)
        if entry["kind"] == "building":
            sections = ["Building Pieces"]
        else:
            sections = sorted({
                station_rows.get(str(candidate.get("station", ""))) or
                STATION_DISPLAY_NAMES.get(str(candidate.get("station", "")), str(candidate.get("station", "")))
                for candidate in entry.get("stationCandidates", [])
                if candidate.get("station") and candidate.get("station") not in INTERNAL_STATION_ROWS
            } - PUBLIC_HIDDEN_STATION_SECTIONS, key=str.casefold) or [station]
        routes = [route for path in entry["unlockPaths"] if (route := web_route(path, entry)) is not None]
        if not routes:
            continue
        output_internal = next((str(output.get("internalName", "")) for output in entry.get("output", [])
                                if output.get("internalName")), "")
        entries.append({
            "id": ids[entry["internalName"]], "name": entry["displayName"],
            "kind": entry["kind"], "section": station, "sections": sections,
            "icon": public_icon(output_internal),
            "types": public_recipe_types(entry),
            "routes": routes,
        })
    entries.sort(key=lambda entry: (entry["section"].casefold(), entry["name"].casefold(), entry["id"]))

    live_ids = {entry["id"] for entry in entries}
    for entry in entries:
        for route in entry["routes"]:
            for step in route["steps"]:
                for target in step["targets"]:
                    target["recipes"] = [recipe_id for recipe_id in target["recipes"] if recipe_id in live_ids]

    adjacency: dict[str, set[str]] = defaultdict(set)
    for entry in entries:
        for route in entry["routes"]:
            for step in route["steps"]:
                for target in step["targets"]:
                    adjacency[entry["id"]].update(target["recipes"])
    cycle_edges: set[tuple[str, str]] = set()
    visited: set[str] = set()
    active: set[str] = set()
    def visit(node: str) -> None:
        if node in visited:
            return
        visited.add(node); active.add(node)
        for child in adjacency.get(node, set()):
            if child in active:
                cycle_edges.add((node, child))
            else:
                visit(child)
        active.remove(node)
    for entry in entries:
        visit(entry["id"])
    edge_count = sum(len(children) for children in adjacency.values())
    building_icons = {
        entry["name"]: entry["icon"] for entry in entries
        if entry["kind"] == "building" and entry.get("icon")
    }
    section_icons = {
        section: building_icons.get(section, "")
        for section in sorted({section for entry in entries for section in entry.get("sections", [])})
    }
    section_icons["Building Pieces"] = public_icon("DA_T1_Foundation_Large")
    section_icons["Brewing Barrel"] = building_icons.get("Imbuing Barrel", "")
    section_icons["Stone Cooking Range"] = building_icons.get("Stone Range", "")
    for entry in entries:
        for section in entry.get("sections", []):
            if not section_icons.get(section) and entry.get("icon"):
                section_icons[section] = entry["icon"]
    payload = {
        "schemaVersion": 1,
        "generatedAt": result["generatedAt"],
        "counts": {"recipes": len(entries), "dependencyEdges": edge_count, "cycles": len(cycle_edges)},
        "sectionIcons": section_icons,
        "entries": entries,
    }
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(json.dumps(payload, ensure_ascii=False, separators=(",", ":")) + "\n", encoding="utf-8")
    if runtime_output_path is not None:
        history: dict[str, list[str]] = {}
        if runtime_history_path is not None and runtime_history_path.exists():
            for line in runtime_history_path.read_text(encoding="utf-8").splitlines():
                fields = line.split("\t")
                if len(fields) == 3 and fields[0] == "C" and len(fields[1]) == 64:
                    history[fields[1]] = sorted(set(value for value in fields[2].split(",") if len(value) == 16))
        prior_catalogs = ([runtime_output_path] if runtime_output_path.exists() else []) + (runtime_history_imports or [])
        for prior_catalog in prior_catalogs:
            old_lines = prior_catalog.read_text(encoding="utf-8").splitlines()
            match = re.search(r"fingerprint=([0-9a-f]{64})", old_lines[0] if old_lines else "")
            old_ids = [line.split("\t", 1)[0] for line in old_lines[1:] if line]
            if match and all(len(recipe_id) == 16 for recipe_id in old_ids):
                history[match.group(1)] = sorted(set(old_ids))
        runtime_rows = sorted(
            (ids[entry["internalName"]], entry["kind"], entry["internalName"])
            for entry in recipes if entry["internalName"] in ids
        )
        runtime_body = "".join(f"{opaque_id}\t{kind}\t{internal}\n" for opaque_id, kind, internal in runtime_rows)
        runtime_fingerprint = hashlib.sha256(runtime_body.encode()).hexdigest()
        runtime_output_path.parent.mkdir(parents=True, exist_ok=True)
        runtime_output_path.write_text(
            f"# schema=1 fingerprint={runtime_fingerprint} count={len(runtime_rows)}\n" + runtime_body,
            encoding="utf-8",
        )
        if runtime_history_path is not None:
            history[runtime_fingerprint] = sorted(row[0] for row in runtime_rows)
            runtime_history_path.parent.mkdir(parents=True, exist_ok=True)
            runtime_history_path.write_text("# schema=1\n" + "".join(
                f"C\t{catalog_fingerprint}\t{','.join(recipe_ids)}\n"
                for catalog_fingerprint, recipe_ids in sorted(history.items())), encoding="utf-8")
    for stale in icon_root.glob("*.png"):
        if stale.name not in used_icons:
            stale.unlink()
    return payload["counts"]


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--assets", type=Path, default=Path("dist/recipe-unlock-assets-current"))
    parser.add_argument("--building-assets", type=Path, action="append", default=None,
                        help="Building JSON root; repeat to merge roots")
    parser.add_argument("--building-catalog", type=Path, default=Path("web/buildings/catalog.json"))
    parser.add_argument("--editor-data", type=Path, default=Path("dist/dw-editor-release/data"))
    parser.add_argument("--usmap", type=Path, default=Path("dist/RSDragonwilds-current.usmap"))
    parser.add_argument("--game-exe", type=Path, default=Path(r"E:\SteamLibrary\steamapps\common\RSDragonwilds\RSDragonwilds\Binaries\Win64\RSDragonwilds-Win64-Shipping.exe"))
    parser.add_argument("--fresh-baseline", type=Path, default=Path("reference/recipe-unlock-testdude-baseline.json"))
    parser.add_argument("--output", type=Path, default=Path("reference/recipe-unlocks.json"))
    parser.add_argument("--docs-dir", type=Path, default=Path("docs"))
    parser.add_argument("--web-output", type=Path, default=Path("web/recipe-unlocks.json"))
    parser.add_argument("--runtime-web-output", type=Path, default=Path("web/recipe-runtime-catalog.tsv"))
    parser.add_argument("--runtime-history-output", type=Path, default=Path("web/recipe-runtime-catalog-history.tsv"))
    parser.add_argument("--runtime-history-import", type=Path, action="append", default=[])
    parser.add_argument("--allow-unresolved", action="store_true")
    args = parser.parse_args()

    editor_recipes = read_json(args.editor_data / "RecipeID.json")
    recipe_exclusions = {entry["Id"]: entry for entry in read_json(args.editor_data / "RecipeExclusions.json")}
    editor_by_guid = {entry["Id"]: entry for entry in editor_recipes}
    building_catalog = read_json(args.building_catalog)
    catalog_pieces = {piece["key"]: piece for piece in building_catalog["pieces"]}

    building_roots = args.building_assets or [Path("dist/recipe-unlock-building-assets-current"), Path("dist/building-piece-all-json-ue56")]
    building_records = []
    for root in building_roots:
        building_records.extend(iter_records(root))
    # The normal exhaustive update uses the same root for recipes, grants, and
    # building assets. Reuse that parsed record set instead of decoding tens of
    # thousands of JSON files three times.
    same_exhaustive_root = (len(building_roots) == 1 and
                            building_roots[0].resolve() == args.assets.resolve())
    asset_records = building_records if same_exhaustive_root else list(iter_records(args.assets))
    dispenser_locations = extract_recipe_dispenser_locations(asset_records)
    quest_reward_contexts = extract_quest_reward_contexts(asset_records)

    recipe_assets: dict[str, tuple[Path, dict[str, Any]]] = {}
    recipe_guid_duplicates: dict[str, list[str]] = defaultdict(list)
    exclusions: list[dict[str, Any]] = []
    for path, record in asset_records:
        if record.get("Type") not in {"RecipeData", "RuneRecipeData"}:
            continue
        props = record.get("Properties") or {}
        guid = str(props.get("PersistenceID", ""))
        if not guid:
            continue
        if guid in recipe_assets:
            recipe_guid_duplicates[guid].append(str(record.get("Package", "")))
        recipe_assets[guid] = (path, record)
        lower = (str(path) + str(record.get("Name", ""))).lower()
        soft = bool(props.get("bSoftDeleted"))
        excluded = recipe_exclusions.get(guid)
        if soft or excluded or any(token in lower for token in ("/deleted", "deprecated", "depreciated", "/test", "_test", "debug")):
            exclusions.append({
                "kind": "crafting",
                "internalName": record.get("Name", ""),
                "persistentId": guid,
                "assetPath": record.get("Package", ""),
                "reason": (excluded or {}).get("Reason") or ("bSoftDeleted" if soft else "internal/deleted asset path"),
            })

    building_assets: dict[str, tuple[Path, dict[str, Any]]] = {}
    for path, record in building_records:
        name = str(record.get("Name", ""))
        if record.get("Type") == "BuildingPieceData" and name in catalog_pieces:
            building_assets[name] = (path, record)

    trigger_map: dict[tuple[str, str], list[dict[str, Any]]] = defaultdict(list)
    station_candidates: dict[str, list[dict[str, str]]] = defaultdict(list)
    station_metadata_candidates: dict[str, list[dict[str, str]]] = defaultdict(list)
    station_map: dict[str, str] = {}
    display_name_map: dict[str, str] = {}
    actor_display_names: dict[str, str] = {}
    evidence_sources = Counter()
    deleted_journal_recipes: set[str] = set()
    perk_context: dict[Path, dict[str, Any]] = {}
    for _, record in asset_records:
        props = record.get("Properties") or {}
        display_name = text(props.get("DisplayName"))
        if display_name:
            buildable_actor = ref_name(props.get("BuildableActor"))
            if buildable_actor:
                actor_display_names[buildable_actor] = display_name
            owner = ref_name(record.get("Outer"))
            if owner.startswith("BP_"):
                actor_display_names[owner] = display_name
    for path, record in asset_records:
        if record.get("Type") != "SkillPerkData":
            continue
        props = record.get("Properties") or {}
        unlock_info = props.get("PerkUnlockInfo") or {}
        perk_context[path] = {
            "name": text(props.get("PerkName")) or str(record.get("Name", "")),
            "skill": ref_name(unlock_info.get("AssociatedSkill")),
            "level": int(unlock_info.get("RequiredSkillLevel", 0) or 0),
            "package": str(record.get("Package", "")),
        }

    for path, record in asset_records:
        props = record.get("Properties") or {}
        package = str(record.get("Package", ""))
        display_label = text(props.get("Name"))
        if display_label and record.get("Name"):
            display_name_map[str(record["Name"])] = display_label
        if record.get("Type") == "JournalEntryRecipeData" and "/deleted" in package.casefold():
            deleted_journal_recipes.update(value["internalName"] for value in refs(props.get("RecipeData")))
        if record.get("Type") == "DataTable" and ref_name(props.get("RowStruct")) == "ProgressionBundleRow":
            for row_name, row in (record.get("Rows") or {}).items():
                if not isinstance(row, dict):
                    continue
                trigger = progression_trigger(package, row_name, row, actor_display_names)
                for value in refs(row.get("UnlockedRecipes", [])):
                    trigger_map[("crafting", value["internalName"])].append(trigger)
                    evidence_sources["progression_recipe"] += 1
                for value in refs(row.get("UnlockedBuildings", [])):
                    trigger_map[("building", value["internalName"])].append(trigger)
                    evidence_sources["progression_building"] += 1

        table_name = str(record.get("Name", ""))
        if record.get("Type") == "DataTable" and table_name in {
            "DT_CraftingStationsDataTable", "DT_ProcessingStationDataTable"
        }:
            recipe_field = "LabeledRecipes" if table_name == "DT_CraftingStationsDataTable" else "Recipes"
            for station_name, station_row in (record.get("Rows") or {}).items():
                if not isinstance(station_row, dict) or station_name in INTERNAL_STATION_ROWS:
                    continue
                for value in refs(station_row.get(recipe_field, [])):
                    station_candidates[value["internalName"]].append({
                        "station": str(station_name),
                        "table": table_name,
                        "property": f"Rows.{station_name}.{recipe_field}",
                        "assetPath": package,
                    })
        if record.get("Type") == "JournalEntryRecipeData":
            station_row = str((props.get("StationTableRowHandle") or {}).get("RowName", ""))
            for value in refs(props.get("RecipeData")):
                if station_row:
                    # Journal rows describe how the journal groups a recipe and
                    # occasionally lag the live station UI catalogs. Preserve
                    # them as research evidence, but never let them override an
                    # authoritative cooked station-table membership.
                    station_metadata_candidates[value["internalName"]].append({
                        "station": station_row,
                        "table": "JournalEntryRecipeData",
                        "property": "StationTableRowHandle.RowName",
                        "assetPath": package,
                    })
        if record.get("Type") == "StationUpgradeComponent":
            station_actor = ref_name(props.get("StationToUpgrade"))
            station_row = STATION_ACTOR_ROWS.get(station_actor, "")
            if station_row:
                for value in refs((props.get("UpgradeInfos") or {}).get("RecipesToAdd", [])):
                    station_candidates[value["internalName"]].append({
                        "station": station_row,
                        "table": "StationUpgradeComponent",
                        "property": "UpgradeInfos.RecipesToAdd",
                        "assetPath": package,
                    })

        record_type = str(record.get("Type", ""))
        quest_context = quest_reward_contexts.get((path, str(record.get("Name", ""))))
        direct: list[tuple[str, str, str, str]] = []
        if record_type == "BP_Consumables_RecipeUnlocker2_C":
            tags = " ".join(map(str, props.get("ItemFilterTags", []))).lower()
            kind = "consume_vestige" if "vestige" in tags or "vestige" in package.lower() else "consume_unlock_item"
            direct.append(("crafting", "RecipesToUnlock", kind, f"Study/consume {text(props.get('Name')) or record.get('Name')}"))
        elif record_type == "SkillPerkModule_Recipes":
            context = perk_context.get(path, {})
            action = f"Unlock {context.get('name') or record.get('Name')}"
            if context.get("skill") and context.get("level"):
                action += f" at {context['skill'].removeprefix('SKILL_')} level {context['level']}"
            direct.append(("crafting", "RecipesToUnlock", "skill_or_perk", action))
        elif record_type == "SkillPerkModule_BuildingPieces":
            context = perk_context.get(path, {})
            action = f"Unlock {context.get('name') or record.get('Name')}"
            if context.get("skill") and context.get("level"):
                action += f" at {context['skill'].removeprefix('SKILL_')} level {context['level']}"
            direct.append(("building", "BuildingPiecesToUnlock", "skill_or_perk", action))
        elif record_type == "QFT_GiveCraftingRecipe_C":
            reward_kind = "quest_flow_event" if (quest_context or {}).get("milestone", {}).get("type") == "quest_flow_event" else "complete_quest"
            direct.append(("crafting", "Summon Item Recipes", reward_kind,
                           (quest_context or {}).get("action", f"Reach quest-flow reward node in {path.stem}")))
        elif record_type == "QFT_UnlockBuildingPieces_C":
            reward_kind = "quest_flow_event" if (quest_context or {}).get("milestone", {}).get("type") == "quest_flow_event" else "complete_quest"
            direct.append(("building", "Building Pieces", reward_kind,
                           (quest_context or {}).get("action", f"Reach quest-flow reward node in {path.stem}")))
        elif record_type == "EntitlementData":
            label = str(props.get("BackendID") or record.get("Name"))
            direct.append(("crafting", "GrantedRecipes", "entitlement", f"Own the {label} entitlement"))
            direct.append(("building", "GrantedBuildingPieces", "entitlement", f"Own the {label} entitlement"))
        elif record_type == "SkillData" and props.get("TrimmedCapeRecipe"):
            direct.append(("crafting", "TrimmedCapeRecipe", "skill_or_perk", f"Meet trimmed-cape eligibility for {record.get('Name')}"))
        elif record_type == "BP_Consumables_Plan_Base_C":
            direct.append(("building", "BuildingPieceToUnlock", "consume_unlock_item",
                           f"Study/consume {text(props.get('Name')) or record.get('Name')}"))
        elif record_type == "AgilityCourseDataAsset":
            course_name = text((props.get("TimeTrialData") or {}).get("DisplayName")) or str(record.get("Name"))
            medal_labels = {
                "GoldMedal": "gold", "SilverMedal": "silver", "BronzeMedal": "bronze", "NoMedal": "completion"
            }
            for medal_key, medal_label in medal_labels.items():
                reward = (((props.get("TimeTrialData") or {}).get(medal_key) or {}).get("FirstTimeCourseReward") or {})
                for target in refs(reward.get("RecipeUnlocks", [])):
                    trigger_map[("crafting", target["internalName"])].append(
                        direct_trigger("skill_or_perk", f"Earn the {medal_label} first-time reward in {course_name}",
                                       str(path), record, f"TimeTrialData.{medal_key}.FirstTimeCourseReward.RecipeUnlocks"))
                    evidence_sources["agility_course_recipe"] += 1
                for target in refs(reward.get("BuildingPieceUnlocks", [])):
                    trigger_map[("building", target["internalName"])].append(
                        direct_trigger("skill_or_perk", f"Earn the {medal_label} first-time reward in {course_name}",
                                       str(path), record, f"TimeTrialData.{medal_key}.FirstTimeCourseReward.BuildingPieceUnlocks"))
                    evidence_sources["agility_course_building"] += 1
        elif record_type.startswith("BP_BossAltar_") and record_type.endswith("_C"):
            direct.append(("crafting", "Summon Item Recipes", "open_station_ui",
                           "Interact with the boss altar"))
        elif record_type.startswith("BP_VaultRecipeDispenser_") and record_type.endswith("_C"):
            dispenser_name = str(record.get("Name", ""))
            dispenser_class = record_type.removesuffix("_C")
            locations = dispenser_locations.get(dispenser_class, [])
            location_names = list(dict.fromkeys(
                f"{location['name']} ({location['region']['displayName']})"
                if location.get("region") and not (
                    location["region"]["name"] in location["name"] and
                    (not location["region"].get("parent") or location["region"]["parent"] in location["name"])
                )
                else location["name"]
                for location in locations if location["name"] != "unknown world placement"
            ))
            action = f"Interact with {text(props.get('DisplayName')) or dispenser_name}"
            if location_names:
                action += " in " + " or ".join(location_names)
            direct.append(("crafting", "Recipes", "interact_recipe_dispenser", action))
        for recipe_kind, field, kind, action in direct:
            for target in refs(props.get(field, [])):
                trigger = direct_trigger(kind, action, str(path), record, field)
                if kind == "interact_recipe_dispenser":
                    trigger["locations"] = dispenser_locations.get(str(record.get("Type", "")).removesuffix("_C"), [])
                if kind in {"complete_quest", "quest_flow_event"} and quest_context:
                    trigger.update({key: value for key, value in quest_context.items() if key != "action"})
                    trigger["source"]["assetPath"] = quest_context["quest"].get("assetPath") or str(path)
                trigger_map[(recipe_kind, target["internalName"])].append(trigger)
                evidence_sources[kind] += 1

    # Journal metadata remains a useful fallback for recipes absent from every
    # live station/upgrade catalog, but conflicting journal rows are not active
    # workstation assignments.
    for recipe_name, candidates in station_metadata_candidates.items():
        if not station_candidates.get(recipe_name):
            station_candidates[recipe_name].extend(candidates)

    # A small number of old/quest recipes are not present in a current global
    # row. Their journal row, registry station, or cooked XP event still states
    # the UI family. Keep this provenance explicit instead of collapsing it to
    # the ambiguous legacy label "Crafting Bench".
    for guid, (_, recipe_record) in recipe_assets.items():
        recipe_name = str(recipe_record.get("Name", ""))
        if station_candidates.get(recipe_name):
            continue
        editor_station = str(editor_by_guid.get(guid, {}).get("StationName") or "")
        if editor_station:
            normalized = "CraftingTable" if editor_station == "Crafting Bench" else editor_station
            station_candidates[recipe_name].append({
                "station": normalized,
                "table": "legacyRecipeRegistry",
                "property": "StationName",
                "assetPath": str(args.editor_data / "RecipeID.json"),
            })
            continue
        recipe_props = recipe_record.get("Properties") or {}
        xp_row = str(((recipe_props.get("OnCraftXpEvent") or {}).get("RowName")) or "")
        inferred_station = ""
        if xp_row.startswith("Craft_Crafting_Table"):
            inferred_station = "CraftingTable"
        elif xp_row.startswith("Craft_Smithing_Forge"):
            if any(token in recipe_name for token in ("_Staff", "_Wand")):
                inferred_station = "MysticForge"
            elif "_Armour_" in recipe_name:
                inferred_station = "ArmourBench"
            elif any(token in recipe_name for token in ("_Bow", "_Arrow", "_Bolt")):
                inferred_station = "FletchingBenchv2"
            else:
                inferred_station = "ToolsMeleeBench"
        if inferred_station:
            station_candidates[recipe_name].append({
                "station": inferred_station,
                "table": "RecipeData",
                "property": "OnCraftXpEvent.RowName",
                "assetPath": str(recipe_record.get("Package", "")),
            })

    for recipe_name, candidates in station_candidates.items():
        # Higher-tier/specialized station wins when the cooked catalogs expose
        # the same recipe at multiple workstations.  Lexical order makes equal
        # priorities deterministic across exports and platforms.
        station_map[recipe_name] = max(
            candidates,
            key=lambda candidate: (STATION_PRIORITY.get(candidate["station"], 30),
                                   candidate["station"]),
        )["station"]

    catalog: list[dict[str, Any]] = []
    game_granted_recipe_names = {name for (kind, name), paths in trigger_map.items() if kind == "crafting" and paths}
    game_granted_recipe_guids = {
        guid for guid, (_, record) in recipe_assets.items()
        if str(record.get("Name", "")) in game_granted_recipe_names
    }
    station_recipe_guids = {
        guid for guid, (_, record) in recipe_assets.items()
        if any(candidate.get("table") in {
            "DT_CraftingStationsDataTable", "DT_ProcessingStationDataTable",
            "StationUpgradeComponent",
        } for candidate in station_candidates.get(str(record.get("Name", "")), []))
    }
    # RecipeID.json is useful legacy metadata, but it lags newly shipped game
    # features. An authoritative cooked grant promotes a RecipeData even when
    # that external registry has not yet learned its GUID.
    active_recipe_guids = (set(editor_by_guid) | game_granted_recipe_guids | station_recipe_guids) - set(recipe_exclusions)
    for guid in sorted(active_recipe_guids):
        editor = editor_by_guid.get(guid, {})
        asset_pair = recipe_assets.get(guid)
        if not asset_pair:
            exclusions.append({"kind": "crafting", "persistentId": guid, "internalName": "", "assetPath": "", "reason": "save-editor GUID has no exported RecipeData asset"})
            continue
        path, record = asset_pair
        props = record.get("Properties") or {}
        name = str(record.get("Name", ""))
        if name in deleted_journal_recipes:
            exclusions.append({"kind": "crafting", "internalName": name, "persistentId": guid,
                               "assetPath": record.get("Package", ""),
                               "reason": "referenced only by a deleted journal entry; no shipped unlock provider"})
            continue
        rejected_name = name.casefold()
        rejected = any(token in rejected_name for token in ("test", "debug", "deprecated", "_old", "refund", "journal", "vendor", "proxy", "deriveddata"))
        if rejected:
            exclusions.append({"kind": "crafting", "internalName": name, "persistentId": guid,
                               "assetPath": record.get("Package", ""), "reason": "not an active player recipe (runtime rejection rule)"})
        if rejected or any(entry.get("persistentId") == guid for entry in exclusions):
            continue
        output_ref = refs(props.get("ItemsCreated", []))
        unlocks = list(trigger_map.get(("crafting", name), []))
        if not unlocks and not props.get("bIgnoreAutomaticUnlocks"):
            unlocks.append({
                "type": "default",
                "action": "Available without an explicit unlock action",
                "source": {
                    "assetPath": record.get("Package", ""),
                    "property": "bIgnoreAutomaticUnlocks=false (inherited default)",
                    "baseline": str(args.fresh_baseline),
                },
                "evidence": "static:complete_unlock_provider_complement;runtime:fresh_character_save",
                "confidence": "proven",
                "validationNote": "No progression, item, station, quest, skill, entitlement, or dispenser grants this active RecipeData; the fresh save contains no explicit grant record for implicit defaults.",
            })
            evidence_sources["default_crafting"] += 1
        unlocks, redundant_unlocks = canonicalize_unlock_paths(unlocks)
        status = "active" if unlocks else "unresolved"
        catalog.append({
            "kind": "crafting",
            "internalName": name,
            "persistentId": guid,
            "assetPath": record.get("Package", ""),
            "displayName": editor.get("Name") or
                           (display_name_map.get(output_ref[0]["internalName"], output_ref[0]["internalName"]) if output_ref else name),
            "station": station_map.get(name) or editor.get("StationName") or "",
            "stationCandidates": station_candidates.get(name, []),
            "stationMetadataCandidates": station_metadata_candidates.get(name, []),
            "stationEvidence": "cookedStationCatalog" if name in station_map else "legacyRecipeRegistry",
            "category": "crafting",
            "active": True,
            "obtainableStatus": status,
            "output": output_ref,
            "unlockPaths": unlocks,
            "redundantUnlockPaths": redundant_unlocks,
            "properties": {"ignoreAutomaticUnlocks": bool(props.get("bIgnoreAutomaticUnlocks")), "softDeleted": bool(props.get("bSoftDeleted"))},
            "activeSources": (["legacyRecipeRegistry"] if guid in editor_by_guid else []) +
                             (["cookedUnlockGrant"] if name in game_granted_recipe_names else []) +
                             (["cookedStationCatalog"] if name in station_map else []),
            "evidenceAssets": [str(path)],
        })

    accounted_recipe_ids = ({entry["persistentId"] for entry in catalog if entry["kind"] == "crafting"} |
                            {entry.get("persistentId", "") for entry in exclusions if entry.get("kind") == "crafting"})
    for guid in sorted(set(recipe_assets) - accounted_recipe_ids):
        path, record = recipe_assets[guid]
        exclusions.append({
            "kind": "crafting",
            "internalName": record.get("Name", ""),
            "persistentId": guid,
            "assetPath": record.get("Package", ""),
            "reason": "exported RecipeData is absent from the active runtime/save-editor recipe registry",
            "evidence": {"asset": str(path), "activeRegistry": str(args.editor_data / "RecipeID.json")},
        })

    for name in sorted(catalog_pieces):
        metadata = catalog_pieces[name]
        asset_pair = building_assets.get(name)
        guid = ""
        package = metadata.get("assetPath", "")
        evidence: list[str] = []
        props: dict[str, Any] = {}
        if asset_pair:
            path, record = asset_pair
            props = record.get("Properties") or {}
            guid = str(props.get("PersistenceID", ""))
            package = record.get("Package", package)
            evidence.append(str(path))
        unlocks = list(trigger_map.get(("building", name), []))
        unlocks, redundant_unlocks = canonicalize_unlock_paths(unlocks)
        if not unlocks:
            exclusions.append({
                "kind": "building",
                "internalName": name,
                "persistentId": guid,
                "assetPath": package,
                "reason": "present in the shipped building catalogue but absent from every cooked progression, perk, quest, entitlement, plan, or reward grant; unobtainable",
                "evidence": {
                    "ignoreAutomaticUnlocks": bool(props.get("bIgnoreAutomaticUnlocks")),
                    "catalog": str(args.building_catalog),
                },
            })
            evidence_sources["unobtainable_building"] += 1
            continue
        status = "active"
        catalog.append({
            "kind": "building",
            "internalName": name,
            "persistentId": guid,
            "assetPath": package,
            "displayName": metadata.get("name", name),
            "station": "Building menu",
            "category": "building",
            "active": True,
            "obtainableStatus": status,
            "output": [{"internalName": name, "assetPath": package}],
            "unlockPaths": unlocks,
            "redundantUnlockPaths": redundant_unlocks,
            "properties": {"ignoreAutomaticUnlocks": bool(props.get("bIgnoreAutomaticUnlocks"))},
            "evidenceAssets": evidence,
        })

    catalog.sort(key=lambda entry: (entry["kind"], entry["displayName"].casefold(), entry["internalName"]))
    canonical_exclusions: dict[tuple[str, str], dict[str, Any]] = {}
    for entry in exclusions:
        key = (entry.get("kind", ""), entry.get("persistentId") or entry.get("internalName", ""))
        previous = canonical_exclusions.get(key)
        if previous is None:
            canonical_exclusions[key] = entry
            continue
        reasons = [str(previous.get("reason", "")), str(entry.get("reason", ""))]
        previous["reason"] = "; ".join(dict.fromkeys(reason for reason in reasons if reason))
        if entry.get("evidence"):
            prior_evidence = previous.get("evidence")
            previous["evidence"] = [prior_evidence, entry["evidence"]] if prior_evidence else entry["evidence"]
    exclusions = sorted(canonical_exclusions.values(), key=lambda entry: (entry.get("kind", ""), entry.get("internalName", "")))
    unresolved = [entry for entry in catalog if entry["obtainableStatus"] == "unresolved"]
    duplicate_ids = [guid for guid, count in Counter(entry["persistentId"] for entry in catalog if entry["persistentId"]).items() if count > 1]
    duplicate_exclusion_ids = [guid for guid, count in Counter(entry.get("persistentId") for entry in exclusions if entry.get("persistentId")).items() if count > 1]
    accounted_buildings = ({entry["internalName"] for entry in catalog if entry["kind"] == "building"} |
                           {entry.get("internalName", "") for entry in exclusions if entry.get("kind") == "building"})
    unaccounted_buildings = sorted(set(catalog_pieces) - accounted_buildings)
    accounted_recipe_ids = ({entry["persistentId"] for entry in catalog if entry["kind"] == "crafting"} |
                            {entry.get("persistentId", "") for entry in exclusions if entry.get("kind") == "crafting"})
    unaccounted_recipe_ids = sorted(set(recipe_assets) - accounted_recipe_ids)
    manifest_path = args.assets / ".uas-manifest.json"
    manifest = read_json(manifest_path)
    fingerprint_files = [path for path in (args.game_exe, args.usmap, manifest_path, args.building_catalog, args.fresh_baseline) if path.exists()]
    result = {
        "schemaVersion": 1,
        "generatedAt": datetime.now(timezone.utc).isoformat(),
        "gameBuild": {
            "uasVersion": manifest.get("uasVersion", ""),
            "game": manifest.get("game", ""),
            "files": [{"path": str(path), "sha256": sha256(path)} for path in fingerprint_files],
        },
        "counts": {
            "activeCraftingRecipes": sum(entry["kind"] == "crafting" for entry in catalog),
            "activeCraftingRecipesBeyondLegacyRegistry": sum(
                entry["kind"] == "crafting" and entry["persistentId"] not in editor_by_guid for entry in catalog
            ),
            "activeBuildingRecipes": sum(entry["kind"] == "building" for entry in catalog),
            "totalActive": len(catalog),
            "excluded": len(exclusions),
            "unresolved": len(unresolved),
            "sourceRecipeAssets": len(recipe_assets),
            "accountedSourceRecipeAssets": len(set(recipe_assets) & accounted_recipe_ids),
            "sourceBuildingCatalogPieces": len(catalog_pieces),
            "accountedBuildingCatalogPieces": len(accounted_buildings),
        },
        "evidenceSummary": dict(sorted(evidence_sources.items())),
        "validation": {
            "duplicatePersistentIds": duplicate_ids,
            "duplicateExclusionPersistentIds": duplicate_exclusion_ids,
            "duplicateSourceRecipeIds": dict(sorted(recipe_guid_duplicates.items())),
            "missingBuildingAssets": sorted(set(catalog_pieces) - set(building_assets)),
            "unaccountedBuildingCatalogPieces": unaccounted_buildings,
            "unaccountedSourceRecipeIds": unaccounted_recipe_ids,
            "unresolved": [{"kind": entry["kind"], "internalName": entry["internalName"], "persistentId": entry["persistentId"]} for entry in unresolved],
        },
        "recipes": catalog,
        "exclusions": exclusions,
    }
    web_counts = render_web_catalog(result, asset_records, args.web_output, args.runtime_web_output,
                                    args.runtime_history_output, args.runtime_history_import)
    result["validation"]["webCatalog"] = web_counts
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    render_docs(result, args.docs_dir)
    print(json.dumps(result["counts"], sort_keys=True))
    failures = bool(duplicate_ids or duplicate_exclusion_ids or recipe_guid_duplicates or result["validation"]["missingBuildingAssets"] or
                    unaccounted_buildings or unaccounted_recipe_ids or unresolved)
    return 0 if (not failures or args.allow_unresolved) else 2


if __name__ == "__main__":
    raise SystemExit(main())
