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
assert "actualTotal!=desiredTotal" in normalize
assert "unexpected" in normalize and "controlled" not in normalize
assert "!NormalizeAutofillAdd" in storage and "familyCircuitBroken[size_t(OperationFamily::Autofill)]=true" in storage
assert "ActionKind::Remove" not in reconcile
assert "familyCircuitBroken[size_t(OperationFamily::Autofill)]" in reconcile
assert "plan.sourceSlot" in reconcile and "plan.destinationSlot" in reconcile

for token in ("storage-autofill.db", "ConfigureAdminStorageAutofill", "ReconcileAdminStorageAutofill", "adminStorageAutofillCompatibility", "adminStorageAutofillProtectedSkips"):
    assert token in main or token in header
for token in ("adminStorageAutofillMutationSignals", "adminStorageAutofillPriorityChecks", "adminStorageAutofillPriorityMatches", "adminStorageAutofillPriorityDrops"):
    assert token in main
for token in ("adminStorageAutofillTargetedRefreshes", "adminStorageAutofillTargetedRefreshFallbacks", "adminStorageAutofillTargetedRefreshMaximumMicroseconds"):
    assert token in main
assert "HasPendingAdminStorageAutofillMutation" in main and "urgentStorageAutofill" in main
assert "std::chrono::milliseconds(500)" in main
for token in ("Autofill from snapshot", "snapshotted slot", "never overwritten or removed", "/api/admin/storage/autofill", "storageAutofillAvailable"):
    assert token in web

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
assert "tests/storage_autofill_store_test.cpp" in entry["tests"]
assert "zanik-data/storage-autofill.db" in installer
assert "storage-autofill.db" in backups and "preserve" in backups
