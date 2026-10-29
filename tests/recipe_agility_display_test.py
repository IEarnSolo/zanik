#!/usr/bin/env python3
import json
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from tools.generate_recipe_unlock_catalog import render_web_catalog


COURSES = {
    "DA_AgilityGymCourse1": "Chasm Dash",
    "DA_AgilityGymCourse2": "Thunder Assault",
    "DA_AgilityGymCourse3": "Grave Run",
    "DA_AgilityGymCourse4": "Tower Climb",
    "DA_AgilityGymCourse5": "Lava Fields",
    "DA_AgilityGymCourse6_Ghornfell2": "Fractured Ruins",
}


def recipe(tier: str) -> dict:
    targets = [{"internalName": internal} for internal in COURSES]
    return {
        "kind": "building",
        "persistentId": f"medal-{tier.casefold()}",
        "internalName": f"BUILDPIECE_Medal_{tier}",
        "displayName": f"{tier} Medal",
        "station": "",
        "stationCandidates": [],
        "output": [{"internalName": f"BUILDPIECE_Medal_{tier}"}],
        "unlockPaths": [{
            "type": "skill_or_perk",
            "action": f"ALL {tier.upper()}[backend identifiers]",
            "query": {"operator": "AND", "operands": [{
                "type": "skill_or_perk",
                "condition": "AllMatch",
                "minimumMedal": tier,
                "targets": targets,
            }]},
        }],
    }


records = [
    (Path(f"{internal}.json"), {
        "Type": "AgilityCourseDataAsset",
        "Name": internal,
        "Properties": {"TimeTrialData": {"DisplayName": display}},
    })
    for internal, display in COURSES.items()
]

with tempfile.TemporaryDirectory() as temporary:
    output = Path(temporary) / "recipe-unlocks.json"
    result = {
        "generatedAt": "2026-10-29T00:00:00Z",
        "recipes": [recipe(tier) for tier in ("Bronze", "Silver", "Gold")],
    }
    render_web_catalog(result, records, output)
    payload = json.loads(output.read_text(encoding="utf-8"))

by_name = {entry["name"]: entry for entry in payload["entries"]}
for tier in ("Bronze", "Silver", "Gold"):
    step = by_name[f"{tier} Medal"]["routes"][0]["steps"][0]
    assert step["text"] == f"Earn a {tier} medal in every agility course"
    assert step["targetText"] == f"Earn {tier}"
    assert [target["name"] for target in step["targets"]] == list(COURSES.values())
    assert "DA_AgilityGymCourse" not in json.dumps(step)

print("agility medal recipe requirements use clean authored course names: ok")
