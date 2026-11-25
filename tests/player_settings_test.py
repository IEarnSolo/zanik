#!/usr/bin/env python3
import json
from pathlib import Path

root = Path(__file__).resolve().parents[1]
schema = json.loads((root / "config/zanik-config.schema.json").read_text())
setting = schema["properties"]["FEATURE_ADMIN_PLAYER_SETTINGS"]
assert setting["default"] is False and setting["dangerous"] is True
assert "FEATURE_ADMIN_PLAYER_SETTINGS" in schema["required"]

feature = (root / "src/feature_config.h").read_text()
common = (root / "scripts/lib/zanik-common.sh").read_text()
launcher = (root / "scripts/zanik-server").read_text()
for source in (feature, common, launcher):
    assert "ADMIN_PLAYER_SETTINGS" in source
assert 'Flag("ZANIK_FEATURE_ADMIN_PLAYER_SETTINGS", false)' in feature

main = (root / "src/main.cpp").read_text()
for token in ('"/api/admin/player-settings"', "AdminPlayerSettingsAvailable()",
              "ConfigurePlayerSettings", 'setting!="freeBuilding"', 'JsonBoolean(request.body,"enabled",enabled)',
              'JsonValue(request.body,"csrf")!=s.csrf', "player-setting:free-building"):
    assert token in main

web = (root / "src/web_page.h").read_text()
assert web.index('data-player-view="gravestones"') < web.index('data-player-view="settings"')
assert "Ignore building material requirements and consume no building materials for this character only" in web
for token in ("capabilities.playerSettings", "/api/admin/player-settings", "freeBuildingToggle"):
    assert token in web

storage = (root / "src/engine/admin_storage.cpp").read_text()
for offset in ("0x0A3F4F80", "0x0A3E7000", "0x0A3F5020", "0x0A3E70A0",
               "0x0A3F4FB0", "0x0A3E7030", "0x0A3C6DF0", "0x0A3C6E90", "0x0A3C6E20",
               "0x0A3CB6E0", "0x0A3CB780", "0x0A3CB710", "0x0A3E8FAE", "0x0A3E904E", "0x0A3E8FDE"):
    assert offset in storage
for token in ("HookValidateBuildingMaterials", "HookValidateBuildingDonation",
              "HookShouldIgnoreBuildingMaterials", "ZanikShouldIgnoreBuildingMaterials",
              "HookAddBuildingCompletion", "needed-done",
              "HookCompleteBuildingConsumption", "HookPartialBuildingConsumption",
              "PlayerOwnsObject(context,player)", "captured.count>=0",
              "freeBuildingSuppressions", "freeBuildingAttributionDrops",
              "freeBuildingValidationBypasses", "freeBuildingSpawnBypasses", "freeBuildingConsumptionBypasses",
              "freeBuildingCompletionCalls", "freeBuildingCompletionAttributed",
              "freeBuildingCompletionLayoutRejects", "freeBuildingCompletionItemRejects",
              "freeBuildingCompletionAdvances", "InitializeFreeBuildingClientDifficulty",
              "CreateFreeBuildingDifficultyComponent", "ReconcileOneFreeBuildingClientDifficulty",
              "Difficulty.Progression.BuildingMaterialCostScale", "AddComponentByClass",
              "ReplicatedDifficultySettings", "freeBuildingClientOverrides",
              "freeBuildingClientRestores", "freeBuildingClientOverrideFailures",
              "freeBuildingClientCreateFailures", "freeBuildingClientSourceFailures",
              "freeBuildingClientApplyFailures", "freeBuildingClientSourceFailureStage",
              "freeBuildingClientInitFailureStage", "Conv_StringToName", "difficultyNow", "milliseconds(250)",
              "freeBuildingStringToNameInputOffset", "freeBuildingStringToNameReturnOffset",
              "baselineCaptured", "baselineEntries", "baselineFingerprint",
              "GetGNames", "ReadDifficultyPayload(state.component",
              "MarkPropertyDirty", "freeBuildingDifficultyPropertyNameIndex", "FindLoadedNameIndex",
              "DifficultyFunctionLibrary", "GetSettingValueAsFloat", "baselineBuildingValue",
              "K2_DestroyComponent", "DestroyFreeBuildingDifficultyComponent", "replacement.component",
              "Client_OnBuildingsUnlocked", "fromLoadedState=true",
              "NotifyFreeBuildingClientRefresh", "ResolveFreeBuildingProgress",
              "freeBuildingClientRefreshes", "freeBuildingClientRefreshFailures", "refreshNotBefore", "milliseconds(750)",
              "FMemoryReallocOffset", "0x04A9C330"):
    assert token in storage
assert "if(ZanikConfig::AdminPlayerSettings())" in storage

analyzer = (root / "tools/compat_analyzer.cpp").read_text()
for token in ("ValidateMaterialsForInventory", "ValidateDonationMaterials",
              "Server_SpawnBuilding_Implementation", "ShouldIgnoreMaterialCosts",
              "FBuildingPiece::AddToCompletion", "addCompletionTarget",
              "validateMaterialsTarget", "validateDonationTarget",
              "building material validation or completion ABI changed"):
    assert token in analyzer
assert "Z_Construct_UFunction_UKismetStringLibrary_Conv_StringToName" in analyzer
assert "Z_Construct_UFunction_UDifficultyFunctionLibrary_GetSettingValueAsFloat" in analyzer
assert "Z_Construct_UFunction_UActorComponent_K2_DestroyComponent" in analyzer
assert "Z_Construct_UFunction_UProgressComponent_Client_OnBuildingsUnlocked" in analyzer
assert "UProgressComponent::Client_OnBuildingsUnlocked(TArray<FDominionDataAssetNetId" in analyzer
assert "Client_DisplayTextNotification" not in storage

capabilities = {entry["id"]: entry for entry in
                json.loads((root / "deploy/compatibility-capabilities.json").read_text())["capabilities"]}
capability = capabilities["admin-player-free-building"]
assert "zanik-data/player-settings.db" in capability["persistence"]
assert any("none of" in check for check in capability["liveValidation"])
assert any("another character" in check for check in capability["liveValidation"])

print("per-character player settings tests passed")
