#!/usr/bin/env python3
from pathlib import Path


root = Path(__file__).resolve().parents[1]
engine = (root / "src/engine/admin_teleport.cpp").read_text()
persistence = (root / "src/home_location_persistence.cpp").read_text()
header = (root / "src/engine/dom_engine.h").read_text()
main = (root / "src/main.cpp").read_text()
page = (root / "src/web_page.h").read_text()
analyzer = (root / "tools/compat_analyzer.cpp").read_text()
catalog = (root / "config/player-activity-unlock-catalog.tsv").read_text()

assert "ResolvePlayerHomeLocation" in header
assert "FlushAdminHomeLocations" in header
assert 'RuntimePaths::DataFile("home-locations.db")' in engine
assert "LoadHomeLocations()" in engine
assert "homeLocationsDirty" in engine
assert "fsync(descriptor)" in persistence
assert "homePersistenceCompatibility" in engine
assert "homePersistenceFailures" in engine
assert "homeMalformedRecords" in engine
assert "DomEngine::FlushAdminHomeLocations();" in main
assert 'FindFunction("DominionRuntimeBlueprintLibrary","GetPlayerBedRespawnLocationAndRotation")' in engine
assert 'FindObject("DominionRuntimeBlueprintLibrary",true)' in engine
for field, offset in (("PlayerController", "0x0"), ("OutLocation", "0x8"), ("OutRotation", "0x20"), ("ReturnValue", "0x38")):
    assert f'has("{field}",{offset})' in engine
assert 'request.action="home"' in engine
assert 'request.action=="home"' in engine

assert '"/api/admin/home/teleport"' in main
assert 'path=="/api/player-home"' in main
assert '"compatibility\\":"<<diagnostics.homeLocationCompatibility' in main
assert '"persistenceCompatibility\\":"<<diagnostics.homePersistenceCompatibility' in main
assert '"adminHomePersistenceCompatibility\\":"+std::to_string(adminTeleport.homePersistenceCompatibility)' in main
assert "QueueAdminHomeTeleport" in main
assert "capabilities?.locationTeleport" in page
assert "PerkV2_Runecraftng_HomeTeleport.png" in page
assert "Teleport to home" in page
assert "without runes" in page
assert "event.type==='death'?'/assets/gravestone-icon.png'" in page
assert "event.type==='region_discovered'?'/assets/region-discovery-icon.png'" in page
assert "value==='/assets/region-discovery-icon.png'?'/assets/region-discovery-icon.png?v=places-selected-gold-1'" in page
assert ".overview-home{grid-area:home;container-type:inline-size}" in page
assert "@container(max-width:34rem)" in page
assert "grid-template-areas:'icon copy map' '. . button' '. . message'" in page

symbol = "UDominionRuntimeBlueprintLibrary::GetPlayerBedRespawnLocationAndRotation"
assert symbol in analyzer
assert "home teleport bed-respawn ABI changed" in analyzer
assert "PerkV2_Runecraftng_HomeTeleport\tspell\tUSD_HomeTeleport\tHome Teleport" in catalog

print("home teleport compatibility checks passed")
