from pathlib import Path


root = Path(__file__).resolve().parents[1]
graves = (root / "src/engine/admin_gravestone.cpp").read_text()
storage = (root / "src/engine/admin_storage.cpp").read_text()
teleport = (root / "src/engine/admin_teleport.cpp").read_text()
kills = (root / "src/engine/kills.cpp").read_text()
resources = (root / "src/engine/resources.cpp").read_text()
quests = (root / "src/engine/player_quests.cpp").read_text()
logger = (root / "src/utils/logger.h").read_text()
trace = (root / "src/performance_trace.h").read_text()

# The first gravestone after a restart must not hash the executable or write a
# research capture from the game thread. The reviewed executable hash is
# published during compatibility initialization instead.
assert "std::atomic<uintptr_t> cachedDisplayTextOffset" in graves
assert "cachedDisplayTextOffset.store(hash==CurrentSha?0x04AD5A80:((hash==LegacySha||hash==PreviousSha)" in graves
assert "grave-respawn-capture.bin" not in graves
assert "spud-state-index-capture.bin" not in graves
assert "WriteGraveRespawnCapture" not in graves

# Authoritative lifecycle hooks copy fixed records only. In particular, the
# unregister hook may consult the fixed lifecycle cache but may not walk the
# Unreal registry it is intercepting.
unregister = graves.split("void HookUnregisterGravestone", 1)[1].split(
    "bool InstallUnregisterGravestoneHook", 1
)[0]
assert "GraveLifecycleCacheCapacity=128" in graves
assert "GetObjectCount" not in unregister
assert "records.Num" not in unregister
assert "pthread_mutex_lock" not in unregister

# Slow naming/persistence remains worker-side, request handoff is non-blocking,
# and the shared dispatcher rotates one latency-sensitive consumer per frame.
execute = graves.split("bool Execute(", 1)[1].split("void FlushAdminGravestoneSnapshots", 1)[0]
assert "LearnSavedItemMetadata" not in execute
assert "PlayerInventory(" not in execute
assert "FindPlayer(" not in execute
assert "if(!scanComplete)return false" in execute
assert "preparedGraveInventoryIdentity" in execute
assert "actual==expected" in execute
assert "ResolveSavedItemData" not in execute
assert "SnapshotInventory(" not in execute
assert "std::vector<unsigned char>" not in execute
assert "capacityValues.data()" in execute
assert "++r->restoreIndex;RecordTime(start);return false" in execute
capture = graves.split("void CapturePendingGraveSnapshot()", 1)[1].split(
    "void FinalizeCompletedGraveCaptures()", 1
)[0]
assert "captureOneRawSlot" in capture
assert "CompletedGraveCaptureCapacity=8" in graves
assert "GetObjectName" not in capture
assert "ReadFString" not in capture
assert "MaxStack" not in capture
assert "SavedItem" not in capture
assert "push_back" not in capture
assert "FinalizeCompletedGraveCaptures();" in graves.split(
    "void FlushAdminGravestoneSnapshots()", 1
)[1]
cache_registry = graves.split("bool CacheRegistrySnapshot(", 1)[1].split(
    "bool ResolveUnloadedSnapshot(", 1
)[0]
assert cache_registry.count("++captureCompleted") == 2
assert "pthread_mutex_trylock(&queueMutex)" in graves
assert "pthread_mutex_trylock(&emptyRemovalMutex)" in graves
assert "const auto selected=phase++%4" in storage
assert "Exactly one latency-sensitive consumer" in storage
performance = (root / "src" / "server_performance_store.cpp").read_text()
assert 'ReadNetworkCounters()' in performance
assert 'std::ifstream input("/proc/net/dev")' in performance
assert 'sample.players=latestPlayerNetwork' in (root / "src" / "main.cpp").read_text()

# Slow-path attribution remains fixed-size and non-blocking on the game thread;
# player-name resolution, persistence, and JSON formatting stay on the worker.
assert "Capacity = 512" in trace
assert "std::array<Event, Capacity>" in trace
record_trace = trace.split("inline void Record(", 1)[1].split("inline std::vector<Event> Drain", 1)[0]
assert "microseconds < 100" in record_trace
assert "pthread_mutex_trylock" in record_trace
assert "std::vector" not in record_trace
main = (root / "src" / "main.cpp").read_text()
assert "PerformanceTrace::Drain()" in main
assert "TimingScope::NativeGame" in storage and "TimingScope::NativeGame" in graves
assert 'timingScope=="native-game"' in performance
assert "item.playerName=player.name" in main
assert 'PerformanceTrace::Record("kills","kill event capture"' in kills
assert 'PerformanceTrace::Record("gravestones","death / grave observer"' in graves
assert 'PerformanceTrace::Record("gravestones","grave registration"' in graves
assert 'PerformanceTrace::Record("storage",r.operation.c_str()' in storage

# Native event hooks hand fixed evidence to bounded queues. Attribution,
# persistence and API object creation occur after the hook returns.
kill_hook = kills.split("void HookOnAIKilled", 1)[1].split("}\n\n}", 1)[0]
assert "rawKillQueue" in kill_hook
assert "RecordNpcDeath" not in kill_hook
assert "ResolveCachedPlayer" not in kill_hook
assert "RawResourceEventCapacity=256" in resources
assert "TrackedQuestQueueCapacity=64" in quests

# Resource hooks provide the near-real-time path. The registry reconciliation
# is a bounded, yielded safety fallback rather than a two-second full walk.
assert "sliceDeadline=sliceStarted+std::chrono::microseconds(300)" in resources
assert "inspected<512" in resources
assert "nextResourceReconciliation=wall+std::chrono::seconds(30)" in resources
resolve_time_provider = resources.split("uintptr_t ResolveTimeProvider", 1)[1].split(
    "std::string IdentityPath", 1
)[0]
assert "GetObjectCount" not in resolve_time_provider

# Hook-side logging is a bounded enqueue; file writes happen only when the
# service worker explicitly flushes the queue.
assert "QueueCapacity = 256" in logger
assert "void FlushLogMessages()" in logger
assert "std::fopen" not in logger.split("void LogMessage", 1)[1].split(
    "void FlushLogMessages", 1
)[0]
assert "FlushNativeTeleportCapture();" not in teleport.split("void HookTick", 1)[1].split(
    "bool InitializeAdminTeleport", 1
)[0]

print("game-thread non-interference architecture checks passed")
