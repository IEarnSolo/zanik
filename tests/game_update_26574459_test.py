#!/usr/bin/env python3
import json
from pathlib import Path

root = Path(__file__).resolve().parents[1]
server_sha = "1b779665120e8d0df8c5d481969cdb2d1a4df52441cd5569ad2309ec62da1d26"

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
assert "GObjects_26574459 = 0x0DCAF510" in header
assert "GNames_26574459   = 0x0DBF3FF8" in header
assert "GWorld_26574459   = 0x0DDCB5F8" in header

resources = (root / "src" / "engine" / "resources.cpp").read_text()
assert f'hash=="{server_sha}"' in resources
for offset in ("0x0A7A99F0", "0x0A850A90", "0x0AAEE900", "0x0AB0C890", "0x0AAC3030", "0x0AAEDD80", "0x0AAEF3D0"):
    assert offset in resources

storage_reader = (root / "src" / "engine" / "uobject.cpp").read_text()
assert f'candidateSha="{server_sha}"' in storage_reader
assert "0x0A6728E0" in storage_reader
assert "0x0A6758C0" in storage_reader

policy = (root / "src" / "engine" / "storage_compatibility_policy.h").read_text()
assert server_sha in policy
assert "LatestReviewedSha" in policy
assert "hash == LatestReviewedSha" in policy
assert "IsCandidateOnly" in policy

storage = (root / "src" / "engine" / "admin_storage.cpp").read_text()
for offset in (
    "0x0A6698D0", "0x0A66A460", "0x0A66BFA0", "0x0A66F030",
    "0x0A66B430", "0x0A677E00", "0x0A3E9A20", "0x04A94480",
    "0x07ACAF90", "0x0A0FDAC0", "0x0A0FDD30", "0x0A0FE110",
    "0x04ADAAE0", "0x079DB8A0", "0x04A2A9F0", "0x0A3C4FA0", "0x0A68BA70",
):
    assert offset in storage
assert "hash==CurrentSha||hash==LatestSha" in storage
assert "hash==LatestSha?0x0A5A3920" in storage
assert "(hash==CurrentSha||hash==LatestSha)?0x07ACAF90" in storage
assert "hash!=CurrentSha&&hash!=LatestSha" in storage

graves = (root / "src" / "engine" / "admin_gravestone.cpp").read_text()
assert f'LatestSha="{server_sha}"' in graves
assert "hash==CurrentSha||hash==LatestSha" in graves
for offset in (
    "0x0A66BFA0", "0x0A66F030", "0x0A66A460", "0x0A66B430",
    "0x0A6698D0", "0x0A66C000", "0x0A660E30", "0x0A65FA30",
    "0x0A660010", "0x0A65E850", "0x0A65C190", "0x0A65C470",
    "0x09E239E0", "0x09E1A890", "0x09E23310",
):
    assert offset in graves

teleport = (root / "src" / "engine" / "admin_teleport.cpp").read_text()
assert "{0xA779820,0xA586500,0xA586AE0,17" in teleport

analyzer = (root / "tools" / "compat_analyzer.cpp").read_text()
assert server_sha in analyzer
assert "--require-storage-production" in analyzer
assert "storage mutation production promotion is pending" in analyzer
assert "storage withdrawal RPC parameter ABI changed" in analyzer
assert "moveInventoryRpcLayout" in analyzer
assert "splitPutRpcLayout" in analyzer
assert "splitRpcLayout" in analyzer

recipe = json.loads((root / "asset-tools" / "extraction-recipes.json").read_text())
assert recipe["schemaVersion"] == 20
profile = recipe["catalogProfile"]
assert profile["version"] == "client-26574764-reviewed-11"
assert profile["clientBuild"] == "26574764"
assert profile["exportCounts"] == {"jsonFiles": 18279, "worldPartitionCells": 1731}
assert profile["catalogCounts"] == {
    "buildings/catalog.json:pieces": 850,
    "npc-catalog.json:nodes": 2964,
    "npc-catalog.json:types": 171,
    "quest-catalog.json:objectives": 223,
    "quest-catalog.json:quests": 42,
    "quest-step-locations.json:markers": 702,
    "quest-step-locations.json:quests": 42,
    "quest-step-locations.json:steps": 235,
    "recipe-unlocks.json:entries": 1615,
    "resource-catalog.json:nodes": 31910,
    "resource-catalog.json:types": 26,
}

print("game update 26574459 promoted compatibility checks passed")
