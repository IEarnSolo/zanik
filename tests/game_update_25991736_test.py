#!/usr/bin/env python3
from pathlib import Path


root = Path(__file__).resolve().parents[1]
server_sha = "b5017cf67bc6127dbec443e00fed1f818e82afd5d6d17f912d959a1dd87d5687"

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
assert "GObjects_25991736 = 0x0DC97510" in header
assert "GNames_25991736   = 0x0DBDBFF8" in header
assert "GWorld_25991736   = 0x0DDB35F8" in header

dispatcher = (root / "src" / "engine" / "admin_storage.cpp").read_text()
assert server_sha in dispatcher
assert f'ExpectedSha(hash){{return hash=="{server_sha}"' not in dispatcher
assert "Keep mutation compatibility closed" in dispatcher
assert "currentDispatcher?0x07ABFE00" in dispatcher
for offset in (
    "0x0A653420", "0x0A653CC0", "0x0A6554F0", "0x0A656FD0",
    "0x0A654980", "0x0A65F7E0", "0x0A3DAA00", "0x04A89480",
    "0x0A0EF130", "0x0A0EF3A0", "0x0A0EF780", "0x04ACFAE0",
    "0x079D0710", "0x04A1F9F0", "0x0A3B5970",
):
    assert offset in dispatcher

gravestones = (root / "src" / "engine" / "admin_gravestone.cpp").read_text()
assert server_sha in gravestones
for offset in (
    "0x0A6554F0", "0x0A656FD0", "0x0A653CC0", "0x0A654980",
    "0x0A655550", "0x0A64FDD0", "0x0A64E920", "0x0A64EF50",
    "0x0A64D740", "0x0A64B360", "0x08731130", "0x08731180", "0x07E66460",
    "0x08731610", "0x09E17F00", "0x09E0EDB0", "0x09E17830",
    "0x07E59100", "0x0837ECB0", "0x08737420", "0x07E6B520",
):
    assert offset in gravestones

teleport = (root / "src" / "engine" / "admin_teleport.cpp").read_text()
assert "{0xA7628F0,0xA5765A0,0xA576B80,17" in teleport
assert "wrapper[0x26]!=0xE8" in teleport
assert "wrapper+0x2b+relative" in teleport
assert "if(storageTeleportTemplateValid.load())" in teleport
assert "storageTeleportCircuitBroken=true" in teleport
assert "ReadNearbyBuildingBounds(chestActor,origin)" not in teleport
assert "ReadAdminStorageLocations(request.targetCharacterGuid" not in teleport
assert 'FindFunction("TeleportationSubsystem","TeleportToNamedLocation")' in teleport
assert "const auto fields=ReflectedFields(nodestoneNamedFunction);" in teleport
assert 'has("PlayerCharacter",0x0)' in teleport
assert 'has("LocationName",0x8)' in teleport
assert 'has("TeleportSuccessCallbackDelegate",0x10)' in teleport
assert 'has("bShowLoadingScreen",0x20)' in teleport
assert 'has("OptionalTeleportType",0x21)' in teleport
assert 'has("OptionalGraveyard",0x28)' in teleport
assert 'has("ReturnValue",0x30)' in teleport
assert "params[0x20]=1;params[0x21]=0" in teleport
assert "nodestoneTeleportCircuitBroken=true" in teleport
assert "nodestoneInitializationAttempts.load()>=8" in teleport
assert "RefreshAdminNodestoneTeleportCompatibility" in teleport

main_source = (root / "src" / "main.cpp").read_text()
assert 'path=="/api/admin/session"' in main_source
assert "DomEngine::RefreshAdminNodestoneTeleportCompatibility();" in main_source
nodestone_reader = (root / "src" / "engine" / "kills.cpp").read_text()
assert "RefreshAdminNodestoneTeleportCompatibility();" in nodestone_reader

analyzer = (root / "tools" / "compat_analyzer.cpp").read_text()
assert "--native-offsets" in analyzer
assert 'field("graveGetInventory",graveGetInventoryAt,first)' in analyzer
assert "ASwitchableTeleporter::PerformTeleport" in analyzer
assert "UTeleportationSubsystem::TeleportToNamedLocation" in analyzer
assert "native nodestone named-location call relationship changed" in analyzer

resources = (root / "src" / "engine" / "resources.cpp").read_text()
assert f'hash=="{server_sha}"' in resources
assert "getRespawnIntervalOffset=0x0AACD130" in resources

storage = (root / "src" / "engine" / "uobject.cpp").read_text()
assert f'latestSha="{server_sha}"' in storage
assert "0x0A65A8B0" in storage
assert "0x0A65D330" in storage
assert "displaySignature" in storage
assert "emptySignature" in storage
assert "filledSignature" in storage

main = (root / "src" / "main.cpp").read_text()
page = (root / "src" / "web_page.h").read_text()
assert '\\"capabilities\\":' in main
assert '\\"storageRead\\"' in main
assert '\\"gravestoneRead\\"' in main
assert "Promise.all([fetch('/api/admin/session'" not in page
assert "storageAdmin.hidden=!capabilities.storageRead" in page
assert "gravestoneAdmin.hidden=!capabilities.gravestoneRead" in page
assert "if(storageMutationAvailable){try{const response=await fetch('/api/admin/storage/catalog'" in page

print("game update 25991736 compatibility checks passed")
