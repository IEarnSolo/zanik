#!/usr/bin/env python3
from pathlib import Path


root = Path(__file__).resolve().parents[1]
server_sha = "45cd13cf24658153f4c53777816ffa38ac893a471ebdf5d9c9768f7e4e74c966"

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
assert "GObjects_26161880 = 0x0DC97510" in header
assert "GNames_26161880   = 0x0DBDBFF8" in header
assert "GWorld_26161880   = 0x0DDB35F8" in header

resources = (root / "src" / "engine" / "resources.cpp").read_text()
assert f'hash=="{server_sha}"' in resources
for offset in ("0x0A77C8E0", "0x0A83A570", "0x0AACDA50", "0x0AAEF6D0", "0x0AAACB20", "0x0AAABC40", "0x0AACE520"):
    assert offset in resources

storage_reader = (root / "src" / "engine" / "uobject.cpp").read_text()
assert f'currentSha="{server_sha}"' in storage_reader
assert "0x0A65B160" in storage_reader
assert "0x0A65DBE0" in storage_reader

storage = (root / "src" / "engine" / "admin_storage.cpp").read_text()
assert f'CurrentSha="{server_sha}"' in storage
for offset in (
    "0x0A653CD0", "0x0A654570", "0x0A655DA0", "0x0A657880",
    "0x0A655230", "0x0A660090", "0x0A3DA9D0", "0x04A89480",
    "0x07ABFE00", "0x0A0EF090", "0x0A0EF300", "0x0A0EF6E0",
    "0x04ACFAE0", "0x079D0710", "0x04A1F9F0", "0x0A3B58D0",
):
    assert offset in storage

graves = (root / "src" / "engine" / "admin_gravestone.cpp").read_text()
assert f'CurrentSha="{server_sha}"' in graves
for offset in (
    "0x0A655DA0", "0x0A657880", "0x0A654570", "0x0A655230",
    "0x0A655E00", "0x0A650680", "0x0A64F1D0", "0x0A64F800",
    "0x0A64DFF0", "0x09E17E60", "0x09E0ED10", "0x09E17790",
):
    assert offset in graves

teleport = (root / "src" / "engine" / "admin_teleport.cpp").read_text()
assert "{0xA7631E0,0xA576C80,0xA577260,17" in teleport

quests = (root / "src" / "engine" / "player_quests.cpp").read_text()
assert 'FindFunction("QuestProgressComponent","Server_NotifyQuestTracked")' in quests
assert 'FindFunction("QuestProgressComponent","Client_NotifyTrackedQuestLoaded")' in quests
assert 'FindUniqueFunctionByName("Server_NotifyQuestTracked")' in quests
assert 'FindUniqueFunctionByName("Client_NotifyTrackedQuestLoaded")' in quests
assert "knownQuestComponents" in quests
assert 'name=="Server_NotifyQuestTracked"' in quests
assert "trackedNotifyQuestOffset=0;return true" in quests
assert "PrologueOffset=14,HookBytes=13" in quests
assert "0x55,0x48,0x89,0xE5,0x41,0x57,0x41,0x56,0x41,0x55,0x41,0x54,0x53" in quests

analyzer = (root / "tools" / "compat_analyzer.cpp").read_text()
assert server_sha in analyzer

updater = (root / "scripts" / "update-game.sh").read_text()
assert 'while [ "$attempt" -le 2 ]' in updater
assert "Zanik SteamCMD attempt $attempt" in updater

print("game update 26161880 compatibility checks passed")
