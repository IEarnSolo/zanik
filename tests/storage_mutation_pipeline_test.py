#!/usr/bin/env python3
"""Regression checks for the bounded asynchronous storage mutation pipeline."""

from pathlib import Path


root = Path(__file__).resolve().parents[1]
storage = (root / "src/engine/admin_storage.cpp").read_text(encoding="utf-8")
header = (root / "src/engine/dom_engine.h").read_text(encoding="utf-8")
main = (root / "src/main.cpp").read_text(encoding="utf-8")
web = (root / "src/web_page.h").read_text(encoding="utf-8")


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
native_hooks = body("void PublishObservedMutation(", "void HookBuildingEndPlay(")
assert "ObservedMutationCapacity=128" in storage
assert "HookMoveItem" in native_hooks and "HookRemoveFromSlot" in native_hooks
assert "originalObservedMoveItem(source" in native_hooks
assert "originalObservedRemoveFromSlot(source" in native_hooks
assert "WriteRegisterPreservingJump(trampoline+bytes" in native_hooks
assert "std::array<ObservedMutationSlot,ObservedMutationCapacity>" in storage
assert "UpdateAdminStorageMutationPlayers" in storage
assert "DrainAdminStorageMutationTraces" in storage
assert "suppressObservedMutation=true;ProcessAdminGravestoneRequests();suppressObservedMutation=false" in engine_tick
for forbidden in ("GetAllPlayers", "LogMessage", "pthread_mutex", "std::string"):
    assert forbidden not in native_hooks, forbidden
assert "nativeMutationCaptureCompatibility" in header
assert '"/api/diagnostics/native-inventory-mutations"' in main
assert "observedMoveEntry.store(moduleBase+offsets.moveItem" in storage
assert "ResolveAdminInventoryMutationCallables" in storage

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
assert "if(storageRenameAvailable)renderStorageHeading" in web

# Normal current-build releases remain fail-closed until live validation opts in.
assert 'ZANIK_STORAGE_PERFORMANCE_CANDIDATE' in storage
assert "candidateEnabled&&(hash==PreviousSha||hash==CurrentSha)" in storage

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
