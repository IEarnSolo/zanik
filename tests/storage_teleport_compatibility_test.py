#!/usr/bin/env python3
from pathlib import Path


root = Path(__file__).resolve().parents[1]
teleport = (root / "src" / "engine" / "admin_teleport.cpp").read_text()
gravestones = (root / "src" / "engine" / "admin_gravestone.cpp").read_text()
main = (root / "src" / "main.cpp").read_text()
analyzer = (root / "tools" / "compat_analyzer.cpp").read_text()
header = (root / "src" / "engine" / "dom_engine.h").read_text()
web = (root / "src" / "web_page.h").read_text()

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
assert 'const int32_t slot=pending.nextSlot++' in gravestones
assert 'captureOneRawSlot' in gravestones
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
assert 'ResolveAdminInventoryMutationCallables(moduleBase+offsets.moveItem,moduleBase+offsets.removeFromSlot' in gravestones
assert 'sharedInventoryHooks?sharedMove:moduleBase+offsets.moveItem' in gravestones
assert 'sharedInventoryHooks?sharedRemove:moduleBase+offsets.removeFromSlot' in gravestones
assert 'ResolveAdminInventoryMutationCallables' in header
assert 'GraveRegisterQueueCapacity=32' in gravestones
assert 'write-read<GraveRegisterQueueCapacity' in gravestones
assert 'DrainGraveRegisterEvents();if(captureCircuitBroken.load()' in gravestones
assert 'Queued newly registered gravestone capture id=' in gravestones
assert 'GraveUnregisterQueueCapacity=32' in gravestones
assert 'write-read<GraveUnregisterQueueCapacity' in gravestones
assert 'DrainGraveUnregisterEvents();if(!savedSnapshotsDirty.exchange(false))return' in gravestones
assert 'if(!seenIds.insert(id).second)continue;' in gravestones
assert 'if(IsGraveRemoved(id)||IsRemovedActorLocation(location))continue;' in gravestones
assert 'if(IsGraveRemoved(id)||IsRemovedActorLocation(location)){seenIds.insert(id);continue;}' not in gravestones
assert 'std::string reason="Claimed in game"' in gravestones
assert 'reason="Claimed from website"' in gravestones
assert 'reason="Deleted from website"' in gravestones
assert 'if(!filled){if(EmptyGraveConfirmed(id))' in gravestones
assert 'WasRecentlyLive(id)&&PlayerNearGrave(location)' not in gravestones
assert 'removalCaptureCompatibility.load()==1' in gravestones
assert 'std::unordered_set<std::string> verifiedActiveGraves' in gravestones
assert 'const bool newlyVerified=verifiedActiveGraves.insert(id).second;saved=savedGraveSnapshots.count(id)!=0' in gravestones
assert 'verifiedActiveGraves.count(entry.first)' in gravestones
assert 'verifiedActiveGraves.erase(id)' in gravestones
assert 'verifiedActiveGraves.count(entry.first)?"A ":"G "' in gravestones
assert "kind!='G'&&kind!='A'&&kind!='R'" in gravestones
assert "if(kind=='G'||(kind=='R'&&retiredAt<=0)){++discardedLegacy;continue;}" in gravestones
assert 'out<<"T "<<item.slot' in gravestones
assert 'EncodePersistenceId(data)' in gravestones
assert 'DecodePersistenceId(item.persistence,persistence)' in gravestones
assert 'objectName=item.icon.substr(0,item.icon.size()-4)' in gravestones
assert 'g_Engine->GetObjectName(data)==objectName' in gravestones
assert 'item.guid=objectName' in gravestones
assert 'known.second.icon==item.icon' in gravestones
assert 'void BackfillSavedItemPersistence()' in gravestones
assert 'Normalized durable asset keys for ' in gravestones
assert 'nextBackfill=now+std::chrono::seconds(2);BackfillSavedItemPersistence();' in gravestones
assert 'if(!unresolved)savedItemPersistenceBackfillComplete=true' in gravestones
assert "if(kind=='A'){savedGraveSnapshots[id]" in gravestones
assert 'retiredGraveSnapshots[id]=std::move(grave)' in gravestones
assert 'retiredSnapshotMeta[id]={retiredAt,reason}' in gravestones
assert 'retiredGraveSnapshots[id]=snapshot->second' in gravestones
assert 'retiredSnapshotMeta[id]={static_cast<std::int64_t>(std::time(nullptr)),reason}' in gravestones
assert 'retiredSnapshotMeta[id]={0,"active archive"}' in gravestones
assert 'if(!retiredGraveSnapshots.count(id)){retiredGraveSnapshots[id]=snapshot->second' in gravestones
assert 'const bool retiredRestore=r->operation.rfind("restore_",0)==0' in gravestones
assert 'addItemByData(reinterpret_cast<void*>(target)' in gravestones
assert '"restore_all_owner","restore_all_self","restore_item_owner","restore_item_self"' in gravestones
assert 'id="activeGraveTab"' in web
assert 'id="retiredGraveTab"' in web
assert "graveView==='retired'?Boolean(grave.retired):!grave.retired" in web
assert "grave.retired?'restore_all_owner':'claim_all_owner'" in web
assert "grave.retired?'restore_item_self':'claim_item_self'" in web
assert "finally{graveBusy=false}if(updated)await refreshGravestones(true)" in web
assert "activePlayerView==='gravestones')refreshGravestones()" in web
assert 'graveLoading' in web
assert 'operation=="restore_item_owner"||operation=="restore_item_self"' in main
assert 'if(newlyVerified&&saved)savedSnapshotsDirty=true' in gravestones
assert 'ResolveKnownGraveLocation(r->ownerGuid,r->graveId,location)' in gravestones
assert 'return ResolveRegistryGrave(owner,id,location)||ResolveVerifiedSnapshotGrave(owner,id,location)' in gravestones
assert 'if(!verifiedActiveGraves.count(id))return false' in gravestones
assert 'identity->second.owner!=owner' in gravestones
assert 'for(const std::string&level:{std::string("L_World"),cell})' in gravestones
assert 'Native saved gravestone restore id=' in gravestones
assert 'bool ResolveGraveBounded' in gravestones
assert 'constexpr int32_t ObjectInspectionBudget=128' in gravestones
assert 'ResolveGraveBounded(r->ownerGuid,r->graveId,r->graveCursor,grave,scanComplete)' in gravestones
assert 'return ResolveKnownGraveLocation(owner,id,location);' in gravestones
assert 'CaptureLiveGraveSnapshots' not in gravestones
assert 'void FlushAdminGravestoneSnapshots()' in gravestones
assert 'DomEngine::FlushAdminGravestoneSnapshots();' in main
assert 'adminGravestoneCaptureMaximumMicroseconds' in main
assert 'adminGravestoneCaptureCircuitBroken' in main
assert 'adminGravestoneRemovalCaptureCompatibility' in main
assert 'adminGravestoneUnregisterMaximumMicroseconds' in main
assert 'std::string SavedSnapshotsPath(){return RuntimePaths::DataFile("gravestone-snapshots.v1");}' in gravestones
assert 'std::string SeedGraveItemsPath(){return RuntimePaths::WebFile("grave-items.tsv");}' in gravestones
assert 'std::string SavedGraveItemsPath(){return RuntimePaths::DataFile("grave-items.tsv");}' in gravestones
assert 'PersistSavedItemMetaLocked()' in gravestones
assert 'append.open(SeedGraveItemsPath()' not in gravestones
assert 'pthread_once(&savedSnapshotsOnce,&LoadSavedSnapshots)' in gravestones
assert 'fsync(fd)==0' in gravestones
assert 'rename(temporary.c_str(),path.c_str())' in gravestones
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
assert 'SnapshotInventory(grave,refreshed)' not in gravestones
assert 'PrepareCapacityBatch(grave.inventory,*r)' in gravestones
assert 'r->capacityValues.data()' in gravestones
assert '++r->restoreIndex;RecordTime(start);return false' in gravestones
assert 'gravestoneViewGeneration.fetch_add(1);RecordTime(start);return false' in gravestones
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
