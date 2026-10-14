#!/usr/bin/env python3
"""Recipe cards use the authored item icon even for non-ItemData subclasses."""

import json
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
from generate_recipe_unlock_catalog import render_web_catalog  # noqa: E402


with tempfile.TemporaryDirectory(prefix="zanik-recipe-icon-") as temporary:
    web = Path(temporary) / "web"
    items = web / "items"
    items.mkdir(parents=True)
    (items / "ITEM_Ammo_Arrows_Bone_Bodkin.png").write_bytes(b"authored-fang-arrow")
    (items / "ITEM_Ammo_Arrows_Bone_Poison.png").write_bytes(b"authored-fang-arrow")
    output = web / "recipe-unlocks.json"
    render_web_catalog({
        "generatedAt": "1970-01-01T00:00:00Z",
        "recipes": [{
            "internalName": "RECIPE_Ammo_Arrows_Bone_Bodkin",
            "persistentId": "o2uJiKtVrUuvPusCoWjixQ",
            "displayName": "Fang Arrow",
            "kind": "crafting",
            "station": "Fletching Bench",
            "stationCandidates": [{"station": "Fletching Bench"}],
            "output": [{"internalName": "ITEM_Ammo_Arrows_Bone_Bodkin"}],
            "unlockPaths": [{
                "type": "unknown_trigger",
                "action": "Unlock trigger not yet identified",
                "query": {"operator": "AND", "operands": []},
            }],
        }, {
            "internalName": "RECIPE_Ammo_Arrows_Bone_Poison",
            "persistentId": "fVvNA014sI0CgqWmvJlJtA",
            "displayName": "Poisoned Fang Arrow",
            "kind": "crafting",
            "station": "Fletching Bench",
            "stationCandidates": [{"station": "Fletching Bench"}],
            "output": [{"internalName": "ITEM_Ammo_Arrows_Bone_Poison"}],
            "unlockPaths": [{
                "type": "unknown_trigger",
                "action": "Unlock trigger not yet identified",
                "query": {"operator": "AND", "operands": []},
            }],
        }],
    }, [], output)
    payload = json.loads(output.read_text(encoding="utf-8"))
    icons = {entry["name"]: entry["icon"] for entry in payload["entries"]}
    assert icons == {
        "Fang Arrow": "/assets/items/ITEM_Ammo_Arrows_Bone_Bodkin.png",
        "Poisoned Fang Arrow": "/assets/items/ITEM_Ammo_Arrows_Bone_Poison.png",
    }

print("recipe icon resolution tests passed")
