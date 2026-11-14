#!/usr/bin/env python3
from pathlib import Path
import json


root = Path(__file__).resolve().parents[1]
server_sha = "a350b998ea38955345619b7adde31aa9e29db629e9e9aa32287013957eb41901"

rows = [
    line.split("\t")
    for line in (root / "deploy" / "compatibility.tsv").read_text().splitlines()
    if line and not line.startswith("#")
]
candidate = [row for row in rows if row[0] == server_sha]
assert len(candidate) == 1
assert len(candidate[0]) == 49
assert candidate[0][10:14] == ["-11075", "-117685", "420000", "world-map-1.0.0.2"]

header = (root / "src" / "engine" / "dom_engine.h").read_text()
assert "GObjects_26419371 = 0x0DCAF510" in header
assert "GNames_26419371   = 0x0DBF3FF8" in header
assert "GWorld_26419371   = 0x0DDCB5F8" in header

resources = (root / "src" / "engine" / "resources.cpp").read_text()
assert f'hash=="{server_sha}"' in resources
for offset in ("0x0A7A9950", "0x0A8509F0", "0x0AAEE990", "0x0AB0C920", "0x0AAC30C0", "0x0AAEDE10", "0x0AAEF460"):
    assert offset in resources

storage_reader = (root / "src" / "engine" / "uobject.cpp").read_text()
assert f'nextSha="{server_sha}"' in storage_reader
assert "0x0A672840" in storage_reader
assert "0x0A675820" in storage_reader

storage = (root / "src" / "engine" / "admin_storage.cpp").read_text()
analyzer = (root / "tools" / "compat_analyzer.cpp").read_text()
assert "AActor::FlushNetDormancy()" in analyzer
assert "AActor::ForceNetUpdate()" in analyzer
assert "storage autofill replication wake symbols missing" in analyzer
policy = (root / "src" / "engine" / "storage_compatibility_policy.h").read_text()
assert server_sha in policy
assert "IsProductionReviewed" in policy
assert "StorageCompatibilityPolicy::CurrentReviewedSha" in storage
assert "0x0A688410" in storage
assert "InstallNativeBuildingConsumptionCapture" in storage
assert "nativeCompleteBuildingTarget" in analyzer and "nativePartialBuildingTarget" in analyzer
assert "0x0A688400" in storage
for offset in (
    "0x0A669830", "0x0A66A3C0", "0x0A66BF00", "0x0A66EF90",
    "0x0A66B390", "0x0A677D60", "0x0A3E9980", "0x04A94480",
    "0x07ACAF90", "0x0A0FDA20", "0x0A0FDC90", "0x0A0FE070",
    "0x04ADAAE0", "0x079DB8A0", "0x04A2A9F0", "0x0A3C4F00", "0x0A5A3880",
):
    assert offset in storage

graves = (root / "src" / "engine" / "admin_gravestone.cpp").read_text()
assert f'CurrentSha="{server_sha}"' in graves
for offset in (
    "0x0A66BF00", "0x0A66EF90", "0x0A66A3C0", "0x0A66B390",
    "0x0A669830", "0x0A66BF60", "0x0A660D90", "0x0A65F990",
    "0x0A65FF70", "0x0A65E7B0", "0x0A65C0F0", "0x0A65C3D0",
    "0x0873C390", "0x0873C3E0", "0x07E71760", "0x0873C870",
    "0x09E23940", "0x09E1A7F0", "0x09E23270", "0x07E64400",
    "0x08389FB0", "0x08742680", "0x07E76820",
):
    assert offset in graves

teleport = (root / "src" / "engine" / "admin_teleport.cpp").read_text()
assert "{0xA779780,0xA586460,0xA586A40,17" in teleport

analyzer = (root / "tools" / "compat_analyzer.cpp").read_text()
assert server_sha in analyzer
assert "--require-storage-production" in analyzer
assert "storage mutation production promotion is pending" in analyzer
assert "admin delayed teleport capture signature changed" in analyzer
assert "admin delayed teleport wrapper signature changed" in analyzer
assert "admin delayed teleport wrapper call relationship changed" in analyzer
assert "binary[delayedWrapperFile+0x26]!=0xe8" in analyzer

history = json.loads((root / "asset-tools" / "extraction-recipe-history.json").read_text())["recipes"]
profile = next(entry["recipe"]["catalogProfile"] for entry in history
               if entry["recipe"]["schemaVersion"] == 19)
assert profile["version"] == "client-26420214-reviewed-10"
assert profile["clientBuild"] == "26420214"
assert profile["exportCounts"] == {"jsonFiles": 18279, "worldPartitionCells": 1731}
assert profile["catalogCounts"]["buildings/catalog.json:pieces"] == 850
assert profile["catalogCounts"]["quest-catalog.json:quests"] == 42
assert profile["catalogCounts"]["quest-catalog.json:objectives"] == 223
assert profile["catalogCounts"]["quest-step-locations.json:steps"] == 235
assert profile["catalogCounts"]["quest-step-locations.json:markers"] == 702
assert profile["catalogCounts"]["recipe-unlocks.json:entries"] == 1615
rules = json.loads((root / "asset-tools" / "catalog-rules.json").read_text())
assert len(rules["reviewedUnknownRecipeTriggers"]) == 49
for name in (
    "RECIPE_Consumable_Fish_ArmouredCatfish_ReelDowner",
    "RECIPE_Consumable_Fish_Pristine_DesertSole_ReelDowner",
    "RECIPE_Consumable_Fish_Sardine_ReelDowner",
):
    assert name in rules["reviewedUnknownRecipeTriggers"]

print("game update 26419371 compatibility checks passed")
