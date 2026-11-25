#!/usr/bin/env python3
import json
from pathlib import Path

root = Path(__file__).resolve().parents[1]
server_sha = "248e13b8f37f052e3591a726c78ee6bc79c2a6df5d22825c11782cd9c950ae11"

rows = [line.split("\t") for line in (root / "deploy/compatibility.tsv").read_text().splitlines()
        if line and not line.startswith("#")]
candidate = [row for row in rows if row[0] == server_sha]
assert len(candidate) == 1 and len(candidate[0]) == 56
assert candidate[0][49:56] == ["0xa66a900", "0xa676140", "0xa68c7c0", "0xa68c540", "0xa68e000", "0xa68dda0", "0xa68e390"]
assert candidate[0][10:14] == ["-11075", "-117685", "420000", "world-map-1.0.0.2"]

header = (root / "src/engine/dom_engine.h").read_text()
for token in ("GObjects_26670300 = 0x0DCAF510", "GNames_26670300   = 0x0DBF3FF8",
              "GWorld_26670300   = 0x0DDCB5F8"):
    assert token in header

resources = (root / "src/engine/resources.cpp").read_text()
assert f'hash=="{server_sha}"' in resources
for offset in ("0x0A7AA060", "0x0A851100", "0x0AAEEBE0", "0x0AB0CB70",
               "0x0AAC35B0", "0x0AAEE060", "0x0AAEF6B0"):
    assert offset in resources

reader = (root / "src/engine/uobject.cpp").read_text()
assert f'previewSha="{server_sha}"' in reader
assert "0x0A672E80" in reader and "0x0A675E60" in reader

policy = (root / "src/engine/storage_compatibility_policy.h").read_text()
assert server_sha in policy and "CandidateSha" in policy
assert "hash == LatestReviewedSha || hash == CandidateSha" in policy
assert "return hash == LegacyCandidateSha;" in policy

storage = (root / "src/engine/admin_storage.cpp").read_text()
for offset in ("0x0A669E70", "0x0A66AA00", "0x0A66C540", "0x0A66F5D0",
               "0x0A66B9D0", "0x0A6783A0", "0x0A3E99B0", "0x07ACAF90",
               "0x0A0FDA60", "0x0A0FDCD0", "0x0A0FE0B0", "0x0A3C4F30",
               "0x0A68C010", "0x0AAD4730", "0x0A6900F0", "0x0A5A3EC0",
               "0x0A688A50", "0x0A688A40", "0x04ADAB60", "0x0A0FDBA0",
               "0x080A8980", "0x0A695870", "0x0A696710", "0x0A697100",
               "0x0A6B6A50", "0x0A6B6570", "0x0A6B67E0", "0x0A3F4FB0",
               "0x0A3E7030"):
    assert offset in storage
assert "hash!=CurrentSha&&hash!=LatestSha&&hash!=CandidateSha" in storage
quests = (root / "src/engine/player_quests.cpp").read_text()
assert 'header[index]=="fTextDisplayStringOffset"' in quests
assert 'header[index]=="fTextDestructor"' in quests
assert "f.back()" not in quests
activity_test = (root / "src/engine/admin_activity_test.cpp").read_text()
for offset in ("0x0A72E940", "0x0A72F5F0", "0x0A533090"):
    assert offset in activity_test

graves = (root / "src/engine/admin_gravestone.cpp").read_text()
assert f'CandidateSha="{server_sha}"' in graves
for offset in ("0x0A6613D0", "0x0A65FFD0", "0x0A6605B0", "0x0A65EDF0",
               "0x0A65C730", "0x0A65CA10", "0x09E23940", "0x09E1A7F0", "0x09E23270"):
    assert offset in graves

teleport = (root / "src/engine/admin_teleport.cpp").read_text()
assert "{0xA779E90,0xA586AA0,0xA587080,17" in teleport

analyzer = (root / "tools/compat_analyzer.cpp").read_text()
assert server_sha in analyzer and "StorageCompatibilityPolicy::CandidateSha" in analyzer
for token in ("storageSetLinkableNative", "storageRegisterLinked", "storageUnregisterLinked",
              "nativeCompleteBuildingTarget", "nativePartialBuildingTarget", "playerPagedSlotValid",
              "playerPagedSlotOffset", "playerPagedSlotCount", "playerLoadoutSlotValid",
              "playerLoadoutAddItemToSlot", "loadoutAddItemSignature",
              "playerLoadoutRemoveItem", "loadoutRemoveItemSignature"):
    assert token in analyzer

capabilities = {entry["id"]: entry for entry in
                json.loads((root / "deploy/compatibility-capabilities.json").read_text())["capabilities"]}
for capability in ("activity-skill-level", "activity-death", "activity-recipe-unlocked",
                   "activity-quest-started", "activity-quest-progressed", "activity-quest-completed",
                   "region-discovery", "activity-material-discovered", "activity-perk-unlocked",
                   "activity-spell-unlocked"):
    assert capability in capabilities
    assert "tests/game_update_26670300_test.py" in capabilities[capability]["tests"]
assert "admin-player-free-building" in capabilities
assert "tests/game_update_26670300_test.py" in capabilities["admin-player-free-building"]["tests"]

print("game update 26670300 compatibility wiring tests passed")
