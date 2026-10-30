#!/usr/bin/env python3
from pathlib import Path


root = Path(__file__).resolve().parents[1]
teleport = (root / "src" / "engine" / "admin_teleport.cpp").read_text()
gravestones = (root / "src" / "engine" / "admin_gravestone.cpp").read_text()
main = (root / "src" / "main.cpp").read_text()
analyzer = (root / "tools" / "compat_analyzer.cpp").read_text()
header = (root / "src" / "engine" / "dom_engine.h").read_text()

# Storage and gravestone teleport share one exact native transport profile,
# but independently prepare and identity-bind their destinations off-thread.
assert "ResolveAdminStorageActor(targetCharacterGuid,chestId,actor,location)" in teleport
assert "ResolveAdminGravestoneLocation(ownerCharacterGuid,graveId,actor,location)" in teleport
assert "preparedActorIdentity" in teleport
assert "GetObjectIdentity(chestActor)!=request.preparedActorIdentity" in teleport
assert 'request.action="storage"' in teleport
assert 'request.action="gravestone"' in teleport
assert 'request.action="quest"' in teleport
assert "QueueAdminQuestTeleport" in header
assert 'request.action=="storage"||request.action=="gravestone"||request.action=="quest"' in teleport

# Gravestone contents are captured only after the authoritative registry adds
# a grave. Object discovery is sliced across ticks, and durable writes happen
# on the worker thread rather than blocking the game thread.
assert 'void ProcessAdminGravestoneRequests(){ProcessEmptyGraveRemoval();CapturePendingGraveSnapshot();' in gravestones
assert 'constexpr int32_t ObjectInspectionBudget=32' in gravestones
assert 'constexpr int32_t RegistryDiscoveryBudget=32' in gravestones
assert 'started+std::chrono::microseconds(200)' in gravestones
assert 'processed<2' in gravestones
assert 'pthread_mutex_trylock(&savedSnapshotMutex)' in gravestones
assert 'captureCircuitBroken=true;persistenceCompatibility=-2' in gravestones
assert 'persistenceCompatibility=persistencePathHealthy?1:-1' in gravestones
assert 'registry-triggered capture remains available' in gravestones
assert 'InstallUnregisterGravestoneHook' in gravestones
assert 'InstallRegisterGravestoneHook' in gravestones
assert 'AGameplayObjectRegistry::RegisterGravestone(FGravestoneData const&)' in analyzer
assert 'authoritative gravestone register signature changed' in analyzer
assert 'AGameplayObjectRegistry::UnregisterGravestone(FGuid const&)' in analyzer
assert 'authoritative gravestone unregister signature changed' in analyzer
assert 'Authoritative gravestone registration and removal capture armed' in gravestones
assert 'GraveRegisterQueueCapacity=32' in gravestones
assert 'write-read<GraveRegisterQueueCapacity' in gravestones
assert 'DrainGraveRegisterEvents();if(captureCircuitBroken.load()' in gravestones
assert 'Queued newly registered gravestone capture id=' in gravestones
assert 'GraveUnregisterQueueCapacity=32' in gravestones
assert 'write-read<GraveUnregisterQueueCapacity' in gravestones
assert 'DrainGraveUnregisterEvents();if(!savedSnapshotsDirty.exchange(false))return' in gravestones
assert 'if(!filled){seenIds.insert(id);if(EmptyGraveConfirmed(id))' in gravestones
assert 'WasRecentlyLive(id)&&PlayerNearGrave(location)' not in gravestones
assert 'removalCaptureCompatibility.load()==1' in gravestones
assert 'std::unordered_set<std::string> verifiedActiveGraves' in gravestones
assert 'const bool newlyVerified=verifiedActiveGraves.insert(id).second;saved=savedGraveSnapshots.count(id)!=0' in gravestones
assert 'verifiedActiveGraves.count(entry.first)' in gravestones
assert 'verifiedActiveGraves.erase(id)' in gravestones
assert 'verifiedActiveGraves.count(entry.first)?"A ":"G "' in gravestones
assert "const bool authoritative=kind=='A'" in gravestones
assert 'if(authoritative)verifiedActiveGraves.insert(id)' in gravestones
assert 'if(newlyVerified&&saved)savedSnapshotsDirty=true' in gravestones
assert 'ResolveKnownGraveLocation(r->ownerGuid,r->graveId,location)' in gravestones
assert 'return ResolveRegistryGrave(owner,id,location)||ResolveVerifiedSnapshotGrave(owner,id,location)' in gravestones
assert 'if(!verifiedActiveGraves.count(id))return false' in gravestones
assert 'identity->second.owner!=owner' in gravestones
assert 'for(const std::string&level:{std::string("L_World"),cell})' in gravestones
assert 'Native saved gravestone restore id=' in gravestones
assert 'bool ResolveGraveBounded' in gravestones
assert 'constexpr int32_t ObjectInspectionBudget=128' in gravestones
assert 'ResolveGraveBounded(r->ownerGuid,r->graveId,r->graveCursor,grave)' in gravestones
assert 'return ResolveKnownGraveLocation(owner,id,location);' in gravestones
assert 'CaptureLiveGraveSnapshots' not in gravestones
assert 'void FlushAdminGravestoneSnapshots()' in gravestones
assert 'DomEngine::FlushAdminGravestoneSnapshots();' in main
assert 'adminGravestoneCaptureMaximumMicroseconds' in main
assert 'adminGravestoneCaptureCircuitBroken' in main
assert 'adminGravestoneRemovalCaptureCompatibility' in main
assert 'adminGravestoneUnregisterMaximumMicroseconds' in main
assert 'constexpr const char* SavedSnapshotsPath="/home/container/zanik-data/gravestone-snapshots.v1"' in gravestones
assert 'pthread_once(&savedSnapshotsOnce,&LoadSavedSnapshots)' in gravestones
assert 'fsync(fd)==0' in gravestones
assert 'rename(temporary.c_str(),SavedSnapshotsPath)' in gravestones
assert 'CacheLiveSnapshot(owner,location,std::move(liveSnapshot))' in gravestones
assert 'void PreserveSnapshotMetadata(SavedGrave&fresh,const SavedGrave&known)' in gravestones
assert 'if(cached!=savedGraveSnapshots.end())PreserveSnapshotMetadata(saved,cached->second)' in gravestones
assert 'if(!prior.name.empty())item.name=prior.name' in gravestones
assert 'if(!prior.icon.empty())item.icon=prior.icon' in gravestones
assert 'if(prior.maximum>1)item.maximum=prior.maximum' in gravestones
assert 'if(!prior.guid.empty())item.guid=prior.guid' in gravestones
assert 'bool DecodePersistenceId' in gravestones
assert 'void ResolveLoadedItemMetadata(SavedGrave&grave)' in gravestones
assert 'g_Engine->FindPropertyOffset(data,"PersistenceID")' in gravestones
assert 'g_Engine->IsA(data,"ItemData")' in gravestones
assert 'Recovered gravestone item metadata persistence=' in gravestones
assert 'const bool changed=cached==savedGraveSnapshots.end()||!SameSnapshot(cached->second,saved)' in gravestones
assert 'if(changed)savedSnapshotsDirty=true' in gravestones
assert 'std::unordered_set<std::string> liveInventorySnapshots' in gravestones
assert 'if(liveInventorySnapshots.count(id)&&cached!=savedGraveSnapshots.end())' in gravestones
assert 'bool ResolveUnloadedSnapshot' in gravestones
assert 'bool SnapshotInventory(const Grave&grave,SavedGrave&snapshot)' in gravestones
assert 'else CacheLiveSnapshot(r->ownerGuid,grave.location,std::move(refreshed))' in gravestones
assert 'if(r->operation!="delete_grave")removeGrave(reinterpret_cast<void*>(grave.actor))' in gravestones
assert 'void ProcessEmptyGraveRemoval()' in gravestones
assert 'ProcessAdminGravestoneRequests(){ProcessEmptyGraveRemoval();CapturePendingGraveSnapshot();' in gravestones
assert 'QueueEmptyGraveRemoval(actor,owner,id,location);MarkGraveRemoved(id,&location)' in gravestones
assert 'now-found->second>=std::chrono::milliseconds(500)' in gravestones
assert 'if(savedValid&&saved.items.empty())continue' in gravestones
assert "ResolveQuestTeleportDestination" in main
assert 'active->status!="in_progress"' in main
assert 'active->objective!=objective' in main
assert '"/api/admin/quest/teleport"' in main

# The game thread performs one ground trace and one authoritative native
# scheduling call, rather than broad object scans or multi-candidate loops.
storage_branch = teleport[teleport.index('if(request.action=="storage"'):teleport.index('if(!request.action.empty())')]
assert storage_branch.count("TraceGround(") == 1
assert storage_branch.count("bpTriggerDelayedTeleport(") == 1
assert "ReadNearbyBuildingBounds(" not in storage_branch
assert "ReadAdminStorageLocations(" not in storage_branch
assert "GetObjectCount(" not in storage_branch

# One slow indivisible native call disables only storage/gravestone transport.
assert "nativeMicros>=1000" in storage_branch
assert "storageTeleportCircuitBroken=true" in storage_branch
assert "storageTransportCompatibility" in header
assert "diagnostics.storageTransportCompatibility!=1" in main

print("storage and gravestone teleport compatibility checks passed")
