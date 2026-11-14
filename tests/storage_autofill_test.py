#!/usr/bin/env python3
"""Contracts for persistent, slot-exact storage snapshot autofill."""

import json
from pathlib import Path

root = Path(__file__).resolve().parents[1]
storage = (root / "src/engine/admin_storage.cpp").read_text(encoding="utf-8")
header = (root / "src/engine/dom_engine.h").read_text(encoding="utf-8")
main = (root / "src/main.cpp").read_text(encoding="utf-8")
web = (root / "src/web_page.h").read_text(encoding="utf-8")
config = (root / "src/feature_config.h").read_text(encoding="utf-8")
schema = json.loads((root / "config/zanik-config.schema.json").read_text(encoding="utf-8"))
capabilities = json.loads((root / "deploy/compatibility-capabilities.json").read_text(encoding="utf-8"))
installer = (root / "tests/installer_test.sh").read_text(encoding="utf-8")
backups = (root / "docs/backups.md").read_text(encoding="utf-8")

assert 'Flag("ZANIK_FEATURE_QOL_STORAGE_AUTOFILL", false)' in config
assert schema["properties"]["FEATURE_QOL_STORAGE_AUTOFILL"]["default"] is False
assert "FEATURE_QOL_STORAGE_AUTOFILL" in schema["required"]
assert '"/api/admin/storage/autofill"' in main[main.index("postRoutes"):main.index("auto requirePost")]

route = main[main.index('path=="/api/admin/storage/autofill"'):main.index('path=="/api/admin/storage/mutation-status"')]
assert route.index("authenticate(s,raw)") < route.index("QolStorageAutofill()") < route.index('JsonValue(request.body,"csrf")')
assert "SetAdminStorageAutofill" in route and "JsonBoolean" in route and "adminStore->Audit" in route
assert 'scope=="selection"' in route
assert 'JsonStringArray(request.body,"itemIds")' in route
assert "selection?!itemIds.empty():itemIds.empty()" in route

reconcile = storage[storage.index("void ReconcileAdminStorageAutofill()"):storage.index("std::string GetAdminStorageMutationStatus")]
assert 'request->operation="autofill"' in reconcile
assert "ActionKind::Add" in reconcile
assert "ActionKind::Move" in reconcile
assert "ActionKind::SetStack" in reconcile
assert "PlanStorageAutofill" in reconcile
assert "canAddItemByData" in reconcile
assert "autofillNormalize" in reconcile
assert "snapshotOccupiedSlots" in reconcile
assert "request->autofillNormalize.push_back({item.slot,item.quantity,item.maxStack,item.key})" in reconcile
assert "autofillMutationTriggers.front()" in reconcile
assert "candidateChest->inventory==trigger.inventory" in reconcile
assert "autofillPriorityMatches" in reconcile
assert "RefreshKnownAutofillStorage(candidate,trigger,candidateKnown)" in reconcile
targeted = storage[storage.index("std::shared_ptr<const StorageSnapshot> RefreshKnownAutofillStorage"):storage.index("std::shared_ptr<const StorageSnapshot> BuildStorageSnapshot")]
assert "RefreshKnownStorageChest" in targeted
assert "GetObjectCount" not in targeted and "GetObjectByIndex" not in targeted
assert "autofillTargetedRefreshMaximumMicros" in targeted
assert "NormalizeAutofillAdd" in storage
normalize = storage[storage.index("bool NormalizeAutofillAdd"):storage.index("bool RunNativeAction")]
assert "addItemByData" not in normalize
assert "moveItem" in normalize and "setStackSize" in normalize
assert normalize.count("actualTotal!=desiredTotal") == 1
assert normalize.index("setStackSize") < normalize.index("actualTotal!=desiredTotal")
assert "PlanStorageAutofillRelocation" in normalize
assert "!NormalizeAutofillAdd" in storage and "familyCircuitBroken[size_t(OperationFamily::Autofill)]=true" in storage
assert "autofillNormalizationCompatibility=-1" in storage
assert "ActionKind::Remove" not in reconcile
assert "familyCircuitBroken[size_t(OperationFamily::Autofill)]" in reconcile
assert "plan.sourceSlot" in reconcile and "plan.destinationSlot" in reconcile

settings = storage[storage.index("bool SetAdminStorageAutofill("):storage.index("void ReconcileAdminStorageAutofill()")]
assert "const std::vector<std::string>&itemIds" in settings
assert "autofillStore->Find(characterGuid,chestId,value)" in settings
assert "SnapshotFindItem(*chest,id)" in settings
assert "ApplyStorageAutofillSelection(value,replacements,enabled)" in settings
assert "!enabled&&value.items.empty()" in settings
assert "autofillStore->Remove(characterGuid,chestId)" in settings
assert "Selected items added to the autofill snapshot" in settings
assert "Selected items removed from the autofill snapshot" in settings

for token in ("storage-autofill.db", "ConfigureAdminStorageAutofill", "ReconcileAdminStorageAutofill", "adminStorageAutofillCompatibility", "adminStorageAutofillProtectedSkips"):
    assert token in main or token in header
for token in ("adminStorageAutofillNormalizationCompatibility", "adminStorageAutofillNormalizationFailures"):
    assert token in main
for token in ("adminStorageAutofillMutationSignals", "adminStorageAutofillPriorityChecks", "adminStorageAutofillPriorityMatches", "adminStorageAutofillPriorityDrops"):
    assert token in main
for token in ("adminStorageAutofillTargetedRefreshes", "adminStorageAutofillTargetedRefreshFallbacks", "adminStorageAutofillTargetedRefreshMaximumMicroseconds"):
    assert token in main
assert "HasPendingAdminStorageAutofillMutation" in main and "urgentStorageAutofill" in main
assert "std::chrono::milliseconds(500)" in main
for token in ("Autofill from snapshot", "snapshotted slot", "never overwritten or removed", "/api/admin/storage/autofill", "storageAutofillAvailable"):
    assert token in web
assert "Matching snapshotted stacks may be rebalanced after refilling" not in web
for token in ("updateStorageAutofill", "scope='selection'", "Disable autofill", "mixed selection", "storage-context-disabled"):
    assert token in web
assert "enabledCount>0&&enabledCount<entries.length" in web
assert "autofill.disabled=true" in web
assert "entries.map(value=>value.id)" in web
context_menu = web[web.index("function showStorageContext("):web.index("async function moveSelectedStorageStacks(")]
assert context_menu.index("addGroup('Organize'") < context_menu.index("if(storageAutofillAvailable)") < context_menu.index("remove.textContent='Delete'")

# The authenticated storage response classifies only occupied slots that were
# non-empty in the durable template. Exact item matches and protected foreign
# replacements receive distinct, accessible indicators and hover details.
for token in ("autofillSnapshotState", "autofillSnapshotName", "autofillSnapshotQuantity"):
    assert token in storage and token in web
assert 'saved->key==item.key?"matched":"foreign"' in storage
assert "if(const auto*saved=savedAt(item.slot))" in storage
assert "autofill-snapshot-match" in web and "autofill-snapshot-foreign" in web
assert "Autofill snapshot — restores this slot" in web
assert "Foreign item in a snapshotted slot" in web
assert "will not overwrite or remove it" in web
assert "badge.textContent=snapshotState==='matched'?'↻':'!'" in web

entry = next(item for item in capabilities["capabilities"] if item["id"] == "storage-autofill")
assert entry["persistence"] == ["zanik-data/storage-autofill.db"]
assert "adminStorageAutofillNormalizationCompatibility" in entry["runtimeStatusFields"]
assert "adminStorageAutofillNormalizationFailures" in entry["runtimeDiagnosticFields"]
assert "tests/storage_autofill_store_test.cpp" in entry["tests"]
building_capture = next(item for item in capabilities["capabilities"] if item["id"] == "storage-autofill-building-consumption-capture")
assert building_capture["runtimeStatusFields"] == ["adminStorageAutofillBuildingCaptureCompatibility"]
assert "adminStorageAutofillBuildingCaptureDrops" in building_capture["runtimeDiagnosticFields"]
assert any("native thunks plus trampoline-safe implementation-entry prologues" in value for value in building_capture["staticChecks"])
validator = (root / "deploy/validate-dev-update.ps1").read_text()
for token in ("adminStorageAutofillCompatibility", "adminStorageAutofillNormalizationCompatibility",
              "adminStorageAutofillNormalizationFailures", "adminStorageAutofillBuildingCaptureCompatibility",
              "adminStorageAutofillBuildingCaptureDrops", "adminStorageCircuitBreakers"):
    assert token in validator
assert "zanik-data/storage-autofill.db" in installer
assert "storage-autofill.db" in backups and "preserve" in backups
