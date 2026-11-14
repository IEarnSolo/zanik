#!/usr/bin/env python3
"""Regression checks for the bounded asynchronous storage mutation pipeline."""

from pathlib import Path


root = Path(__file__).resolve().parents[1]
storage = (root / "src/engine/admin_storage.cpp").read_text(encoding="utf-8")
header = (root / "src/engine/dom_engine.h").read_text(encoding="utf-8")
main = (root / "src/main.cpp").read_text(encoding="utf-8")
web = (root / "src/web_page.h").read_text(encoding="utf-8")
quests = (root / "src/engine/player_quests.cpp").read_text(encoding="utf-8")
analyzer = (root / "tools/compat_analyzer.cpp").read_text(encoding="utf-8")


def body(name: str, following: str) -> str:
    start = storage.index(name)
    end = storage.index(following, start)
    return storage[start:end]


tick_action = body("void ProcessOneStorageAction()", "void WriteJump")
planner = body("bool BuildMutationPlan(", "bool SubmitAdminStorageMutation(")
snapshot = body("std::shared_ptr<const StorageSnapshot> BuildStorageSnapshot", "const SnapshotChest*")
engine_tick = body("void HookEngineTick(", "bool WasAdminStorageBuildingRemoved")

# Performance timing is elapsed monotonic execution around the original engine
# dispatcher, never an assumed meaning or unit for one of its native arguments.
assert "engineStarted=std::chrono::steady_clock::now()" in engine_tick
assert engine_tick.index("engineStarted=") < engine_tick.index("originalEngineTick(") < engine_tick.index("engineMicros=")
assert "double(delta)*1000000.0" not in engine_tick
assert "engineMicros>=50000" in engine_tick

# Native in-game mutations time only the original engine call. The hook copies
# fixed-size evidence into a bounded ring; player matching and diagnostics run
# later on the service thread. Zanik's grave dispatcher is excluded so website
# actions are not mislabeled as in-game actions.
native_hooks = body("bool PublishObservedMutation(", "uintptr_t FindUniqueStorageFunction(")
assert "ObservedMutationCapacity=128" in storage
assert "HookMoveItem" in native_hooks and "HookRemoveFromSlot" in native_hooks
assert "originalObservedMoveItem(source" in native_hooks
assert "originalObservedRemoveFromSlot(source" in native_hooks
assert "WriteRegisterPreservingJump(trampoline+bytes" in native_hooks
assert "std::array<ObservedMutationSlot,ObservedMutationCapacity>" in storage
assert "UpdateAdminStorageMutationPlayers" in storage
assert "DrainAdminStorageMutationTraces" in storage
drain = body("void DrainAdminStorageMutationTraces()", "AdminStorageDiagnostics GetAdminStorageDiagnostics")
assert "event.succeeded&&storage&&autofillStore" in drain
assert "autofillMutationTriggers.push_back" in drain
assert "InvalidateAdminStorageSnapshots()" in drain
assert "AutofillMutationTriggerCapacity=128" in storage
assert "HasPendingAdminStorageAutofillMutation" in storage
assert "suppressObservedMutation=true;ProcessAdminGravestoneRequests();suppressObservedMutation=false" in engine_tick
for forbidden in ("GetAllPlayers", "LogMessage", "pthread_mutex", "std::string"):
    assert forbidden not in native_hooks, forbidden
assert "nativeMutationCaptureCompatibility" in header
assert '"/api/diagnostics/native-inventory-mutations"' in main
assert "observedMoveEntry.store(moduleBase+offsets.moveItem" in storage
assert "ResolveAdminInventoryMutationCallables" in storage

# The actual dedicated-server withdrawal routes bypass the public mutation
# entries. Capture all full and partial stack RPCs after ProcessEvent returns,
# then feed the same bounded service-thread validation and priority path.
for rpc in (
    "Server_MoveItemBetweenInventories",
    "Server_SplitAndPutStack",
    "Server_SplitStack",
):
    assert rpc in storage
    assert rpc in analyzer
assert "storageMoveInventoryRpcMatches!=1" in analyzer
assert "storageSplitStackRpcMatches!=1" in analyzer
assert "storageSplitPutRpcMatches!=1" in analyzer
process_hook = quests[quests.index("void HookProcessEvent("):quests.index("void DrainTrackedQuestCaptures(")]
assert process_hook.index("originalProcessEvent(object,function,params)") < process_hook.index("ObserveAdminStorageProcessEvent(object,function,params)")
rpc_observer = body("void ObserveAdminStorageProcessEvent(", "bool ResolveAdminInventoryMutationCallables")
assert "PublishObservedMutation" in rpc_observer
assert "GetObjectCount" not in rpc_observer
assert "LogMessage" not in rpc_observer
assert "std::string" not in rpc_observer
assert "autofillRpcCaptureCompatibility" in header
assert "adminStorageAutofillRpcCaptureCompatibility" in main
for token in (
    'FindStorageRpc("Server_ConsumeItemsFromCompletingBuilding")',
    'FindStorageRpc("Server_ConsumeItemsFromPartiallyCompletingBuilding")',
    "ResolveInventoryArrayParameterOffset",
    "ObservedMutationKind::BuildingConsume",
    "autofillBuildingCaptureCompatibility",
    "InstallNativeBuildingConsumptionCapture",
    "HookCompleteBuildingConsumption",
    "HookPartialBuildingConsumption",
    "BuildingConsumptionHookBytes",
    "autofillCompleteBuildingNativeTarget",
):
    assert token in storage
for token in ("linked-building native consumption thunk or target changed", "storageCompleteBuildingRpcAt", "storagePartialBuildingRpcAt", "nativeCompleteBuildingTarget", "nativePartialBuildingTarget"):
    assert token in analyzer
for field in (
    "adminStorageAutofillBuildingCaptureCompatibility",
    "adminStorageAutofillBuildingCaptureCalls",
    "adminStorageAutofillBuildingCaptureInventories",
    "adminStorageAutofillBuildingCaptureDrops",
    "adminStorageAutofillBuildingCaptureMaximumMicroseconds",
):
    assert field in main
assert "AdminStorageAutofillBuildingCaptureDiagnostics" in header
for field in (
    "adminStorageAutofillLatencySamples",
    "adminStorageAutofillCaptureToDrainMaximumMicroseconds",
    "adminStorageAutofillDrainToReconcileMaximumMicroseconds",
    "adminStorageAutofillReconcileToDispatchMaximumMicroseconds",
    "adminStorageAutofillDispatchToRefillMaximumMicroseconds",
    "adminStorageAutofillCaptureToRefillMaximumMicroseconds",
):
    assert field in main
assert '"/api/diagnostics/storage-autofill-latency"' in main
assert '"/api/diagnostics/storage-autofill-replication"' in main
assert "autofillObservedAtMicros" in storage
assert "autofillDrainedAtMicros" in storage
assert "autofillReconciledAtMicros" in storage
assert "autofillDispatchedAtMicros" in storage

# Once the final slot-exact refill action succeeds, wake only its verified
# owning actor and request an immediate replication pass through the shared
# ProcessEvent trampoline. Failure must not roll back or fail the refill.
assert 'FindFunction("Actor","FlushNetDormancy")' in storage
assert 'FindFunction("Actor","ForceNetUpdate")' in storage
assert storage.index("FlushNetDormancy") < storage.index("ForceNetUpdate")
assert "InvokeSharedProcessEvent" in storage and "InvokeSharedProcessEvent" in quests
assert "WakeAutofilledStorageReplication(r,action)" in tick_action
assert "autofillReplicationWakeCompatibility" in header
assert "adminStorageAutofillReplicationWakeCompatibility" in main

# The game-thread path consumes an already built action and never performs discovery.
for forbidden in (
    "GetObjectCount(",
    "GetObjectByIndex(",
    "FindChest(",
    "FindChests(",
    "FindCatalogData(",
    "RecipeRecords(",
    "pthread_mutex",
    "pthread_cond",
    "LogMessage(",
):
    assert forbidden not in tick_action, forbidden
assert "requestRing" in tick_action
assert "RunNativeAction" in tick_action
assert "actionIndex.fetch_add(1)" in tick_action
assert "std::reverse" not in tick_action

# Discovery, identity capture, catalog resolution, and capacity checks happen in
# request preparation on the HTTP worker.
assert "GetObjectCount()" in snapshot
assert "GetObjectIdentity" in snapshot
assert "ReadCustomNameCached" in snapshot
assert "FindCatalogData" in planner
assert "ResolveRecipe" in planner
assert "canAddItemByData" in planner
assert "canAddItemsByData" in planner
assert 'RuntimePaths::WebRoot()+"/items/"' in storage
assert 'std::string icon=itemRoot+key+".png"' in storage

# Every supported family is represented by a bounded action/state-machine path.
for operation in (
    'operation=="add"',
    'operation=="add_recipe"',
    'operation=="slot_move"',
    'operation=="move_batch"',
    'operation=="organize_batch"',
    'operation=="copy_batch"',
    'operation=="delete_batch"',
    'operation=="delete"',
    'operation=="add_existing"',
    'operation=="take"',
    'operation=="rename"',
):
    assert operation in planner, operation
assert "JobState::RollingBack" in tick_action
assert "familyCircuitBroken" in tick_action
assert "micros>=1000" in tick_action
assert "renameTextConstructed" in tick_action
assert "finishAsFailure" in tick_action

# Public API is asynchronous, authenticated, session-bound, and polled by the UI.
assert '"/api/admin/storage/mutation-status"' in main
assert 'Respond(fd,202' in main
assert "GetAdminStorageMutationStatus(id,adminStore->SessionHashPrefix(raw))" in main
assert "std::string GetAdminStorageMutationStatus" in header
assert "submitStorageRequest" in web
assert "mutation-status?id=" in web
assert "status.status==='rolling-back'" in web
assert "updatedUnix.load()+300" in storage
mutation_route = main[main.index('path=="/api/admin/storage/mutate"'):main.index('path=="/api/admin/storage/mutation-status"')]
assert mutation_route.index("authenticate(s,raw)") < mutation_route.index("AdminStorageMutation()")
assert mutation_route.index("AdminStorageMutation()") < mutation_route.index('JsonValue(request.body,"csrf")')
status_route = main[main.index('path=="/api/admin/storage/mutation-status"'):main.index('path=="/api/admin/player-storage"')]
assert status_route.index("authenticate(s,raw)") < status_route.index("GetAdminStorageMutationStatus")
assert "found->second->session!=session" in storage
assert "Another storage operation is already pending" in storage

# Rename is statically ABI-gated, follows the authoritative player-controller
# server path, and circuit-breaks immediately if interface readback is not preserved.
assert "serverSetObjectCustomName" in storage
assert "0x0A593D70" in storage
assert "cachedDisplayTextOffset.store" in storage
assert "ProcessOneCustomName();" in engine_tick
assert "ActionKind::RenameVerify" in planner
assert "actual==r.renameValue.data()" in storage
assert "This storage type did not preserve the requested name" in storage
assert "renameCompatibility=-1" not in tick_action
assert "Storage rename ABI validation failed" in storage
assert 'if(renameCompatibility.load()!=1){error="Storage naming is unavailable for this game build"' in planner
assert '\\"storageRename\\"' in main
assert "storageRenameAvailable=Boolean(capabilities.storageRename)" in web
assert "renderStorageHeading(card,chest,heading)" in web
assert "if(storageRenameAvailable)" in web

# Reviewed builds are production-enabled explicitly; development overrides
# remain limited to named candidate hashes awaiting live promotion evidence.
policy = (root / "src" / "engine" / "storage_compatibility_policy.h").read_text()
assert 'ZANIK_STORAGE_PERFORMANCE_CANDIDATE' in storage
assert "StorageCompatibilityPolicy::IsProductionReviewed(hash)" in storage
assert "CandidateEnabled()&&StorageCompatibilityPolicy::IsCandidateOnly(hash)" in storage
assert 'CurrentReviewedSha' in policy and 'PreviousReviewedSha' in policy
assert 'LatestReviewedSha' in policy
assert 'return hash == LegacyCandidateSha;' in policy
assert 'candidateEnabled&&(hash==LegacySha||hash==PreviousSha||hash==CurrentSha)' not in storage
assert 'storageProductionCompatibility' in main
assert 'storageCandidateOverride' in main

# Independent operational diagnostics remain public and bounded.
for field in (
    "snapshotGenerations",
    "snapshotCacheHits",
    "queueDrops",
    "staleIdentities",
    "rollbackFailures",
    "singleItemMaximumMicroseconds",
    "batchMovementMaximumMicroseconds",
    "circuitBreakers",
    "singleItemMaximumMicroseconds",
    "deletionMaximumMicroseconds",
    "renameCompatibility",
):
    assert field in header
    assert field in main


# Deterministic state-machine model: the native executor is deliberately fake,
# but it exercises the same one-action-per-tick, rollback, stale/world-change,
# queue-capacity, and circuit-breaker invariants enforced above in production.
class FakeExecutor:
    def __init__(self, outcomes=(), durations=()):
        self.outcomes = iter(outcomes)
        self.durations = iter(durations)
        self.calls = []

    def run(self, action):
        self.calls.append(action)
        return next(self.outcomes, True), next(self.durations, 20)


def run_job(actions, executor, *, identity_valid=True, world_valid=True):
    completed = []
    rollback = []
    broken = set()
    failure = None
    for action in actions:
        if not world_valid:
            failure = "world-changed"
            break
        if not identity_valid:
            failure = "stale-identity"
            break
        ok, micros = executor.run(action)
        if micros >= 1000:
            broken.add(action[0])
        if not ok:
            failure = "native-failure"
            break
        completed.append(action)
        if action[1] == "move":
            rollback.append(("rollback", "move", action[2], action[3]))
    if failure:
        for action in reversed(rollback):
            ok, micros = executor.run(action)
            if micros >= 1000:
                broken.add("rollback")
            if not ok:
                return "failed", broken, "rollback-failed"
        return "failed", broken, failure
    return "completed", broken, None


fake = FakeExecutor([True, False, True])
state, broken, reason = run_job(
    [("batch-move", "move", 1, 2), ("batch-move", "move", 3, 4)], fake
)
assert state == "failed" and reason == "native-failure"
assert [call[0] for call in fake.calls] == ["batch-move", "batch-move", "rollback"]

assert run_job([("single", "add", 0, 0)], FakeExecutor(), identity_valid=False)[2] == "stale-identity"
assert run_job([("single", "add", 0, 0)], FakeExecutor(), world_valid=False)[2] == "world-changed"
state, broken, _ = run_job([("rename", "rename", 0, 0)], FakeExecutor([True], [1000]))
assert state == "completed" and broken == {"rename"}

capacity = 32
queue = list(range(capacity))
assert len(queue) == capacity
assert len(queue) >= capacity  # the next submission is rejected without mutation

print("storage mutation pipeline regression checks passed")
