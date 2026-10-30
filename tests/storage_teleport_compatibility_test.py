#!/usr/bin/env python3
from pathlib import Path


root = Path(__file__).resolve().parents[1]
teleport = (root / "src" / "engine" / "admin_teleport.cpp").read_text()
gravestones = (root / "src" / "engine" / "admin_gravestone.cpp").read_text()
main = (root / "src" / "main.cpp").read_text()
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

# Gravestone contents are captured by the bounded game-thread dispatcher, not
# only by an admin page request. The atomic snapshot survives World Partition
# unloading and a server restart while the registry remains authoritative.
assert 'void ProcessAdminGravestoneRequests(){CaptureLiveGraveSnapshots();' in gravestones
assert 'constexpr const char* SavedSnapshotsPath="/home/container/zanik-data/gravestone-snapshots.v1"' in gravestones
assert 'pthread_once(&savedSnapshotsOnce,&LoadSavedSnapshots)' in gravestones
assert 'fsync(fd)==0' in gravestones
assert 'rename(temporary.c_str(),SavedSnapshotsPath)' in gravestones
assert 'CacheLiveSnapshot(owner,location,std::move(liveSnapshot))' in gravestones
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
