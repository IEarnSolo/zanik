#!/usr/bin/env python3
"""Safety and compatibility contract for the admin recent-activity laboratory."""

import json
from pathlib import Path

root = Path(__file__).resolve().parents[1]
native = (root / "src/engine/admin_activity_test.cpp").read_text()
main = (root / "src/main.cpp").read_text()
page = (root / "src/web_page.h").read_text()
config = json.loads((root / "config/zanik-config.schema.json").read_text())
capabilities = json.loads((root / "deploy/compatibility-capabilities.json").read_text())

assert config["properties"]["FEATURE_ADMIN_ACTIVITY_TESTING"]["default"] is False
assert config["properties"]["FEATURE_ADMIN_ACTIVITY_TESTING"]["dangerous"] is True
assert "FEATURE_ADMIN_ACTIVITY_TESTING" in config["required"]
assert 'Flag("ZANIK_FEATURE_ADMIN_ACTIVITY_TESTING", false)' in (root / "src/feature_config.h").read_text()

for function in (
    "domResetProgress", "domResetQuests",
    "domGiveQuest", "domCompleteQuest", "domAddItem", "domHurtMe", "domRevive",
    "UnlockRecipes", "UnlockBuildings", "SetQuestObjective", "RevealMapFog",
    "OnRep_RevealedMapRegionsBitmap", "SavePlayerState",
):
    assert function in native

for safety_contract in (
    "ProcessAdminActivityTestRequests", "Target character must be online",
    "Another activity test is pending for this character", "Activity test queue is full",
    "The selected catalog entry is unavailable in the current game build",
    "The native XP trigger did not reach the requested level",
    "The game death path accepted the trigger", "EntryLive", "Mem::SafeWrite",
    "DominionCheatClass",
    "CheatPropertyOffsets", "IsA(t.controller,\"PlayerController\")",
    "managerOffset=0x428;classOffset=0x430", "ClassIsA(desiredClass,\"CheatManager\")",
    "ConstructCheatManager", "ParamsConstructorOffset=0x04EB6B10",
    "ConstructObjectOffset=0x04EB37A0", "InitCheatManagerOffset=0x07CCF370",
    "Mem::SafeWrite(property,created)",
    "ClassIsA(configuredClass,\"DominionCheatManager\")", "constructionClass",
    "ResetSkillNative", "nativeSkillReset", "skillResetSignature",
    "if(SkillLevel(t,key)==1)return true", "nativeSkillOperation", "if(skillOperation)ok=ResetSkillNative",
    "The selected skill component is not authoritative",
    "The game did not change the authoritative skill level",
    "Offsets::DomPlayerController_SkillComponent",
    "t.skills+Offsets::Skill_Skills",
    "Mem::Readable(list.Data,size_t(list.Num)*Offsets::FSkill_Size)",
    "QueueSkillXp", "nativeSkillXp", "skillXpSignature",
    "Status::SkillXpForLevel", "The native XP trigger did not reach",
    "GetMapRegionObjectName", "region-name:", "resolvedRegionEntries",
    "GetRevealedMapFogComponent", "The authoritative revealed-region component is unavailable",
    "The game did not retain the revealed-region bitmap reset",
    "WakeRegionReplication", 'FindFunction("Actor","FlushNetDormancy")',
    'FindFunction("Actor","ForceNetUpdate")',
    "The revealed-region client refresh could not be queued",
    "regionStateKnown", "regionBitmap", "discovered",
    "MarkRegionBitmapDirty", 'FindFunction("NetPushModelHelpers","MarkPropertyDirty")',
    'FindObject("NetPushModelHelpers",true)', "regionBitmapNameIndex",
    "LiveRegionObject", "LiveDefaultObject", "The selected live region object is unavailable",
):
    assert safety_contract in native

for route in (
    "/api/admin/activity-test/catalog", "/api/admin/activity-test/retest",
    "/api/admin/activity-test/status", "/api/admin/activity-test/diagnostics",
):
    assert route in main
assert "Type the target character name exactly to confirm" in main
assert "Acknowledge the full reset scope" in main
assert "quest reset does not reverse world state" in main
assert "RetestReady" in main and "BeginRetest" in main
assert "ConfirmRetestPreparation" in main
assert "failedTriggerGuid" in main and "ReleaseRetestTrigger(failedTriggerGuid)" in main
assert 'else if(phase=="trigger")playerActivityStore->ReleaseRetestTrigger(guid)' in main
assert "adminStore->Audit" in main
assert "adminActivityTestRegionCatalogEntries" in main
assert "adminActivityTestResolvedRegionCatalogEntries" in main
assert "adminActivityTestRegionReplicationCompatibility" in main
assert "adminActivityTestRegionReplicationWakeFailures" in main
assert "adminActivityTestRegionPropertyDirtyCompatibility" in main
assert "adminActivityTestRegionPropertyDirtyFailures" in main

assert 'id="adminSessionsSection"' in page
assert 'role="tablist" aria-label="Administration tools"' in page
assert 'id="sessionsTab"' in page and 'id="activityTab"' in page
assert 'role="tabpanel" aria-labelledby="sessionsTab"' in page
assert 'role="tabpanel" aria-labelledby="activityTab" hidden' in page
assert "selectAdminTab" in page and "#activity-lab" in page and "ArrowRight" in page
assert "session-create" in page and "sessions-table" in page
assert "table-layout:fixed" in page and "overflow-wrap:anywhere" in page
assert "Recent activity test laboratory" in page
assert "Use test characters only" in page
assert "not a preview" in page
assert "Wait for Zanik’s authoritative scanners" in page
assert "slice(0,100)" in page
assert "discovery status unavailable" in page
assert "not discovered" in page
assert "refreshCatalogEntries" in page
assert '<option value="progress">Advance to the next objective</option>' in page

inventory = {entry["id"]: entry for entry in capabilities["capabilities"]}
capability = inventory["admin-recent-activity-testing"]
assert capability["diagnosticsEndpoint"] == "/api/admin/activity-test/diagnostics"
assert "zanik-data/player-activity.db" in capability["persistence"]
assert len(capability["assets"]) == 4
assert "adminActivityTestResolvedRegionCatalogEntries" in capability["runtimeStatusFields"]
assert "adminActivityTestRegionReplicationCompatibility" in capability["runtimeStatusFields"]
assert "adminActivityTestRegionReplicationWakeFailures" in capability["runtimeStatusFields"]
assert "adminActivityTestRegionPropertyDirtyCompatibility" in capability["runtimeStatusFields"]
assert "adminActivityTestRegionPropertyDirtyFailures" in capability["runtimeStatusFields"]

analyzer = (root / "tools/compat_analyzer.cpp").read_text()
for symbol in (
    "domResetProgress", "domResetQuests",
    "SavePlayerState", "UnlockRecipes", "UnlockBuildings", "SetQuestObjective", "RevealMapFog",
    "OnRep_RevealedMapRegionsBitmap",
    "Z_Construct_UFunction_UNetPushModelHelpers_MarkPropertyDirty()",
    "UNetPushModelHelpers::MarkPropertyDirty(UObject*, FName)",
):
    assert symbol in analyzer
assert "APlayerController::APlayerController(FObjectInitializer const&)" in analyzer
assert "admin activity cheat-manager native layout changed" in analyzer
assert "adminCheatSlotsLayout" in analyzer and "adminCheatClassLayout" in analyzer
assert "admin activity cheat-manager construction ABI changed" in analyzer
assert "adminParamsConstructorLayout" in analyzer and "adminStaticConstructLayout" in analyzer
assert "adminCheatInitLayout" in analyzer
assert "USkillComponent::Reset(USkillData const*)" in analyzer
assert "admin activity native skill-reset ABI changed" in analyzer
assert "USkillComponent::QueueEventSkillXP(TObjectPtr<USkillData>, int)" in analyzer
assert "admin activity native skill-XP ABI changed" in analyzer
assert "URevealedMapFogComponent::IsMapRegionRevealed(unsigned int) const" in analyzer
assert "recent activity revealed-region bitmap ABI changed" in analyzer
assert "admin activity test reflection symbols missing" in analyzer

discovery = (root / "src/engine/player_activity_discovery.cpp").read_text()
getter = discovery.index("if(getRevealedMapFogComponent&&processEvent)", discovery.index("RegionComponentFor"))
cache = discovery.index("auto known=regionCache.find", discovery.index("RegionComponentFor"))
assert getter < cache
assert "old UObject remains live until GC" in discovery

print("admin activity test safety contract passed")
