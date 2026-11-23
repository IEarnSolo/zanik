#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "dom_engine.h"
#include "storage_compatibility_policy.h"
#include "../performance_trace.h"
#include "../storage_autofill_store.h"
#include "../player_inventory_snapshot_store.h"
#include "../utils/logger.h"
#include "../utils/runtime_paths.h"
#include "../utils/memory.h"
#include "../utils/unicode.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <atomic>
#include <chrono>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <fstream>
#include <link.h>
#include <memory>
#include <mutex>
#include <pthread.h>
#include <regex>
#include <shared_mutex>
#include <sstream>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unordered_map>
#include <unordered_set>
#include <unistd.h>

namespace DomEngine {
namespace {
constexpr const char* LegacySha=StorageCompatibilityPolicy::LegacyCandidateSha.data();
constexpr const char* PreviousSha=StorageCompatibilityPolicy::PreviousReviewedSha.data();
constexpr const char* CurrentSha=StorageCompatibilityPolicy::CurrentReviewedSha.data();
constexpr const char* LatestSha=StorageCompatibilityPolicy::LatestReviewedSha.data();
constexpr const char* CandidateSha=StorageCompatibilityPolicy::CandidateSha.data();
bool CandidateEnabled(){const char*candidate=std::getenv("ZANIK_STORAGE_PERFORMANCE_CANDIDATE");struct stat marker{};const std::string markerPath=RuntimePaths::DataFile("storage-performance-candidate");return (candidate&&std::string(candidate)=="1")||stat(markerPath.c_str(),&marker)==0;}
bool ExpectedSha(const std::string& hash){return StorageCompatibilityPolicy::IsProductionReviewed(hash)||(CandidateEnabled()&&StorageCompatibilityPolicy::IsCandidateOnly(hash));}
bool ExpectedDispatcherSha(const std::string& hash){return ExpectedSha(hash)||hash=="49512c0ee933c3ee4dda5fb5c2f0a25e66d7f26e34795c45b91b5c1d12b61b0c"||hash==LegacySha||hash==PreviousSha||hash==CurrentSha;}
constexpr std::size_t QueueCapacity=32;
constexpr uintptr_t AddItemByDataOffset=0x0A6118E0;
constexpr uintptr_t CanAddItemByDataOffset=0x0A612180;
constexpr uintptr_t MoveItemOffset=0x0A6139B0;
constexpr uintptr_t RemoveFromSlotOffset=0x0A615490;
constexpr uintptr_t CanAddItemsByDataOffset=0x0A612E40;
constexpr uintptr_t AddItemsByDataOffset=0x0A61DCF0;
constexpr uintptr_t BuildingScaledRequirementsOffset=0x0A3B7FC0;
constexpr uintptr_t FMemoryFreeOffset=0x04A60480;
constexpr uintptr_t EngineTickOffset=0x07A96BA0;
constexpr uintptr_t RenameableGetCustomNameOffset=0x0A0BBC60;
constexpr uintptr_t RenameableGetSetterIdOffset=0x0A0BBED0;
constexpr uintptr_t RenameableSetCustomNameOffset=0x0A0BC2B0;
constexpr uintptr_t FTextFromStringOffset=0x04AA6B60;
constexpr uintptr_t UniqueNetIdDestructorOffset=0x079A74B0;
constexpr uintptr_t FTextDestructorOffset=0x049F69F0;
constexpr uintptr_t SetStackSizeOffset=0x0A636850;
constexpr size_t EngineTickHookBytes=16;
constexpr size_t MoveItemHookBytes=12;
constexpr size_t RemoveFromSlotHookBytes=17;
constexpr size_t BuildingConsumptionHookBytes=17;
struct StorageOffsets {
    uintptr_t addItemByData=AddItemByDataOffset,canAddItemByData=CanAddItemByDataOffset,moveItem=MoveItemOffset,removeFromSlot=RemoveFromSlotOffset;
    uintptr_t canAddItemsByData=CanAddItemsByDataOffset,addItemsByData=AddItemsByDataOffset,buildingRequirements=BuildingScaledRequirementsOffset,memoryFree=FMemoryFreeOffset,engineTick=EngineTickOffset;
    uintptr_t renameGet=RenameableGetCustomNameOffset,renameSetter=RenameableGetSetterIdOffset,renameSet=RenameableSetCustomNameOffset,textFromString=FTextFromStringOffset,uniqueIdDestructor=UniqueNetIdDestructorOffset,textDestructor=FTextDestructorOffset;
    uintptr_t buildingEndPlay=0x0A37C2D0;
    uintptr_t setStackSize=SetStackSizeOffset;
    uintptr_t setOwningCharacterGuid=0;
    uintptr_t setIsLinkable=0;
    uintptr_t renameableStaticClass=0;
    uintptr_t doesImplementInterface=0;
    uintptr_t pagedSlotValid=0;
    uintptr_t pagedSlotOffset=0;
    uintptr_t pagedSlotCount=0;
    uintptr_t loadoutSlotValid=0;
    uintptr_t canAddItem=0;
    uintptr_t addItem=0;
    uintptr_t copyItem=0;
    uintptr_t getItemPowerLevel=0;
    uintptr_t canHaveMasterworkUpgrade=0;
    uintptr_t applyMasterworkUpgrade=0;
    uintptr_t onRepMasterworkUpgrades=0;
    uintptr_t addLoadoutItemToSlot=0;
    uintptr_t removeLoadoutItem=0;
};
StorageOffsets OffsetsFor(const std::string& hash){
    StorageOffsets value;
    if(hash==LegacySha)value={0x0A653420,0x0A653CC0,0x0A6554F0,0x0A656FD0,0x0A654980,0x0A65F7E0,0x0A3DAA00,0x04A89480,0x07ABFE00,0x0A0EF130,0x0A0EF3A0,0x0A0EF780,0x04ACFB60,0x079D0710,0x04A1F9F0,0x0A3B5970,0x0A678C00};
    else if(hash==PreviousSha)value={0x0A653CD0,0x0A654570,0x0A655DA0,0x0A657880,0x0A655230,0x0A660090,0x0A3DA9D0,0x04A89480,0x07ABFE00,0x0A0EF090,0x0A0EF300,0x0A0EF6E0,0x04ACFB60,0x079D0710,0x04A1F9F0,0x0A3B58D0,0x0A6794B0};
    else if(hash==CurrentSha)value={0x0A669830,0x0A66A3C0,0x0A66BF00,0x0A66EF90,0x0A66B390,0x0A677D60,0x0A3E9980,0x04A94480,0x07ACAF90,0x0A0FDA20,0x0A0FDC90,0x0A0FE070,0x04ADAB60,0x079DB8A0,0x04A2A9F0,0x0A3C4F00,0x0A68B9D0,0x0AAD44E0,0x0A68FAB0,0x0A0FDB60,0x080A8980,0x0A695230,0x0A6960D0,0x0A696AC0,0x0A6B6410};
    else if(hash==LatestSha)value={0x0A6698D0,0x0A66A460,0x0A66BFA0,0x0A66F030,0x0A66B430,0x0A677E00,0x0A3E9A20,0x04A94480,0x07ACAF90,0x0A0FDAC0,0x0A0FDD30,0x0A0FE110,0x04ADAB60,0x079DB8A0,0x04A2A9F0,0x0A3C4FA0,0x0A68BA70,0x0AAD4450,0x0A68FB50,0x0A0FDC00,0x080A8980,0x0A6952D0,0x0A696170,0x0A696B60,0x0A6B64B0};
    else if(hash==CandidateSha)value={0x0A669E70,0x0A66AA00,0x0A66C540,0x0A66F5D0,0x0A66B9D0,0x0A6783A0,0x0A3E99B0,0x04A94480,0x07ACAF90,0x0A0FDA60,0x0A0FDCD0,0x0A0FE0B0,0x04ADAB60,0x079DB8A0,0x04A2A9F0,0x0A3C4F30,0x0A68C010,0x0AAD4730,0x0A6900F0,0x0A0FDBA0,0x080A8980,0x0A695870,0x0A696710,0x0A697100,0x0A6B6A50};
    if(hash==CurrentSha){value.canAddItem=0x0A66A2C0;value.addItem=0x0A675B00;value.copyItem=0x0A68C180;value.getItemPowerLevel=0x0A68BF00;value.canHaveMasterworkUpgrade=0x0A68D9C0;value.applyMasterworkUpgrade=0x0A68D760;value.onRepMasterworkUpgrades=0x0A68DD50;value.addLoadoutItemToSlot=0x0A6B5F30;value.removeLoadoutItem=0x0A6B61A0;}
    else if(hash==LatestSha){value.canAddItem=0x0A66A360;value.addItem=0x0A675BA0;value.copyItem=0x0A68C220;value.getItemPowerLevel=0x0A68BFA0;value.canHaveMasterworkUpgrade=0x0A68DA60;value.applyMasterworkUpgrade=0x0A68D800;value.onRepMasterworkUpgrades=0x0A68DDF0;value.addLoadoutItemToSlot=0x0A6B5FD0;value.removeLoadoutItem=0x0A6B6240;}
    else if(hash==CandidateSha){value.canAddItem=0x0A66A900;value.addItem=0x0A676140;value.copyItem=0x0A68C7C0;value.getItemPowerLevel=0x0A68C540;value.canHaveMasterworkUpgrade=0x0A68E000;value.applyMasterworkUpgrade=0x0A68DDA0;value.onRepMasterworkUpgrades=0x0A68E390;value.addLoadoutItemToSlot=0x0A6B6570;value.removeLoadoutItem=0x0A6B67E0;}
    return value;
}
std::atomic<uintptr_t> cachedDisplayTextOffset{0};
uintptr_t DisplayTextOffset(){auto value=cachedDisplayTextOffset.load();if(value)return value;const std::string hash=FileSha256("/proc/self/exe");value=(hash==CurrentSha||hash==LatestSha||hash==CandidateSha)?0x04AD5A80:((hash==LegacySha||hash==PreviousSha)?0x04ACAA80:0x04AA1A80);cachedDisplayTextOffset.store(value);return value;}

using AddItemByDataFn=bool(*)(void*,void*,int,float,const void*);
using CanAddItemByDataFn=bool(*)(void*,void*,int);
using MoveItemFn=bool(*)(void*,int,void*,int,void*,int);
using RemoveFromSlotFn=bool(*)(void*,int,int,void*);
using CanAddItemsByDataFn=bool(*)(void*,const FUEArray*);
using AddItemsByDataFn=void(*)(void*,const FUEArray*,const FUEArray*,float,const void*);
using BuildingScaledRequirementsFn=FUEArray*(*)(FUEArray*,void*,void*);
using FMemoryFreeFn=void(*)(void*);
using GetMaxStackSizeFn=int(*)(void*);
using EngineTickFn=void(*)(void*,float,int,void*);
using ExecuteGetCustomNameFn=void*(*)(void*,void*);
using ExecuteGetSetterIdFn=void*(*)(void*,void*);
using ExecuteSetCustomNameFn=void(*)(void*,const void*,const void*);
using ServerSetObjectCustomNameFn=void(*)(void*,void*,const void*,const void*);
using FTextFromStringFn=void*(*)(void*,const FString*);
using RenameableStaticClassFn=void*(*)();
using DoesImplementInterfaceFn=bool(*)(const void*,const void*);
using UniqueNetIdDestructorFn=void(*)(void*);
using FTextDestructorFn=void(*)(void*);
using SetStackSizeFn=void(*)(void*,int);
using SetOwningCharacterGuidFn=void(*)(void*,const void*);
using SetIsLinkableFn=void(*)(void*,bool);
using IsValidSlotForItemFn=bool(*)(const void*,const void*,int,void*);
using GetPagedSlotOffsetFn=int(*)(const void*,std::uint8_t,bool);
using GetPagedSlotCountFn=int(*)(const void*,std::uint8_t);
using CanAddItemFn=bool(*)(const void*,void*);
using AddItemFn=bool(*)(void*,void*);
using CopyItemFn=void*(*)(const void*,void*);
using GetItemPowerLevelFn=float(*)(const void*);
using CanHaveMasterworkUpgradeFn=bool(*)(const void*);
using ApplyMasterworkUpgradeFn=void(*)(void*);
using OnRepMasterworkUpgradesFn=void(*)(void*);
using AddLoadoutItemToSlotFn=bool(*)(void*,void*,int);
using RemoveLoadoutItemFn=bool(*)(void*,void*);
AddItemByDataFn addItemByData=nullptr;
CanAddItemByDataFn canAddItemByData=nullptr;
MoveItemFn moveItem=nullptr;
RemoveFromSlotFn removeFromSlot=nullptr;
MoveItemFn originalObservedMoveItem=nullptr;
RemoveFromSlotFn originalObservedRemoveFromSlot=nullptr;
CanAddItemsByDataFn canAddItemsByData=nullptr;
AddItemsByDataFn addItemsByData=nullptr;
BuildingScaledRequirementsFn buildingScaledRequirements=nullptr;
FMemoryFreeFn memoryFree=nullptr;
EngineTickFn originalEngineTick=nullptr;
ExecuteGetCustomNameFn executeGetCustomName=nullptr;
ExecuteGetSetterIdFn executeGetSetterId=nullptr;
ExecuteSetCustomNameFn executeSetCustomName=nullptr;
ServerSetObjectCustomNameFn serverSetObjectCustomName=nullptr;
FTextFromStringFn fTextFromString=nullptr;
RenameableStaticClassFn renameableStaticClass=nullptr;
DoesImplementInterfaceFn doesImplementInterface=nullptr;
UniqueNetIdDestructorFn destroyUniqueNetId=nullptr;
FTextDestructorFn destroyFText=nullptr;
SetStackSizeFn setStackSize=nullptr;
SetOwningCharacterGuidFn setOwningCharacterGuid=nullptr;
SetIsLinkableFn setIsLinkable=nullptr;
IsValidSlotForItemFn pagedSlotValid=nullptr,loadoutSlotValid=nullptr;
GetPagedSlotOffsetFn pagedSlotOffset=nullptr;
GetPagedSlotCountFn pagedSlotCount=nullptr;
CanAddItemFn canAddItem=nullptr;
AddItemFn addItem=nullptr;
CopyItemFn copyItem=nullptr;
GetItemPowerLevelFn getItemPowerLevel=nullptr;
CanHaveMasterworkUpgradeFn canHaveMasterworkUpgrade=nullptr;
ApplyMasterworkUpgradeFn applyMasterworkUpgrade=nullptr;
OnRepMasterworkUpgradesFn onRepMasterworkUpgrades=nullptr;
AddLoadoutItemToSlotFn addLoadoutItemToSlot=nullptr;
RemoveLoadoutItemFn removeLoadoutItem=nullptr;

enum class JobState : std::uint8_t { Queued,Running,Persisting,RollingBack,Completed,Failed };
enum class ActionKind : std::uint8_t { Add,AddMasterwork,CopyItem,Remove,Move,EquipHeld,UnequipHeld,RelocateHeld,SetStack,RenameConstruct,RenameSetterIdentity,RenameSet,RenameVerify,RenameDestroyIdentity,RenameDestroyText,LinkableSet,LinkableVerify,OwnershipSet,OwnershipVerify };
enum class OperationFamily : std::uint8_t { SingleItem,SlotMovement,BatchAddition,BatchMovement,Deletion,Rename,Ownership,Rollback,Autofill,Equipment,Count };
struct NativeAction {
    ActionKind kind=ActionKind::Add;
    OperationFamily family=OperationFamily::SingleItem;
    uintptr_t source=0,destination=0,item=0,itemData=0,actor=0,controller=0;
    std::uint64_t sourceIdentity=0,destinationIdentity=0,itemIdentity=0,itemDataIdentity=0,actorIdentity=0,controllerIdentity=0;
    int sourceSlot=-1,destinationSlot=-1,loadoutSourceSlot=-1,quantity=0,expectedQuantity=0,desiredPowerLevel=0,matchingItemCount=0,matchingQuantity=0;
    std::array<std::uint32_t,4> ownerGuid{};
    bool linkable=false;
};
struct Request {
    std::string id,operation,session,conflictKey,successMessage,adminId,targetId,targetGuid;
    std::vector<NativeAction> actions,rollback;
    std::string autofillNormalizeKey;
    std::string oldOwnerGuid,newOwnerGuid,oldChestId,newChestId;
    std::vector<StorageAutofillItem> autofillNormalize;
    std::atomic<JobState> state{JobState::Queued};
    std::atomic<std::size_t> actionIndex{0};
    std::atomic<std::size_t> rollbackIndex{0};
    std::atomic<int> resultCode{0};
    std::atomic<std::uint64_t> updatedUnix{0};
    std::atomic<bool> partialDelete{false};
    std::atomic<bool> finishAsFailure{false};
    std::atomic<bool> cancelled{false};
    std::uint64_t autofillObservedAtMicros=0;
    std::uint64_t autofillDrainedAtMicros=0;
    std::uint64_t autofillReconciledAtMicros=0;
    std::uint64_t autofillDispatchedAtMicros=0;
    bool autofillCompletionRecorded=false;
    bool autofillReplicationWakeRecorded=false;
    bool renameTextConstructed=false;
    bool renameSetterConstructed=false;
    bool transferWasLinked=false;
    bool requiresTargetOnline=false;
    uintptr_t expectedWorld=0;
    std::uint64_t expectedWorldIdentity=0;
    alignas(16) unsigned char renameText[16]{};
    alignas(16) unsigned char renameSetter[48]{};
    std::array<char,257> renameValue{};
    std::array<char16_t,65> renameUtf16{};
    std::size_t renameUtf16Units=0;
};
std::array<Request*,QueueCapacity> requestRing{};
std::atomic<std::size_t> requestHead{0},requestTail{0};
Request* activeRequest=nullptr;
std::mutex jobsMutex;
std::unordered_map<std::string,std::shared_ptr<Request>> jobs;
std::atomic<int> compatibility{0};
std::atomic<int> productionCompatibility{0};
std::atomic<bool> candidateOverride{false};
std::atomic<int> dispatcherCompatibility{0},batchCompatibility{0},renameCompatibility{0},ownershipCompatibility{0},removalTrackingCompatibility{0},catalogCompatibility{0},unlockCatalogCompatibility{0},planCatalogCompatibility{0};
std::atomic<int> nativeMutationCaptureCompatibility{0};
std::atomic<int> playerInventoryPagesCompatibility{0},equipmentSlotCompatibility{0},heldItemEquipmentCompatibility{0},personalInventoryCompatibility{0};
std::atomic<int> itemInstanceCompatibility{0};
std::atomic<uintptr_t> observedMoveEntry{0},observedRemoveEntry{0};
std::atomic<int> buildingTaxonomyCompatibility{0},buildingRequirementsCompatibility{0},buildingIconsCompatibility{0},craftingRecipeCompatibility{0};
std::atomic<std::uint64_t> resolvedBuildingPieces{0},unavailableBuildingPieces{0};
std::atomic<std::uint64_t> queued{0},accepted{0},rejected{0},stale{0},maximumMicros{0};
std::atomic<std::uint64_t> tickCalls{0},maximumTickMicros{0},slowTickCalls{0};
std::atomic<std::uint64_t> performanceFrames{0},performanceMaximumFrameMicros{0},performanceMaximumZanikMicros{0},performanceHitches{0};
std::atomic<std::uint64_t> maximumRecipeTickMicros{0},maximumCustomNameTickMicros{0};
std::atomic<std::uint64_t> maximumStorageTickMicros{0},maximumGravestoneTickMicros{0};
std::atomic<unsigned> pendingCount{0};
std::atomic<std::uint64_t> snapshotGenerations{0},snapshotHits{0},snapshotMisses{0},snapshotInvalidations{0},maximumSnapshotMicros{0};
std::atomic<std::uint64_t> queueDrops{0},runningJobs{0},completedJobs{0},failedJobs{0},staleIdentities{0},worldChanges{0},rollbackAttempts{0},rollbackFailures{0};
std::atomic<std::uint64_t> ownershipTransfers{0},ownershipFailures{0},ownershipSnapshotMigrations{0},ownershipLinkedMigrations{0};
std::atomic<std::uint64_t> nativeMutationCalls{0},nativeMutationAttributed{0},nativeMutationUnattributed{0},nativeMutationDrops{0},nativeMutationMaximumMicros{0},nativeMutationCaptureMaximumMicros{0};
std::atomic<int> autofillRpcCaptureCompatibility{0};
std::atomic<int> autofillBuildingCaptureCompatibility{0};
std::atomic<int> autofillNormalizationCompatibility{0};
std::atomic<int> autofillReplicationWakeCompatibility{0};
std::atomic<uintptr_t> autofillFlushNetDormancyFunction{0},autofillForceNetUpdateFunction{0};
std::atomic<std::uint64_t> autofillReplicationWakeCalls{0},autofillReplicationWakeFailures{0},autofillReplicationWakeMaximumMicros{0};
std::atomic<uintptr_t> autofillMoveRpc{0},autofillSplitPutRpc{0},autofillSplitRpc{0};
std::atomic<uintptr_t> autofillCompleteBuildingRpc{0},autofillPartialBuildingRpc{0};
std::atomic<uintptr_t> autofillCompleteBuildingNativeTarget{0},autofillPartialBuildingNativeTarget{0};
std::atomic<std::uint64_t> autofillRpcCaptureCalls{0},autofillRpcFullStackCalls{0},autofillRpcSplitStackCalls{0},autofillRpcCaptureDrops{0},autofillRpcCaptureMaximumMicros{0};
std::atomic<std::uint64_t> autofillBuildingCaptureCalls{0},autofillBuildingCaptureInventories{0},autofillBuildingCaptureDrops{0},autofillBuildingCaptureMaximumMicros{0};
std::atomic<std::uint64_t> autofillLatencySamples{0};
std::atomic<std::uint64_t> autofillCaptureToDrainLastMicros{0},autofillCaptureToDrainMaximumMicros{0};
std::atomic<std::uint64_t> autofillDrainToReconcileLastMicros{0},autofillDrainToReconcileMaximumMicros{0};
std::atomic<std::uint64_t> autofillReconcileToDispatchLastMicros{0},autofillReconcileToDispatchMaximumMicros{0};
std::atomic<std::uint64_t> autofillDispatchToRefillLastMicros{0},autofillDispatchToRefillMaximumMicros{0};
std::atomic<std::uint64_t> autofillCaptureToRefillLastMicros{0},autofillCaptureToRefillMaximumMicros{0};
std::size_t autofillMoveSourceOffset=0,autofillMoveDestinationOffset=16;
std::size_t autofillSplitPutSourceOffset=0,autofillSplitPutDestinationOffset=16;
std::size_t autofillSplitSourceOffset=0;
std::size_t autofillCompleteBuildingInventoriesOffset=0,autofillPartialBuildingInventoriesOffset=0;
std::unique_ptr<StorageAutofillStore> autofillStore;
std::unique_ptr<PlayerInventorySnapshotStore> playerInventorySnapshotStore;
std::atomic<int> playerInventorySnapshotPersistenceCompatibility{0};
std::mutex playerInventorySnapshotOnlineMutex;
std::unordered_set<std::string> playerInventorySnapshotOnline;
std::atomic<int> autofillPersistenceCompatibility{0};
std::atomic<std::uint64_t> autofillChecks{0},autofillQueued{0},autofillItemsAdded{0},autofillNormalizations{0},autofillProtectedSkips{0},autofillFailures{0};
std::atomic<std::uint64_t> autofillNormalizationFailures{0};
std::atomic<std::uint64_t> autofillMutationSignals{0},autofillPriorityChecks{0},autofillPriorityMatches{0},autofillPriorityDrops{0},autofillTargetedRefreshes{0},autofillTargetedRefreshFallbacks{0},autofillTargetedRefreshMaximumMicros{0};
std::atomic<std::uint64_t> autofillContinuationRequeues{0},autofillContinuationWaits{0},autofillContinuationSettled{0},autofillContinuationAborted{0};
std::atomic<int> autofillDestructionCompatibility{0};
std::atomic<std::uint64_t> autofillDestructionSignals{0},autofillDestructionDrops{0},autofillDestroyedSnapshots{0},autofillDestructionFailures{0},autofillDestructionMaximumMicros{0};
std::size_t autofillCursor=0;
struct AutofillMutationTrigger { uintptr_t inventory=0;std::uint64_t identity=0,observedAtMicros=0,drainedAtMicros=0;std::string ownerGuid,chestId,continuationRequestId; };
constexpr std::size_t AutofillMutationTriggerCapacity=128;
std::deque<AutofillMutationTrigger> autofillMutationTriggers;
struct RecentExactAutofillMutation { uintptr_t inventory=0;std::uint64_t identity=0,observedAtMicros=0; };
constexpr std::uint64_t AutofillRpcCorrelationMicroseconds=250000;
constexpr std::uint64_t AutofillExactRetentionMicroseconds=1000000;
std::deque<RecentExactAutofillMutation> recentExactAutofillMutations;
std::array<std::atomic<std::uint64_t>,size_t(OperationFamily::Count)> familyMaximum{};
std::array<std::atomic<bool>,size_t(OperationFamily::Count)> familyCircuitBroken{};
constexpr std::uint64_t StorageGameThreadLimitMicroseconds=1000;
constexpr std::uint64_t AutofillGameThreadLimitMicroseconds=10000;
constexpr std::uint64_t EquipmentGameThreadLimitMicroseconds=10000;
std::uint64_t GameThreadLimit(OperationFamily family){return family==OperationFamily::Autofill?AutofillGameThreadLimitMicroseconds:family==OperationFamily::Equipment?EquipmentGameThreadLimitMicroseconds:StorageGameThreadLimitMicroseconds;}
std::atomic<std::uint64_t> storageGeneration{1};
std::atomic<bool> earlyHookInstalled{false};
enum class ObservedMutationKind : std::uint8_t { Move,Remove,RpcMove,RpcSplit,BuildingConsume,BuildingDestroyed };
struct ObservedMutationEvent {
    ObservedMutationKind kind=ObservedMutationKind::Move;
    uintptr_t source=0,destination=0,controller=0;
    std::uint64_t nativeMicros=0,captureMicros=0;
    bool succeeded=false;
    bool timing=true;
    std::uint64_t observedAtMicros=0;
    std::uint64_t identity=0;
    int sourceSlot=-1,destinationSlot=-1;
};
struct ObservedMutationSlot {std::atomic<std::uint8_t> state{0};ObservedMutationEvent event;};
constexpr std::size_t ObservedMutationCapacity=128;
std::array<ObservedMutationSlot,ObservedMutationCapacity> observedMutationRing{};
std::atomic<std::uint64_t> observedMutationHead{0};
std::shared_ptr<const std::vector<PlayerInfo>> observedMutationPlayers=std::make_shared<const std::vector<PlayerInfo>>();
thread_local bool suppressObservedMutation=false;
constexpr size_t BuildingEndPlayHookBytes=16;
constexpr uintptr_t BuildingEndPlayOffset=0x0A37C2D0;
using BuildingEndPlayFn=void(*)(void*,int);
BuildingEndPlayFn originalBuildingEndPlay=nullptr;
pthread_mutex_t removedBuildingsMutex=PTHREAD_MUTEX_INITIALIZER;
std::unordered_set<std::uint64_t> removedBuildingIdentities;
std::deque<std::uint64_t> removedBuildingOrder;
std::atomic<std::uint64_t> buildingRemovalEvents{0};
struct CachedName {std::string value;std::chrono::steady_clock::time_point refreshed;};
pthread_mutex_t customNameMutex=PTHREAD_MUTEX_INITIALIZER;
std::unordered_map<std::uint64_t,CachedName> customNames;
std::unordered_map<std::uint64_t,uintptr_t> pendingCustomNames;

std::string Escape(const std::string&s){std::string o;for(unsigned char c:s){if(c=='"'||c=='\\'){o+='\\';o+=char(c);}else if(c>=32)o+=char(c);}return o;}
const std::unordered_map<std::string,std::string>& BuildingActorNames(){
    static const auto names=[](){std::unordered_map<std::string,std::string> values;std::ifstream input(RuntimePaths::WebFile("buildings/metadata.tsv"));std::string line;while(std::getline(input,line)){if(line.empty()||line[0]=='#')continue;auto a=line.find('\t'),b=a==std::string::npos?std::string::npos:line.find('\t',a+1),c=b==std::string::npos?std::string::npos:line.find('\t',b+1);if(a==std::string::npos||b==std::string::npos||c==std::string::npos)continue;std::string name=line.substr(a+1,b-a-1),actor=line.substr(c+1);if(!actor.empty()&&actor.back()=='\r')actor.pop_back();if(!name.empty()&&!actor.empty())values[std::move(actor)]=std::move(name);}return values;}();return names;
}
std::string CleanType(std::string value){auto exact=BuildingActorNames().find(value);if(exact!=BuildingActorNames().end())return exact->second;const std::string p="BP_BaseBuilding_";if(value.rfind(p,0)==0)value.erase(0,p.size());if(value.size()>2&&value.compare(value.size()-2,2,"_C")==0)value.resize(value.size()-2);std::string readable;for(size_t i=0;i<value.size();++i){char c=value[i];if(c=='_'){if(!readable.empty()&&readable.back()!=' ')readable+=' ';continue;}if(i&&std::isupper(static_cast<unsigned char>(c))&&std::islower(static_cast<unsigned char>(value[i-1]))&&readable.back()!=' ')readable+=' ';readable+=c;}return readable.empty()?"Storage":readable;}
std::string HexHash(std::uint64_t hash){char id[17]{};std::snprintf(id,sizeof(id),"%016llx",static_cast<unsigned long long>(hash));return id;}
std::uint64_t HashBytes(std::uint64_t hash,const void*p,size_t n){for(size_t i=0;i<n;++i){hash^=reinterpret_cast<const unsigned char*>(p)[i];hash*=1099511628211ULL;}return hash;}
std::string ChestId(uintptr_t actor,const std::string&type,const std::string&guid){
    std::uint64_t h=1469598103934665603ULL;h=HashBytes(h,type.data(),type.size());h=HashBytes(h,guid.data(),guid.size());
    uintptr_t root=Mem::ReadPtr(actor+Offsets::Actor_RootComponent);FVector3d location{};
    if(root&&Mem::Readable(reinterpret_cast<void*>(root+Offsets::SceneComp_RelativeLocation),sizeof(location)))std::memcpy(&location,reinterpret_cast<void*>(root+Offsets::SceneComp_RelativeLocation),sizeof(location));
    h=HashBytes(h,&location,sizeof(location));return HexHash(h);
}
std::string ItemId(const std::string&chest,int slot,const std::string&key,int quantity){
    std::uint64_t h=1469598103934665603ULL;h=HashBytes(h,chest.data(),chest.size());h=HashBytes(h,&slot,sizeof(slot));h=HashBytes(h,key.data(),key.size());h=HashBytes(h,&quantity,sizeof(quantity));return HexHash(h);
}
std::string CatalogId(const std::string&key){std::uint64_t h=1469598103934665603ULL;h=HashBytes(h,key.data(),key.size());return HexHash(h);}
struct CatalogMeta {std::string category,aliases;int categoryOrder=999,tier=0,recipeOrder=0;};
CatalogMeta CatalogMetadata(const std::string&key,const std::string&name);
struct RecipeIngredient {uintptr_t data=0;int quantity=0;};
struct RecipeRecord {uintptr_t object=0;std::string id,key,name,type,category,station,icon,unavailableReason;int tier=0,order=0;bool available=true;std::vector<RecipeIngredient> ingredients;};
struct BuildingMeta {std::string name,icon,actor,assetPath,description;};
struct BuildingSection {int order=0;bool favourites=false;std::string key,label;std::vector<std::string> pieces;};
struct BuildingTab {int order=0;std::string key,label;std::vector<BuildingSection> sections;};
struct BuildingCatalog {bool valid=false,iconsValid=false;int expectedPieces=0,expectedReferences=0,schema=0;std::string fingerprint;std::vector<BuildingTab> tabs;std::unordered_map<std::string,BuildingMeta> pieces;};
struct PlanCatalog {bool valid=false;int expected=0;std::string fingerprint;std::unordered_set<std::string> keys;};
std::vector<std::string> SplitTabs(const std::string&line){std::vector<std::string> out;size_t start=0;for(;;){auto at=line.find('\t',start);out.push_back(line.substr(start,at==std::string::npos?std::string::npos:at-start));if(at==std::string::npos)return out;start=at+1;}}
const BuildingCatalog& AuthoritativeBuildingCatalog(){
    static const auto catalog=[](){BuildingCatalog value;std::string base=RuntimePaths::WebRoot()+"/buildings/";std::ifstream input(base+"catalog.tsv");std::string line;int declaredTabs=0,declaredSections=0;
        while(std::getline(input,line)){if(!line.empty()&&line.back()=='\r')line.pop_back();auto f=SplitTabs(line);if(f.empty())continue;if(f[0]=="# zanik-building-catalog"&&f.size()>=7){value.schema=std::atoi(f[1].c_str());declaredTabs=std::atoi(f[2].c_str());declaredSections=std::atoi(f[3].c_str());value.expectedReferences=std::atoi(f[4].c_str());value.expectedPieces=std::atoi(f[5].c_str());value.fingerprint=f[6];}
            else if(f[0]=="T"&&f.size()>=4){BuildingTab tab;tab.order=std::atoi(f[1].c_str());tab.key=f[2];tab.label=f[3];value.tabs.push_back(std::move(tab));}
            else if(f[0]=="S"&&f.size()>=6){int tab=std::atoi(f[1].c_str());if(tab<0||size_t(tab)>=value.tabs.size())continue;BuildingSection section;section.order=std::atoi(f[2].c_str());section.favourites=f[3]=="1";section.key=f[4];section.label=f[5];value.tabs[tab].sections.push_back(std::move(section));}
            else if(f[0]=="M"&&f.size()>=5){int tab=std::atoi(f[1].c_str()),section=std::atoi(f[2].c_str());if(tab>=0&&section>=0&&size_t(tab)<value.tabs.size()&&size_t(section)<value.tabs[tab].sections.size())value.tabs[tab].sections[section].pieces.push_back(f[4]);}
            else if(f[0]=="P"&&f.size()>=7)value.pieces.emplace(f[1],BuildingMeta{f[2],f[3],f[4],f[5],f[6]});}
        size_t sections=0,references=0;bool icons=true;for(const auto&tab:value.tabs)for(const auto&section:tab.sections){++sections;references+=section.pieces.size();}for(const auto&p:value.pieces){struct stat st{};if(stat((base+p.second.icon).c_str(),&st)!=0||st.st_size<1){icons=false;break;}}
        const bool fingerprintValid=value.fingerprint.size()==64&&std::all_of(value.fingerprint.begin(),value.fingerprint.end(),[](unsigned char c){return std::isxdigit(c)!=0;});
        value.iconsValid=icons;value.valid=value.schema==1&&fingerprintValid&&declaredTabs==8&&declaredSections==64&&value.expectedReferences==867&&value.expectedPieces==850&&value.tabs.size()==size_t(declaredTabs)&&sections==size_t(declaredSections)&&references==size_t(value.expectedReferences)&&value.pieces.size()==size_t(value.expectedPieces);return value;}();return catalog;
}
const std::unordered_map<std::string,BuildingMeta>& BuildingMetadata(){
    return AuthoritativeBuildingCatalog().pieces;
}
const PlanCatalog& PlanItemCatalog(){
    static const auto catalog=[](){PlanCatalog value;std::string base=RuntimePaths::WebRoot();std::ifstream input(base+"/plan-items.tsv");std::string line;
        while(std::getline(input,line)){if(!line.empty()&&line.back()=='\r')line.pop_back();auto f=SplitTabs(line);if(f.empty())continue;if(f[0]=="# zanik-plan-items"&&f.size()==4){if(f[1]!="1")return value;value.expected=std::atoi(f[2].c_str());value.fingerprint=f[3];continue;}if(f.size()!=4||f[0].rfind("DA_Consumable_Plan",0)!=0||f[1].rfind("PLAN: ",0)!=0||f[2].size()!=22||f[3]!=f[0]+".png")return PlanCatalog{};struct stat st{};if(stat((base+"/items/"+f[3]).c_str(),&st)!=0||st.st_size<1)return PlanCatalog{};value.keys.insert(f[0]);}
        const bool fingerprintValid=value.fingerprint.size()==64&&std::all_of(value.fingerprint.begin(),value.fingerprint.end(),[](unsigned char c){return std::isxdigit(c)!=0;});value.valid=value.expected==141&&value.keys.size()==size_t(value.expected)&&fingerprintValid;return value;}();return catalog;
}
std::string CleanAssetName(std::string value){for(const char*prefix:{"RECIPE_","BUILDPIECE_","DA_"})if(value.rfind(prefix,0)==0){value.erase(0,std::strlen(prefix));break;}std::replace(value.begin(),value.end(),'_',' ');return value;}
bool RejectedRecipeKey(const std::string&key){std::string lower=key;std::transform(lower.begin(),lower.end(),lower.begin(),[](unsigned char c){return char(std::tolower(c));});return lower.find("test")!=std::string::npos||lower.find("debug")!=std::string::npos||lower.find("deprecated")!=std::string::npos||lower.find("_old")!=std::string::npos||lower.find("refund")!=std::string::npos||lower.find("journal")!=std::string::npos||lower.find("vendor")!=std::string::npos||lower.find("proxy")!=std::string::npos||lower.find("deriveddata")!=std::string::npos;}
std::string ItemDisplayName(uintptr_t data){using Fn=const FString*(*)(const void*);auto fn=reinterpret_cast<Fn>(g_Engine->GetModuleBase()+DisplayTextOffset());const FString* text=fn(reinterpret_cast<const void*>(data+0x68));if(text&&Mem::Readable(text,sizeof(FString))){auto value=g_Engine->ReadFString(reinterpret_cast<uintptr_t>(text));if(!value.empty())return value;}return CleanAssetName(g_Engine->GetObjectName(data));}
bool ReadRecipeIngredients(uintptr_t object,std::vector<RecipeIngredient>&out){uintptr_t data=Mem::ReadPtr(object+0x60);int count=Mem::ReadI32(object+0x68);if(count<=0||count>128||!data||!Mem::Readable(reinterpret_cast<void*>(data),size_t(count)*16))return false;for(int i=0;i<count;++i){uintptr_t item=Mem::ReadPtr(data+uintptr_t(i)*16);int quantity=Mem::ReadI32(data+uintptr_t(i)*16+8);if(!item||!g_Engine->IsA(item,"ItemData")||quantity<=0||quantity>1000000)return false;out.push_back({item,quantity});}return !out.empty();}
bool ReadBuildingIngredients(uintptr_t object,std::vector<RecipeIngredient>&out){FUEArray scaled{};buildingScaledRequirements(&scaled,reinterpret_cast<void*>(object),reinterpret_cast<void*>(g_Engine->GetWorld()));if(scaled.Num<=0||scaled.Num>128||!scaled.Data||!Mem::Readable(scaled.Data,size_t(scaled.Num)*16)){if(scaled.Data)memoryFree(scaled.Data);return false;}auto base=reinterpret_cast<uintptr_t>(scaled.Data);for(int i=0;i<scaled.Num;++i){int quantity=Mem::ReadI32(base+uintptr_t(i)*16);uintptr_t item=Mem::ReadPtr(base+uintptr_t(i)*16+8);if(!item||!g_Engine->IsA(item,"ItemData")||quantity<=0||quantity>1000000){memoryFree(scaled.Data);return false;}out.push_back({item,quantity});}memoryFree(scaled.Data);return !out.empty();}
std::vector<RecipeRecord> RecipeRecords(){std::vector<RecipeRecord> result;if(!g_Engine||!buildingScaledRequirements||!memoryFree)return result;std::unordered_set<std::string> resolved;for(int32_t i=0;i<g_Engine->GetObjectCount();++i){uintptr_t object=g_Engine->GetObjectByIndex(i);if(!object)continue;std::string key=g_Engine->GetObjectName(object);bool crafting=g_Engine->IsA(object,"RecipeData"),building=g_Engine->IsA(object,"BuildingPieceData");if((!crafting&&!building)||key.rfind("Default__",0)==0||RejectedRecipeKey(key))continue;auto buildingMeta=BuildingMetadata().find(key);if(building&&buildingMeta==BuildingMetadata().end())continue;RecipeRecord r;r.object=object;r.key=key;r.id=CatalogId(std::string(crafting?"recipe:":"building:")+key);r.type=crafting?"crafting":"building";if(crafting){if(!ReadRecipeIngredients(object,r.ingredients))continue;uintptr_t outputs=Mem::ReadPtr(object+0x90);int count=Mem::ReadI32(object+0x98);if(count<=0||count>64||!outputs||!Mem::Readable(reinterpret_cast<void*>(outputs),size_t(count)*16))continue;uintptr_t output=Mem::ReadPtr(outputs);if(!output||!g_Engine->IsA(output,"ItemData"))continue;r.name=ItemDisplayName(output);r.icon="/assets/items/"+g_Engine->GetObjectName(output)+".png";auto meta=CatalogMetadata(g_Engine->GetObjectName(output),r.name);r.category=meta.category;r.tier=meta.tier;r.order=meta.recipeOrder;r.station=key.find("Process")!=std::string::npos?"Processing station":"Crafting";}else{if(!ReadBuildingIngredients(object,r.ingredients))continue;r.name=buildingMeta->second.name;r.icon="/assets/buildings/"+buildingMeta->second.icon;r.category="Building";std::smatch match;if(std::regex_search(key,match,std::regex("(?:^|_)T([0-9]+)(?:_|$)")))r.tier=std::stoi(match[1].str());r.station="Building";resolved.insert(key);}result.push_back(std::move(r));}for(const auto&entry:BuildingMetadata())if(!resolved.count(entry.first)){RecipeRecord r;r.key=entry.first;r.id=CatalogId("building:"+entry.first);r.name=entry.second.name;r.icon="/assets/buildings/"+entry.second.icon;r.type="building";r.category="Building";r.station="Building";r.available=false;r.unavailableReason="Recipe materials unavailable for this game build";result.push_back(std::move(r));}resolvedBuildingPieces=resolved.size();unavailableBuildingPieces=AuthoritativeBuildingCatalog().expectedPieces>int(resolved.size())?AuthoritativeBuildingCatalog().expectedPieces-resolved.size():0;std::sort(result.begin(),result.end(),[](const auto&a,const auto&b){if(a.type!=b.type)return a.type<b.type;if(a.category!=b.category)return a.category<b.category;if(a.tier!=b.tier)return a.tier<b.tier;return a.name<b.name;});return result;}
bool ResolveRecipe(const std::string&id,RecipeRecord&out){for(auto&r:RecipeRecords())if(r.id==id&&r.available){out=std::move(r);return true;}return false;}
CatalogMeta CatalogMetadata(const std::string&key,const std::string&name){
    CatalogMeta m;std::string lower=key;std::transform(lower.begin(),lower.end(),lower.begin(),[](unsigned char c){return char(std::tolower(c));});
    auto set=[&](const char* category,int order){m.category=category;m.categoryOrder=order;};
    if(name.find("(Expired)")!=std::string::npos)set("Miscellaneous",999);
    else if(lower.rfind("da_consumable_vestige_",0)==0)set("Vestiges",105);
    else if(lower.rfind("da_consumabe_mount_",0)==0)set("Mounts",106);
    else if(lower.rfind("item_consumable_raidpack_",0)==0)set("Miscellaneous",999);
    else if(lower.find("_consumable_potion_")!=std::string::npos||lower.find("curediseasepotion")!=std::string::npos)set("Potions",50);
    else if(lower.find("_consumable_tea_")!=std::string::npos||lower.find("_consumable_water_")!=std::string::npos||lower.find("_drink_")!=std::string::npos||lower.find("_ale_")!=std::string::npos||lower.find("_beer_")!=std::string::npos||lower.find("_wine_")!=std::string::npos)set("Drinks",40);
    else if(lower.rfind("item_consumable_tome_",0)==0)set("Tomes",75);
    else if(lower.rfind("item_consumable_planpack_",0)==0)set("Plans",85);
    else if(lower=="item_consumable_magicfocus"||lower=="item_consumable_weaponbarbs"||lower=="item_consumable_weaponpoison"||lower=="item_consumable_whetstone"||lower=="item_resources_fireoil")set("Weapon enhancements",35);
    else if((lower.rfind("item_consumable_",0)==0&&lower.find("pack")!=std::string::npos)||lower=="item_consumable_birdnest"||lower=="item_consumable_scarecrowscavenging")set("Reward packs",87);
    else if(lower.find("item_consumable_")==0)set("Food",30);
    else if(lower.find("item_armour_")==0||lower.find("item_cape_")==0||lower.find("item_shield_")==0||lower.find("item_jewellery_")==0||lower.find("item_trinket_")==0||lower.rfind("item_masterworks_shield_",0)==0)set("Armour",20);
    else if(lower.find("item_sword_")==0||lower.find("item_dagger_")==0||lower.find("item_mace_")==0||lower.find("item_club_")==0||lower.find("item_greataxe_")==0||lower.find("item_greatsword_")==0||lower.find("item_scimitar_")==0||lower.find("item_staff_")==0||lower.find("item_wand_")==0||lower.find("item_shortbow_")==0||lower.find("item_longbow_")==0||lower.find("item_crossbow_")==0||lower.rfind("item_hammer_",0)==0||lower=="item_masterworks_greatsword_titanswrath"||lower=="item_masterwork_staff_zamorak")set("Weapons",10);
    else if(lower.find("item_pickaxe_")==0||lower.find("item_logging_")==0||lower.find("item_spade_")==0||lower.find("item_fishingrod_")==0||lower.find("item_fishingnet_")==0||lower.find("item_wateringcan_")==0||lower.find("item_secateurs_")==0||lower=="item_torch"||lower.find("item_torch_")==0)set("Tools",25);
    else if(lower.find("item_ammo_")==0||lower.find("item_fishbait_")==0||lower.find("item_fishingbait_")==0)set("Ammunition",60);
    else if(lower.find("item_rune_")==0)set("Runes",70);
    else if(lower.rfind("da_consumable_plan",0)==0)set("Plans",85);
    else if(lower.find("item_farming_seed_")==0||lower.find("_seed_")!=std::string::npos||lower.find("_treeseed_")!=std::string::npos)set("Seeds",80);
    else if(lower.rfind("item_currency_",0)==0||lower.find("_currency_")!=std::string::npos)set("Currencies",92);
    else if(lower=="item_masterworks_imbued_granite_maul_head"||lower=="item_masterworks_imbued_leather_wrappings"||lower=="item_masterworks_ornate_maul_handle"||lower=="item_swampweed")set("Resources",90);
    else if(lower.find("item_resources_")==0||lower.find("item_resource_")==0||lower.find("item_herb_")==0)set("Resources",90);
    else if(lower.find("item_farming_")==0)set("Farming",100);
    else if(lower.find("item_quest_")==0||lower.find("item_activity_")==0||lower.find("item_journal_")==0||lower.find("item_artifact_")==0||lower=="item_masterworks_challenge_horn"||lower=="item_saradomin_symbol_puzzle")set("Quest items",110);
    else if(lower.find("item_fuel_")==0||lower.find("item_bucket_")==0)set("Materials",95);
    else set("Miscellaneous",999);
    std::smatch match;if(std::regex_search(key,match,std::regex("(?:^|_)T([0-9]+)(?:_|$)")))m.tier=std::stoi(match[1].str());
    static const std::pair<const char*,int> progression[]={{"Wood",1},{"Stone",1},{"Bone",2},{"Bronze",3},{"Iron",4},{"Steel",5},{"Mithril",6},{"Adamant",7},{"Rune",8}};
    if(!m.tier)for(const auto& value:progression){std::string token=value.first;std::transform(token.begin(),token.end(),token.begin(),[](unsigned char c){return char(std::tolower(c));});if(lower.find("_"+token+"_")!=std::string::npos||(lower.size()>token.size()&&lower.compare(lower.size()-token.size(),token.size(),token)==0)){m.tier=value.second;break;}}
    std::string alias=key.rfind("ITEM_",0)==0?key.substr(5):key;std::replace(alias.begin(),alias.end(),'_',' ');m.aliases=alias+" "+name;m.recipeOrder=m.tier?m.tier*100:100000;return m;
}
struct Chest {uintptr_t actor=0,inventory=0;std::string id,type,guid;};
std::vector<Chest> FindChests(const std::string&guid){
    std::vector<Chest> result;if(!g_Engine)return result;std::unordered_map<uintptr_t,uintptr_t> owners;std::unordered_set<uintptr_t> linked;
    const int32_t count=g_Engine->GetObjectCount();
    for(int32_t i=0;i<count;++i){uintptr_t o=g_Engine->GetObjectByIndex(i);if(!o||g_Engine->GetObjectName(o).rfind("Default__",0)==0)continue;uintptr_t actor=Mem::ReadPtr(o+Offsets::UObject_Outer);if(!actor)continue;if(g_Engine->IsA(o,"OwnershipComponent"))owners[actor]=o;else if(g_Engine->IsA(o,"LinkedInventoryComponent"))linked.insert(actor);}
    for(int32_t i=0;i<count;++i){uintptr_t inv=g_Engine->GetObjectByIndex(i);if(!inv||!g_Engine->IsObjectLive(inv)||!g_Engine->IsA(inv,"InventoryComponent")||g_Engine->IsA(inv,"LinkedInventoryComponent"))continue;uintptr_t actor=Mem::ReadPtr(inv+Offsets::UObject_Outer);auto it=owners.find(actor);if(it==owners.end()||!linked.count(actor)||!g_Engine->IsObjectLive(actor)||WasAdminStorageBuildingRemoved(actor)||g_Engine->ReadGuid(it->second+0x158)!=guid)continue;bool beingDestroyed=false;if(g_Engine->ReadBoolProperty(actor,"bActorIsBeingDestroyed",beingDestroyed)&&beingDestroyed)continue;std::string type=CleanType(g_Engine->GetObjectClassName(actor));result.push_back({actor,inv,ChestId(actor,type,guid),type,guid});}
    return result;
}
bool FindChest(const std::string&guid,const std::string&id,Chest&out){
    if(!g_Engine)return false;std::unordered_map<uintptr_t,uintptr_t> owners;
    const int32_t count=g_Engine->GetObjectCount();
    for(int32_t i=0;i<count;++i){uintptr_t o=g_Engine->GetObjectByIndex(i);if(!o||g_Engine->GetObjectName(o).rfind("Default__",0)==0)continue;uintptr_t actor=Mem::ReadPtr(o+Offsets::UObject_Outer);if(actor&&g_Engine->IsA(o,"OwnershipComponent"))owners[actor]=o;}
    for(int32_t i=0;i<count;++i){uintptr_t inv=g_Engine->GetObjectByIndex(i);if(!inv||!g_Engine->IsObjectLive(inv)||!g_Engine->IsA(inv,"InventoryComponent")||g_Engine->IsA(inv,"LinkedInventoryComponent"))continue;uintptr_t actor=Mem::ReadPtr(inv+Offsets::UObject_Outer);auto it=owners.find(actor);if(it==owners.end()||!g_Engine->IsObjectLive(actor)||WasAdminStorageBuildingRemoved(actor)||g_Engine->ReadGuid(it->second+0x158)!=guid)continue;bool beingDestroyed=false;if(g_Engine->ReadBoolProperty(actor,"bActorIsBeingDestroyed",beingDestroyed)&&beingDestroyed)continue;std::string type=CleanType(g_Engine->GetObjectClassName(actor));std::string candidate=ChestId(actor,type,guid);if(candidate==id){out={actor,inv,candidate,type,guid};return true;}}
    return false;
}

bool ResolveStorageLocationInternal(const std::string& characterGuid,const std::string& chestId,FVector3d& location){
    Chest chest;if(!FindChest(characterGuid,chestId,chest))return false;
    const uintptr_t root=Mem::ReadPtr(chest.actor+Offsets::Actor_RootComponent);
    if(!root||!Mem::Readable(reinterpret_cast<void*>(root+Offsets::SceneComp_RelativeLocation),sizeof(location)))return false;
    std::memcpy(&location,reinterpret_cast<void*>(root+Offsets::SceneComp_RelativeLocation),sizeof(location));
    return std::isfinite(location.X)&&std::isfinite(location.Y)&&std::isfinite(location.Z);
}
bool ReadSlots(uintptr_t inventory,uintptr_t&data,int32_t&num){data=Mem::ReadPtr(inventory+0x168);num=Mem::ReadI32(inventory+0x170);int32_t max=Mem::ReadI32(inventory+0x174);return num>=0&&num<=512&&max>=num&&(!num||(data&&Mem::Readable(reinterpret_cast<void*>(data),size_t(num)*8)));}
int MaxStackSize(uintptr_t itemData){if(!itemData||!Mem::Readable(reinterpret_cast<void*>(itemData),sizeof(uintptr_t)))return 1;uintptr_t table=Mem::ReadPtr(itemData),fn=table?Mem::ReadPtr(table+0x2D8):0;if(!fn||!Mem::InImage(reinterpret_cast<void*>(fn)))return 1;int value=reinterpret_cast<GetMaxStackSizeFn>(fn)(reinterpret_cast<void*>(itemData));return value>=1&&value<=1000000?value:1;}
int BasePowerLevel(uintptr_t itemData){if(!itemData||!Mem::Readable(reinterpret_cast<void*>(itemData+0x128),sizeof(int32_t)))return 0;const int value=Mem::ReadI32(itemData+0x128);return value>=0&&value<=100?value:0;}
bool IsMasterworkItemData(uintptr_t itemData){if(!itemData||!Mem::Readable(reinterpret_cast<void*>(itemData+0x08),sizeof(int32_t))||!Mem::Readable(reinterpret_cast<void*>(itemData+0x209),1))return false;return (Mem::ReadI32(itemData+0x08)&0x40000000)==0&&Mem::ReadU8(itemData+0x209)!=2&&BasePowerLevel(itemData)>0&&MaxStackSize(itemData)==1;}
int ItemPowerLevel(uintptr_t item){if(!item||!getItemPowerLevel)return 0;const float value=getItemPowerLevel(reinterpret_cast<const void*>(item));if(!std::isfinite(value)||value<0||value>100)return 0;return int(std::lround(value));}
struct SnapshotItem {
    std::string id,key,name,icon;
    int slot=-1,displaySlot=-1,quantity=0,maxStack=1,basePowerLevel=0,powerLevel=0;
    bool equippable=false,canUnequip=true,masterwork=false,held=false;
    uintptr_t object=0,data=0;
    std::uint64_t objectIdentity=0,dataIdentity=0;
};
struct SnapshotChest {
    std::string id,type,customName;
    uintptr_t actor=0,inventory=0,ownership=0,linkedComponent=0,mutationController=0;
    std::uint64_t actorIdentity=0,inventoryIdentity=0,ownershipIdentity=0,linkedComponentIdentity=0;
    FVector3d location{};
    bool linked=false,privateStorage=false,mapValid=false;
    double mapX=0,mapY=0;
    int totalSlots=0,hotbarStart=-1,hotbarCount=0;
    std::vector<int> backingSlots;
    std::vector<SnapshotItem> items;
};

bool IsPlayerInventoryContainerId(const std::string&id){return id=="0000000000000001"||id=="0000000000000002"||id=="0000000000000003"||id=="0000000000000004"||id=="0000000000000005";}
int DisplaySlot(const SnapshotItem&item){return item.displaySlot>=0?item.displaySlot:item.slot;}
int BackingSlot(const SnapshotChest&container,int displaySlot){return displaySlot>=0&&displaySlot<int(container.backingSlots.size())?container.backingSlots[size_t(displaySlot)]:displaySlot;}
constexpr int HeldRightBackingSlot=7;
constexpr int HeldLeftBackingSlot=8;
bool IsMirroredHeldLeftItem(const SnapshotChest&container,const SnapshotItem&item){
    if(container.id!="0000000000000002"||item.slot!=HeldLeftBackingSlot||!item.objectIdentity)return false;
    return std::any_of(container.items.begin(),container.items.end(),[&](const auto&candidate){return candidate.slot==HeldRightBackingSlot&&candidate.objectIdentity==item.objectIdentity;});
}
struct StorageSnapshot {
    std::uint64_t generation=0,removalGeneration=0;
    uintptr_t world=0,adminController=0,adminPawn=0,adminInventory=0,targetController=0,targetPawn=0;
    std::uint64_t worldIdentity=0,adminControllerIdentity=0,adminPawnIdentity=0,adminInventoryIdentity=0;
    bool adminFound=false,adminSpawned=false,adminDead=false,targetFound=false,targetSpawned=false,targetDead=false;
    std::string targetGuid,adminGuid;
    std::vector<SnapshotChest> chests;
    std::chrono::steady_clock::time_point createdAt,discoveredAt;
};
std::shared_mutex snapshotMutex;
std::unordered_map<std::string,std::shared_ptr<const StorageSnapshot>> snapshotCache;
struct CachedPlayerInventory {uintptr_t inventory=0;std::uint64_t identity=0,controllerIdentity=0,pawnIdentity=0;};
std::mutex playerInventoryCacheMutex;
std::unordered_map<std::string,CachedPlayerInventory> playerInventoryCache;
struct KnownAutofillStorage {std::uint64_t actorIdentity=0;std::string ownerGuid,chestId;};
std::mutex knownAutofillStorageMutex;
std::unordered_map<uintptr_t,KnownAutofillStorage> knownAutofillStorages;

void RememberAutofillStorage(const std::string&ownerGuid,const SnapshotChest&chest){
    if(!autofillStore||!chest.actor||!chest.actorIdentity)return;StorageAutofillTemplate desired;if(!autofillStore->Find(ownerGuid,chest.id,desired))return;
    std::lock_guard<std::mutex> lock(knownAutofillStorageMutex);knownAutofillStorages[chest.actor]={chest.actorIdentity,ownerGuid,chest.id};
}

std::string ReadCustomNameDirect(uintptr_t actor){
    if(!g_Engine||!actor)return {};const int32_t offset=g_Engine->FindPropertyOffset(actor,"CustomName");
    if(offset<0||offset>0x4000||!Mem::Readable(reinterpret_cast<void*>(actor+uintptr_t(offset)),16))return {};
    using DisplayFn=const FString*(*)(const void*);auto display=reinterpret_cast<DisplayFn>(g_Engine->GetModuleBase()+DisplayTextOffset());
    const FString* shown=display(reinterpret_cast<const void*>(actor+uintptr_t(offset)));
    return shown&&Mem::Readable(shown,sizeof(FString))?g_Engine->ReadFString(reinterpret_cast<uintptr_t>(shown)):std::string{};
}

std::string ReadCustomNameOnGameThread(uintptr_t actor){
    if(!g_Engine||!actor||!executeGetCustomName||!destroyFText)return {};
    alignas(16) unsigned char text[16]{};
    executeGetCustomName(text,reinterpret_cast<void*>(actor));
    using DisplayFn=const FString*(*)(const void*);
    auto display=reinterpret_cast<DisplayFn>(g_Engine->GetModuleBase()+DisplayTextOffset());
    const FString* shown=display(text);
    std::string value=shown&&Mem::Readable(shown,sizeof(FString))?g_Engine->ReadFString(reinterpret_cast<uintptr_t>(shown)):std::string();
    destroyFText(text);
    return value;
}

void PublishCustomName(std::uint64_t identity,const std::string& value){
    if(!identity||pthread_mutex_trylock(&customNameMutex)!=0)return;
    customNames[identity]={value,std::chrono::steady_clock::now()};pendingCustomNames.erase(identity);
    if(customNames.size()>4096)customNames.erase(customNames.begin());
    pthread_mutex_unlock(&customNameMutex);
}

std::string ReadCustomNameCached(uintptr_t actor){
    if(!g_Engine||!actor)return {};
    const auto identity=g_Engine->GetObjectIdentity(actor);if(!identity)return ReadCustomNameDirect(actor);
    const auto now=std::chrono::steady_clock::now();std::string value;bool current=false;
    pthread_mutex_lock(&customNameMutex);auto found=customNames.find(identity);
    if(found!=customNames.end()){value=found->second.value;current=now-found->second.refreshed<std::chrono::seconds(4);}
    if(!current)pendingCustomNames[identity]=actor;pthread_mutex_unlock(&customNameMutex);
    if(current)return value;const auto reflected=ReadCustomNameDirect(actor);return value.empty()?reflected:value;
}

void ProcessOneCustomName(){
    if(!executeGetCustomName||pthread_mutex_trylock(&customNameMutex)!=0)return;
    std::uint64_t identity=0;uintptr_t actor=0;if(!pendingCustomNames.empty()){auto it=pendingCustomNames.begin();identity=it->first;actor=it->second;pendingCustomNames.erase(it);}pthread_mutex_unlock(&customNameMutex);
    if(!identity||!actor||!g_Engine||g_Engine->GetObjectIdentity(actor)!=identity)return;
    PublishCustomName(identity,ReadCustomNameOnGameThread(actor));
}

bool PlayerOwnsInventory(uintptr_t inventory,const PlayerInfo&player){
    uintptr_t outer=inventory;for(int depth=0;depth<7&&outer;++depth){outer=Mem::ReadPtr(outer+Offsets::UObject_Outer);if(outer&&(outer==player.controllerPtr||outer==player.pawnPtr||outer==player.playerStatePtr))return true;}return false;
}
uintptr_t ReflectedPlayerComponent(const PlayerInfo&player,const char*type,const std::initializer_list<const char*>properties){
    if(!g_Engine||!player.controllerPtr)return 0;
    for(const char*property:properties){const int32_t offset=g_Engine->FindPropertyOffset(player.controllerPtr,property);if(offset<0||offset>0x3000)continue;const auto value=Mem::ReadPtr(player.controllerPtr+uintptr_t(offset));if(value&&g_Engine->IsObjectLive(value)&&g_Engine->IsA(value,type))return value;}
    return 0;
}
uintptr_t ReflectedPlayerInventoryByLayout(const PlayerInfo&player,int expectedSlots,bool equipment){
    uintptr_t match=0;
    for(const auto&field:g_Engine->GetPropertyOffsets(player.controllerPtr)){if(field.second<0||field.second>0x3000)continue;std::string name=field.first;std::transform(name.begin(),name.end(),name.begin(),[](unsigned char value){return char(std::tolower(value));});const bool relevant=equipment?(name.find("loadout")!=std::string::npos||name.find("equipment")!=std::string::npos):name.find("inventory")!=std::string::npos;if(!relevant)continue;const auto value=Mem::ReadPtr(player.controllerPtr+uintptr_t(field.second));if(!value||!g_Engine->IsObjectLive(value)||!g_Engine->IsA(value,"InventoryComponent")||g_Engine->IsA(value,"LinkedInventoryComponent")||(!equipment&&g_Engine->IsA(value,"LoadoutComponent")))continue;uintptr_t slots=0;int32_t count=0;if(!ReadSlots(value,slots,count)||count<expectedSlots)continue;if(match&&match!=value)return 0;match=value;}
    return match;
}
bool PlayerInventoryCandidate(uintptr_t inventory,const PlayerInfo&player){
    if(!inventory||!g_Engine->IsObjectLive(inventory)||!g_Engine->IsA(inventory,"InventoryComponent")||g_Engine->IsA(inventory,"LinkedInventoryComponent")||g_Engine->IsA(inventory,"PersonalInventoryComponent")||g_Engine->IsA(inventory,"LoadoutComponent")||!PlayerOwnsInventory(inventory,player))return false;
    uintptr_t slots=0;int32_t count=0;return g_Engine->GetObjectName(inventory).find("Inventory")!=std::string::npos&&ReadSlots(inventory,slots,count);
}
uintptr_t FindPlayerInventory(const PlayerInfo&player,const std::vector<uintptr_t>*known=nullptr){
    if(!g_Engine||!player.controllerPtr)return 0;const auto controllerIdentity=g_Engine->GetObjectIdentity(player.controllerPtr),pawnIdentity=g_Engine->GetObjectIdentity(player.pawnPtr);{std::lock_guard<std::mutex> lock(playerInventoryCacheMutex);auto cached=playerInventoryCache.find(player.characterGuid);if(cached!=playerInventoryCache.end()&&cached->second.identity==g_Engine->GetObjectIdentity(cached->second.inventory)&&cached->second.controllerIdentity==controllerIdentity&&cached->second.pawnIdentity==pawnIdentity&&PlayerInventoryCandidate(cached->second.inventory,player))return cached->second.inventory;}
    auto reflected=ReflectedPlayerComponent(player,"InventoryComponent",{"Inventory","BP_Components_Inventory","InventoryComponent"});if(!reflected)reflected=ReflectedPlayerInventoryByLayout(player,40,false);if(reflected&&!g_Engine->IsA(reflected,"LinkedInventoryComponent")&&!g_Engine->IsA(reflected,"LoadoutComponent")){uintptr_t slots=0;int32_t count=0;if(ReadSlots(reflected,slots,count)&&count>=40){std::lock_guard<std::mutex> lock(playerInventoryCacheMutex);playerInventoryCache[player.characterGuid]={reflected,g_Engine->GetObjectIdentity(reflected),controllerIdentity,pawnIdentity};return reflected;}}
    uintptr_t fallback=0;auto consider=[&](uintptr_t inventory){if(!PlayerInventoryCandidate(inventory,player))return false;if(g_Engine->GetObjectClassName(inventory)=="InventoryComponent"){fallback=inventory;return true;}if(!fallback)fallback=inventory;return false;};if(known){for(uintptr_t inventory:*known)if(consider(inventory))break;}else{const int32_t count=g_Engine->GetObjectCount();for(int32_t i=0;i<count;++i)if(consider(g_Engine->GetObjectByIndex(i)))break;}
    {std::lock_guard<std::mutex> lock(playerInventoryCacheMutex);if(fallback)playerInventoryCache[player.characterGuid]={fallback,g_Engine->GetObjectIdentity(fallback),controllerIdentity,pawnIdentity};else playerInventoryCache.erase(player.characterGuid);}return fallback;
}
uintptr_t FindPlayerLoadout(const PlayerInfo&player,const std::vector<uintptr_t>*known=nullptr){
    if(!g_Engine||!player.controllerPtr)return 0;
    auto valid=[&](uintptr_t inventory){uintptr_t slots=0;int32_t count=0;return inventory&&g_Engine->IsObjectLive(inventory)&&g_Engine->IsA(inventory,"LoadoutComponent")&&ReadSlots(inventory,slots,count)&&count>=6;};
    auto reflected=ReflectedPlayerComponent(player,"LoadoutComponent",{"Loadout","BP_Components_Loadout","LoadoutComponent"});if(!reflected)reflected=ReflectedPlayerInventoryByLayout(player,6,true);if(valid(reflected))return reflected;
    auto matches=[&](uintptr_t inventory){return valid(inventory)&&PlayerOwnsInventory(inventory,player);};
    if(known)for(const auto inventory:*known)if(matches(inventory))return inventory;
    for(int32_t index=0;index<g_Engine->GetObjectCount();++index){const auto inventory=g_Engine->GetObjectByIndex(index);if(matches(inventory))return inventory;}
    return 0;
}
uintptr_t FindPlayerPersonalInventory(const PlayerInfo&player,const std::vector<uintptr_t>*known=nullptr){
    if(!g_Engine||!player.controllerPtr)return 0;
    auto valid=[&](uintptr_t inventory){uintptr_t slots=0;int32_t count=0;return inventory&&g_Engine->IsObjectLive(inventory)&&g_Engine->IsA(inventory,"PersonalInventoryComponent")&&PlayerOwnsInventory(inventory,player)&&ReadSlots(inventory,slots,count)&&count>0&&count<=512;};
    auto reflected=ReflectedPlayerComponent(player,"PersonalInventoryComponent",{"PersonalInventoryComponent","BP_Components_PersonalInventory","PersonalInventory"});
    if(valid(reflected))return reflected;
    if(known)for(const auto inventory:*known)if(valid(inventory))return inventory;
    for(int32_t index=0;index<g_Engine->GetObjectCount();++index){const auto inventory=g_Engine->GetObjectByIndex(index);if(valid(inventory))return inventory;}
    return 0;
}
const std::vector<int>& EquipmentSlots(){static const std::vector<int> slots{0,1,2,3,4,11,7,8};return slots;}
bool PopulatePlayerContainer(SnapshotChest&container,const char*id,const char*type,uintptr_t inventory,uintptr_t controller,const std::vector<int>&backingSlots,uintptr_t loadout=0){
    uintptr_t slots=0;int32_t count=0;if(!inventory||backingSlots.empty()||!ReadSlots(inventory,slots,count))return false;
    for(const int slot:backingSlots)if(slot<0||slot>=count)return false;
    container.id=id;container.type=type;container.inventory=inventory;container.inventoryIdentity=g_Engine->GetObjectIdentity(inventory);container.mutationController=controller;container.totalSlots=int(backingSlots.size());container.backingSlots=backingSlots;
    if(!container.inventoryIdentity)return false;
    for(size_t display=0;display<backingSlots.size();++display){const int slot=backingSlots[display];const uintptr_t item=Mem::ReadPtr(slots+uintptr_t(slot)*8);if(!item)continue;if(!g_Engine->IsObjectLive(item)||!g_Engine->IsA(item,"Item"))return false;const uintptr_t data=Mem::ReadPtr(item+0xB0);const int quantity=Mem::ReadI32(item+0x58);if(!data||quantity<1||quantity>1000000)return false;SnapshotItem value;value.slot=slot;value.displaySlot=int(display);value.quantity=quantity;value.object=item;value.data=data;value.objectIdentity=g_Engine->GetObjectIdentity(item);value.dataIdentity=g_Engine->GetObjectIdentity(data);if(!value.objectIdentity||!value.dataIdentity)return false;value.key=g_Engine->GetObjectName(data);value.id=ItemId(container.id,slot,value.key,quantity);value.name=ItemDisplayName(data);value.icon="/assets/items/"+value.key+".png";value.maxStack=MaxStackSize(data);value.masterwork=IsMasterworkItemData(data);value.basePowerLevel=value.masterwork?BasePowerLevel(data):0;value.powerLevel=value.masterwork?ItemPowerLevel(item):0;value.canUnequip=true;if(loadout&&loadoutSlotValid)for(const int target:EquipmentSlots()){if(loadoutSlotValid(reinterpret_cast<void*>(loadout),reinterpret_cast<void*>(item),target,nullptr)){value.equippable=true;break;}}container.items.push_back(std::move(value));}
    return true;
}
std::vector<int> SequentialSlots(int offset,int count){std::vector<int> result;if(offset<0||count<1||count>512)return result;result.reserve(size_t(count));for(int index=0;index<count;++index)result.push_back(offset+index);return result;}
std::vector<int> PagedSlots(uintptr_t inventory,std::uint8_t type){if(!pagedSlotOffset||!pagedSlotCount)return {};const int offset=pagedSlotOffset(reinterpret_cast<void*>(inventory),type,true),count=pagedSlotCount(reinterpret_cast<void*>(inventory),type);return SequentialSlots(offset,count);}
std::shared_ptr<const StorageSnapshot> BuildPlayerInventorySnapshot(const std::string&targetGuid,const std::string&adminGuid){
    if(!g_Engine)return {};auto snapshot=std::make_shared<StorageSnapshot>();snapshot->targetGuid=targetGuid;snapshot->adminGuid=adminGuid;snapshot->world=g_Engine->GetWorld();snapshot->worldIdentity=g_Engine->GetObjectIdentity(snapshot->world);if(!snapshot->world||!snapshot->worldIdentity)return {};
    const auto players=g_Engine->GetAllPlayers();std::vector<uintptr_t> inventories;for(int32_t index=0;index<g_Engine->GetObjectCount();++index){const auto object=g_Engine->GetObjectByIndex(index);if(object&&g_Engine->IsA(object,"InventoryComponent"))inventories.push_back(object);}
    for(const auto&player:players){if(player.characterGuid==targetGuid){snapshot->targetFound=true;snapshot->targetSpawned=player.spawned;snapshot->targetDead=player.isDead;snapshot->targetController=player.controllerPtr;snapshot->targetPawn=player.pawnPtr;if(player.spawned&&player.controllerPtr){const auto playerInventory=FindPlayerInventory(player,&inventories),loadout=FindPlayerLoadout(player,&inventories),personalInventory=FindPlayerPersonalInventory(player,&inventories);auto items=PagedSlots(playerInventory,1),quick=PagedSlots(playerInventory,0),runes=PagedSlots(playerInventory,2),ammunition=PagedSlots(playerInventory,3);uintptr_t personalSlots=0;int32_t personalCount=0;SnapshotChest inventory,equipment,personal,runePouch,ammunitionPouch;if(!items.empty()&&!quick.empty()){const int hotbarCount=int(quick.size());quick.insert(quick.end(),items.begin(),items.end());if(PopulatePlayerContainer(inventory,"0000000000000001","Inventory",playerInventory,player.controllerPtr,quick,loadout)){inventory.hotbarStart=0;inventory.hotbarCount=hotbarCount;snapshot->chests.push_back(std::move(inventory));}}if(PopulatePlayerContainer(equipment,"0000000000000002","Equipment",loadout,player.controllerPtr,EquipmentSlots()))snapshot->chests.push_back(std::move(equipment));if(personalInventory&&ReadSlots(personalInventory,personalSlots,personalCount)&&PopulatePlayerContainer(personal,"0000000000000005","Personal chest",personalInventory,player.controllerPtr,SequentialSlots(0,personalCount),loadout)){snapshot->chests.push_back(std::move(personal));personalInventoryCompatibility=1;}else personalInventoryCompatibility=-1;if(PopulatePlayerContainer(runePouch,"0000000000000003","Rune pouch",playerInventory,player.controllerPtr,runes,loadout))snapshot->chests.push_back(std::move(runePouch));if(PopulatePlayerContainer(ammunitionPouch,"0000000000000004","Ammunition pouch",playerInventory,player.controllerPtr,ammunition,loadout))snapshot->chests.push_back(std::move(ammunitionPouch));}}if(player.characterGuid==adminGuid){snapshot->adminFound=true;snapshot->adminSpawned=player.spawned;snapshot->adminDead=player.isDead;snapshot->adminController=player.controllerPtr;snapshot->adminPawn=player.pawnPtr;snapshot->adminControllerIdentity=g_Engine->GetObjectIdentity(player.controllerPtr);snapshot->adminPawnIdentity=g_Engine->GetObjectIdentity(player.pawnPtr);snapshot->adminInventory=FindPlayerInventory(player,&inventories);snapshot->adminInventoryIdentity=g_Engine->GetObjectIdentity(snapshot->adminInventory);}}
    SnapshotChest* inventory=nullptr;const SnapshotChest* equipment=nullptr;for(auto&container:snapshot->chests){if(container.id=="0000000000000001")inventory=&container;else if(container.id=="0000000000000002")equipment=&container;}if(inventory&&equipment)for(auto&item:inventory->items)item.held=std::any_of(equipment->items.begin(),equipment->items.end(),[&](const auto&candidate){return (candidate.slot==HeldRightBackingSlot||candidate.slot==HeldLeftBackingSlot)&&candidate.objectIdentity==item.objectIdentity;});
    return snapshot;
}
void RefreshSnapshotAdministrator(StorageSnapshot&snapshot,const std::vector<uintptr_t>*inventories=nullptr){
    snapshot.adminController=snapshot.adminPawn=snapshot.adminInventory=0;snapshot.adminControllerIdentity=snapshot.adminPawnIdentity=snapshot.adminInventoryIdentity=0;snapshot.adminFound=snapshot.adminSpawned=snapshot.adminDead=false;
    if(!g_Engine)return;auto players=std::atomic_load(&observedMutationPlayers);std::vector<PlayerInfo> fallback;if(!players||players->empty()){fallback=g_Engine->GetAllPlayers();players=std::make_shared<const std::vector<PlayerInfo>>(std::move(fallback));}for(const auto&player:*players)if(player.characterGuid==snapshot.adminGuid){snapshot.adminFound=true;snapshot.adminSpawned=player.spawned;snapshot.adminDead=player.isDead;snapshot.adminController=player.controllerPtr;snapshot.adminPawn=player.pawnPtr;snapshot.adminControllerIdentity=g_Engine->GetObjectIdentity(player.controllerPtr);snapshot.adminPawnIdentity=g_Engine->GetObjectIdentity(player.pawnPtr);snapshot.adminInventory=FindPlayerInventory(player,inventories);snapshot.adminInventoryIdentity=g_Engine->GetObjectIdentity(snapshot.adminInventory);break;}
}

bool RefreshKnownStorageChest(const SnapshotChest&prior,SnapshotChest&chest){
    if(!g_Engine||!prior.actor||!prior.inventory||g_Engine->GetObjectIdentity(prior.actor)!=prior.actorIdentity||
       g_Engine->GetObjectIdentity(prior.inventory)!=prior.inventoryIdentity||
       !prior.ownership||g_Engine->GetObjectIdentity(prior.ownership)!=prior.ownershipIdentity||
       !prior.linkedComponent||g_Engine->GetObjectIdentity(prior.linkedComponent)!=prior.linkedComponentIdentity||
       !g_Engine->IsObjectLive(prior.actor)||!g_Engine->IsObjectLive(prior.inventory)||
       WasAdminStorageBuildingRemoved(prior.actor))return false;
    bool destroying=false;if(g_Engine->ReadBoolProperty(prior.actor,"bActorIsBeingDestroyed",destroying)&&destroying)return false;
    uintptr_t slots=0;int32_t count=0;if(!ReadSlots(prior.inventory,slots,count))return false;
    chest=prior;chest.customName=ReadCustomNameCached(chest.actor);chest.totalSlots=count;chest.items.clear();
    for(int32_t slot=0;slot<count;++slot){
        uintptr_t item=Mem::ReadPtr(slots+uintptr_t(slot)*8);if(!item||!g_Engine->IsA(item,"Item"))continue;
        uintptr_t data=Mem::ReadPtr(item+0xB0);int quantity=Mem::ReadI32(item+0x58);if(!data||quantity<=0||quantity>1000000)continue;
        const auto itemIdentity=g_Engine->GetObjectIdentity(item),dataIdentity=g_Engine->GetObjectIdentity(data);if(!itemIdentity||!dataIdentity)continue;
        auto old=std::find_if(prior.items.begin(),prior.items.end(),[&](const auto&value){return value.slot==slot&&value.objectIdentity==itemIdentity&&value.dataIdentity==dataIdentity;});
        SnapshotItem value;if(old!=prior.items.end())value=*old;else{value.key=g_Engine->GetObjectName(data);value.name=ItemDisplayName(data);value.icon="/assets/items/"+value.key+".png";value.maxStack=MaxStackSize(data);}value.masterwork=IsMasterworkItemData(data);value.basePowerLevel=value.masterwork?BasePowerLevel(data):0;value.powerLevel=value.masterwork?ItemPowerLevel(item):0;
        value.slot=slot;value.quantity=quantity;value.object=item;value.data=data;value.objectIdentity=itemIdentity;value.dataIdentity=dataIdentity;value.id=ItemId(chest.id,slot,value.key,quantity);chest.items.push_back(std::move(value));
    }
    return true;
}

std::shared_ptr<const StorageSnapshot> RefreshKnownStorageSnapshot(const std::shared_ptr<const StorageSnapshot>&known){
    if(!known||!g_Engine||known->world!=g_Engine->GetWorld()||
       known->worldIdentity!=g_Engine->GetObjectIdentity(known->world))return {};
    auto snapshot=std::make_shared<StorageSnapshot>(*known);snapshot->createdAt=std::chrono::steady_clock::now();snapshot->generation=storageGeneration.load();snapshot->removalGeneration=buildingRemovalEvents.load();snapshot->chests.clear();RefreshSnapshotAdministrator(*snapshot);
    for(const auto&prior:known->chests){SnapshotChest chest;if(RefreshKnownStorageChest(prior,chest))snapshot->chests.push_back(std::move(chest));}
    return snapshot;
}

std::shared_ptr<const StorageSnapshot> RefreshKnownAutofillStorage(const StorageAutofillTemplate&desired,const AutofillMutationTrigger&trigger,bool&knownMatch){
    knownMatch=false;const auto started=std::chrono::steady_clock::now();std::shared_ptr<const StorageSnapshot> known;{std::shared_lock<std::shared_mutex> lock(snapshotMutex);auto found=snapshotCache.find(desired.ownerGuid+'\n');if(found!=snapshotCache.end())known=found->second;}
    if(!known||!g_Engine||known->world!=g_Engine->GetWorld()||known->worldIdentity!=g_Engine->GetObjectIdentity(known->world))return {};
    const auto prior=std::find_if(known->chests.begin(),known->chests.end(),[&](const auto&chest){return chest.id==desired.chestId&&chest.inventory==trigger.inventory&&chest.inventoryIdentity==trigger.identity;});if(prior==known->chests.end())return {};knownMatch=true;
    SnapshotChest chest;if(!RefreshKnownStorageChest(*prior,chest))return {};auto snapshot=std::make_shared<StorageSnapshot>(*known);snapshot->createdAt=std::chrono::steady_clock::now();snapshot->generation=storageGeneration.load();snapshot->removalGeneration=buildingRemovalEvents.load();snapshot->chests.clear();snapshot->chests.push_back(std::move(chest));
    ++autofillTargetedRefreshes;const auto micros=std::uint64_t(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-started).count());auto maximum=autofillTargetedRefreshMaximumMicros.load();while(micros>maximum&&!autofillTargetedRefreshMaximumMicros.compare_exchange_weak(maximum,micros)){}return snapshot;
}

std::shared_ptr<const StorageSnapshot> BuildStorageSnapshot(const std::string&targetGuid,const std::string&adminGuid,bool allowCache){
    const auto started=std::chrono::steady_clock::now();const std::string cacheKey=targetGuid+'\n'+adminGuid;
    if(allowCache){
        std::shared_ptr<const StorageSnapshot> known;{std::shared_lock<std::shared_mutex> lock(snapshotMutex);auto found=snapshotCache.find(cacheKey);if(found!=snapshotCache.end())known=found->second;}
        const auto now=std::chrono::steady_clock::now();const bool valid=known&&known->generation==storageGeneration.load()&&known->world==g_Engine->GetWorld()&&known->worldIdentity==g_Engine->GetObjectIdentity(known->world)&&known->removalGeneration==buildingRemovalEvents.load();
        if(valid&&now-known->createdAt<std::chrono::seconds(2)){++snapshotHits;return known;}
        if(valid&&now-known->discoveredAt<std::chrono::seconds(30))if(auto refreshed=RefreshKnownStorageSnapshot(known)){std::unique_lock<std::shared_mutex> lock(snapshotMutex);snapshotCache[cacheKey]=refreshed;++snapshotHits;return refreshed;}
    }
    ++snapshotMisses;auto snapshot=std::make_shared<StorageSnapshot>();snapshot->createdAt=snapshot->discoveredAt=std::chrono::steady_clock::now();snapshot->generation=storageGeneration.load();snapshot->removalGeneration=buildingRemovalEvents.load();snapshot->targetGuid=targetGuid;snapshot->adminGuid=adminGuid;snapshot->world=g_Engine?g_Engine->GetWorld():0;snapshot->worldIdentity=g_Engine&&snapshot->world?g_Engine->GetObjectIdentity(snapshot->world):0;
    if(!g_Engine||!snapshot->world||!snapshot->worldIdentity)return {};
    std::unordered_map<uintptr_t,uintptr_t> owners,linked;std::vector<std::pair<uintptr_t,uintptr_t>> inventories;std::vector<uintptr_t> inventoryObjects;const int32_t count=g_Engine->GetObjectCount();
    for(int32_t i=0;i<count;++i){uintptr_t object=g_Engine->GetObjectByIndex(i);if(!object)continue;const std::string name=g_Engine->GetObjectName(object);if(name.rfind("Default__",0)==0||name.find("GEN_VARIABLE")!=std::string::npos)continue;uintptr_t actor=Mem::ReadPtr(object+Offsets::UObject_Outer);if(!actor)continue;if(g_Engine->IsA(object,"OwnershipComponent"))owners[actor]=object;else if(g_Engine->IsA(object,"LinkedInventoryComponent"))linked[actor]=object;else if(g_Engine->IsA(object,"InventoryComponent")){inventories.push_back({actor,object});inventoryObjects.push_back(object);}}
    RefreshSnapshotAdministrator(*snapshot,&inventoryObjects);
    for(const auto&entry:inventories){uintptr_t actor=entry.first,inventory=entry.second;auto own=owners.find(actor),link=linked.find(actor);
        if(own==owners.end())continue;
        if(link==linked.end()||g_Engine->ReadGuid(own->second+0x158)!=targetGuid||!g_Engine->IsObjectLive(actor)||!g_Engine->IsObjectLive(inventory))continue;bool destroying=false;if(g_Engine->ReadBoolProperty(actor,"bActorIsBeingDestroyed",destroying)&&destroying)continue;
        SnapshotChest chest;chest.actor=actor;chest.inventory=inventory;chest.ownership=own->second;chest.linkedComponent=link->second;chest.actorIdentity=g_Engine->GetObjectIdentity(actor);chest.inventoryIdentity=g_Engine->GetObjectIdentity(inventory);chest.ownershipIdentity=g_Engine->GetObjectIdentity(own->second);chest.linkedComponentIdentity=g_Engine->GetObjectIdentity(link->second);if(!chest.actorIdentity||!chest.inventoryIdentity||!chest.ownershipIdentity||!chest.linkedComponentIdentity)continue;chest.type=CleanType(g_Engine->GetObjectClassName(actor));chest.id=ChestId(actor,chest.type,targetGuid);chest.customName=ReadCustomNameCached(actor);chest.linked=Mem::ReadU8(link->second+0xF8)!=0;chest.privateStorage=Mem::ReadU8(own->second+0x169)!=0;
        uintptr_t root=Mem::ReadPtr(actor+Offsets::Actor_RootComponent);if(root&&Mem::Readable(reinterpret_cast<void*>(root+Offsets::SceneComp_RelativeLocation),sizeof(chest.location)))std::memcpy(&chest.location,reinterpret_cast<void*>(root+Offsets::SceneComp_RelativeLocation),sizeof(chest.location));chest.mapValid=g_Engine->ProjectWorldToMap(chest.location,chest.mapX,chest.mapY)&&std::isfinite(chest.mapX)&&std::isfinite(chest.mapY)&&chest.mapX>=0&&chest.mapX<=1&&chest.mapY>=0&&chest.mapY<=1;
        uintptr_t slots=0;int32_t num=0;if(!ReadSlots(inventory,slots,num))continue;chest.totalSlots=num;for(int32_t slot=0;slot<num;++slot){uintptr_t item=Mem::ReadPtr(slots+uintptr_t(slot)*8);if(!item||!g_Engine->IsA(item,"Item"))continue;uintptr_t data=Mem::ReadPtr(item+0xB0);int quantity=Mem::ReadI32(item+0x58);if(!data||quantity<=0||quantity>1000000)continue;SnapshotItem value;value.slot=slot;value.quantity=quantity;value.object=item;value.data=data;value.objectIdentity=g_Engine->GetObjectIdentity(item);value.dataIdentity=g_Engine->GetObjectIdentity(data);value.key=g_Engine->GetObjectName(data);value.id=ItemId(chest.id,slot,value.key,quantity);value.name=ItemDisplayName(data);value.icon="/assets/items/"+value.key+".png";value.maxStack=MaxStackSize(data);value.masterwork=IsMasterworkItemData(data);value.basePowerLevel=value.masterwork?BasePowerLevel(data):0;value.powerLevel=value.masterwork?ItemPowerLevel(item):0;chest.items.push_back(std::move(value));}snapshot->chests.push_back(std::move(chest));}
    std::sort(snapshot->chests.begin(),snapshot->chests.end(),[](const auto&a,const auto&b){return a.id<b.id;});for(const auto&chest:snapshot->chests)RememberAutofillStorage(targetGuid,chest);{std::unique_lock<std::shared_mutex> lock(snapshotMutex);snapshotCache[cacheKey]=snapshot;}++snapshotGenerations;const auto micros=std::uint64_t(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-started).count());if(micros>maximumSnapshotMicros.load())maximumSnapshotMicros.store(micros);return snapshot;
}
std::shared_ptr<const StorageSnapshot> BuildInventoryDepositSnapshot(const std::string&targetGuid,const std::string&adminGuid){
    const auto inventory=BuildPlayerInventorySnapshot(targetGuid,adminGuid),storage=BuildStorageSnapshot(targetGuid,adminGuid,true);
    if(!inventory||!storage||inventory->world!=storage->world||inventory->worldIdentity!=storage->worldIdentity)return {};
    auto combined=std::make_shared<StorageSnapshot>(*inventory);combined->chests.reserve(inventory->chests.size()+storage->chests.size());
    combined->chests.insert(combined->chests.end(),storage->chests.begin(),storage->chests.end());return combined;
}
const SnapshotChest* SnapshotFindChest(const StorageSnapshot&s,const std::string&id){for(const auto&chest:s.chests)if(chest.id==id)return &chest;return nullptr;}
const SnapshotItem* SnapshotFindItem(const SnapshotChest&chest,const std::string&id){for(const auto&item:chest.items)if(item.id==id)return &item;return nullptr;}
bool IsUnlockCatalogKey(const std::string&key){return key.rfind("DA_Consumable_Vestige_",0)==0||key.rfind("DA_Consumabe_Mount_",0)==0;}
bool AllowedCatalogKey(const std::string&key){return key.rfind("ITEM_",0)==0||(unlockCatalogCompatibility.load()==1&&IsUnlockCatalogKey(key))||(planCatalogCompatibility.load()==1&&PlanItemCatalog().keys.count(key));}
bool UnlockCatalogAssetsValid(){
    static const char* required[]={"DA_Consumable_Vestige_Weapon_Club_Swingslash.png","DA_Consumable_Vestige_Weapon_Longbow_HunterStag.png","DA_Consumable_Vestige_Weapon_Staff_GrievingMoon.png","DA_Consumable_Vestige_Weapon_Staff_ToktzMejTal.png","DA_Consumable_Vestige_Weapon_Whip_Abysmal.png","DA_Consumabe_Mount_TerrorBird_02_04.png","DA_Consumabe_Mount_TerrorBird_02_02.png"};
    const std::string itemRoot=RuntimePaths::WebRoot()+"/items/";
    for(const char*name:required){struct stat st{};std::string path=itemRoot+name;if(stat(path.c_str(),&st)!=0||st.st_size<1)return false;}return true;
}
uintptr_t FindCatalogData(const std::string&id,std::string*keyOut=nullptr){
    if(!g_Engine)return 0;const int32_t count=g_Engine->GetObjectCount();
    for(int32_t i=0;i<count;++i){uintptr_t data=g_Engine->GetObjectByIndex(i);if(!data||!g_Engine->IsA(data,"ItemData"))continue;std::string key=g_Engine->GetObjectName(data);if(!AllowedCatalogKey(key)||CatalogId(key)!=id)continue;if(keyOut)*keyOut=key;return data;}return 0;
}
bool FindItem(const Chest&chest,const std::string&id,int&slot,uintptr_t&item,uintptr_t&itemData,int&quantity,std::string&key){
    uintptr_t data=0;int32_t num=0;if(!ReadSlots(chest.inventory,data,num))return false;
    for(int32_t i=0;i<num;++i){uintptr_t candidate=Mem::ReadPtr(data+uintptr_t(i)*8);if(!candidate||!g_Engine->IsA(candidate,"Item"))continue;uintptr_t candidateData=Mem::ReadPtr(candidate+0xB0);int q=Mem::ReadI32(candidate+0x58);if(!candidateData||q<=0)continue;std::string candidateKey=g_Engine->GetObjectName(candidateData);if(ItemId(chest.id,i,candidateKey,q)==id){slot=i;item=candidate;itemData=candidateData;quantity=q;key=candidateKey;return true;}}
    return false;
}
struct ResolvedItem {int slot=-1,quantity=0;uintptr_t item=0,data=0;std::string key,id;};
bool ResolveItems(const Chest&chest,const std::vector<std::string>&ids,std::vector<ResolvedItem>&out){
    if(ids.empty()||ids.size()>128)return false;std::unordered_set<std::string> unique;
    for(const auto&id:ids){if(id.size()!=16||!unique.insert(id).second)return false;ResolvedItem value;value.id=id;
        if(!FindItem(chest,id,value.slot,value.item,value.data,value.quantity,value.key))return false;out.push_back(std::move(value));}
    return true;
}
bool BatchFits(const Chest&chest,const std::vector<ResolvedItem>&items){
    std::vector<unsigned char> containers(items.size()*16);
    for(size_t i=0;i<items.size();++i){void* data=reinterpret_cast<void*>(items[i].data);std::memcpy(containers.data()+i*16,&data,8);std::memcpy(containers.data()+i*16+8,&items[i].quantity,4);}
    FUEArray values{containers.data(),int32_t(items.size()),int32_t(items.size())};return canAddItemsByData(reinterpret_cast<void*>(chest.inventory),&values);
}
PlayerInfo FindPlayer(const std::string&guid,bool&found){found=false;PlayerInfo result;if(!g_Engine)return result;for(const auto&p:g_Engine->GetAllPlayers())if(p.characterGuid==guid){result=p;found=true;break;}return result;}

struct Lock{pthread_mutex_t* value;explicit Lock(pthread_mutex_t&v):value(&v){pthread_mutex_lock(value);}~Lock(){pthread_mutex_unlock(value);}};
bool IdentityMatches(uintptr_t object,std::uint64_t identity){return object&&identity&&g_Engine&&g_Engine->GetObjectIdentity(object)==identity;}
bool ParseCharacterGuid(const std::string&value,std::array<std::uint32_t,4>&parts){
    if(value.size()!=32||value.find_first_not_of("0123456789abcdefABCDEF")!=std::string::npos)return false;
    for(size_t i=0;i<parts.size();++i){char*end=nullptr;const std::string chunk=value.substr(i*8,8);const auto parsed=std::strtoul(chunk.c_str(),&end,16);if(!end||*end)return false;parts[i]=std::uint32_t(parsed);}return true;
}
std::uint64_t UnixNow(){return std::uint64_t(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count());}
std::uint64_t MonotonicMicros(){return std::uint64_t(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());}
void RecordAutofillLatency(std::atomic<std::uint64_t>&last,std::atomic<std::uint64_t>&maximum,std::uint64_t value){last.store(value,std::memory_order_relaxed);auto current=maximum.load(std::memory_order_relaxed);while(value>current&&!maximum.compare_exchange_weak(current,value,std::memory_order_relaxed)){} }
void WakeAutofilledStorageReplication(Request&r,const NativeAction&a){
    r.autofillReplicationWakeRecorded=true;
    const auto started=std::chrono::steady_clock::now();
    const auto flush=autofillFlushNetDormancyFunction.load(std::memory_order_acquire),force=autofillForceNetUpdateFunction.load(std::memory_order_acquire);
    const bool valid=autofillReplicationWakeCompatibility.load(std::memory_order_acquire)==1&&a.actor&&IdentityMatches(a.actor,a.actorIdentity);
    const bool flushed=valid&&InvokeSharedProcessEvent(reinterpret_cast<void*>(a.actor),reinterpret_cast<void*>(flush),nullptr);
    const bool forced=flushed&&InvokeSharedProcessEvent(reinterpret_cast<void*>(a.actor),reinterpret_cast<void*>(force),nullptr);
    const auto micros=std::uint64_t(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-started).count());
    auto maximum=autofillReplicationWakeMaximumMicros.load(std::memory_order_relaxed);while(micros>maximum&&!autofillReplicationWakeMaximumMicros.compare_exchange_weak(maximum,micros,std::memory_order_relaxed)){}
    ++autofillReplicationWakeCalls;if(!forced)++autofillReplicationWakeFailures;
}
void FinishJob(Request&r,bool success,int code){r.resultCode.store(code);r.updatedUnix.store(UnixNow());r.state.store(success?JobState::Completed:JobState::Failed,std::memory_order_release);pendingCount.fetch_sub(1);if(success){++accepted;++completedJobs;if(r.operation=="autofill"&&!r.actions.empty()&&r.actions.front().kind==ActionKind::Add){autofillItemsAdded.fetch_add(std::uint64_t(r.actions.front().quantity));++autofillNormalizations;}}else{++rejected;++failedJobs;if(r.operation=="autofill"&&code!=-7)++autofillFailures;}snapshotInvalidations.fetch_add(1);storageGeneration.fetch_add(1);}
struct AutofillLiveStack { int slot=-1,quantity=0;uintptr_t item=0,data=0;std::string key; };
bool ReadAutofillLiveStacks(uintptr_t inventory,std::vector<AutofillLiveStack>&out,int32_t&slotCount){
    uintptr_t slots=0;if(!ReadSlots(inventory,slots,slotCount))return false;out.clear();out.reserve(std::size_t(slotCount));
    for(int32_t slot=0;slot<slotCount;++slot){const uintptr_t item=Mem::ReadPtr(slots+uintptr_t(slot)*8);if(!item)continue;const uintptr_t data=Mem::ReadPtr(item+0xB0);const int quantity=Mem::ReadI32(item+0x58);if(!data||quantity<1||quantity>1000000||!g_Engine->IsObjectLive(item)||!g_Engine->IsObjectLive(data))return false;out.push_back({slot,quantity,item,data,g_Engine->GetObjectName(data)});}return true;
}
bool NormalizeAutofillAdd(Request&r,const NativeAction&a){
    if(r.autofillNormalize.empty()||r.autofillNormalizeKey.empty()||r.autofillNormalize.size()>512)return false;
    struct ScopedObservedMutationSuppression { bool previous=suppressObservedMutation;ScopedObservedMutationSuppression(){suppressObservedMutation=true;}~ScopedObservedMutationSuppression(){suppressObservedMutation=previous;} } suppress;
    std::unordered_map<int,int> desired;long long desiredTotal=0;for(const auto&saved:r.autofillNormalize){if(saved.slot<0||saved.quantity<1||!desired.emplace(saved.slot,saved.quantity).second)return false;desiredTotal+=saved.quantity;}
    auto read=[&](std::vector<AutofillLiveStack>&live,int32_t&count){return ReadAutofillLiveStacks(a.destination,live,count);};
    std::vector<AutofillLiveStack> live;int32_t count=0;if(!read(live,count))return false;for(const auto&entry:desired)if(entry.first>=count)return false;
    auto matchingAt=[&](int slot)->AutofillLiveStack*{for(auto&stack:live)if(stack.slot==slot&&stack.key==r.autofillNormalizeKey)return &stack;return nullptr;};
    auto occupied=[&](int slot){return std::any_of(live.begin(),live.end(),[&](const auto&stack){return stack.slot==slot;});};
    for(;;){std::vector<StorageAutofillItem> current;current.reserve(live.size());for(const auto&stack:live)current.push_back({stack.slot,stack.quantity,stack.quantity,stack.key});const auto relocation=PlanStorageAutofillRelocation(r.autofillNormalize,current,r.autofillNormalizeKey);if(!relocation.available)break;if(!moveItem(reinterpret_cast<void*>(a.destination),relocation.sourceSlot,reinterpret_cast<void*>(a.destination),relocation.destinationSlot,nullptr,relocation.quantity)||!read(live,count))return false;}
    for(const auto&stack:live)if(stack.key==r.autofillNormalizeKey&&!desired.count(stack.slot))return false;
    for(const auto&entry:desired){if(matchingAt(entry.first))continue;if(occupied(entry.first))return false;auto source=std::find_if(live.begin(),live.end(),[&](const auto&stack){return stack.key==r.autofillNormalizeKey&&desired.count(stack.slot)&&stack.quantity>1;});if(source==live.end()||!moveItem(reinterpret_cast<void*>(a.destination),source->slot,reinterpret_cast<void*>(a.destination),entry.first,nullptr,1))return false;if(!read(live,count)||!matchingAt(entry.first))return false;}
    long long actualTotal=0;for(const auto&stack:live)if(stack.key==r.autofillNormalizeKey){if(!desired.count(stack.slot))return false;actualTotal+=stack.quantity;}
    // A whole-stack add can be temporarily clamped or rebalanced by the
    // linked-inventory implementation. Each controlled quantity is the saved
    // minimum or the higher pre-add live quantity, so repair to those targets
    // below and keep the exact final readback as the fail-closed boundary.
    for(const auto&entry:desired){auto*stack=matchingAt(entry.first);if(!stack)return false;if(stack->quantity>entry.second)setStackSize(reinterpret_cast<void*>(stack->item),entry.second);}
    if(!read(live,count))return false;
    for(const auto&entry:desired){auto*stack=matchingAt(entry.first);if(!stack)return false;if(stack->quantity<entry.second)setStackSize(reinterpret_cast<void*>(stack->item),entry.second);}
    if(!read(live,count))return false;actualTotal=0;for(const auto&stack:live)if(stack.key==r.autofillNormalizeKey){if(!desired.count(stack.slot))return false;actualTotal+=stack.quantity;}if(actualTotal!=desiredTotal)return false;for(const auto&entry:desired){const auto*stack=matchingAt(entry.first);if(!stack||stack->quantity!=entry.second)return false;}return true;
}
bool AddMasterworkItem(Request&r,const NativeAction&a,const unsigned char*emptyTags){
    if(!addItemByData||!getItemPowerLevel||!canHaveMasterworkUpgrade||!applyMasterworkUpgrade||!onRepMasterworkUpgrades||a.quantity!=1||a.desiredPowerLevel<1){r.resultCode=-17;return false;}
    uintptr_t slots=0;int32_t count=0;if(!ReadSlots(a.destination,slots,count)){r.resultCode=-17;return false;}std::unordered_set<uintptr_t> before;for(int32_t slot=0;slot<count;++slot){const auto item=Mem::ReadPtr(slots+uintptr_t(slot)*8);if(item)before.insert(item);}
    if(!addItemByData(reinterpret_cast<void*>(a.destination),reinterpret_cast<void*>(a.itemData),1,1.0f,emptyTags)){r.resultCode=-17;return false;}
    if(!ReadSlots(a.destination,slots,count)){r.resultCode=-17;return false;}std::vector<std::pair<int,uintptr_t>> createdItems;for(int32_t slot=0;slot<count;++slot){const auto item=Mem::ReadPtr(slots+uintptr_t(slot)*8);if(item&&!before.count(item)&&Mem::ReadPtr(item+0xB0)==a.itemData)createdItems.push_back({slot,item});}
    auto removeCreated=[&](){for(auto it=createdItems.rbegin();it!=createdItems.rend();++it)removeFromSlot(reinterpret_cast<void*>(a.destination),it->first,1,reinterpret_cast<void*>(a.controller));};
    if(createdItems.size()!=1||!g_Engine->IsObjectLive(createdItems.front().second)){removeCreated();r.resultCode=-17;return false;}const uintptr_t created=createdItems.front().second;
    int power=ItemPowerLevel(created);auto setReplicatedPower=[&](){
        // Imported power-9 items prove that the authoritative replicated
        // upgrade count supports levels beyond some items' released native
        // ascension-core ladder. Accept that fallback from any native stopping
        // point only when the live count exactly explains the effective power.
        const int base=BasePowerLevel(a.itemData),desiredUpgrades=a.desiredPowerLevel-base;
        if(a.desiredPowerLevel!=9||power<base||power>=a.desiredPowerLevel||desiredUpgrades<1||desiredUpgrades>255||!Mem::Readable(reinterpret_cast<void*>(created+0xB9),2))return false;
        const auto priorCount=Mem::ReadU8(created+0xB9),priorPrevious=Mem::ReadU8(created+0xBA);if(int(priorCount)!=power-base||desiredUpgrades<=priorCount)return false;
        const auto desiredCount=static_cast<std::uint8_t>(desiredUpgrades);if(!Mem::SafeWrite(created+0xBA,priorCount)||!Mem::SafeWrite(created+0xB9,desiredCount)){Mem::SafeWrite(created+0xB9,priorCount);Mem::SafeWrite(created+0xBA,priorPrevious);onRepMasterworkUpgrades(reinterpret_cast<void*>(created));return false;}
        onRepMasterworkUpgrades(reinterpret_cast<void*>(created));const int updated=ItemPowerLevel(created);if(updated!=a.desiredPowerLevel){Mem::SafeWrite(created+0xB9,priorCount);Mem::SafeWrite(created+0xBA,priorPrevious);onRepMasterworkUpgrades(reinterpret_cast<void*>(created));power=ItemPowerLevel(created);return false;}power=updated;return true;
    };for(int step=0;power<a.desiredPowerLevel&&step<16;++step){
        if(!canHaveMasterworkUpgrade(reinterpret_cast<const void*>(created))){
            if(!setReplicatedPower()){removeCreated();r.resultCode=-17;return false;}break;
        }
        const int prior=power;applyMasterworkUpgrade(reinterpret_cast<void*>(created));power=ItemPowerLevel(created);if(power!=prior+1){if(power<a.desiredPowerLevel&&setReplicatedPower())break;removeCreated();r.resultCode=-17;return false;}
    }
    if(power!=a.desiredPowerLevel){removeCreated();r.resultCode=-17;return false;}return true;
}
bool CopyExactItem(Request&r,const NativeAction&a){
    if(!copyItem||!canAddItem||!addItem){r.resultCode=-18;return false;}void* clone=copyItem(reinterpret_cast<const void*>(a.item),reinterpret_cast<void*>(a.destination));if(!clone||clone==reinterpret_cast<void*>(a.item)){r.resultCode=-18;return false;}const auto cloned=reinterpret_cast<uintptr_t>(clone);if(!g_Engine->IsObjectLive(cloned)||Mem::ReadPtr(cloned+0xB0)!=a.itemData||Mem::ReadI32(cloned+0x58)!=a.expectedQuantity||ItemPowerLevel(cloned)!=ItemPowerLevel(a.item)||!canAddItem(reinterpret_cast<const void*>(a.destination),clone)){r.resultCode=-18;return false;}if(!addItem(reinterpret_cast<void*>(a.destination),clone)){r.resultCode=-18;return false;}return true;
}
bool MatchingInventoryItems(uintptr_t inventory,uintptr_t itemData,int&count,int&quantity){
    count=0;quantity=0;uintptr_t slots=0;int32_t slotCount=0;if(!ReadSlots(inventory,slots,slotCount))return false;
    std::unordered_set<uintptr_t> seen;
    for(int32_t slot=0;slot<slotCount;++slot){const auto item=Mem::ReadPtr(slots+uintptr_t(slot)*8);if(!item||!seen.insert(item).second||Mem::ReadPtr(item+0xB0)!=itemData)continue;const int stack=Mem::ReadI32(item+0x58);if(stack<1||stack>1000000)return false;++count;quantity+=stack;}
    return true;
}
bool ReadInventoryState(uintptr_t inventory,std::vector<std::array<uintptr_t,4>>&state){
    state.clear();uintptr_t slots=0;int32_t slotCount=0;if(!ReadSlots(inventory,slots,slotCount))return false;state.reserve(size_t(slotCount));
    for(int32_t slot=0;slot<slotCount;++slot){const auto item=Mem::ReadPtr(slots+uintptr_t(slot)*8);state.push_back({uintptr_t(slot),item,item?Mem::ReadPtr(item+0xB0):0,item?uintptr_t(std::max(0,Mem::ReadI32(item+0x58))):0});}
    return true;
}
bool RunNativeAction(Request&r,const NativeAction&a){
    const bool cleanup=a.kind==ActionKind::RenameDestroyIdentity||a.kind==ActionKind::RenameDestroyText;
    if(cleanup){
        if(a.kind==ActionKind::RenameDestroyIdentity&&r.renameSetterConstructed){destroyUniqueNetId(r.renameSetter);r.renameSetterConstructed=false;}
        if(a.kind==ActionKind::RenameDestroyText&&r.renameTextConstructed){destroyFText(r.renameText);r.renameTextConstructed=false;}
        return true;
    }
    if(!g_Engine||g_Engine->GetWorld()!=r.expectedWorld||g_Engine->GetObjectIdentity(r.expectedWorld)!=r.expectedWorldIdentity){++worldChanges;r.resultCode=-3;return false;}
    if(familyCircuitBroken[size_t(a.family)].load()){r.resultCode=-4;return false;}
    if((a.source&&!IdentityMatches(a.source,a.sourceIdentity))||(a.destination&&!IdentityMatches(a.destination,a.destinationIdentity))||(a.item&&!IdentityMatches(a.item,a.itemIdentity))||(a.itemData&&!IdentityMatches(a.itemData,a.itemDataIdentity))||(a.actor&&!IdentityMatches(a.actor,a.actorIdentity))||(a.controller&&!IdentityMatches(a.controller,a.controllerIdentity))){++stale;++staleIdentities;r.resultCode=-1;return false;}
    if(a.item&&a.sourceSlot>=0){uintptr_t slots=0;int32_t count=0;if(!ReadSlots(a.source,slots,count)||a.sourceSlot>=count||Mem::ReadPtr(slots+uintptr_t(a.sourceSlot)*8)!=a.item||Mem::ReadPtr(a.item+0xB0)!=a.itemData||(a.expectedQuantity>0&&Mem::ReadI32(a.item+0x58)!=a.expectedQuantity)){++stale;++staleIdentities;r.resultCode=-1;return false;}}
    bool ok=true;alignas(8) unsigned char emptyTags[32]{};switch(a.kind){
        case ActionKind::Add:ok=addItemByData(reinterpret_cast<void*>(a.destination),reinterpret_cast<void*>(a.itemData),a.quantity,1.0f,emptyTags);if(ok&&r.operation=="autofill"&&!NormalizeAutofillAdd(r,a)){autofillNormalizationCompatibility=-1;++autofillNormalizationFailures;familyCircuitBroken[size_t(OperationFamily::Autofill)]=true;ok=false;}break;
        case ActionKind::AddMasterwork:ok=AddMasterworkItem(r,a,emptyTags);break;
        case ActionKind::CopyItem:ok=CopyExactItem(r,a);break;
        case ActionKind::Remove:ok=removeFromSlot(reinterpret_cast<void*>(a.source),a.sourceSlot,a.quantity,reinterpret_cast<void*>(a.controller));break;
        case ActionKind::Move:ok=moveItem(reinterpret_cast<void*>(a.source),a.sourceSlot,reinterpret_cast<void*>(a.destination),a.destinationSlot,reinterpret_cast<void*>(a.controller),a.quantity);break;
        case ActionKind::EquipHeld:{
            int beforeCount=0,beforeQuantity=0;std::vector<std::array<uintptr_t,4>>beforeState,afterState;if(!addLoadoutItemToSlot||!MatchingInventoryItems(a.source,a.itemData,beforeCount,beforeQuantity)||beforeCount!=a.matchingItemCount||beforeQuantity!=a.matchingQuantity||!ReadInventoryState(a.source,beforeState)){r.resultCode=-19;ok=false;break;}
            addLoadoutItemToSlot(reinterpret_cast<void*>(a.destination),reinterpret_cast<void*>(a.item),a.destinationSlot);
            uintptr_t sourceSlots=0,destinationSlots=0;int32_t sourceCount=0,destinationCount=0;int afterCount=0,afterQuantity=0;
            const bool sourcePreserved=ReadSlots(a.source,sourceSlots,sourceCount)&&a.sourceSlot>=0&&a.sourceSlot<sourceCount&&Mem::ReadPtr(sourceSlots+uintptr_t(a.sourceSlot)*8)==a.item&&Mem::ReadPtr(a.item+0xB0)==a.itemData&&Mem::ReadI32(a.item+0x58)==a.expectedQuantity;
            const bool heldReference=ReadSlots(a.destination,destinationSlots,destinationCount)&&a.destinationSlot>=0&&a.destinationSlot<destinationCount&&Mem::ReadPtr(destinationSlots+uintptr_t(a.destinationSlot)*8)==a.item;
            const bool duplicatesPreserved=MatchingInventoryItems(a.source,a.itemData,afterCount,afterQuantity)&&afterCount==beforeCount&&afterQuantity==beforeQuantity;
            const bool inventoryPreserved=ReadInventoryState(a.source,afterState)&&afterState==beforeState;
            ok=sourcePreserved&&heldReference&&duplicatesPreserved&&inventoryPreserved;if(!ok)r.resultCode=-19;
            break;
        }
        case ActionKind::UnequipHeld:{
            int beforeCount=0,beforeQuantity=0;std::vector<std::array<uintptr_t,4>>beforeState,afterState;if(!removeLoadoutItem||!MatchingInventoryItems(a.source,a.itemData,beforeCount,beforeQuantity)||beforeCount!=a.matchingItemCount||beforeQuantity!=a.matchingQuantity||!ReadInventoryState(a.source,beforeState)){r.resultCode=-20;ok=false;break;}
            ok=removeLoadoutItem(reinterpret_cast<void*>(a.destination),reinterpret_cast<void*>(a.item));
            uintptr_t sourceSlots=0,destinationSlots=0;int32_t sourceCount=0,destinationCount=0;int afterCount=0,afterQuantity=0;
            const bool sourcePreserved=ReadSlots(a.source,sourceSlots,sourceCount)&&a.sourceSlot>=0&&a.sourceSlot<sourceCount&&Mem::ReadPtr(sourceSlots+uintptr_t(a.sourceSlot)*8)==a.item&&Mem::ReadPtr(a.item+0xB0)==a.itemData&&Mem::ReadI32(a.item+0x58)==a.expectedQuantity;
            bool heldReferencesCleared=ReadSlots(a.destination,destinationSlots,destinationCount)&&destinationCount>HeldLeftBackingSlot;if(heldReferencesCleared)for(const int slot:{HeldRightBackingSlot,HeldLeftBackingSlot})if(Mem::ReadPtr(destinationSlots+uintptr_t(slot)*8)==a.item){heldReferencesCleared=false;break;}
            const bool duplicatesPreserved=MatchingInventoryItems(a.source,a.itemData,afterCount,afterQuantity)&&afterCount==beforeCount&&afterQuantity==beforeQuantity;
            const bool inventoryPreserved=ReadInventoryState(a.source,afterState)&&afterState==beforeState;
            ok=ok&&sourcePreserved&&heldReferencesCleared&&duplicatesPreserved&&inventoryPreserved;if(!ok)r.resultCode=-20;
            break;
        }
        case ActionKind::RelocateHeld:{
            int beforeCount=0,beforeQuantity=0;std::vector<std::array<uintptr_t,4>>beforeState,afterState;uintptr_t loadoutSlots=0;int32_t loadoutCount=0;
            const bool prepared=addLoadoutItemToSlot&&removeLoadoutItem&&MatchingInventoryItems(a.source,a.itemData,beforeCount,beforeQuantity)&&beforeCount==a.matchingItemCount&&beforeQuantity==a.matchingQuantity&&ReadInventoryState(a.source,beforeState)&&ReadSlots(a.destination,loadoutSlots,loadoutCount)&&a.loadoutSourceSlot>=0&&a.loadoutSourceSlot<loadoutCount&&a.destinationSlot>=0&&a.destinationSlot<loadoutCount&&Mem::ReadPtr(loadoutSlots+uintptr_t(a.loadoutSourceSlot)*8)==a.item;
            const bool removed=prepared&&removeLoadoutItem(reinterpret_cast<void*>(a.destination),reinterpret_cast<void*>(a.item));if(removed)addLoadoutItemToSlot(reinterpret_cast<void*>(a.destination),reinterpret_cast<void*>(a.item),a.destinationSlot);
            uintptr_t sourceSlots=0;int32_t sourceCount=0;int afterCount=0,afterQuantity=0;const bool sourcePreserved=ReadSlots(a.source,sourceSlots,sourceCount)&&a.sourceSlot>=0&&a.sourceSlot<sourceCount&&Mem::ReadPtr(sourceSlots+uintptr_t(a.sourceSlot)*8)==a.item&&Mem::ReadPtr(a.item+0xB0)==a.itemData&&Mem::ReadI32(a.item+0x58)==a.expectedQuantity;const bool destinationSet=ReadSlots(a.destination,loadoutSlots,loadoutCount)&&a.destinationSlot<loadoutCount&&Mem::ReadPtr(loadoutSlots+uintptr_t(a.destinationSlot)*8)==a.item&&Mem::ReadPtr(loadoutSlots+uintptr_t(a.loadoutSourceSlot)*8)!=a.item;const bool duplicatesPreserved=MatchingInventoryItems(a.source,a.itemData,afterCount,afterQuantity)&&afterCount==beforeCount&&afterQuantity==beforeQuantity;const bool inventoryPreserved=ReadInventoryState(a.source,afterState)&&afterState==beforeState;ok=prepared&&removed&&sourcePreserved&&destinationSet&&duplicatesPreserved&&inventoryPreserved;
            if(!ok){bool restored=!removed;if(removed&&removeLoadoutItem&&addLoadoutItemToSlot){removeLoadoutItem(reinterpret_cast<void*>(a.destination),reinterpret_cast<void*>(a.item));addLoadoutItemToSlot(reinterpret_cast<void*>(a.destination),reinterpret_cast<void*>(a.item),a.loadoutSourceSlot);restored=ReadSlots(a.destination,loadoutSlots,loadoutCount)&&a.loadoutSourceSlot<loadoutCount&&Mem::ReadPtr(loadoutSlots+uintptr_t(a.loadoutSourceSlot)*8)==a.item;afterState.clear();restored=restored&&ReadInventoryState(a.source,afterState)&&afterState==beforeState;}r.resultCode=restored?-21:-22;}
            break;
        }
        case ActionKind::SetStack:setStackSize(reinterpret_cast<void*>(a.item),a.quantity);break;
        case ActionKind::RenameConstruct:{FString value{r.renameUtf16.data(),int32_t(r.renameUtf16Units+1),int32_t(r.renameUtf16Units+1)};fTextFromString(r.renameText,&value);r.renameTextConstructed=true;using DisplayFn=const FString*(*)(const void*);auto display=reinterpret_cast<DisplayFn>(g_Engine->GetModuleBase()+DisplayTextOffset());const FString*shown=display(r.renameText);const std::string constructed=shown&&Mem::Readable(shown,sizeof(FString))?g_Engine->ReadFString(reinterpret_cast<uintptr_t>(shown)):std::string();ok=constructed==r.renameValue.data();if(!ok)r.resultCode=-14;}break;
        case ActionKind::RenameSetterIdentity:{if(renameableStaticClass&&doesImplementInterface){void* interfaceClass=renameableStaticClass();ok=interfaceClass&&doesImplementInterface(reinterpret_cast<void*>(a.actor),&interfaceClass);if(!ok){r.resultCode=-15;break;}}executeGetSetterId(r.renameSetter,reinterpret_cast<void*>(a.actor));r.renameSetterConstructed=true;}break;
        case ActionKind::RenameSet:if(serverSetObjectCustomName&&a.controller)serverSetObjectCustomName(reinterpret_cast<void*>(a.controller),reinterpret_cast<void*>(a.actor),r.renameText,r.renameSetter);else executeSetCustomName(reinterpret_cast<void*>(a.actor),r.renameText,r.renameSetter);break;
        case ActionKind::RenameVerify:{const auto actual=ReadCustomNameOnGameThread(a.actor);ok=actual==r.renameValue.data();if(ok)PublishCustomName(a.actorIdentity,actual);else r.resultCode=-6;}break;
        case ActionKind::RenameDestroyIdentity:break;
        case ActionKind::RenameDestroyText:break;
        case ActionKind::LinkableSet:{
            ok=setIsLinkable;
            if(ok)setIsLinkable(reinterpret_cast<void*>(a.destination),a.linkable);
            if(!ok)r.resultCode=-11;
        }break;
        case ActionKind::LinkableVerify:ok=(Mem::ReadU8(a.destination+0xF8)!=0)==a.linkable;if(!ok)r.resultCode=-12;break;
        case ActionKind::OwnershipSet:{
            std::uint8_t canChange=0;
            if(!Mem::SafeRead(a.source+0x168,canChange)){r.resultCode=-10;ok=false;break;}
            const std::uint8_t enabled=1;
            if(!canChange&&!Mem::SafeWrite(a.source+0x168,enabled)){r.resultCode=-10;ok=false;break;}
            setOwningCharacterGuid(reinterpret_cast<void*>(a.source),a.ownerGuid.data());
            if(!canChange&&!Mem::SafeWrite(a.source+0x168,canChange)){ownershipCompatibility=-1;r.resultCode=-10;ok=false;}
        }break;
        case ActionKind::OwnershipVerify:ok=g_Engine->ReadGuid(a.source+0x158)==[&](){char value[33]{};std::snprintf(value,sizeof(value),"%08X%08X%08X%08X",a.ownerGuid[0],a.ownerGuid[1],a.ownerGuid[2],a.ownerGuid[3]);return std::string(value);}();if(!ok)r.resultCode=-9;break;
    }
    if(!ok&&r.operation=="transfer_ownership"&&r.state.load()!=JobState::RollingBack)++ownershipFailures;
    return ok;
}
void ProcessOneStorageAction(){
    if(!activeRequest){const auto tail=requestTail.load(std::memory_order_relaxed),head=requestHead.load(std::memory_order_acquire);if(tail==head)return;activeRequest=requestRing[tail%QueueCapacity];requestTail.store(tail+1,std::memory_order_release);if(!activeRequest)return;if(activeRequest->operation=="autofill"&&activeRequest->autofillReconciledAtMicros){activeRequest->autofillDispatchedAtMicros=MonotonicMicros();if(activeRequest->autofillDispatchedAtMicros>=activeRequest->autofillReconciledAtMicros)RecordAutofillLatency(autofillReconcileToDispatchLastMicros,autofillReconcileToDispatchMaximumMicros,activeRequest->autofillDispatchedAtMicros-activeRequest->autofillReconciledAtMicros);}if(activeRequest->state.load()!=JobState::RollingBack)activeRequest->state.store(JobState::Running);activeRequest->updatedUnix.store(UnixNow());++runningJobs;}
    Request&r=*activeRequest;const bool rolling=r.state.load()==JobState::RollingBack;if((rolling&&r.rollbackIndex.load()==0)||(!rolling&&r.actionIndex.load()>=r.actions.size())){if(rolling){const int code=r.resultCode.load();FinishJob(r,false,code?code:-2);}else if(r.finishAsFailure.load())FinishJob(r,false,r.resultCode.load()?r.resultCode.load():-2);else if(r.operation=="transfer_ownership"){r.state=JobState::Persisting;r.updatedUnix=UnixNow();runningJobs.fetch_sub(1);activeRequest=nullptr;return;}else FinishJob(r,true,r.resultCode.load()==2?2:1);runningJobs.fetch_sub(1);activeRequest=nullptr;return;}
    if(r.cancelled.load()){FinishJob(r,false,-7);runningJobs.fetch_sub(1);activeRequest=nullptr;return;}
    if(!rolling&&r.requiresTargetOnline&&r.actionIndex.load()==0){bool online=false;for(const auto&player:g_Engine->GetAllPlayers())if(player.characterGuid==r.targetGuid&&player.spawned&&player.controllerPtr){online=true;break;}if(!online){r.resultCode=-16;FinishJob(r,false,-16);runningJobs.fetch_sub(1);activeRequest=nullptr;return;}}
    const NativeAction action=rolling?r.rollback[r.rollbackIndex.load()-1]:r.actions[r.actionIndex.load()];const auto started=std::chrono::steady_clock::now();const bool ok=RunNativeAction(r,action);const auto micros=std::uint64_t(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-started).count());auto& maximum=familyMaximum[size_t(action.family)];if(micros>maximum.load())maximum.store(micros);if(micros>maximumMicros.load())maximumMicros.store(micros);
    const bool nativeGameAction=action.kind==ActionKind::Add||action.kind==ActionKind::AddMasterwork||action.kind==ActionKind::CopyItem||action.kind==ActionKind::Remove||action.kind==ActionKind::Move||action.kind==ActionKind::EquipHeld||action.kind==ActionKind::UnequipHeld||action.kind==ActionKind::RelocateHeld||action.kind==ActionKind::SetStack||action.kind==ActionKind::RenameConstruct||action.kind==ActionKind::RenameSet||action.kind==ActionKind::LinkableSet||action.kind==ActionKind::OwnershipSet;
    PerformanceTrace::Record("storage",r.operation.c_str(),micros,r.adminId.c_str(),'p',"website",nativeGameAction?PerformanceTrace::TimingScope::NativeGame:PerformanceTrace::TimingScope::Zanik);if(micros>=GameThreadLimit(action.family)){familyCircuitBroken[size_t(action.family)]=true;if(r.resultCode.load()==0)r.resultCode=2;}
    if(ok){if(rolling)r.rollbackIndex.fetch_sub(1);else{if(r.operation=="autofill"&&r.actionIndex.load()+1>=r.actions.size()){if(action.actor&&!r.autofillReplicationWakeRecorded)WakeAutofilledStorageReplication(r,action);if(!r.autofillCompletionRecorded&&r.autofillObservedAtMicros&&r.autofillDispatchedAtMicros){const auto refilledAt=MonotonicMicros();if(refilledAt>=r.autofillDispatchedAtMicros)RecordAutofillLatency(autofillDispatchToRefillLastMicros,autofillDispatchToRefillMaximumMicros,refilledAt-r.autofillDispatchedAtMicros);if(refilledAt>=r.autofillObservedAtMicros)RecordAutofillLatency(autofillCaptureToRefillLastMicros,autofillCaptureToRefillMaximumMicros,refilledAt-r.autofillObservedAtMicros);r.autofillCompletionRecorded=true;++autofillLatencySamples;}}r.actionIndex.fetch_add(1);if(action.kind==ActionKind::Move){NativeAction reverse=action;std::swap(reverse.source,reverse.destination);std::swap(reverse.sourceIdentity,reverse.destinationIdentity);std::swap(reverse.sourceSlot,reverse.destinationSlot);uintptr_t slots=0;int32_t count=0;if(ReadSlots(reverse.source,slots,count)&&reverse.sourceSlot>=0&&reverse.sourceSlot<count){reverse.item=Mem::ReadPtr(slots+uintptr_t(reverse.sourceSlot)*8);reverse.itemIdentity=g_Engine->GetObjectIdentity(reverse.item);reverse.itemData=reverse.item?Mem::ReadPtr(reverse.item+0xB0):0;reverse.itemDataIdentity=g_Engine->GetObjectIdentity(reverse.itemData);reverse.expectedQuantity=reverse.item?Mem::ReadI32(reverse.item+0x58):0;}reverse.family=OperationFamily::Rollback;r.rollback.push_back(reverse);}}return;}
    if(!rolling&&(r.renameTextConstructed||r.renameSetterConstructed)){r.finishAsFailure=true;r.actionIndex=r.renameSetterConstructed?r.actions.size()-2:r.actions.size()-1;return;}
    if(!rolling&&!r.rollback.empty()){++rollbackAttempts;r.rollbackIndex=r.rollback.size();r.state=JobState::RollingBack;return;}
    if(rolling)++rollbackFailures;if(action.kind==ActionKind::Remove&&r.actionIndex.load()>0){r.partialDelete=true;r.resultCode=-5;}FinishJob(r,false,r.resultCode.load()?r.resultCode.load():-2);runningJobs.fetch_sub(1);activeRequest=nullptr;
}

void WriteJump(unsigned char* at,uintptr_t target){at[0]=0x48;at[1]=0xB8;std::memcpy(at+2,&target,8);at[10]=0xFF;at[11]=0xE0;}
void WriteRegisterPreservingJump(unsigned char*at,uintptr_t target){at[0]=0xff;at[1]=0x25;std::memset(at+2,0,4);std::memcpy(at+6,&target,8);}
void UpdateMaximum(std::atomic<std::uint64_t>&maximum,std::uint64_t value){auto current=maximum.load(std::memory_order_relaxed);if(value>current)maximum.compare_exchange_strong(current,value,std::memory_order_relaxed);}
bool PublishObservedMutation(ObservedMutationEvent event,const std::chrono::steady_clock::time_point&captureStarted){
    const auto sequence=observedMutationHead.fetch_add(1,std::memory_order_relaxed);auto&slot=observedMutationRing[sequence%ObservedMutationCapacity];std::uint8_t empty=0;
    if(!slot.state.compare_exchange_strong(empty,1,std::memory_order_acquire)){if(event.kind==ObservedMutationKind::BuildingDestroyed){++autofillDestructionDrops;autofillDestructionCompatibility=-1;}else nativeMutationDrops.fetch_add(1,std::memory_order_relaxed);return false;}
    event.captureMicros=static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-captureStarted).count());slot.event=event;slot.state.store(2,std::memory_order_release);if(event.kind==ObservedMutationKind::BuildingConsume)UpdateMaximum(autofillBuildingCaptureMaximumMicros,event.captureMicros);else if(event.kind==ObservedMutationKind::BuildingDestroyed)UpdateMaximum(autofillDestructionMaximumMicros,event.captureMicros);else UpdateMaximum(event.timing?nativeMutationCaptureMaximumMicros:autofillRpcCaptureMaximumMicros,event.captureMicros);return true;
}
void CleanupDestroyedStorageAutofill(uintptr_t actor,std::uint64_t identity){
    const auto started=std::chrono::steady_clock::now();KnownAutofillStorage known;
    if(!identity)return;
    {std::lock_guard<std::mutex> lock(knownAutofillStorageMutex);const auto found=knownAutofillStorages.find(actor);if(found!=knownAutofillStorages.end()&&found->second.actorIdentity==identity){known=found->second;knownAutofillStorages.erase(found);}}
    if(known.ownerGuid.empty()){
        std::shared_lock<std::shared_mutex> lock(snapshotMutex);
        for(const auto&entry:snapshotCache){if(!entry.second)continue;for(const auto&chest:entry.second->chests)if(chest.actor==actor&&chest.actorIdentity==identity){known={identity,entry.second->targetGuid,chest.id};break;}if(!known.ownerGuid.empty())break;}
    }
    if(known.ownerGuid.empty())return;
    if(!autofillStore)return;StorageAutofillTemplate existing;if(!autofillStore->Find(known.ownerGuid,known.chestId,existing))return;
    {std::lock_guard<std::mutex> lock(jobsMutex);for(const auto&entry:jobs)if(entry.second->operation=="autofill"&&entry.second->conflictKey==known.ownerGuid&&entry.second->targetId==known.chestId)entry.second->cancelled=true;}
    if(autofillStore->Remove(known.ownerGuid,known.chestId))++autofillDestroyedSnapshots;else{autofillPersistenceCompatibility=-1;autofillDestructionCompatibility=-1;++autofillDestructionFailures;++autofillFailures;}
    UpdateMaximum(autofillDestructionMaximumMicros,std::uint64_t(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-started).count()));
}
bool HookMoveItem(void*source,int sourceSlot,void*destination,int destinationSlot,void*controller,int quantity){
    const auto started=std::chrono::steady_clock::now();const bool result=originalObservedMoveItem(source,sourceSlot,destination,destinationSlot,controller,quantity);const auto nativeMicros=static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-started).count());
    nativeMutationCalls.fetch_add(1,std::memory_order_relaxed);UpdateMaximum(nativeMutationMaximumMicros,nativeMicros);if(!suppressObservedMutation){const auto captureStarted=std::chrono::steady_clock::now();ObservedMutationEvent event{ObservedMutationKind::Move,reinterpret_cast<uintptr_t>(source),reinterpret_cast<uintptr_t>(destination),reinterpret_cast<uintptr_t>(controller),nativeMicros,0,result};event.observedAtMicros=MonotonicMicros();event.sourceSlot=sourceSlot;event.destinationSlot=destinationSlot;PublishObservedMutation(event,captureStarted);}return result;
}
bool HookRemoveFromSlot(void*source,int slot,int quantity,void*controller){
    const auto started=std::chrono::steady_clock::now();const bool result=originalObservedRemoveFromSlot(source,slot,quantity,controller);const auto nativeMicros=static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-started).count());
    nativeMutationCalls.fetch_add(1,std::memory_order_relaxed);UpdateMaximum(nativeMutationMaximumMicros,nativeMicros);if(!suppressObservedMutation){const auto captureStarted=std::chrono::steady_clock::now();ObservedMutationEvent event{ObservedMutationKind::Remove,reinterpret_cast<uintptr_t>(source),0,reinterpret_cast<uintptr_t>(controller),nativeMicros,0,result};event.observedAtMicros=MonotonicMicros();event.sourceSlot=slot;PublishObservedMutation(event,captureStarted);}return result;
}
template<typename Function> bool InstallObservedMutationHook(uintptr_t moduleBase,uintptr_t moduleEnd,uintptr_t offset,const unsigned char*signature,std::size_t bytes,uintptr_t hook,Function&original){
    if(original)return true;if(moduleBase+offset<moduleBase||moduleBase+offset+bytes>moduleEnd)return false;auto*target=reinterpret_cast<unsigned char*>(moduleBase+offset);if(std::memcmp(target,signature,bytes))return false;auto*trampoline=static_cast<unsigned char*>(mmap(nullptr,64,PROT_READ|PROT_WRITE|PROT_EXEC,MAP_PRIVATE|MAP_ANONYMOUS,-1,0));if(trampoline==MAP_FAILED)return false;std::memcpy(trampoline,target,bytes);WriteRegisterPreservingJump(trampoline+bytes,reinterpret_cast<uintptr_t>(target+bytes));original=reinterpret_cast<Function>(trampoline);long pageSize=sysconf(_SC_PAGESIZE);uintptr_t page=reinterpret_cast<uintptr_t>(target)&~(uintptr_t(pageSize)-1);if(mprotect(reinterpret_cast<void*>(page),size_t(pageSize),PROT_READ|PROT_WRITE|PROT_EXEC)!=0){original=nullptr;munmap(trampoline,64);return false;}unsigned char patch[RemoveFromSlotHookBytes];std::memset(patch,0x90,bytes);WriteJump(patch,hook);std::memcpy(target,patch,bytes);__builtin___clear_cache(reinterpret_cast<char*>(target),reinterpret_cast<char*>(target+bytes));mprotect(reinterpret_cast<void*>(page),size_t(pageSize),PROT_READ|PROT_EXEC);return true;
}
bool InstallObservedMutationHooks(uintptr_t moduleBase,uintptr_t moduleEnd,const StorageOffsets&offsets){
    const unsigned char moveSignature[MoveItemHookBytes]={0x55,0x53,0x50,0x4c,0x89,0xc0,0x48,0x89,0xfb,0x4c,0x8b,0x17};
    const unsigned char removeSignature[RemoveFromSlotHookBytes]={0x55,0x41,0x57,0x41,0x56,0x53,0x48,0x83,0xec,0x28,0x80,0xbf,0x68,0x02,0x00,0x00,0x00};
    if(moduleBase+offsets.moveItem+sizeof(moveSignature)>moduleEnd||moduleBase+offsets.removeFromSlot+sizeof(removeSignature)>moduleEnd||std::memcmp(reinterpret_cast<void*>(moduleBase+offsets.moveItem),moveSignature,sizeof(moveSignature))||std::memcmp(reinterpret_cast<void*>(moduleBase+offsets.removeFromSlot),removeSignature,sizeof(removeSignature)))return false;
    if(!InstallObservedMutationHook(moduleBase,moduleEnd,offsets.moveItem,moveSignature,sizeof(moveSignature),reinterpret_cast<uintptr_t>(&HookMoveItem),originalObservedMoveItem))return false;moveItem=originalObservedMoveItem;
    if(!InstallObservedMutationHook(moduleBase,moduleEnd,offsets.removeFromSlot,removeSignature,sizeof(removeSignature),reinterpret_cast<uintptr_t>(&HookRemoveFromSlot),originalObservedRemoveFromSlot))return false;removeFromSlot=originalObservedRemoveFromSlot;observedMoveEntry.store(moduleBase+offsets.moveItem,std::memory_order_release);observedRemoveEntry.store(moduleBase+offsets.removeFromSlot,std::memory_order_release);return true;
}
uintptr_t FindUniqueStorageFunction(const char*name){
    uintptr_t match=0;if(!g_Engine)return 0;for(int32_t i=0;i<g_Engine->GetObjectCount();++i){const auto object=g_Engine->GetObjectByIndex(i);if(!object||g_Engine->GetObjectClassName(object)!="Function"||g_Engine->GetObjectName(object)!=name)continue;if(match&&match!=object)return 0;match=object;}return match;
}
uintptr_t FindStorageRpc(const char*name){auto function=g_Engine->FindFunction("InventoryController",name);return function?function:FindUniqueStorageFunction(name);}
bool ResolveInventoryParameterOffsets(uintptr_t function,std::size_t expected,std::size_t&source,std::size_t*destination){
    const auto fields=g_Engine->GetStructPropertyOffsets(function);if(fields.empty())return false;std::vector<std::size_t> inventories;for(const auto&field:fields){std::string name=field.first;std::transform(name.begin(),name.end(),name.begin(),[](unsigned char value){return char(std::tolower(value));});if(name.find("inventory")==std::string::npos||field.second<0||field.second>64||field.second%alignof(uintptr_t))continue;inventories.push_back(std::size_t(field.second));}std::sort(inventories.begin(),inventories.end());inventories.erase(std::unique(inventories.begin(),inventories.end()),inventories.end());if(inventories.size()!=expected)return false;source=inventories.front();if(destination)*destination=inventories.back();return true;
}
bool ResolveInventoryArrayParameterOffset(uintptr_t function,std::size_t&offset){
    const auto fields=g_Engine->GetStructPropertyOffsets(function);if(fields.empty())return false;std::vector<std::size_t> inventories;for(const auto&field:fields){std::string name=field.first;std::transform(name.begin(),name.end(),name.begin(),[](unsigned char value){return char(std::tolower(value));});if(name.find("inventor")==std::string::npos||field.second<0||field.second>64||field.second%alignof(uintptr_t))continue;inventories.push_back(std::size_t(field.second));}std::sort(inventories.begin(),inventories.end());inventories.erase(std::unique(inventories.begin(),inventories.end()),inventories.end());if(inventories.size()!=1)return false;offset=inventories.front();return true;
}
bool InitializeAutofillRpcCapture(const std::string&hash,uintptr_t moduleBase,uintptr_t moduleEnd){
    const auto move=FindStorageRpc("Server_MoveItemBetweenInventories"),splitPut=FindStorageRpc("Server_SplitAndPutStack"),split=FindStorageRpc("Server_SplitStack");if(!move||!splitPut||!split)return false;
    auto moveSource=std::size_t{},moveDestination=std::size_t{},splitPutSource=std::size_t{},splitPutDestination=std::size_t{},splitSource=std::size_t{};
    const bool moveLayout=ResolveInventoryParameterOffsets(move,2,moveSource,&moveDestination),splitPutLayout=ResolveInventoryParameterOffsets(splitPut,2,splitPutSource,&splitPutDestination),splitLayout=ResolveInventoryParameterOffsets(split,1,splitSource,nullptr);
    // The reviewed current Linux build strips parameter-field metadata from
    // some RPC UFunctions. Its generated RPC ABI is two aligned inventory
    // pointers at +0/+16 (or one at +0 for an in-place split). Restrict this
    // fallback to the exact reviewed executable; every later build must either
    // expose valid reflection metadata or receive a new compatibility review.
    if(!(moveLayout&&splitPutLayout&&splitLayout)){if(hash!=CurrentSha&&hash!=LatestSha&&hash!=CandidateSha)return false;moveSource=0;moveDestination=16;splitPutSource=0;splitPutDestination=16;splitSource=0;}
    if(!EnsurePlayerProcessEventCapture(moduleBase,moduleEnd))return false;
    autofillMoveSourceOffset=moveSource;autofillMoveDestinationOffset=moveDestination;autofillSplitPutSourceOffset=splitPutSource;autofillSplitPutDestinationOffset=splitPutDestination;autofillSplitSourceOffset=splitSource;
    autofillMoveRpc.store(move,std::memory_order_release);autofillSplitPutRpc.store(splitPut,std::memory_order_release);autofillSplitRpc.store(split,std::memory_order_release);return true;
}
using CompleteBuildingConsumptionFn=void(*)(void*,const FUEArray*,const void*);
using PartialBuildingConsumptionFn=void(*)(void*,const FUEArray*,int,const void*);
struct CapturedBuildingInventories { std::array<uintptr_t,128> values{};int32_t count=0;bool valid=false; };
CapturedBuildingInventories CaptureBuildingInventories(const FUEArray*inventories){
    CapturedBuildingInventories captured;captured.valid=inventories&&Mem::Readable(inventories,sizeof(*inventories));if(!captured.valid)return captured;captured.count=inventories->Num;captured.valid=captured.count>=1&&captured.count<=int32_t(captured.values.size())&&inventories->Max>=captured.count&&inventories->Data&&Mem::Readable(inventories->Data,std::size_t(captured.count)*sizeof(uintptr_t));if(captured.valid)std::memcpy(captured.values.data(),inventories->Data,std::size_t(captured.count)*sizeof(uintptr_t));return captured;
}
void PublishBuildingConsumption(const CapturedBuildingInventories&captured,const std::chrono::steady_clock::time_point&started){
    ++autofillBuildingCaptureCalls;if(!captured.valid){++autofillBuildingCaptureDrops;return;}for(int32_t index=0;index<captured.count;++index){const auto inventory=captured.values[std::size_t(index)];if(!inventory||!g_Engine||!g_Engine->IsObjectLive(inventory)||!g_Engine->IsA(inventory,"InventoryComponent")){++autofillBuildingCaptureDrops;continue;}++autofillBuildingCaptureInventories;if(!PublishObservedMutation({ObservedMutationKind::BuildingConsume,inventory,0,0,0,0,true,false,MonotonicMicros()},started))++autofillBuildingCaptureDrops;}UpdateMaximum(autofillBuildingCaptureMaximumMicros,std::uint64_t(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-started).count()));
}
void HookCompleteBuildingConsumption(void*controller,const FUEArray*inventories,const void*completionStatus){
    const auto captured=CaptureBuildingInventories(inventories);
    const auto target=autofillCompleteBuildingNativeTarget.load(std::memory_order_acquire);const auto started=std::chrono::steady_clock::now();
    if(target)reinterpret_cast<CompleteBuildingConsumptionFn>(target)(controller,inventories,completionStatus);PublishBuildingConsumption(captured,started);
}
void HookPartialBuildingConsumption(void*controller,const FUEArray*inventories,int quantity,const void*itemData){
    const auto captured=CaptureBuildingInventories(inventories);const auto target=autofillPartialBuildingNativeTarget.load(std::memory_order_acquire);const auto started=std::chrono::steady_clock::now();if(target)reinterpret_cast<PartialBuildingConsumptionFn>(target)(controller,inventories,quantity,itemData);PublishBuildingConsumption(captured,started);
}
bool InstallNativeBuildingConsumptionHook(uintptr_t offset,uintptr_t moduleBase,uintptr_t moduleEnd,const unsigned char*targetPrefix,std::size_t targetPrefixSize,uintptr_t hook,std::atomic<uintptr_t>&nativeTarget){
    constexpr std::size_t ThunkBytes=12;if(!offset||moduleBase+offset<moduleBase||moduleBase+offset+ThunkBytes>moduleEnd||targetPrefixSize!=BuildingConsumptionHookBytes)return false;
    auto*entry=reinterpret_cast<unsigned char*>(moduleBase+offset);if(entry[0]!=0xe9||!std::all_of(entry+5,entry+ThunkBytes,[](unsigned char value){return value==0xcc;}))return false;
    std::int32_t displacement=0;std::memcpy(&displacement,entry+1,sizeof(displacement));const auto target=uintptr_t(std::int64_t(moduleBase+offset+5)+displacement);if(target<moduleBase||target+targetPrefixSize>moduleEnd||std::memcmp(reinterpret_cast<void*>(target),targetPrefix,targetPrefixSize))return false;
    // Native callers may branch directly to the implementation and bypass the
    // exported reflection thunk. Patch the validated implementation entry and
    // retain its complete prologue in a trampoline for the hook's original call.
    auto*trampoline=static_cast<unsigned char*>(mmap(nullptr,64,PROT_READ|PROT_WRITE|PROT_EXEC,MAP_PRIVATE|MAP_ANONYMOUS,-1,0));if(trampoline==MAP_FAILED)return false;std::memcpy(trampoline,reinterpret_cast<void*>(target),BuildingConsumptionHookBytes);WriteJump(trampoline+BuildingConsumptionHookBytes,target+BuildingConsumptionHookBytes);nativeTarget.store(reinterpret_cast<uintptr_t>(trampoline),std::memory_order_release);
    const long pageSize=sysconf(_SC_PAGESIZE);const uintptr_t page=target&~(uintptr_t(pageSize)-1);const std::size_t protectedBytes=((target+BuildingConsumptionHookBytes-1)&~(uintptr_t(pageSize)-1))+std::size_t(pageSize)-page;if(mprotect(reinterpret_cast<void*>(page),protectedBytes,PROT_READ|PROT_WRITE|PROT_EXEC)!=0){nativeTarget=0;munmap(trampoline,64);return false;}unsigned char patch[BuildingConsumptionHookBytes];std::memset(patch,0x90,sizeof(patch));WriteJump(patch,hook);std::memcpy(reinterpret_cast<void*>(target),patch,sizeof(patch));__builtin___clear_cache(reinterpret_cast<char*>(target),reinterpret_cast<char*>(target+sizeof(patch)));mprotect(reinterpret_cast<void*>(page),protectedBytes,PROT_READ|PROT_EXEC);return true;
}
bool InstallNativeBuildingConsumptionCapture(const std::string&hash,uintptr_t moduleBase,uintptr_t moduleEnd){
    const uintptr_t completeOffset=hash==CurrentSha?0x0A688410:(hash==LatestSha?0x0A6884B0:(hash==CandidateSha?0x0A688A50:0)),partialOffset=hash==CurrentSha?0x0A688400:(hash==LatestSha?0x0A6884A0:(hash==CandidateSha?0x0A688A40:0));const unsigned char completePrefix[]={0x55,0x41,0x57,0x41,0x56,0x41,0x55,0x41,0x54,0x53,0x48,0x83,0xec,0x58,0x48,0x89,0xf3},partialPrefix[]={0x55,0x41,0x57,0x41,0x56,0x41,0x55,0x41,0x54,0x53,0x48,0x83,0xec,0x18,0x48,0x89,0xcb};
    return InstallNativeBuildingConsumptionHook(completeOffset,moduleBase,moduleEnd,completePrefix,sizeof(completePrefix),reinterpret_cast<uintptr_t>(&HookCompleteBuildingConsumption),autofillCompleteBuildingNativeTarget)&&InstallNativeBuildingConsumptionHook(partialOffset,moduleBase,moduleEnd,partialPrefix,sizeof(partialPrefix),reinterpret_cast<uintptr_t>(&HookPartialBuildingConsumption),autofillPartialBuildingNativeTarget);
}
bool InitializeAutofillBuildingCapture(const std::string&hash,uintptr_t moduleBase,uintptr_t moduleEnd){
    const auto complete=FindStorageRpc("Server_ConsumeItemsFromCompletingBuilding"),partial=FindStorageRpc("Server_ConsumeItemsFromPartiallyCompletingBuilding");if(!complete||!partial||complete==partial)return false;
    std::size_t completeInventories=0,partialInventories=0;const bool completeLayout=ResolveInventoryArrayParameterOffset(complete,completeInventories),partialLayout=ResolveInventoryArrayParameterOffset(partial,partialInventories);
    // The reviewed Linux builds retain a leading TArray<UInventoryComponent*>
    // parameter even when cooked reflection omits its field metadata.
    if(!(completeLayout&&partialLayout)){if(hash!=CurrentSha&&hash!=LatestSha&&hash!=CandidateSha)return false;completeInventories=0;partialInventories=0;}
    if(!EnsurePlayerProcessEventCapture(moduleBase,moduleEnd)||!InstallNativeBuildingConsumptionCapture(hash,moduleBase,moduleEnd))return false;autofillCompleteBuildingInventoriesOffset=completeInventories;autofillPartialBuildingInventoriesOffset=partialInventories;autofillCompleteBuildingRpc.store(complete,std::memory_order_release);autofillPartialBuildingRpc.store(partial,std::memory_order_release);return true;
}
bool InitializeAutofillReplicationWake(uintptr_t moduleBase,uintptr_t moduleEnd){
    if(!EnsurePlayerProcessEventCapture(moduleBase,moduleEnd))return false;
    const auto flush=g_Engine->FindFunction("Actor","FlushNetDormancy"),force=g_Engine->FindFunction("Actor","ForceNetUpdate");
    if(!flush||!force||!g_Engine->GetStructPropertyOffsets(flush).empty()||!g_Engine->GetStructPropertyOffsets(force).empty())return false;
    autofillFlushNetDormancyFunction.store(flush,std::memory_order_release);autofillForceNetUpdateFunction.store(force,std::memory_order_release);return true;
}
void HookBuildingEndPlay(void* actor,int reason){
    if(reason==0){const auto started=std::chrono::steady_clock::now();const auto value=reinterpret_cast<uintptr_t>(actor);const auto identity=g_Engine&&value?g_Engine->GetObjectIdentity(value):0;if(identity&&PublishObservedMutation({ObservedMutationKind::BuildingDestroyed,value,0,0,0,0,true,false,0,identity},started))++autofillDestructionSignals;else if(!identity){++autofillDestructionDrops;autofillDestructionCompatibility=-1;}++buildingRemovalEvents;++snapshotInvalidations;storageGeneration.fetch_add(1);}
    originalBuildingEndPlay(actor,reason);
}
bool InstallBuildingEndPlayHook(uintptr_t moduleBase,uintptr_t moduleEnd,uintptr_t offset){
    if(originalBuildingEndPlay)return true;if(moduleBase+offset+BuildingEndPlayHookBytes>moduleEnd)return false;auto* target=reinterpret_cast<unsigned char*>(moduleBase+offset);const unsigned char expected[BuildingEndPlayHookBytes]={0x55,0x41,0x57,0x41,0x56,0x41,0x55,0x41,0x54,0x53,0x50,0x89,0xf5,0x49,0x89,0xfe};if(std::memcmp(target,expected,sizeof(expected)))return false;
    // Only the complete instructions replaced by the trampoline belong to the
    // hook contract.  The following call remains in the original function and
    // its relative displacement may legitimately move between game builds.
    if(!Mem::Readable(target,sizeof(expected)))return false;
    auto* trampoline=static_cast<unsigned char*>(mmap(nullptr,64,PROT_READ|PROT_WRITE|PROT_EXEC,MAP_PRIVATE|MAP_ANONYMOUS,-1,0));if(trampoline==MAP_FAILED)return false;std::memcpy(trampoline,target,BuildingEndPlayHookBytes);WriteJump(trampoline+BuildingEndPlayHookBytes,reinterpret_cast<uintptr_t>(target+BuildingEndPlayHookBytes));originalBuildingEndPlay=reinterpret_cast<BuildingEndPlayFn>(trampoline);
    long pageSize=sysconf(_SC_PAGESIZE);uintptr_t page=reinterpret_cast<uintptr_t>(target)&~(uintptr_t(pageSize)-1);if(mprotect(reinterpret_cast<void*>(page),size_t(pageSize),PROT_READ|PROT_WRITE|PROT_EXEC)!=0){originalBuildingEndPlay=nullptr;munmap(trampoline,64);return false;}unsigned char patch[BuildingEndPlayHookBytes];std::memset(patch,0x90,sizeof(patch));WriteJump(patch,reinterpret_cast<uintptr_t>(&HookBuildingEndPlay));std::memcpy(target,patch,sizeof(patch));__builtin___clear_cache(reinterpret_cast<char*>(target),reinterpret_cast<char*>(target+sizeof(patch)));mprotect(reinterpret_cast<void*>(page),size_t(pageSize),PROT_READ|PROT_EXEC);return true;
}
void HookEngineTick(void* actor,float delta,int levelTick,void* tickFunction){
    const auto engineStarted=std::chrono::steady_clock::now();
    originalEngineTick(actor,delta,levelTick,tickFunction);
    const auto engineMicros=static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-engineStarted).count());
    ++performanceFrames;auto frameMaximum=performanceMaximumFrameMicros.load();while(engineMicros>frameMaximum&&!performanceMaximumFrameMicros.compare_exchange_weak(frameMaximum,engineMicros)){}if(engineMicros>=50000)++performanceHitches;
    const auto tickStarted=std::chrono::steady_clock::now();++tickCalls;
    auto measure=[](const std::chrono::steady_clock::time_point&started,std::atomic<std::uint64_t>&maximum){
        const auto elapsed=static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now()-started).count());if(elapsed>maximum.load())maximum.store(elapsed);return elapsed;
    };
    // Exactly one latency-sensitive consumer receives each engine tick. This
    // is the aggregate budget: feature-local 250us limits are not additive.
    // Pending storage work preempts the background rotation for one bounded
    // action so an observed in-game mutation can refill on the next game tick.
    // When the queue is empty, the four-way rotation continues to give every
    // background tracker several slices a second.
    static std::uint64_t phase=0,trackerPhase=0;const auto selected=phase++%4;const char*traceFeature="dispatcher";const char*traceOperation="idle";
    auto sectionStarted=std::chrono::steady_clock::now();
    if(pendingCount.load(std::memory_order_acquire)){
        ProcessOneStorageAction();measure(sectionStarted,maximumStorageTickMicros);
    }else if(selected==0){
        switch(trackerPhase++%4){case 0:traceFeature="recipes";traceOperation="player recipe scan";ProcessPlayerRecipeTracking();break;case 1:traceFeature="quests";traceOperation="player quest scan";ProcessPlayerQuestTracking();break;case 2:traceFeature="activity";traceOperation="player unlock scan";ProcessPlayerActivityDiscoveryTracking();break;default:traceFeature="activity-test";traceOperation="admin activity test action";ProcessAdminActivityTestRequests();break;}
        const auto sectionElapsed=measure(sectionStarted,maximumRecipeTickMicros);PerformanceTrace::Record(traceFeature,traceOperation,sectionElapsed);
    }else if(selected==1){
        ProcessOneCustomName();const auto sectionElapsed=measure(sectionStarted,maximumCustomNameTickMicros);PerformanceTrace::Record("storage","in-game name observation",sectionElapsed,nullptr,0,"game");
    }else{
        suppressObservedMutation=true;ProcessAdminGravestoneRequests();suppressObservedMutation=false;const auto sectionElapsed=measure(sectionStarted,maximumGravestoneTickMicros);PerformanceTrace::Record("gravestones","lifecycle or mutation",sectionElapsed);
    }
    const auto elapsed=measure(tickStarted,maximumTickMicros);auto current=performanceMaximumZanikMicros.load();while(elapsed>current&&!performanceMaximumZanikMicros.compare_exchange_weak(current,elapsed)){}if(elapsed>=1000)++slowTickCalls;
}
}

bool WasAdminStorageBuildingRemoved(uintptr_t actor){
    if(!g_Engine)return false;const auto identity=g_Engine->GetObjectIdentity(actor);if(!identity)return false;Lock lock(removedBuildingsMutex);return removedBuildingIdentities.count(identity)!=0;
}

bool ResolveAdminStorageLocation(const std::string& characterGuid,const std::string& chestId,FVector3d& location){
    return ResolveStorageLocationInternal(characterGuid,chestId,location);
}
bool ResolveAdminStorageActor(const std::string& characterGuid,const std::string& chestId,uintptr_t& actor,FVector3d& location){
    Chest chest;if(!FindChest(characterGuid,chestId,chest))return false;actor=chest.actor;
    const uintptr_t root=Mem::ReadPtr(chest.actor+Offsets::Actor_RootComponent);
    if(!root||!Mem::Readable(reinterpret_cast<void*>(root+Offsets::SceneComp_RelativeLocation),sizeof(location)))return false;
    std::memcpy(&location,reinterpret_cast<void*>(root+Offsets::SceneComp_RelativeLocation),sizeof(location));
    return std::isfinite(location.X)&&std::isfinite(location.Y)&&std::isfinite(location.Z);
}
bool ReadAdminStorageLocations(const std::string& characterGuid,std::vector<FVector3d>& locations){
    locations.clear();if(!g_Engine||characterGuid.empty())return false;
    std::unordered_map<uintptr_t,uintptr_t> owners;const int32_t count=g_Engine->GetObjectCount();
    for(int32_t i=0;i<count;++i){
        const uintptr_t object=g_Engine->GetObjectByIndex(i);if(!object||g_Engine->GetObjectName(object).rfind("Default__",0)==0)continue;
        const uintptr_t actor=Mem::ReadPtr(object+Offsets::UObject_Outer);
        if(actor&&g_Engine->IsA(object,"OwnershipComponent")&&g_Engine->ReadGuid(object+0x158)==characterGuid)owners[actor]=object;
    }
    std::unordered_set<uintptr_t> seen;
    for(int32_t i=0;i<count;++i){
        const uintptr_t inventory=g_Engine->GetObjectByIndex(i);if(!inventory||!g_Engine->IsObjectLive(inventory)||
            !g_Engine->IsA(inventory,"InventoryComponent")||g_Engine->IsA(inventory,"LinkedInventoryComponent"))continue;
        const uintptr_t actor=Mem::ReadPtr(inventory+Offsets::UObject_Outer);
        if(!actor||!owners.count(actor)||!seen.insert(actor).second||!g_Engine->IsObjectLive(actor)||WasAdminStorageBuildingRemoved(actor))continue;
        const uintptr_t root=Mem::ReadPtr(actor+Offsets::Actor_RootComponent);FVector3d location{};
        if(!root||!Mem::Readable(reinterpret_cast<void*>(root+Offsets::SceneComp_RelativeLocation),sizeof(location)))continue;
        std::memcpy(&location,reinterpret_cast<void*>(root+Offsets::SceneComp_RelativeLocation),sizeof(location));
        if(std::isfinite(location.X)&&std::isfinite(location.Y)&&std::isfinite(location.Z))locations.push_back(location);
    }
    return true;
}
std::string GetAdminStorageCustomName(uintptr_t actor){return ReadCustomNameCached(actor);}

bool ConfigureAdminStorageAutofill(const std::string& path){
    autofillStore=std::make_unique<StorageAutofillStore>(path);const bool loaded=autofillStore->Load();const auto health=autofillStore->Diagnostics();autofillPersistenceCompatibility=loaded&&health.loaded&&health.readable&&health.writable&&health.restricted&&health.lastSaveSucceeded&&health.malformedRecords==0?1:-1;return autofillPersistenceCompatibility.load()==1;
}
bool ConfigurePlayerInventorySnapshots(const std::string&path){
    playerInventorySnapshotStore=std::make_unique<PlayerInventorySnapshotStore>(path);const bool loaded=playerInventorySnapshotStore->Load();const auto health=playerInventorySnapshotStore->Diagnostics();playerInventorySnapshotPersistenceCompatibility=loaded&&health.loaded&&health.readable&&health.writable&&health.restricted&&health.lastSaveSucceeded&&health.malformedRecords==0?1:-1;return playerInventorySnapshotPersistenceCompatibility.load()==1;
}

int ExecutableBounds(struct dl_phdr_info* info,size_t,void* data){
    if(info->dlpi_name&&info->dlpi_name[0])return 0;
    auto* bounds=reinterpret_cast<uintptr_t*>(data);bool found=false;uintptr_t lowest=0,highest=0;
    for(int i=0;i<info->dlpi_phnum;++i){const auto& p=info->dlpi_phdr[i];if(p.p_type!=PT_LOAD)continue;uintptr_t begin=uintptr_t(info->dlpi_addr)+p.p_vaddr,end=begin+p.p_memsz;if(!found||begin<lowest)lowest=begin;if(!found||end>highest)highest=end;found=true;}
    bounds[0]=lowest;bounds[1]=highest;bounds[2]=found?1:0;
    return found?1:0;
}

bool InitializeAdminStorage(uintptr_t moduleBase,uintptr_t moduleEnd){
    const std::string hash=FileSha256("/proc/self/exe");const bool productionReviewed=StorageCompatibilityPolicy::IsProductionReviewed(hash);productionCompatibility=productionReviewed?1:-1;candidateOverride=!productionReviewed&&CandidateEnabled()&&StorageCompatibilityPolicy::IsCandidateOnly(hash);if(!ExpectedSha(hash)){compatibility=-1;return false;}const auto offsets=OffsetsFor(hash);
    cachedDisplayTextOffset.store((hash==CurrentSha||hash==LatestSha||hash==CandidateSha)?0x04AD5A80:((hash==LegacySha||hash==PreviousSha)?0x04ACAA80:0x04AA1A80));
    const uintptr_t serverRenameOffset=hash==CandidateSha?0x0A5A3EC0:(hash==LatestSha?0x0A5A3920:(hash==CurrentSha?0x0A5A3880:(hash==PreviousSha?0x0A593D70:0)));
    if(!earlyHookInstalled.load()||!originalEngineTick){compatibility=-6;return false;}
    for(uintptr_t o:{offsets.addItemByData,offsets.canAddItemByData,offsets.moveItem,offsets.removeFromSlot,offsets.canAddItemsByData,offsets.addItemsByData,offsets.buildingRequirements,offsets.memoryFree,offsets.engineTick,offsets.renameGet,offsets.renameSetter,offsets.renameSet,offsets.textFromString,offsets.uniqueIdDestructor,offsets.textDestructor,offsets.buildingEndPlay,offsets.setStackSize})if(moduleBase+o<moduleBase||moduleBase+o+EngineTickHookBytes>moduleEnd){compatibility=-2;return false;}
    if(serverRenameOffset&&(moduleBase+serverRenameOffset<moduleBase||moduleBase+serverRenameOffset+EngineTickHookBytes>moduleEnd)){compatibility=-2;return false;}
    const unsigned char addSig[]={0x55,0x41,0x57,0x41,0x56,0x41,0x54,0x53,0x48,0x83,0xEC,0x30};
    const unsigned char moveSig[]={0x55,0x53,0x50,0x4C,0x89,0xC0,0x48,0x89,0xFB};
    const unsigned char removeSig[]={0x55,0x41,0x57,0x41,0x56,0x53,0x48,0x83,0xEC,0x28};
    if(std::memcmp(reinterpret_cast<void*>(moduleBase+offsets.addItemByData),addSig,sizeof(addSig))||std::memcmp(reinterpret_cast<void*>(moduleBase+offsets.moveItem),moveSig,sizeof(moveSig))||std::memcmp(reinterpret_cast<void*>(moduleBase+offsets.removeFromSlot),removeSig,sizeof(removeSig))){compatibility=-3;return false;}
    const unsigned char stackCountSig[]={0x8b,0x43,0x58,0x39,0xe8,0x74,0x3d,0x89,0x43,0x5c,0x89,0x6b,0x58};bool stackCountValid=false;const uintptr_t stackBegin=moduleBase+offsets.setStackSize,stackEnd=std::min(moduleEnd,stackBegin+uintptr_t(0x500));for(uintptr_t at=stackBegin;at+sizeof(stackCountSig)<=stackEnd;++at)if(!std::memcmp(reinterpret_cast<void*>(at),stackCountSig,sizeof(stackCountSig))){stackCountValid=true;break;}if(!stackCountValid){compatibility=-3;return false;}
    const unsigned char batchSig[]={0x55,0x41,0x57,0x41,0x56,0x41,0x55,0x41,0x54};const unsigned char buildSig[]={0x41,0x57,0x41,0x56,0x41,0x54,0x53};batchCompatibility=(!std::memcmp(reinterpret_cast<void*>(moduleBase+offsets.addItemsByData),batchSig,sizeof(batchSig))&&!std::memcmp(reinterpret_cast<void*>(moduleBase+offsets.buildingRequirements),buildSig,sizeof(buildSig)))?1:-1;
    planCatalogCompatibility=PlanItemCatalog().valid?1:-1;
    if(planCatalogCompatibility.load()!=1)LogMessage("Plan item catalog assets unavailable; plan items remain visible in storage but cannot be added through catalog mode");
    addItemByData=reinterpret_cast<AddItemByDataFn>(moduleBase+offsets.addItemByData);canAddItemByData=reinterpret_cast<CanAddItemByDataFn>(moduleBase+offsets.canAddItemByData);moveItem=reinterpret_cast<MoveItemFn>(moduleBase+offsets.moveItem);removeFromSlot=reinterpret_cast<RemoveFromSlotFn>(moduleBase+offsets.removeFromSlot);setStackSize=reinterpret_cast<SetStackSizeFn>(moduleBase+offsets.setStackSize);canAddItemsByData=reinterpret_cast<CanAddItemsByDataFn>(moduleBase+offsets.canAddItemsByData);addItemsByData=reinterpret_cast<AddItemsByDataFn>(moduleBase+offsets.addItemsByData);buildingScaledRequirements=reinterpret_cast<BuildingScaledRequirementsFn>(moduleBase+offsets.buildingRequirements);memoryFree=reinterpret_cast<FMemoryFreeFn>(moduleBase+offsets.memoryFree);executeGetCustomName=reinterpret_cast<ExecuteGetCustomNameFn>(moduleBase+offsets.renameGet);executeGetSetterId=reinterpret_cast<ExecuteGetSetterIdFn>(moduleBase+offsets.renameSetter);executeSetCustomName=reinterpret_cast<ExecuteSetCustomNameFn>(moduleBase+offsets.renameSet);serverSetObjectCustomName=serverRenameOffset?reinterpret_cast<ServerSetObjectCustomNameFn>(moduleBase+serverRenameOffset):nullptr;fTextFromString=reinterpret_cast<FTextFromStringFn>(moduleBase+offsets.textFromString);renameableStaticClass=offsets.renameableStaticClass?reinterpret_cast<RenameableStaticClassFn>(moduleBase+offsets.renameableStaticClass):nullptr;doesImplementInterface=offsets.doesImplementInterface?reinterpret_cast<DoesImplementInterfaceFn>(moduleBase+offsets.doesImplementInterface):nullptr;destroyUniqueNetId=reinterpret_cast<UniqueNetIdDestructorFn>(moduleBase+offsets.uniqueIdDestructor);destroyFText=reinterpret_cast<FTextDestructorFn>(moduleBase+offsets.textDestructor);
    const unsigned char pagedValidSig[]={0x55,0x41,0x57,0x41,0x56,0x53,0x48,0x83,0xEC,0x28};
    const unsigned char pagedOffsetSig[]={0x48,0x63,0x87,0x08,0x04,0x00,0x00,0x48,0x85,0xC0};
    const unsigned char pagedCountSig[]={0x85,0xF6,0x74,0x3E,0x48,0x63,0x8F,0x08,0x04,0x00,0x00};
    const unsigned char loadoutValidSig[]={0x55,0x41,0x57,0x41,0x56,0x41,0x55,0x41,0x54,0x53,0x48,0x83,0xEC,0x18};
    const unsigned char addLoadoutItemSig[]={0x55,0x41,0x57,0x41,0x56,0x41,0x54,0x53,0x48,0x83,0xEC,0x20,0x89,0xD5,0x48,0x89,0xF3,0x49,0x89,0xFF};
    const unsigned char removeLoadoutItemSig[]={0x41,0x57,0x41,0x56,0x41,0x54,0x53,0x48,0x83,0xEC,0x28,0x48,0x89,0xF3,0x49,0x89,0xFF};
    const bool pagedBounds=offsets.pagedSlotValid&&offsets.pagedSlotOffset&&offsets.pagedSlotCount&&moduleBase+offsets.pagedSlotValid+sizeof(pagedValidSig)<=moduleEnd&&moduleBase+offsets.pagedSlotOffset+sizeof(pagedOffsetSig)<=moduleEnd&&moduleBase+offsets.pagedSlotCount+sizeof(pagedCountSig)<=moduleEnd;
    const bool loadoutBounds=offsets.loadoutSlotValid&&moduleBase+offsets.loadoutSlotValid+sizeof(loadoutValidSig)<=moduleEnd;
    const bool addLoadoutItemBounds=offsets.addLoadoutItemToSlot&&moduleBase+offsets.addLoadoutItemToSlot+sizeof(addLoadoutItemSig)<=moduleEnd;
    const bool removeLoadoutItemBounds=offsets.removeLoadoutItem&&moduleBase+offsets.removeLoadoutItem+sizeof(removeLoadoutItemSig)<=moduleEnd;
    playerInventoryPagesCompatibility=pagedBounds&&!std::memcmp(reinterpret_cast<void*>(moduleBase+offsets.pagedSlotValid),pagedValidSig,sizeof(pagedValidSig))&&!std::memcmp(reinterpret_cast<void*>(moduleBase+offsets.pagedSlotOffset),pagedOffsetSig,sizeof(pagedOffsetSig))&&!std::memcmp(reinterpret_cast<void*>(moduleBase+offsets.pagedSlotCount),pagedCountSig,sizeof(pagedCountSig))?1:-1;
    equipmentSlotCompatibility=loadoutBounds&&!std::memcmp(reinterpret_cast<void*>(moduleBase+offsets.loadoutSlotValid),loadoutValidSig,sizeof(loadoutValidSig))?1:-1;
    personalInventoryCompatibility=1;
    pagedSlotValid=playerInventoryPagesCompatibility.load()==1?reinterpret_cast<IsValidSlotForItemFn>(moduleBase+offsets.pagedSlotValid):nullptr;pagedSlotOffset=playerInventoryPagesCompatibility.load()==1?reinterpret_cast<GetPagedSlotOffsetFn>(moduleBase+offsets.pagedSlotOffset):nullptr;pagedSlotCount=playerInventoryPagesCompatibility.load()==1?reinterpret_cast<GetPagedSlotCountFn>(moduleBase+offsets.pagedSlotCount):nullptr;loadoutSlotValid=equipmentSlotCompatibility.load()==1?reinterpret_cast<IsValidSlotForItemFn>(moduleBase+offsets.loadoutSlotValid):nullptr;addLoadoutItemToSlot=equipmentSlotCompatibility.load()==1&&addLoadoutItemBounds&&!std::memcmp(reinterpret_cast<void*>(moduleBase+offsets.addLoadoutItemToSlot),addLoadoutItemSig,sizeof(addLoadoutItemSig))?reinterpret_cast<AddLoadoutItemToSlotFn>(moduleBase+offsets.addLoadoutItemToSlot):nullptr;removeLoadoutItem=equipmentSlotCompatibility.load()==1&&removeLoadoutItemBounds&&!std::memcmp(reinterpret_cast<void*>(moduleBase+offsets.removeLoadoutItem),removeLoadoutItemSig,sizeof(removeLoadoutItemSig))?reinterpret_cast<RemoveLoadoutItemFn>(moduleBase+offsets.removeLoadoutItem):nullptr;heldItemEquipmentCompatibility=addLoadoutItemToSlot&&removeLoadoutItem?1:-1;
    if(playerInventoryPagesCompatibility.load()!=1)LogMessage("Player inventory page ABI validation failed; backpack, hotbar, rune, and ammunition views remain disabled");
    if(equipmentSlotCompatibility.load()!=1)LogMessage("Loadout slot ABI validation failed; equipment detection and mutations remain disabled");
    if(!addLoadoutItemToSlot||!removeLoadoutItem)LogMessage("Held-item loadout ABI validation failed; hand-slot equip and unequip mutations remain disabled");
    const unsigned char canAddItemSig[]={0x55,0x41,0x57,0x41,0x56,0x41,0x54,0x53,0x48,0x85,0xF6};
    const unsigned char addItemSig[]={0x41,0x57,0x41,0x56,0x41,0x54,0x53,0x48,0x83,0xEC,0x28,0x48,0x85,0xF6};
    const unsigned char copyItemSig[]={0x41,0x57,0x41,0x56,0x41,0x54,0x53,0x48,0x83,0xEC,0x28,0x48,0x89,0xFB};
    const unsigned char powerSig[]={0x48,0x8B,0x87,0xB0,0x00,0x00,0x00,0x48,0x85,0xC0,0x74,0x12,0x0F,0xB6,0x8F,0xB9,0x00,0x00,0x00,0x03,0x88,0x28,0x01,0x00,0x00};
    const unsigned char canMasterworkSig[]={0x48,0x8B,0x87,0xB0,0x00,0x00,0x00,0x48,0x85,0xC0,0x74,0x14};
    const unsigned char applyMasterworkSig[]={0x41,0x57,0x41,0x56,0x53,0x48,0x83,0xEC,0x20,0x48,0x8B,0x87,0xB0,0x00,0x00,0x00};
    const unsigned char onRepMasterworkSig[]={0x55,0x41,0x57,0x41,0x56,0x41,0x55,0x41,0x54,0x53,0x48,0x83,0xEC,0x48,0x48,0x89,0xFB,0x48,0x8B,0x87,0xB0,0x00,0x00,0x00};
    const bool itemInstanceBounds=offsets.canAddItem&&offsets.addItem&&offsets.copyItem&&offsets.getItemPowerLevel&&offsets.canHaveMasterworkUpgrade&&offsets.applyMasterworkUpgrade&&offsets.onRepMasterworkUpgrades&&moduleBase+offsets.canAddItem+sizeof(canAddItemSig)<=moduleEnd&&moduleBase+offsets.addItem+sizeof(addItemSig)<=moduleEnd&&moduleBase+offsets.copyItem+sizeof(copyItemSig)<=moduleEnd&&moduleBase+offsets.getItemPowerLevel+sizeof(powerSig)<=moduleEnd&&moduleBase+offsets.canHaveMasterworkUpgrade+sizeof(canMasterworkSig)<=moduleEnd&&moduleBase+offsets.applyMasterworkUpgrade+sizeof(applyMasterworkSig)<=moduleEnd&&moduleBase+offsets.onRepMasterworkUpgrades+sizeof(onRepMasterworkSig)<=moduleEnd;
    const bool itemInstanceReady=itemInstanceBounds&&!std::memcmp(reinterpret_cast<void*>(moduleBase+offsets.canAddItem),canAddItemSig,sizeof(canAddItemSig))&&!std::memcmp(reinterpret_cast<void*>(moduleBase+offsets.addItem),addItemSig,sizeof(addItemSig))&&!std::memcmp(reinterpret_cast<void*>(moduleBase+offsets.copyItem),copyItemSig,sizeof(copyItemSig))&&!std::memcmp(reinterpret_cast<void*>(moduleBase+offsets.getItemPowerLevel),powerSig,sizeof(powerSig))&&!std::memcmp(reinterpret_cast<void*>(moduleBase+offsets.canHaveMasterworkUpgrade),canMasterworkSig,sizeof(canMasterworkSig))&&!std::memcmp(reinterpret_cast<void*>(moduleBase+offsets.applyMasterworkUpgrade),applyMasterworkSig,sizeof(applyMasterworkSig))&&!std::memcmp(reinterpret_cast<void*>(moduleBase+offsets.onRepMasterworkUpgrades),onRepMasterworkSig,sizeof(onRepMasterworkSig));
    itemInstanceCompatibility=itemInstanceReady?1:-1;canAddItem=itemInstanceReady?reinterpret_cast<CanAddItemFn>(moduleBase+offsets.canAddItem):nullptr;addItem=itemInstanceReady?reinterpret_cast<AddItemFn>(moduleBase+offsets.addItem):nullptr;copyItem=itemInstanceReady?reinterpret_cast<CopyItemFn>(moduleBase+offsets.copyItem):nullptr;getItemPowerLevel=itemInstanceReady?reinterpret_cast<GetItemPowerLevelFn>(moduleBase+offsets.getItemPowerLevel):nullptr;canHaveMasterworkUpgrade=itemInstanceReady?reinterpret_cast<CanHaveMasterworkUpgradeFn>(moduleBase+offsets.canHaveMasterworkUpgrade):nullptr;applyMasterworkUpgrade=itemInstanceReady?reinterpret_cast<ApplyMasterworkUpgradeFn>(moduleBase+offsets.applyMasterworkUpgrade):nullptr;onRepMasterworkUpgrades=itemInstanceReady?reinterpret_cast<OnRepMasterworkUpgradesFn>(moduleBase+offsets.onRepMasterworkUpgrades):nullptr;
    if(!itemInstanceReady)LogMessage("Exact item-instance copy and masterwork power selection ABI validation failed; those operations remain disabled");
    const unsigned char renameGetSig[]={0x41,0x56,0x53,0x48,0x83,0xec,0x28};
    const unsigned char renameSetterSig[]={0x41,0x57,0x41,0x56,0x41,0x55,0x41,0x54,0x53,0x48,0x81,0xec,0xa0,0x00,0x00,0x00};
    const unsigned char renameSetSig[]={0x55,0x41,0x57,0x41,0x56,0x41,0x55,0x41,0x54,0x53,0x48,0x83,0xec,0x78};
    const unsigned char fTextUtf16Sig[]={0x55,0x41,0x57,0x41,0x56,0x53,0x48,0x83,0xec,0x18,0x48,0x89,0xfb,0x44,0x8b,0x76,0x08};
    const unsigned char serverRenameSig[]={0x41,0x57,0x41,0x56,0x53,0x48,0x83,0xec,0x10};
    const bool serverRenameValid=!serverRenameOffset||!std::memcmp(reinterpret_cast<void*>(moduleBase+serverRenameOffset),serverRenameSig,sizeof(serverRenameSig));
    const unsigned char renameableClassSig[]={0x48,0x8b,0x05};const unsigned char implementsInterfaceSig[]={0x48,0x85,0xff,0x74};
    const bool interfaceCheckReady=!offsets.renameableStaticClass||(!std::memcmp(reinterpret_cast<void*>(moduleBase+offsets.renameableStaticClass),renameableClassSig,sizeof(renameableClassSig))&&!std::memcmp(reinterpret_cast<void*>(moduleBase+offsets.doesImplementInterface),implementsInterfaceSig,sizeof(implementsInterfaceSig)));
    renameCompatibility=(!std::memcmp(reinterpret_cast<void*>(moduleBase+offsets.renameGet),renameGetSig,sizeof(renameGetSig))&&!std::memcmp(reinterpret_cast<void*>(moduleBase+offsets.renameSetter),renameSetterSig,sizeof(renameSetterSig))&&!std::memcmp(reinterpret_cast<void*>(moduleBase+offsets.renameSet),renameSetSig,sizeof(renameSetSig))&&!std::memcmp(reinterpret_cast<void*>(moduleBase+offsets.textFromString),fTextUtf16Sig,sizeof(fTextUtf16Sig))&&serverRenameValid&&interfaceCheckReady)?1:-1;
    if(renameCompatibility.load()!=1)LogMessage("Storage rename ABI validation failed for this game build; rename remains disabled");
    const unsigned char ownershipSig[]={0x55,0x41,0x57,0x41,0x56,0x41,0x55,0x41,0x54,0x53,0x48,0x83,0xEC,0x58};
    const unsigned char ownershipGuidField[]={0x48,0x8d,0x9f,0x58,0x01,0x00,0x00};
    const unsigned char ownershipChangeGate[]={0x41,0x80,0xbf,0x68,0x01,0x00,0x00,0x00};
    const bool ownershipBounds=offsets.setOwningCharacterGuid&&moduleBase+offsets.setOwningCharacterGuid>=moduleBase&&moduleBase+offsets.setOwningCharacterGuid+sizeof(ownershipSig)<=moduleEnd;
    bool ownershipGuidFieldFound=false,ownershipChangeGateFound=false;const uintptr_t ownershipBegin=moduleBase+offsets.setOwningCharacterGuid,ownershipEnd=std::min(moduleEnd,ownershipBegin+uintptr_t(0x100));if(ownershipBounds)for(uintptr_t at=ownershipBegin;at<ownershipEnd;++at){if(at+sizeof(ownershipGuidField)<=ownershipEnd&&!std::memcmp(reinterpret_cast<void*>(at),ownershipGuidField,sizeof(ownershipGuidField)))ownershipGuidFieldFound=true;if(at+sizeof(ownershipChangeGate)<=ownershipEnd&&!std::memcmp(reinterpret_cast<void*>(at),ownershipChangeGate,sizeof(ownershipChangeGate)))ownershipChangeGateFound=true;}
    const unsigned char linkableSetterSig[]={0x41,0x56,0x53,0x48,0x83,0xEC,0x18,0x89,0xF3,0x38,0x9F,0xF8,0x00,0x00,0x00,0x0F,0x84,0xAE,0x00,0x00,0x00};
    const bool linkableSetterReady=offsets.setIsLinkable&&moduleBase+offsets.setIsLinkable>=moduleBase&&moduleBase+offsets.setIsLinkable+sizeof(linkableSetterSig)<=moduleEnd&&!std::memcmp(reinterpret_cast<void*>(moduleBase+offsets.setIsLinkable),linkableSetterSig,sizeof(linkableSetterSig));
    setIsLinkable=linkableSetterReady?reinterpret_cast<SetIsLinkableFn>(moduleBase+offsets.setIsLinkable):nullptr;
    ownershipCompatibility=ownershipBounds&&!std::memcmp(reinterpret_cast<void*>(ownershipBegin),ownershipSig,sizeof(ownershipSig))&&ownershipGuidFieldFound&&ownershipChangeGateFound&&linkableSetterReady?1:-1;
    setOwningCharacterGuid=ownershipCompatibility.load()==1?reinterpret_cast<SetOwningCharacterGuidFn>(moduleBase+offsets.setOwningCharacterGuid):nullptr;
    if(ownershipCompatibility.load()!=1)LogMessage("Storage ownership or linked-inventory migration ABI validation failed for this game build; ownership transfer remains disabled");
    autofillBuildingCaptureCompatibility=InitializeAutofillBuildingCapture(hash,moduleBase,moduleEnd)?1:-1;
    if(autofillBuildingCaptureCompatibility.load()!=1)LogMessage("Authoritative linked-building consumption capture unavailable; storage autofill retains its bounded polling fallback");else LogMessage("Authoritative linked-building consumption capture armed for storage autofill");
    catalogCompatibility=1;unlockCatalogCompatibility=UnlockCatalogAssetsValid()?1:-1;const auto& buildCatalog=AuthoritativeBuildingCatalog();buildingTaxonomyCompatibility=buildCatalog.valid?1:-1;buildingIconsCompatibility=buildCatalog.iconsValid?1:-1;buildingRequirementsCompatibility=batchCompatibility.load()==1?1:-1;craftingRecipeCompatibility=batchCompatibility.load()==1?1:-1;if(unlockCatalogCompatibility.load()!=1)LogMessage("Vestige and mount unlock catalog assets unavailable; ordinary item catalog remains enabled");if(buildingTaxonomyCompatibility.load()!=1)LogMessage("Authoritative building catalogue is invalid; crafting recipes remain enabled");if(buildingIconsCompatibility.load()!=1)LogMessage("One or more authoritative building catalogue icons are unavailable");removalTrackingCompatibility=InstallBuildingEndPlayHook(moduleBase,moduleEnd,offsets.buildingEndPlay)?1:-1;if(removalTrackingCompatibility.load()!=1)LogMessage("Immediate storage removal tracking unavailable; storage editing remains enabled with ordinary actor reconciliation");nativeMutationCaptureCompatibility=InstallObservedMutationHooks(moduleBase,moduleEnd,offsets)?1:-1;if(nativeMutationCaptureCompatibility.load()!=1)LogMessage("Native in-game inventory mutation attribution unavailable; website storage editing remains enabled");else LogMessage("Native in-game inventory mutation attribution armed");autofillRpcCaptureCompatibility=InitializeAutofillRpcCapture(hash,moduleBase,moduleEnd)?1:-1;if(autofillRpcCaptureCompatibility.load()!=1)LogMessage("Authoritative full-stack and split-stack withdrawal capture unavailable; storage autofill retains its bounded polling fallback");else LogMessage("Authoritative full-stack and split-stack withdrawal capture armed for storage autofill");autofillNormalizationCompatibility=1;autofillReplicationWakeCompatibility=InitializeAutofillReplicationWake(moduleBase,moduleEnd)?1:-1;if(autofillReplicationWakeCompatibility.load()!=1)LogMessage("Immediate storage replication wake unavailable; autofill retains normal actor replication");else LogMessage("Immediate storage replication wake armed for successful autofills");dispatcherCompatibility=1;compatibility=1;return true;
}
bool InitializeAdminStorageHookEarly(){
    if(earlyHookInstalled.load())return true;
    std::string hash=FileSha256("/proc/self/exe");if(!ExpectedDispatcherSha(hash)){LogMessage("Admin storage dispatcher fingerprint mismatch: "+hash);return false;}
    const bool currentDispatcher=hash=="49512c0ee933c3ee4dda5fb5c2f0a25e66d7f26e34795c45b91b5c1d12b61b0c"||hash==LegacySha||hash==PreviousSha||hash==CurrentSha||hash==LatestSha||hash==CandidateSha;
    const uintptr_t dispatcherOffset=(hash==CurrentSha||hash==LatestSha||hash==CandidateSha)?0x07ACAF90:(currentDispatcher?0x07ABFE00:EngineTickOffset);
    uintptr_t bounds[3]={0,0,0};dl_iterate_phdr(&ExecutableBounds,bounds);if(!bounds[2]||bounds[0]+dispatcherOffset+EngineTickHookBytes>bounds[1]){LogMessage("Admin storage dispatcher executable bounds unavailable");return false;}
    auto* target=reinterpret_cast<unsigned char*>(bounds[0]+dispatcherOffset);
    const unsigned char expected[EngineTickHookBytes]={0x53,0x48,0x83,0xEC,0x10,0x8B,0x47,0x08,0xA8,0x10,0x0F,0x85,0x92,0x00,0x00,0x00};
    if(std::memcmp(target,expected,sizeof(expected))){char message[160]{};std::snprintf(message,sizeof(message),"Admin storage dispatcher signature mismatch base=0x%llx target=0x%llx bytes=%02x%02x%02x%02x",static_cast<unsigned long long>(bounds[0]),static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(target)),target[0],target[1],target[2],target[3]);LogMessage(message);return false;}
    auto* trampoline=static_cast<unsigned char*>(mmap(nullptr,64,PROT_READ|PROT_WRITE|PROT_EXEC,MAP_PRIVATE|MAP_ANONYMOUS,-1,0));if(trampoline==MAP_FAILED)return false;
    std::memcpy(trampoline,target,10);
    // Relocate the original JNE: false continues at +16; true returns through
    // the original function's shared epilogue at +0xA2.
    trampoline[10]=0x0F;trampoline[11]=0x84;std::int32_t skip=12;std::memcpy(trampoline+12,&skip,4);
    WriteJump(trampoline+16,reinterpret_cast<uintptr_t>(target+0xA2));
    WriteJump(trampoline+28,reinterpret_cast<uintptr_t>(target+EngineTickHookBytes));
    originalEngineTick=reinterpret_cast<EngineTickFn>(trampoline);
    long pageSize=sysconf(_SC_PAGESIZE);uintptr_t page=reinterpret_cast<uintptr_t>(target)&~(uintptr_t(pageSize)-1);if(mprotect(reinterpret_cast<void*>(page),size_t(pageSize),PROT_READ|PROT_WRITE|PROT_EXEC)!=0){originalEngineTick=nullptr;munmap(trampoline,64);return false;}
    unsigned char patch[EngineTickHookBytes];std::memset(patch,0x90,sizeof(patch));WriteJump(patch,reinterpret_cast<uintptr_t>(&HookEngineTick));std::memcpy(target,patch,sizeof(patch));__builtin___clear_cache(reinterpret_cast<char*>(target),reinterpret_cast<char*>(target+sizeof(patch)));mprotect(reinterpret_cast<void*>(page),size_t(pageSize),PROT_READ|PROT_EXEC);earlyHookInstalled=true;dispatcherCompatibility=1;return true;
}
void ProcessAdminStorageRequests(){ProcessOneStorageAction();}
void FinalizeAdminStorageOwnershipTransfers(){
    std::shared_ptr<Request> request;
    {std::lock_guard<std::mutex> lock(jobsMutex);for(const auto&entry:jobs)if(entry.second->state.load()==JobState::Persisting){request=entry.second;break;}}
    if(!request)return;
    bool transferred=false;
    if(autofillStore&&autofillStore->Transfer(request->oldOwnerGuid,request->oldChestId,request->newOwnerGuid,request->newChestId,transferred)){
        if(transferred){++ownershipSnapshotMigrations;std::lock_guard<std::mutex> lock(knownAutofillStorageMutex);for(auto&entry:knownAutofillStorages)if(entry.second.ownerGuid==request->oldOwnerGuid&&entry.second.chestId==request->oldChestId){entry.second.ownerGuid=request->newOwnerGuid;entry.second.chestId=request->newChestId;}}
        if(request->transferWasLinked)++ownershipLinkedMigrations;++ownershipTransfers;FinishJob(*request,true,request->resultCode.load()==2?2:1);return;
    }
    autofillPersistenceCompatibility=-1;++ownershipFailures;request->resultCode=-8;request->rollbackIndex=request->rollback.size();request->state=JobState::RollingBack;request->updatedUnix=UnixNow();
    std::lock_guard<std::mutex> lock(jobsMutex);const auto head=requestHead.load(std::memory_order_relaxed),tail=requestTail.load(std::memory_order_acquire);if(head-tail>=QueueCapacity){request->state=JobState::Persisting;return;}requestRing[head%QueueCapacity]=request.get();requestHead.store(head+1,std::memory_order_release);
}
std::string NewRequestId(){static std::atomic<std::uint64_t> sequence{1};std::uint64_t a=std::uint64_t(std::chrono::steady_clock::now().time_since_epoch().count()),b=sequence.fetch_add(1);return HexHash(a)+HexHash(b);}
NativeAction BaseAction(const StorageSnapshot&s,const SnapshotChest&chest,OperationFamily family){NativeAction a;a.family=chest.id=="0000000000000002"?OperationFamily::Equipment:family;a.actor=chest.actor;a.actorIdentity=chest.actorIdentity;a.controller=chest.mutationController?chest.mutationController:s.adminController;a.controllerIdentity=g_Engine&&a.controller?g_Engine->GetObjectIdentity(a.controller):0;return a;}
bool ValidDestinationSlot(const SnapshotChest&destination,const SnapshotItem&item,int backingSlot){
    if(g_Engine&&g_Engine->IsA(destination.inventory,"LoadoutComponent"))return loadoutSlotValid&&loadoutSlotValid(reinterpret_cast<void*>(destination.inventory),reinterpret_cast<void*>(item.object),backingSlot,nullptr);
    if(g_Engine&&g_Engine->IsA(destination.inventory,"PagedInventoryWithLogComponent"))return pagedSlotValid&&pagedSlotValid(reinterpret_cast<void*>(destination.inventory),reinterpret_cast<void*>(item.object),backingSlot,nullptr);
    return true;
}
bool AddMovePlan(Request&r,const StorageSnapshot&s,const SnapshotChest&destination,const std::vector<std::pair<const SnapshotChest*,const SnapshotItem*>>&items,std::string&error,const std::vector<int>*quantities=nullptr){
    struct VirtualSlot{uintptr_t data=0;int quantity=0,maxStack=1;};std::vector<VirtualSlot> slots(size_t(destination.totalSlots));for(const auto&item:destination.items){const int display=DisplaySlot(item);if(display>=0&&display<destination.totalSlots)slots[size_t(display)]={item.data,item.quantity,item.maxStack};}
    for(size_t index=0;index<items.size();++index){const auto&entry=items[index];const auto&source=*entry.first;const auto*item=entry.second;int remaining=quantities?(*quantities)[index]:item->quantity;for(int phase=0;phase<2&&remaining>0;++phase)for(int slot=0;slot<destination.totalSlots&&remaining>0;++slot){const int backing=BackingSlot(destination,slot);if(!ValidDestinationSlot(destination,*item,backing))continue;auto&target=slots[size_t(slot)];if((phase==0&&(target.data!=item->data||target.quantity>=target.maxStack))||(phase==1&&target.data))continue;const int amount=std::min(remaining,phase==0?target.maxStack-target.quantity:item->maxStack);if(amount<=0)continue;NativeAction action=BaseAction(s,source,OperationFamily::BatchMovement);action.kind=ActionKind::Move;action.source=source.inventory;action.sourceIdentity=source.inventoryIdentity;action.destination=destination.inventory;action.destinationIdentity=destination.inventoryIdentity;action.item=item->object;action.itemIdentity=item->objectIdentity;action.itemData=item->data;action.itemDataIdentity=item->dataIdentity;action.sourceSlot=item->slot;action.destinationSlot=backing;action.quantity=amount;action.expectedQuantity=item->quantity-(quantities?(*quantities)[index]:item->quantity)+remaining;r.actions.push_back(action);if(!target.data){target.data=item->data;target.maxStack=item->maxStack;}target.quantity+=amount;remaining-=amount;}if(remaining){error="The destination storage does not have enough valid space for the complete selection";return false;}}
    return true;
}
bool AddMatchingDepositPlan(Request&r,const StorageSnapshot&s,const SnapshotChest&source,const std::vector<const SnapshotChest*>&destinations,const std::vector<const SnapshotItem*>&sourceItems,const std::unordered_set<int>&protectedSlots,bool skipHotbar,std::string&error,const char*successMessage,const char*noMatchingDestinationError=nullptr){
    constexpr std::size_t MaxDepositActions=512;
    struct VirtualSlot{const SnapshotChest*chest=nullptr;int slot=-1;uintptr_t data=0;int quantity=0,maxStack=1;};std::vector<VirtualSlot>targets;
    bool matchingDestination=false;
    for(const auto*destination:destinations){std::vector<const SnapshotItem*>bySlot(size_t(destination->totalSlots));for(const auto&item:destination->items){const int display=DisplaySlot(item);if(display>=0&&display<destination->totalSlots)bySlot[size_t(display)]=&item;}for(int slot=0;slot<destination->totalSlots;++slot){const auto*item=bySlot[size_t(slot)];targets.push_back({destination,BackingSlot(*destination,slot),item?item->data:0,item?item->quantity:0,item?item->maxStack:1});}}
    for(const auto*sourceItem:sourceItems){const auto&item=*sourceItem;const int display=DisplaySlot(item);if(skipHotbar&&display>=source.hotbarStart&&display<source.hotbarStart+source.hotbarCount)continue;if(protectedSlots.count(display))continue;int remaining=item.quantity;
        std::unordered_set<const SnapshotChest*>eligible;for(const auto*destination:destinations)if(std::any_of(destination->items.begin(),destination->items.end(),[&](const auto&existing){return existing.data==item.data;}))eligible.insert(destination);matchingDestination=matchingDestination||!eligible.empty();
        for(int phase=0;phase<2&&remaining>0;++phase)for(auto&target:targets){if(remaining<=0)break;if(!eligible.count(target.chest))continue;if((phase==0&&(target.data!=item.data||target.quantity>=target.maxStack))||(phase==1&&target.data))continue;const int amount=std::min(remaining,phase==0?target.maxStack-target.quantity:item.maxStack);if(amount<=0)continue;if(r.actions.size()>=MaxDepositActions){error="The inventory deposit requires too many storage moves";return false;}NativeAction action=BaseAction(s,source,OperationFamily::BatchMovement);action.kind=ActionKind::Move;action.source=source.inventory;action.sourceIdentity=source.inventoryIdentity;action.destination=target.chest->inventory;action.destinationIdentity=target.chest->inventoryIdentity;action.item=item.object;action.itemIdentity=item.objectIdentity;action.itemData=item.data;action.itemDataIdentity=item.dataIdentity;action.sourceSlot=item.slot;action.destinationSlot=target.slot;action.quantity=amount;action.expectedQuantity=remaining;r.actions.push_back(action);if(!target.data){target.data=item.data;target.maxStack=item.maxStack;}target.quantity+=amount;remaining-=amount;}
    }
    if(r.actions.empty()){error=!matchingDestination&&noMatchingDestinationError?noMatchingDestinationError:"No eligible items have space in a destination with matching stacks";return false;}r.successMessage=successMessage;return true;
}
bool AddInventoryDepositPlan(Request&r,const StorageSnapshot&s,const SnapshotChest&inventory,const std::string&targetGuid,std::string&error){
    std::unordered_set<int>protectedSlots;StorageAutofillTemplate desired;if(autofillStore&&autofillStore->Find(targetGuid,inventory.id,desired))for(const auto&saved:desired.items)protectedSlots.insert(saved.slot);
    std::vector<const SnapshotChest*>destinations;for(const auto&destination:s.chests)if(!IsPlayerInventoryContainerId(destination.id))destinations.push_back(&destination);
    std::vector<const SnapshotItem*>items;for(const auto&item:inventory.items)items.push_back(&item);
    return AddMatchingDepositPlan(r,s,inventory,destinations,items,protectedSlots,true,error,"Matching inventory items deposited");
}
bool BuildMutationPlan(const std::string&operation,const std::string&adminGuid,const std::string&targetGuid,const std::string&chestId,const std::string&itemId,const std::string&catalogId,int quantity,const std::string&destinationChestId,int destinationSlot,const std::vector<std::string>&itemIds,const std::vector<int>&quantities,const std::string&customName,const std::string&newOwnerGuid,int powerLevel,bool prioritizePersonalChest,Request&r,std::string&error){
    const bool playerInventory=IsPlayerInventoryContainerId(chestId),storageDeposit=operation=="deposit_inventory"||operation=="move_to_storages_batch"||operation=="move_to_storage_batch"||operation=="move_equipment_batch"||operation=="withdraw";auto snapshot=storageDeposit?BuildInventoryDepositSnapshot(targetGuid,adminGuid):(playerInventory?BuildPlayerInventorySnapshot(targetGuid,adminGuid):BuildStorageSnapshot(targetGuid,adminGuid,true));if(!snapshot){error=playerInventory?"Player inventory snapshot preparation failed":"Storage snapshot preparation failed";return false;}if(playerInventory&&(!snapshot->targetFound||!snapshot->targetSpawned||!snapshot->targetController)){error="The selected character must be online";return false;}r.expectedWorld=snapshot->world;r.expectedWorldIdentity=snapshot->worldIdentity;const SnapshotChest* chest=SnapshotFindChest(*snapshot,chestId);if(!chest){error=playerInventory?(chestId=="0000000000000002"?"Character equipment is unavailable":chestId=="0000000000000003"?"Character rune pouch is unavailable":chestId=="0000000000000004"?"Character ammunition pouch is unavailable":chestId=="0000000000000005"?"Character personal chest is unavailable":"Character inventory is unavailable"):"Storage container is no longer available";return false;}const SnapshotChest* destination=destinationChestId.empty()?nullptr:SnapshotFindChest(*snapshot,destinationChestId);auto selected=[&](){std::vector<const SnapshotItem*> out;std::unordered_set<std::string> unique;for(const auto&id:itemIds){if(!unique.insert(id).second)return std::vector<const SnapshotItem*>{};auto* value=SnapshotFindItem(*chest,id);if(!value)return std::vector<const SnapshotItem*>{};out.push_back(value);}return out;};auto moveEntries=[&](const std::vector<const SnapshotItem*>&values,std::vector<std::pair<const SnapshotChest*,const SnapshotItem*>>&out){const auto*inventory=SnapshotFindChest(*snapshot,"0000000000000001");for(auto*item:values){if(chestId=="0000000000000002"&&(item->slot==HeldRightBackingSlot||item->slot==HeldLeftBackingSlot)){if(!inventory){error="Character inventory is unavailable";return false;}const auto held=std::find_if(inventory->items.begin(),inventory->items.end(),[&](const auto&candidate){return candidate.objectIdentity==item->objectIdentity;});if(held==inventory->items.end()){error="The held item changed before the request was queued";return false;}out.push_back({inventory,&*held});}else out.push_back({chest,item});}return true;};
    const bool personalMove=operation=="move_batch"&&((chestId=="0000000000000001"&&destinationChestId=="0000000000000005")||(chestId=="0000000000000005"&&destinationChestId=="0000000000000001")),playerSlotMove=operation=="slot_move"&&destinationChestId==chestId;
    if(playerInventory&&(operation=="rename"||operation=="transfer_ownership"||(operation=="slot_move"&&!playerSlotMove)||operation=="organize_batch"||(operation=="move_batch"&&!personalMove))){error="Invalid player inventory operation";return false;}
    if(playerInventory&&(operation=="take"||operation=="take_batch")&&targetGuid==adminGuid){error="This is already your linked character's inventory";return false;}
    if(operation=="rename"){if(renameCompatibility.load()!=1){error="Storage naming is unavailable for this game build";return false;}if(familyCircuitBroken[size_t(OperationFamily::Rename)]){error="Storage naming was disabled after a failed or slow native call";return false;}if(!Unicode::Utf8ToUtf16(customName,r.renameUtf16.data(),r.renameUtf16.size(),&r.renameUtf16Units)){error="Storage name is not valid UTF-8 or is longer than 64 UTF-16 units";return false;}std::memcpy(r.renameValue.data(),customName.data(),customName.size());NativeAction a=BaseAction(*snapshot,*chest,OperationFamily::Rename);for(ActionKind kind:{ActionKind::RenameConstruct,ActionKind::RenameSetterIdentity,ActionKind::RenameSet,ActionKind::RenameVerify,ActionKind::RenameDestroyIdentity,ActionKind::RenameDestroyText}){a.kind=kind;r.actions.push_back(a);}r.successMessage="Storage renamed";return true;}
    if(operation=="transfer_ownership"){
        if(ownershipCompatibility.load()!=1||autofillPersistenceCompatibility.load()!=1||!setOwningCharacterGuid||!setIsLinkable){error="Storage ownership transfer is unavailable for this game build";return false;}
        if(targetGuid==newOwnerGuid){error="The selected character already owns this storage";return false;}
        std::array<std::uint32_t,4> oldParts{},newParts{};if(!ParseCharacterGuid(targetGuid,oldParts)||!ParseCharacterGuid(newOwnerGuid,newParts)){error="The selected owner identity is invalid";return false;}
        NativeAction linkAction=BaseAction(*snapshot,*chest,OperationFamily::Ownership);linkAction.destination=chest->linkedComponent;linkAction.destinationIdentity=chest->linkedComponentIdentity;
        if(chest->linked){linkAction.kind=ActionKind::LinkableSet;linkAction.linkable=false;r.actions.push_back(linkAction);linkAction.kind=ActionKind::LinkableVerify;r.actions.push_back(linkAction);}
        NativeAction action=BaseAction(*snapshot,*chest,OperationFamily::Ownership);action.source=chest->ownership;action.sourceIdentity=chest->ownershipIdentity;action.kind=ActionKind::OwnershipSet;action.ownerGuid=newParts;r.actions.push_back(action);action.kind=ActionKind::OwnershipVerify;r.actions.push_back(action);
        if(chest->linked){linkAction.kind=ActionKind::LinkableSet;linkAction.linkable=true;r.actions.push_back(linkAction);linkAction.kind=ActionKind::LinkableVerify;r.actions.push_back(linkAction);}
        NativeAction restore=action;restore.family=OperationFamily::Rollback;restore.ownerGuid=oldParts;
        if(chest->linked){linkAction.family=OperationFamily::Rollback;linkAction.kind=ActionKind::LinkableVerify;linkAction.linkable=true;r.rollback.push_back(linkAction);linkAction.kind=ActionKind::LinkableSet;r.rollback.push_back(linkAction);}
        restore.kind=ActionKind::OwnershipVerify;r.rollback.push_back(restore);restore.kind=ActionKind::OwnershipSet;r.rollback.push_back(restore);
        if(chest->linked){linkAction.kind=ActionKind::LinkableVerify;linkAction.linkable=false;r.rollback.push_back(linkAction);linkAction.kind=ActionKind::LinkableSet;r.rollback.push_back(linkAction);}
        r.oldOwnerGuid=targetGuid;r.newOwnerGuid=newOwnerGuid;r.oldChestId=chestId;r.newChestId=ChestId(chest->actor,chest->type,newOwnerGuid);r.transferWasLinked=chest->linked;r.successMessage="Storage ownership transferred";return true;
    }
    const SnapshotChest* addDestination=chestId=="0000000000000002"?SnapshotFindChest(*snapshot,"0000000000000001"):chest;
    if(operation=="withdraw"){
        if(chestId!="0000000000000001"&&chestId!="0000000000000005"){error="Withdraw is only available for Inventory or Personal Chest";return false;}
        std::vector<const SnapshotChest*>sources;const auto*personal=SnapshotFindChest(*snapshot,"0000000000000005");
        if(chestId=="0000000000000001"&&prioritizePersonalChest&&personal)sources.push_back(personal);
        for(const auto&source:snapshot->chests)if(!IsPlayerInventoryContainerId(source.id))sources.push_back(&source);
        if(chestId=="0000000000000001"&&!prioritizePersonalChest&&personal)sources.push_back(personal);
        std::vector<std::pair<const SnapshotChest*,const SnapshotItem*>>moves;std::vector<int>amounts;int remaining=quantity;std::int64_t total=0;
        for(const auto*source:sources)for(const auto&candidate:source->items)if(CatalogId(candidate.key)==catalogId){total+=candidate.quantity;if(remaining>0){const int amount=std::min(remaining,candidate.quantity);moves.push_back({source,&candidate});amounts.push_back(amount);remaining-=amount;}}
        if(total<quantity){error="Only "+std::to_string(total)+" of the selected item remains available";return false;}
        if(moves.empty()||remaining){error="The selected item is no longer available";return false;}
        if(moves.size()>512){error="The withdrawal requires too many source stacks";return false;}
        if(!AddMovePlan(r,*snapshot,*chest,moves,error,&amounts))return false;r.successMessage=chestId=="0000000000000001"?"Items withdrawn to Inventory":"Items withdrawn to Personal Chest";
    }
    else if(operation=="deposit_inventory"){if(chestId!="0000000000000001"){error="Inventory deposit is only available for the backpack";return false;}if(!AddInventoryDepositPlan(r,*snapshot,*chest,targetGuid,error))return false;}
    else if(operation=="deposit_personal"){if(chestId!="0000000000000001"){error="Personal chest deposit is only available for the backpack";return false;}const auto*personal=SnapshotFindChest(*snapshot,"0000000000000005");if(!personal){error="Character personal chest is unavailable";return false;}std::unordered_set<int>protectedSlots;StorageAutofillTemplate desired;if(autofillStore&&autofillStore->Find(targetGuid,chest->id,desired))for(const auto&saved:desired.items)protectedSlots.insert(saved.slot);std::vector<const SnapshotItem*>items;for(const auto&item:chest->items)items.push_back(&item);if(!AddMatchingDepositPlan(r,*snapshot,*chest,{personal},items,protectedSlots,true,error,"Matching inventory items deposited to personal chest"))return false;}
    else if(operation=="move_to_storages_batch"){if(chestId!="0000000000000001"&&chestId!="0000000000000005"){error="Only backpack or personal chest items can move to storages";return false;}auto items=selected();if(items.size()!=itemIds.size()){error="One or more selected items changed";return false;}std::vector<const SnapshotChest*>destinations;for(const auto&candidate:snapshot->chests)if(!IsPlayerInventoryContainerId(candidate.id))destinations.push_back(&candidate);if(!AddMatchingDepositPlan(r,*snapshot,*chest,destinations,items,{},false,error,"Selected items moved to matching storages","No storage contains an existing stack for the selected items"))return false;}
    else if(operation=="move_to_storage_batch"){if(chestId!="0000000000000001"&&chestId!="0000000000000002"&&chestId!="0000000000000005"){error="Only backpack, equipment, or personal chest items can move to a storage";return false;}if(!destination||IsPlayerInventoryContainerId(destination->id)){error="Destination storage is no longer available";return false;}auto values=selected();if(values.size()!=itemIds.size()){error="One or more selected items changed";return false;}std::vector<std::pair<const SnapshotChest*,const SnapshotItem*>>items;if(!moveEntries(values,items)||!AddMovePlan(r,*snapshot,*destination,items,error))return false;if(chestId=="0000000000000002")for(auto&action:r.actions)action.family=OperationFamily::Equipment;r.successMessage="Selected items moved to storage";}
    else if(operation=="move_equipment_batch"){if(chestId!="0000000000000002"||!destination||destination->id!="0000000000000005"){error="Equipment can only move directly to the personal chest";return false;}auto values=selected();if(values.size()!=itemIds.size()){error="One or more selected items changed";return false;}std::vector<std::pair<const SnapshotChest*,const SnapshotItem*>>items;if(!moveEntries(values,items)||!AddMovePlan(r,*snapshot,*destination,items,error))return false;for(auto&action:r.actions)action.family=OperationFamily::Equipment;r.successMessage="Selected equipment moved to personal chest";}
    else if(operation=="equip"){
        auto*item=SnapshotFindItem(*chest,itemId);const auto*equipment=SnapshotFindChest(*snapshot,"0000000000000002");if(!item||!item->equippable){error="This item cannot be equipped";return false;}if(!equipment||!loadoutSlotValid){error="Character equipment is unavailable";return false;}
        std::unordered_set<int>occupied;for(const auto&value:equipment->items)occupied.insert(value.slot);int target=-1,fallback=-1;for(int slot=0;slot<equipment->totalSlots;++slot)if(loadoutSlotValid(reinterpret_cast<void*>(equipment->inventory),reinterpret_cast<void*>(item->object),BackingSlot(*equipment,slot),nullptr)){if(fallback<0)fallback=slot;if(!occupied.count(BackingSlot(*equipment,slot))){target=slot;break;}}if(target<0)target=fallback;if(target<0){error="No compatible equipment slot is available";return false;}
        NativeAction a=BaseAction(*snapshot,*chest,OperationFamily::Equipment);a.family=OperationFamily::Equipment;a.source=chest->inventory;a.sourceIdentity=chest->inventoryIdentity;a.destination=equipment->inventory;a.destinationIdentity=equipment->inventoryIdentity;a.item=item->object;a.itemIdentity=item->objectIdentity;a.itemData=item->data;a.itemDataIdentity=item->dataIdentity;a.sourceSlot=item->slot;a.destinationSlot=BackingSlot(*equipment,target);a.quantity=item->quantity;a.expectedQuantity=item->quantity;
        const bool heldSlot=a.destinationSlot==HeldRightBackingSlot||a.destinationSlot==HeldLeftBackingSlot;
        if(heldSlot){
            if(chest->id!="0000000000000001"){error="Move this held item to the character inventory before equipping it";return false;}
            if(!addLoadoutItemToSlot){error="Held-item equipment is unavailable for this game build";return false;}
            a.kind=ActionKind::EquipHeld;std::unordered_set<uintptr_t>seen;for(const auto&candidate:chest->items)if(candidate.data==item->data&&seen.insert(candidate.object).second){++a.matchingItemCount;a.matchingQuantity+=candidate.quantity;}
        }else a.kind=ActionKind::Move;
        r.actions.push_back(a);r.successMessage="Item equipped";
    }
    else if(operation=="unequip"){
        auto*item=SnapshotFindItem(*chest,itemId);const auto*inventory=SnapshotFindChest(*snapshot,"0000000000000001"),*equipment=SnapshotFindChest(*snapshot,"0000000000000002");if(!item){error="The equipped item changed before the request was queued";return false;}if(!inventory||!equipment){error="Character equipment is unavailable";return false;}
        auto addHeldUnequip=[&](const SnapshotItem&held,const SnapshotItem&reference){if(!removeLoadoutItem){error="Held-item unequip is unavailable for this game build";return false;}NativeAction a=BaseAction(*snapshot,*inventory,OperationFamily::Equipment);a.kind=ActionKind::UnequipHeld;a.source=inventory->inventory;a.sourceIdentity=inventory->inventoryIdentity;a.destination=equipment->inventory;a.destinationIdentity=equipment->inventoryIdentity;a.item=held.object;a.itemIdentity=held.objectIdentity;a.itemData=held.data;a.itemDataIdentity=held.dataIdentity;a.sourceSlot=held.slot;a.destinationSlot=reference.slot;a.expectedQuantity=held.quantity;std::unordered_set<uintptr_t>seen;for(const auto&candidate:inventory->items)if(candidate.data==held.data&&seen.insert(candidate.object).second){++a.matchingItemCount;a.matchingQuantity+=candidate.quantity;}r.actions.push_back(a);return true;};
        if(chestId=="0000000000000001"){
            if(!item->held){error="This inventory item is not equipped";return false;}const auto reference=std::find_if(equipment->items.begin(),equipment->items.end(),[&](const auto&candidate){return (candidate.slot==HeldRightBackingSlot||candidate.slot==HeldLeftBackingSlot)&&candidate.objectIdentity==item->objectIdentity;});if(reference==equipment->items.end()){error="The held item changed before the request was queued";return false;}if(!addHeldUnequip(*item,*reference))return false;
        }else if(chestId=="0000000000000002"){
            if(item->slot==HeldRightBackingSlot||item->slot==HeldLeftBackingSlot){const auto held=std::find_if(inventory->items.begin(),inventory->items.end(),[&](const auto&candidate){return candidate.objectIdentity==item->objectIdentity;});if(held==inventory->items.end()){error="The held item changed before the request was queued";return false;}if(!addHeldUnequip(*held,*item))return false;}else{std::vector<std::pair<const SnapshotChest*,const SnapshotItem*>> items{{chest,item}};if(!AddMovePlan(r,*snapshot,*inventory,items,error))return false;for(auto&action:r.actions)action.family=OperationFamily::Equipment;}
        }else{error="Unequip is only available for Inventory or Equipment";return false;}r.successMessage="Item unequipped";
    }
    else if(operation=="add"){std::string key;uintptr_t data=FindCatalogData(catalogId,&key);if(!data){error="Selected item is no longer available";return false;}if(!addDestination){error="Character inventory is unavailable";return false;}const int basePower=BasePowerLevel(data);const bool selectedPower=powerLevel>0;if(selectedPower&&(itemInstanceCompatibility.load()!=1||!IsMasterworkItemData(data)||MaxStackSize(data)!=1||quantity!=1||basePower<1||powerLevel<basePower||powerLevel>9)){error="The requested masterwork power level is unavailable for this item";return false;}if(!canAddItemByData(reinterpret_cast<void*>(addDestination->inventory),reinterpret_cast<void*>(data),quantity)){error="The storage does not have enough space";return false;}NativeAction a=BaseAction(*snapshot,*addDestination,selectedPower?OperationFamily::Equipment:OperationFamily::SingleItem);a.kind=selectedPower?ActionKind::AddMasterwork:ActionKind::Add;a.destination=addDestination->inventory;a.destinationIdentity=addDestination->inventoryIdentity;a.itemData=data;a.itemDataIdentity=g_Engine->GetObjectIdentity(data);a.quantity=quantity;a.desiredPowerLevel=powerLevel;r.actions.push_back(a);}
    else if(operation=="add_recipe"){RecipeRecord recipe;if(!ResolveRecipe(catalogId,recipe)){error="Selected recipe is no longer available";return false;}if(!addDestination){error="Character inventory is unavailable";return false;}std::vector<unsigned char> capacity(recipe.ingredients.size()*16);for(size_t i=0;i<recipe.ingredients.size();++i){long long amount=1LL*recipe.ingredients[i].quantity*quantity;if(amount<1||amount>1000000){error="Recipe quantity is too large";return false;}void* data=reinterpret_cast<void*>(recipe.ingredients[i].data);int q=int(amount);std::memcpy(capacity.data()+i*16,&data,8);std::memcpy(capacity.data()+i*16+8,&q,4);}FUEArray values{capacity.data(),int32_t(recipe.ingredients.size()),int32_t(recipe.ingredients.size())};if(!canAddItemsByData(reinterpret_cast<void*>(addDestination->inventory),&values)){error="The storage does not have enough space for the complete recipe";return false;}for(const auto&ingredient:recipe.ingredients){NativeAction a=BaseAction(*snapshot,*addDestination,OperationFamily::BatchAddition);a.kind=ActionKind::Add;a.destination=addDestination->inventory;a.destinationIdentity=addDestination->inventoryIdentity;a.itemData=ingredient.data;a.itemDataIdentity=g_Engine->GetObjectIdentity(ingredient.data);a.quantity=ingredient.quantity*quantity;r.actions.push_back(a);}}
    else if(operation=="slot_move"){
        auto*item=SnapshotFindItem(*chest,itemId);if(!item||!destination||destinationSlot<0||destinationSlot>=destination->totalSlots){error="The item or destination changed before the request was queued";return false;}if(playerInventory&&destination->id!=chest->id){error="Inventory grid dragging cannot cross container boundaries";return false;}const int destinationBacking=BackingSlot(*destination,destinationSlot);if(chest->id==destination->id&&item->slot==destinationBacking){r.successMessage="Storage unchanged";return true;}if(!ValidDestinationSlot(*destination,*item,destinationBacking)){error="That item cannot use the selected slot";return false;}const auto displaced=std::find_if(destination->items.begin(),destination->items.end(),[&](const auto&candidate){return candidate.slot==destinationBacking;});if(displaced!=destination->items.end()&&displaced->objectIdentity!=item->objectIdentity&&!ValidDestinationSlot(*chest,*displaced,item->slot)){error="The destination item cannot swap into the source slot";return false;}
        if(chest->id=="0000000000000002"&&(item->slot==HeldRightBackingSlot||item->slot==HeldLeftBackingSlot)){
            if(destinationBacking!=HeldRightBackingSlot&&destinationBacking!=HeldLeftBackingSlot){error="Held items can move only between the hand slots";return false;}if(displaced!=destination->items.end()&&displaced->objectIdentity!=item->objectIdentity){error="Unequip the destination hand item before moving another item there";return false;}const bool mirrored=item->slot==HeldRightBackingSlot&&std::any_of(chest->items.begin(),chest->items.end(),[&](const auto&candidate){return candidate.slot==HeldLeftBackingSlot&&candidate.objectIdentity==item->objectIdentity;});if(mirrored){error="Two-handed items remain in HeldRight";return false;}const auto*inventory=SnapshotFindChest(*snapshot,"0000000000000001");if(!inventory){error="Character inventory is unavailable";return false;}const auto held=std::find_if(inventory->items.begin(),inventory->items.end(),[&](const auto&candidate){return candidate.objectIdentity==item->objectIdentity;});if(held==inventory->items.end()){error="The held item changed before the request was queued";return false;}NativeAction a=BaseAction(*snapshot,*inventory,OperationFamily::Equipment);a.kind=ActionKind::RelocateHeld;a.source=inventory->inventory;a.sourceIdentity=inventory->inventoryIdentity;a.destination=chest->inventory;a.destinationIdentity=chest->inventoryIdentity;a.item=held->object;a.itemIdentity=held->objectIdentity;a.itemData=held->data;a.itemDataIdentity=held->dataIdentity;a.sourceSlot=held->slot;a.loadoutSourceSlot=item->slot;a.destinationSlot=destinationBacking;a.expectedQuantity=held->quantity;std::unordered_set<uintptr_t>seen;for(const auto&candidate:inventory->items)if(candidate.data==held->data&&seen.insert(candidate.object).second){++a.matchingItemCount;a.matchingQuantity+=candidate.quantity;}r.actions.push_back(a);
        }else{NativeAction a=BaseAction(*snapshot,*chest,OperationFamily::SlotMovement);a.kind=ActionKind::Move;a.source=chest->inventory;a.sourceIdentity=chest->inventoryIdentity;a.destination=destination->inventory;a.destinationIdentity=destination->inventoryIdentity;a.item=item->object;a.itemIdentity=item->objectIdentity;a.itemData=item->data;a.itemDataIdentity=item->dataIdentity;a.sourceSlot=item->slot;a.destinationSlot=destinationBacking;a.quantity=item->quantity;a.expectedQuantity=item->quantity;r.actions.push_back(a);}r.successMessage=playerInventory?"Inventory slot updated":"Storage updated";
    }
    else if(operation=="move_batch"||operation=="organize_batch"){if(!destination){error="Destination storage is no longer available";return false;}std::vector<std::pair<const SnapshotChest*,const SnapshotItem*>> items;if(operation=="move_batch"){auto values=selected();for(auto*item:values)items.push_back({chest,item});}else{auto seeds=selected();if(seeds.size()!=itemIds.size()){error="One or more selected items changed";return false;}std::unordered_set<uintptr_t> data;for(auto*item:seeds)data.insert(item->data);for(const auto&source:snapshot->chests)if(source.id!=destination->id)for(const auto&item:source.items)if(data.count(item.data))items.push_back({&source,&item});}if(items.empty()&&operation=="organize_batch"){r.successMessage="All matching items are already organized here";return true;}if(items.size()!=itemIds.size()&&operation=="move_batch"){error="One or more selected items changed";return false;}if(!AddMovePlan(r,*snapshot,*destination,items,error))return false;r.successMessage=operation=="organize_batch"?"Items organized":"Storage updated";}
    else if(operation=="copy_batch"){if(itemInstanceCompatibility.load()!=1){error="Exact item copying is unavailable for this game build";return false;}if(!destination){error="Destination storage is no longer available";return false;}auto items=selected();if(items.size()!=itemIds.size()){error="One or more selected items changed";return false;}std::vector<unsigned char> capacity(items.size()*16);for(size_t i=0;i<items.size();++i){void* data=reinterpret_cast<void*>(items[i]->data);std::memcpy(capacity.data()+i*16,&data,8);std::memcpy(capacity.data()+i*16+8,&items[i]->quantity,4);}FUEArray values{capacity.data(),int32_t(items.size()),int32_t(items.size())};if(!canAddItemsByData(reinterpret_cast<void*>(destination->inventory),&values)){error="The destination storage does not have enough space";return false;}for(auto*item:items){NativeAction a=BaseAction(*snapshot,*chest,OperationFamily::Equipment);a.kind=ActionKind::CopyItem;a.source=chest->inventory;a.sourceIdentity=chest->inventoryIdentity;a.destination=destination->inventory;a.destinationIdentity=destination->inventoryIdentity;a.item=item->object;a.itemIdentity=item->objectIdentity;a.itemData=item->data;a.itemDataIdentity=item->dataIdentity;a.sourceSlot=item->slot;a.expectedQuantity=item->quantity;r.actions.push_back(a);}}
    else if(operation=="take_batch"){if(batchCompatibility.load()!=1){error="Batch Take is unavailable for this game build";return false;}auto items=selected();if(items.size()!=itemIds.size()||quantities.size()!=items.size()){error="One or more selected items changed";return false;}if(!snapshot->adminFound){error="Linked administrator character was not found in the live player list";return false;}if(!snapshot->adminSpawned){error="Linked administrator character is still spawning";return false;}if(snapshot->adminDead){error="Linked administrator character cannot receive items while dead";return false;}if(!snapshot->adminInventory){error="Linked administrator inventory is unavailable";return false;}std::vector<unsigned char> capacity(items.size()*16);std::vector<std::pair<const SnapshotChest*,const SnapshotItem*>> moves;moves.reserve(items.size());for(size_t i=0;i<items.size();++i){if(quantities[i]<1||quantities[i]>items[i]->quantity){error="Requested quantity is no longer available";return false;}void* data=reinterpret_cast<void*>(items[i]->data);std::memcpy(capacity.data()+i*16,&data,8);std::memcpy(capacity.data()+i*16+8,&quantities[i],4);moves.push_back({chest,items[i]});}FUEArray values{capacity.data(),int32_t(items.size()),int32_t(items.size())};if(!canAddItemsByData(reinterpret_cast<void*>(snapshot->adminInventory),&values)){error="Administrator inventory does not have enough space for the complete selection";return false;}SnapshotChest adminDestination;adminDestination.inventory=snapshot->adminInventory;adminDestination.inventoryIdentity=snapshot->adminInventoryIdentity;uintptr_t slots=0;int32_t count=0;if(!ReadSlots(adminDestination.inventory,slots,count)){error="Administrator inventory slots are unavailable";return false;}adminDestination.totalSlots=count;for(int32_t slot=0;slot<count;++slot){const uintptr_t object=Mem::ReadPtr(slots+uintptr_t(slot)*8);if(!object)continue;const uintptr_t data=Mem::ReadPtr(object+0xB0);const int amount=Mem::ReadI32(object+0x58);if(!data||amount<1||amount>1000000){error="Administrator inventory contains an invalid stack";return false;}SnapshotItem current;current.slot=slot;current.quantity=amount;current.data=data;current.maxStack=MaxStackSize(data);adminDestination.items.push_back(current);}if(!AddMovePlan(r,*snapshot,adminDestination,moves,error,&quantities)){error="Administrator inventory does not have enough space for the complete selection";return false;}r.successMessage="Selected stacks moved to your inventory";}
    else if(operation=="delete_batch"){auto items=selected();if(items.size()!=itemIds.size()){error="One or more selected items changed";return false;}std::sort(items.begin(),items.end(),[](auto*a,auto*b){return a->slot>b->slot;});for(auto*item:items){NativeAction a=BaseAction(*snapshot,*chest,OperationFamily::Deletion);a.kind=ActionKind::Remove;a.source=chest->inventory;a.sourceIdentity=chest->inventoryIdentity;a.item=item->object;a.itemIdentity=item->objectIdentity;a.itemData=item->data;a.itemDataIdentity=item->dataIdentity;a.sourceSlot=item->slot;a.quantity=item->quantity;a.expectedQuantity=item->quantity;r.actions.push_back(a);}}
    else {auto* item=SnapshotFindItem(*chest,itemId);if(!item){error="The item changed before the request was queued";return false;}if(quantity<1||(operation!="add_existing"&&quantity>item->quantity)){error="Requested quantity is no longer available";return false;}if(operation=="delete"){NativeAction a=BaseAction(*snapshot,*chest,OperationFamily::Deletion);a.kind=ActionKind::Remove;a.source=chest->inventory;a.sourceIdentity=chest->inventoryIdentity;a.item=item->object;a.itemIdentity=item->objectIdentity;a.itemData=item->data;a.itemDataIdentity=item->dataIdentity;a.sourceSlot=item->slot;a.quantity=quantity;a.expectedQuantity=item->quantity;r.actions.push_back(a);}else if(operation=="add_existing"){if(!addDestination){error="Character inventory is unavailable";return false;}if(!canAddItemByData(reinterpret_cast<void*>(addDestination->inventory),reinterpret_cast<void*>(item->data),quantity)){error="The storage does not have enough space";return false;}const bool exactInstance=item->masterwork&&item->maxStack==1&&quantity==item->quantity;if(exactInstance&&itemInstanceCompatibility.load()!=1){error="Exact item copying is unavailable for this game build";return false;}NativeAction a=BaseAction(*snapshot,*chest,exactInstance?OperationFamily::Equipment:OperationFamily::SingleItem);a.kind=exactInstance?ActionKind::CopyItem:ActionKind::Add;a.source=chest->inventory;a.sourceIdentity=chest->inventoryIdentity;a.destination=addDestination->inventory;a.destinationIdentity=addDestination->inventoryIdentity;a.item=item->object;a.itemIdentity=item->objectIdentity;a.itemData=item->data;a.itemDataIdentity=item->dataIdentity;a.sourceSlot=item->slot;a.expectedQuantity=item->quantity;a.quantity=quantity;r.actions.push_back(a);}else if(operation=="take"){if(!snapshot->adminFound){error="Linked administrator character was not found in the live player list";return false;}if(!snapshot->adminSpawned){error="Linked administrator character is still spawning";return false;}if(snapshot->adminDead){error="Linked administrator character cannot receive items while dead";return false;}if(!snapshot->adminInventory){error="Linked administrator inventory is unavailable";return false;}if(!canAddItemByData(reinterpret_cast<void*>(snapshot->adminInventory),reinterpret_cast<void*>(item->data),quantity)){error="Administrator inventory does not have enough space";return false;}SnapshotChest adminDestination;adminDestination.inventory=snapshot->adminInventory;adminDestination.inventoryIdentity=snapshot->adminInventoryIdentity;uintptr_t slots=0;int32_t count=0;if(!ReadSlots(adminDestination.inventory,slots,count)){error="Administrator inventory slots are unavailable";return false;}adminDestination.totalSlots=count;for(int32_t slot=0;slot<count;++slot){const uintptr_t object=Mem::ReadPtr(slots+uintptr_t(slot)*8);if(!object)continue;const uintptr_t data=Mem::ReadPtr(object+0xB0);const int amount=Mem::ReadI32(object+0x58);if(!data||amount<1||amount>1000000){error="Administrator inventory contains an invalid stack";return false;}SnapshotItem current;current.slot=slot;current.quantity=amount;current.data=data;current.maxStack=MaxStackSize(data);adminDestination.items.push_back(current);}std::vector<std::pair<const SnapshotChest*,const SnapshotItem*>> moves{{chest,item}};std::vector<int> amounts{quantity};if(!AddMovePlan(r,*snapshot,adminDestination,moves,error,&amounts)){error="Administrator inventory does not have enough valid space";return false;}}else{error="Invalid storage operation";return false;}}
    for(const auto&action:r.actions){if(familyCircuitBroken[size_t(action.family)].load()||(action.kind==ActionKind::Move&&familyCircuitBroken[size_t(OperationFamily::Rollback)].load())){error="This storage operation is disabled after exceeding the game-thread limit";return false;}}
    if(r.successMessage.empty())r.successMessage="Storage updated";r.rollback.reserve(r.actions.size());return true;
}
bool SubmitAdminStorageMutation(const std::string&operation,const std::string&adminGuid,const std::string&adminId,const std::string&targetGuid,const std::string&targetId,const std::string&chestId,const std::string&itemId,const std::string&catalogId,int quantity,const std::string&session,std::string&requestId,std::string&result,const std::string&destinationChestId,int destinationSlot,const std::vector<std::string>&itemIds,const std::vector<int>&quantities,const std::string&customName,const std::string&newOwnerGuid,int powerLevel,bool prioritizePersonalChest){
    if(compatibility.load()!=1){result="Storage editing compatibility unavailable";return false;}auto r=std::make_shared<Request>();r->id=NewRequestId();r->operation=operation;r->session=session;r->conflictKey=targetGuid;r->adminId=adminId;r->targetId=targetId;r->targetGuid=targetGuid;r->requiresTargetOnline=IsPlayerInventoryContainerId(chestId);r->updatedUnix=UnixNow();if(!BuildMutationPlan(operation,adminGuid,targetGuid,chestId,itemId,catalogId,quantity,destinationChestId,destinationSlot,itemIds,quantities,customName,newOwnerGuid,powerLevel,prioritizePersonalChest,*r,result))return false;
    {std::lock_guard<std::mutex> lock(jobsMutex);const auto now=UnixNow();for(auto it=jobs.begin();it!=jobs.end();){auto state=it->second->state.load();if((state==JobState::Completed||state==JobState::Failed)&&now>it->second->updatedUnix.load()+300)it=jobs.erase(it);else{if((state==JobState::Queued||state==JobState::Running||state==JobState::Persisting||state==JobState::RollingBack)&&it->second->conflictKey==r->conflictKey){result="Another storage operation is already pending";return false;}++it;}}const auto head=requestHead.load(std::memory_order_relaxed),tail=requestTail.load(std::memory_order_acquire);if(head-tail>=QueueCapacity){++queueDrops;result="Storage mutation queue is full";return false;}jobs[r->id]=r;requestRing[head%QueueCapacity]=r.get();requestHead.store(head+1,std::memory_order_release);pendingCount.fetch_add(1);++queued;}
    requestId=r->id;result="Storage operation queued";return true;
}
bool SetAdminStorageAutofill(const std::string&characterGuid,const std::string&chestId,const std::string&administratorCharacterGuid,bool enabled,const std::vector<std::string>&itemIds,bool&active,std::string&result){
    active=false;
    if(compatibility.load()!=1||autofillPersistenceCompatibility.load()!=1||removalTrackingCompatibility.load()!=1||autofillDestructionCompatibility.load()==-1||!autofillStore){result="Storage autofill compatibility unavailable";return false;}
    const bool playerInventory=chestId=="0000000000000001"||chestId=="0000000000000003"||chestId=="0000000000000004"||chestId=="0000000000000005";
    if(chestId=="0000000000000002"){result="Equipment autofill is unavailable";return false;}
    {std::lock_guard<std::mutex> lock(jobsMutex);for(const auto&entry:jobs)if(entry.second->operation=="autofill"&&entry.second->conflictKey==characterGuid&&entry.second->targetId==chestId)entry.second->cancelled=true;}
    if(itemIds.empty()&&!enabled){if(!autofillStore->Remove(characterGuid,chestId)){autofillPersistenceCompatibility=-1;++autofillFailures;result="Storage autofill settings could not be saved";return false;}result="Storage autofill disabled";return true;}
    std::shared_ptr<const StorageSnapshot> snapshot;SnapshotChest cachedChest;const SnapshotChest*chest=nullptr;std::int64_t capturedAt=std::int64_t(UnixNow());
    if(playerInventory){
        snapshot=BuildPlayerInventorySnapshot(characterGuid,administratorCharacterGuid);const bool online=snapshot&&snapshot->targetFound&&snapshot->targetSpawned&&snapshot->targetController;if(online)chest=SnapshotFindChest(*snapshot,chestId);
        else{PlayerInventoryDisplaySnapshot saved;if(!playerInventorySnapshotStore||!playerInventorySnapshotStore->Find(characterGuid,saved)){result="No saved inventory snapshot is available for this character";return false;}const auto container=std::find_if(saved.containers.begin(),saved.containers.end(),[&](const auto&value){return value.id==chestId;});if(container==saved.containers.end()){result="The saved inventory snapshot does not contain this container";return false;}cachedChest.id=container->id;cachedChest.totalSlots=container->totalSlots;cachedChest.hotbarStart=container->hotbarStart;cachedChest.hotbarCount=container->hotbarCount;for(const auto&item:container->items){SnapshotItem restored;restored.id=item.id;restored.key=item.key;restored.name=item.name;restored.icon=item.icon;restored.slot=item.slot;restored.displaySlot=item.slot;restored.quantity=item.quantity;restored.maxStack=item.maxStack;cachedChest.items.push_back(std::move(restored));}capturedAt=saved.capturedAt;chest=&cachedChest;}
    }else{snapshot=BuildStorageSnapshot(characterGuid,administratorCharacterGuid,true);if(snapshot)chest=SnapshotFindChest(*snapshot,chestId);}
    if(!chest){result=playerInventory?"Character inventory container is unavailable":"Storage container is no longer available";return false;}
    StorageAutofillTemplate value;const bool existed=autofillStore->Find(characterGuid,chestId,value);if(!existed){value.ownerGuid=characterGuid;value.chestId=chestId;}value.capturedAt=capturedAt;value.totalSlots=chest->totalSlots;
    if(itemIds.empty()){value.items.clear();for(const auto&item:chest->items)value.items.push_back({DisplaySlot(item),item.quantity,item.maxStack,item.key});}
    else{
        std::unordered_set<std::string> unique;std::vector<const SnapshotItem*> selected;selected.reserve(itemIds.size());for(const auto&id:itemIds){if(!unique.insert(id).second){result="Duplicate storage item selection";return false;}const auto*item=SnapshotFindItem(*chest,id);if(!item){result="One or more selected items changed before autofill was updated";return false;}selected.push_back(item);}
        std::vector<StorageAutofillItem> replacements;replacements.reserve(selected.size());for(const auto*item:selected)replacements.push_back({DisplaySlot(*item),item->quantity,item->maxStack,item->key});if(!ApplyStorageAutofillSelection(value,replacements,enabled)){result="Selected storage items could not be applied to the autofill snapshot";return false;}
    }
    if(!enabled&&value.items.empty()){if(existed&&!autofillStore->Remove(characterGuid,chestId)){autofillPersistenceCompatibility=-1;++autofillFailures;result="Storage autofill settings could not be saved";return false;}result="Storage autofill disabled";return true;}
    if(!autofillStore->Replace(value)){autofillPersistenceCompatibility=-1;++autofillFailures;result="Storage autofill snapshot could not be saved";return false;}if(!playerInventory)RememberAutofillStorage(characterGuid,*chest);active=true;result=itemIds.empty()?(playerInventory?"Inventory autofill snapshot saved":"Storage autofill snapshot saved"):(enabled?"Selected items added to the autofill snapshot":"Selected items removed from the autofill snapshot");return true;
}
bool IsPlayerInventoryAutofillId(const std::string&id){return id=="0000000000000001"||id=="0000000000000003"||id=="0000000000000004"||id=="0000000000000005";}
std::shared_ptr<const StorageSnapshot> BuildAutofillSnapshot(const StorageAutofillTemplate&value){return IsPlayerInventoryAutofillId(value.chestId)?BuildPlayerInventorySnapshot(value.ownerGuid,""):BuildStorageSnapshot(value.ownerGuid,"",true);}
std::shared_ptr<const StorageSnapshot> BuildTargetedPlayerAutofillSnapshot(const StorageAutofillTemplate&desired,const AutofillMutationTrigger&trigger){
    if(!g_Engine||!IsPlayerInventoryAutofillId(desired.chestId)||!trigger.inventory||g_Engine->GetObjectIdentity(trigger.inventory)!=trigger.identity||!g_Engine->IsObjectLive(trigger.inventory))return {};
    auto players=std::atomic_load(&observedMutationPlayers);if(!players)return {};const auto player=std::find_if(players->begin(),players->end(),[&](const auto&candidate){return candidate.characterGuid==desired.ownerGuid;});if(player==players->end()||!player->spawned||!player->controllerPtr||!PlayerOwnsInventory(trigger.inventory,*player))return {};
    auto snapshot=std::make_shared<StorageSnapshot>();snapshot->targetGuid=desired.ownerGuid;snapshot->targetFound=true;snapshot->targetSpawned=true;snapshot->targetDead=player->isDead;snapshot->targetController=player->controllerPtr;snapshot->targetPawn=player->pawnPtr;snapshot->world=g_Engine->GetWorld();snapshot->worldIdentity=g_Engine->GetObjectIdentity(snapshot->world);if(!snapshot->world||!snapshot->worldIdentity)return {};
    std::vector<int> slots;if(desired.chestId=="0000000000000001"){auto items=PagedSlots(trigger.inventory,1);slots=PagedSlots(trigger.inventory,0);slots.insert(slots.end(),items.begin(),items.end());}else if(desired.chestId=="0000000000000005"){uintptr_t data=0;int32_t count=0;if(!g_Engine->IsA(trigger.inventory,"PersonalInventoryComponent")||!ReadSlots(trigger.inventory,data,count))return {};slots=SequentialSlots(0,count);}else slots=PagedSlots(trigger.inventory,desired.chestId=="0000000000000003"?2:3);
    SnapshotChest container;if(!PopulatePlayerContainer(container,desired.chestId.c_str(),desired.chestId=="0000000000000001"?"Inventory":desired.chestId=="0000000000000003"?"Rune pouch":desired.chestId=="0000000000000004"?"Ammunition pouch":"Personal chest",trigger.inventory,player->controllerPtr,slots))return {};snapshot->chests.push_back(std::move(container));return snapshot;
}
void ReconcileAdminStorageAutofill(){
    if(familyCircuitBroken[size_t(OperationFamily::Autofill)].load()){autofillMutationTriggers.clear();return;}if(compatibility.load()!=1||autofillPersistenceCompatibility.load()!=1||!autofillStore||!g_Engine)return;const auto templates=autofillStore->All();if(templates.empty()){autofillMutationTriggers.clear();return;}const StorageAutofillTemplate*selected=nullptr;std::shared_ptr<const StorageSnapshot> selectedSnapshot;std::uint64_t observedAtMicros=0,drainedAtMicros=0;bool priority=false;AutofillMutationTrigger priorityTrigger;
    if(!autofillMutationTriggers.empty()){
        priority=true;priorityTrigger=autofillMutationTriggers.front();autofillMutationTriggers.pop_front();const auto&trigger=priorityTrigger;
        if(!trigger.continuationRequestId.empty()){std::lock_guard<std::mutex> lock(jobsMutex);const auto previous=jobs.find(trigger.continuationRequestId);if(previous==jobs.end()){++autofillContinuationAborted;return;}const auto state=previous->second->state.load();if(state==JobState::Queued||state==JobState::Running||state==JobState::Persisting||state==JobState::RollingBack){autofillMutationTriggers.push_back(priorityTrigger);++autofillContinuationWaits;return;}if(state==JobState::Failed){++autofillContinuationAborted;return;}priorityTrigger.continuationRequestId.clear();}
        observedAtMicros=trigger.observedAtMicros;drainedAtMicros=trigger.drainedAtMicros;
        if(drainedAtMicros){const auto reconcileStarted=MonotonicMicros();if(reconcileStarted>=drainedAtMicros)RecordAutofillLatency(autofillDrainToReconcileLastMicros,autofillDrainToReconcileMaximumMicros,reconcileStarted-drainedAtMicros);}
        ++autofillPriorityChecks;bool knownMatch=false;
        if(!trigger.ownerGuid.empty()&&!trigger.chestId.empty()){
            const auto found=std::find_if(templates.begin(),templates.end(),[&](const auto&candidate){return candidate.ownerGuid==trigger.ownerGuid&&candidate.chestId==trigger.chestId;});
            if(found!=templates.end()){
                const auto started=std::chrono::steady_clock::now();auto snapshot=IsPlayerInventoryAutofillId(found->chestId)?BuildTargetedPlayerAutofillSnapshot(*found,trigger):BuildAutofillSnapshot(*found);const auto*target=snapshot?SnapshotFindChest(*snapshot,found->chestId):nullptr;
                const auto micros=std::uint64_t(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-started).count());UpdateMaximum(autofillTargetedRefreshMaximumMicros,micros);
                if(target&&target->inventory==trigger.inventory&&target->inventoryIdentity==trigger.identity){selected=&*found;selectedSnapshot=std::move(snapshot);knownMatch=true;++autofillTargetedRefreshes;}
            }
        }
        for(const auto&candidate:templates){
            if(knownMatch)break;
            if(IsPlayerInventoryAutofillId(candidate.chestId))continue;
            bool candidateKnown=false;auto snapshot=RefreshKnownAutofillStorage(candidate,trigger,candidateKnown);if(!candidateKnown)continue;knownMatch=true;
            if(snapshot){selected=&candidate;selectedSnapshot=std::move(snapshot);}
            else{++autofillTargetedRefreshFallbacks;auto rebuilt=BuildAutofillSnapshot(candidate);const auto*rebuiltChest=rebuilt?SnapshotFindChest(*rebuilt,candidate.chestId):nullptr;if(rebuiltChest&&rebuiltChest->inventory==trigger.inventory&&rebuiltChest->inventoryIdentity==trigger.identity){selected=&candidate;selectedSnapshot=std::move(rebuilt);}}
            break;
        }
        if(!knownMatch){
            ++autofillTargetedRefreshFallbacks;
            for(const auto&candidate:templates){auto snapshot=BuildAutofillSnapshot(candidate);if(!snapshot)continue;const auto*candidateChest=SnapshotFindChest(*snapshot,candidate.chestId);if(candidateChest&&candidateChest->inventory==trigger.inventory&&candidateChest->inventoryIdentity==trigger.identity){selected=&candidate;selectedSnapshot=std::move(snapshot);break;}}
        }
        if(!selected)return;++autofillPriorityMatches;
    }else{if(autofillCursor>=templates.size())autofillCursor=0;selected=&templates[autofillCursor++];}
    const auto&desired=*selected;++autofillChecks;
    {std::lock_guard<std::mutex> lock(jobsMutex);const auto now=UnixNow();for(auto iterator=jobs.begin();iterator!=jobs.end();){const auto state=iterator->second->state.load();if((state==JobState::Completed||state==JobState::Failed)&&now>iterator->second->updatedUnix.load()+300){iterator=jobs.erase(iterator);continue;}if((state==JobState::Queued||state==JobState::Running||state==JobState::RollingBack)&&iterator->second->conflictKey==desired.ownerGuid){if(priority){autofillMutationTriggers.push_back(priorityTrigger);++autofillContinuationWaits;}return;}++iterator;}}
    const auto snapshot=selectedSnapshot?selectedSnapshot:BuildAutofillSnapshot(desired);if(!snapshot)return;const auto*chest=SnapshotFindChest(*snapshot,desired.chestId);if(!chest)return;std::vector<StorageAutofillItem> current;current.reserve(chest->items.size());for(const auto&item:chest->items)current.push_back({DisplaySlot(item),item.quantity,item.maxStack,item.key});const auto plan=PlanStorageAutofill(desired,current);
    if(plan.kind==StorageAutofillPlanKind::None||plan.quantity<=0){for(const auto&saved:desired.items){const auto found=std::find_if(chest->items.begin(),chest->items.end(),[&](const auto&item){return DisplaySlot(item)==saved.slot;});if(found!=chest->items.end()&&found->key!=saved.key){++autofillProtectedSkips;break;}}if(priority)++autofillContinuationSettled;return;}
    auto request=std::make_shared<Request>();request->id=NewRequestId();request->operation="autofill";request->conflictKey=desired.ownerGuid;request->targetId=desired.chestId;request->targetGuid=desired.ownerGuid;request->requiresTargetOnline=IsPlayerInventoryAutofillId(desired.chestId);request->successMessage=request->requiresTargetOnline?"Inventory refilled from snapshot":"Storage refilled from snapshot";request->updatedUnix=UnixNow();request->expectedWorld=snapshot->world;request->expectedWorldIdentity=snapshot->worldIdentity;NativeAction action=BaseAction(*snapshot,*chest,OperationFamily::Autofill);action.quantity=plan.quantity;
    if(plan.kind==StorageAutofillPlanKind::Move){if(familyCircuitBroken[size_t(OperationFamily::Rollback)].load()){++autofillFailures;return;}const auto source=std::find_if(chest->items.begin(),chest->items.end(),[&](const auto&item){return DisplaySlot(item)==plan.sourceSlot&&item.key==plan.key;});if(source==chest->items.end()){++autofillFailures;return;}action.kind=ActionKind::Move;action.source=chest->inventory;action.sourceIdentity=chest->inventoryIdentity;action.destination=chest->inventory;action.destinationIdentity=chest->inventoryIdentity;action.item=source->object;action.itemIdentity=source->objectIdentity;action.itemData=source->data;action.itemDataIdentity=source->dataIdentity;action.sourceSlot=source->slot;action.destinationSlot=BackingSlot(*chest,plan.destinationSlot);action.expectedQuantity=source->quantity;}
    else if(plan.kind==StorageAutofillPlanKind::Set){const auto source=std::find_if(chest->items.begin(),chest->items.end(),[&](const auto&item){return DisplaySlot(item)==plan.sourceSlot&&item.key==plan.key;});if(source==chest->items.end()||plan.quantity<1||plan.quantity>=source->quantity){++autofillFailures;return;}action.kind=ActionKind::SetStack;action.source=chest->inventory;action.sourceIdentity=chest->inventoryIdentity;action.item=source->object;action.itemIdentity=source->objectIdentity;action.itemData=source->data;action.itemDataIdentity=source->dataIdentity;action.sourceSlot=source->slot;action.expectedQuantity=source->quantity;}
    else{
        std::unordered_set<int> controlledSlots,snapshotOccupiedSlots;long long currentTotal=0,desiredTotal=0;for(const auto&saved:desired.items)snapshotOccupiedSlots.insert(saved.slot);
        for(const auto&saved:desired.items)if(saved.key==plan.key){const auto found=std::find_if(chest->items.begin(),chest->items.end(),[&](const auto&item){return DisplaySlot(item)==saved.slot;});if(found!=chest->items.end()&&found->key!=plan.key)continue;const int minimumQuantity=found!=chest->items.end()?std::max(saved.quantity,found->quantity):saved.quantity;request->autofillNormalize.push_back({BackingSlot(*chest,saved.slot),minimumQuantity,saved.maxStack,saved.key});controlledSlots.insert(saved.slot);desiredTotal+=minimumQuantity;if(found!=chest->items.end())currentTotal+=found->quantity;}
        for(const auto&item:chest->items){const int display=DisplaySlot(item);if(item.key==plan.key&&!controlledSlots.count(display)){if(snapshotOccupiedSlots.count(display)){++autofillProtectedSkips;return;}request->autofillNormalize.push_back({item.slot,item.quantity,item.maxStack,item.key});controlledSlots.insert(display);currentTotal+=item.quantity;desiredTotal+=item.quantity;}}
        if(request->autofillNormalize.empty()||desiredTotal-currentTotal!=plan.quantity){++autofillProtectedSkips;return;}
        uintptr_t data=0;for(const auto&item:chest->items)if(item.key==plan.key){data=item.data;break;}if(!data){const int32_t count=g_Engine->GetObjectCount();for(int32_t index=0;index<count;++index){const uintptr_t candidate=g_Engine->GetObjectByIndex(index);if(candidate&&g_Engine->IsObjectLive(candidate)&&g_Engine->IsA(candidate,"ItemData")&&g_Engine->GetObjectName(candidate)==plan.key){data=candidate;break;}}}
        if(!data||!canAddItemByData(reinterpret_cast<void*>(chest->inventory),reinterpret_cast<void*>(data),plan.quantity)){++autofillProtectedSkips;return;}request->autofillNormalizeKey=plan.key;action.kind=ActionKind::Add;action.destination=chest->inventory;action.destinationIdentity=chest->inventoryIdentity;action.itemData=data;action.itemDataIdentity=g_Engine->GetObjectIdentity(data);
    }
    request->actions.push_back(action);request->autofillObservedAtMicros=observedAtMicros;request->autofillDrainedAtMicros=drainedAtMicros;request->autofillReconciledAtMicros=observedAtMicros?MonotonicMicros():0;
    {std::lock_guard<std::mutex> lock(jobsMutex);const auto head=requestHead.load(std::memory_order_relaxed),tail=requestTail.load(std::memory_order_acquire);if(head-tail>=QueueCapacity){++queueDrops;++autofillFailures;return;}jobs[request->id]=request;requestRing[head%QueueCapacity]=request.get();requestHead.store(head+1,std::memory_order_release);pendingCount.fetch_add(1);++queued;++autofillQueued;}if(priority){priorityTrigger.continuationRequestId=request->id;autofillMutationTriggers.push_back(priorityTrigger);++autofillContinuationRequeues;}
}
std::string GetAdminStorageMutationStatus(const std::string&id,const std::string&session){
    std::shared_ptr<Request> r;
    {
        std::lock_guard<std::mutex> lock(jobsMutex);
        auto found=jobs.find(id);
        if(found==jobs.end()||found->second->session!=session)return {};
        const auto state=found->second->state.load();
        if((state==JobState::Completed||state==JobState::Failed)&&UnixNow()>found->second->updatedUnix.load()+300){jobs.erase(found);return {};}
        r=found->second;
    }
    const auto state=r->state.load();const char*name=state==JobState::Queued?"queued":state==JobState::Running?"running":state==JobState::Persisting?"persisting":state==JobState::RollingBack?"rolling-back":state==JobState::Completed?"completed":"failed";std::string message;if(state==JobState::Completed)message=r->resultCode.load()==2?r->successMessage+"; this operation was disabled for later requests after exceeding the game-thread limit":r->successMessage;else if(state==JobState::Failed){switch(r->resultCode.load()){case -1:message="A storage object changed before the operation completed";break;case -3:message="The game world changed before the operation completed";break;case -4:message="This storage operation is disabled after exceeding the game-thread limit";break;case -5:message="A delete partially completed; refresh storage immediately";break;case -6:message="This storage type did not preserve the requested name";break;case -7:message="The storage operation was cancelled";break;case -8:message="The autofill snapshot could not be migrated, so storage ownership was restored";break;case -9:message="The game did not preserve the requested storage owner";break;case -10:message="The storage ownership-change gate could not be restored safely";break;case -11:message="The game could not refresh the storage's linked-owner membership";break;case -12:message="The game did not preserve the storage's linked state during ownership transfer";break;case -14:message="The game did not construct the requested storage name";break;case -15:message="This storage actor does not implement the game's rename interface";break;case -16:message="The selected character went offline before the inventory operation completed";break;case -17:message="The game could not create the requested masterwork power level";break;case -18:message="The game could not preserve the copied item's instance properties";break;default:message="The game rejected the storage operation";}}
    const auto progress=state==JobState::RollingBack?r->rollback.size()-r->rollbackIndex.load():r->actionIndex.load();const auto total=state==JobState::RollingBack?r->rollback.size():r->actions.size();
    std::ostringstream out;out<<"{\"id\":\""<<Escape(id)<<"\",\"status\":\""<<name<<"\",\"operation\":\""<<Escape(r->operation)<<"\",\"progress\":"<<progress<<",\"total\":"<<total<<",\"message\":"<<(message.empty()?"null":"\""+Escape(message)+"\"")<<'}';return out.str();
}
std::string GetAdminPlayerStorageJson(const std::string&characterGuid,const std::string&administratorCharacterGuid){
    auto snapshot=BuildStorageSnapshot(characterGuid,administratorCharacterGuid,true);if(!snapshot)return {};
    const bool takeAvailable=snapshot->adminFound&&snapshot->adminSpawned&&!snapshot->adminDead&&snapshot->adminInventory;const char*takeReason=!snapshot->adminFound?"Linked administrator character was not found in the live player list":!snapshot->adminSpawned?"Linked administrator character is still spawning":snapshot->adminDead?"Linked administrator character cannot receive items while dead":!snapshot->adminInventory?"Linked administrator inventory is unavailable":"";
    std::unordered_map<std::string,std::string> displayNames;for(const auto&knownChest:snapshot->chests)for(const auto&knownItem:knownChest.items)displayNames.emplace(knownItem.key,knownItem.name);
    auto snapshotName=[&](const std::string&key){auto found=displayNames.find(key);if(found!=displayNames.end())return found->second;std::string value=key;if(value.rfind("ITEM_",0)==0)value.erase(0,5);return CleanAssetName(value);};
    std::ostringstream out;out<<"{\"compatibility\":true,\"takeAvailable\":"<<(takeAvailable?"true":"false")<<",\"takeUnavailableReason\":\""<<Escape(takeReason)<<"\",\"mapVersion\":\""<<Escape(g_Engine->GetMapAssetVersion())<<"\",\"snapshotGeneration\":"<<snapshot->generation<<",\"chests\":[";bool firstChest=true;std::size_t itemCount=0;
    for(const auto&chest:snapshot->chests){
        StorageAutofillTemplate autofill;const bool autofillEnabled=autofillStore&&autofillStore->Find(characterGuid,chest.id,autofill);
        auto savedAt=[&](int slot)->const StorageAutofillItem*{if(!autofillEnabled)return nullptr;const auto found=std::find_if(autofill.items.begin(),autofill.items.end(),[&](const auto&item){return item.slot==slot;});return found==autofill.items.end()?nullptr:&*found;};
        auto writeItem=[&](const SnapshotItem&item){out<<"{\"id\":\""<<item.id<<"\",\"catalogId\":\""<<CatalogId(item.key)<<"\",\"slot\":"<<item.slot<<",\"revision\":\""<<item.id<<"\",\"name\":\""<<Escape(item.name)<<"\",\"quantity\":"<<item.quantity<<",\"maxStack\":"<<item.maxStack<<",\"icon\":\""<<Escape(item.icon)<<"\",\"basePowerLevel\":"<<item.basePowerLevel<<",\"powerLevel\":"<<item.powerLevel<<",\"masterwork\":"<<(item.masterwork?"true":"false");if(const auto*saved=savedAt(item.slot))out<<",\"autofillSnapshotState\":\""<<(saved->key==item.key?"matched":"foreign")<<"\",\"autofillSnapshotName\":\""<<Escape(snapshotName(saved->key))<<"\",\"autofillSnapshotQuantity\":"<<saved->quantity;out<<'}';};
        if(!firstChest)out<<',';firstChest=false;out<<"{\"id\":\""<<chest.id<<"\",\"type\":\""<<Escape(chest.type)<<"\",\"customName\":"<<(chest.customName.empty()?"null":"\""+Escape(chest.customName)+"\"")<<",\"autofillEnabled\":"<<(autofillEnabled?"true":"false")<<",\"autofillCapturedAt\":"<<(autofillEnabled?std::to_string(autofill.capturedAt):"null")<<",\"filledSlots\":"<<chest.items.size()<<",\"totalSlots\":"<<chest.totalSlots<<",\"gridColumns\":8,\"linked\":"<<(chest.linked?"true":"false")<<",\"private\":"<<(chest.privateStorage?"true":"false")<<",\"mapAvailable\":"<<(chest.mapValid?"true":"false")<<",\"teleportAvailable\":"<<(chest.mapValid?"true":"false");if(chest.mapValid)out<<",\"mapX\":"<<chest.mapX<<",\"mapY\":"<<chest.mapY;
        out<<",\"items\":[";bool firstItem=true;for(const auto&item:chest.items){if(!firstItem)out<<',';firstItem=false;++itemCount;writeItem(item);}out<<"],\"slots\":[";for(int slot=0;slot<chest.totalSlots;++slot){if(slot)out<<',';const SnapshotItem*item=nullptr;for(const auto&candidate:chest.items)if(candidate.slot==slot){item=&candidate;break;}if(!item)out<<"null";else writeItem(*item);}out<<"]}";
    }
    out<<"],\"chestCount\":"<<snapshot->chests.size()<<",\"itemCount\":"<<itemCount<<'}';return out.str();
}
void WritePlayerInventoryLayoutDiagnostics(std::ostringstream&out,const PlayerInfo*player){
    out<<"{\"properties\":[";bool first=true;if(player)for(const auto&field:g_Engine->GetPropertyOffsets(player->controllerPtr)){std::string lower=field.first;std::transform(lower.begin(),lower.end(),lower.begin(),[](unsigned char value){return char(std::tolower(value));});if(lower.find("inventor")==std::string::npos&&lower.find("loadout")==std::string::npos&&lower.find("equipment")==std::string::npos)continue;const auto value=field.second>=0&&field.second<=0x3000?Mem::ReadPtr(player->controllerPtr+uintptr_t(field.second)):0;const bool live=value&&g_Engine->IsObjectLive(value);uintptr_t slots=0;int32_t count=0;const bool readable=live&&g_Engine->IsA(value,"InventoryComponent")&&ReadSlots(value,slots,count);if(!first)out<<',';first=false;out<<"{\"property\":\""<<Escape(field.first)<<"\",\"offset\":"<<field.second<<",\"liveObject\":"<<(live?"true":"false")<<",\"class\":\""<<(live?Escape(g_Engine->GetObjectClassName(value)):"")<<"\",\"slotLayoutReadable\":"<<(readable?"true":"false")<<",\"slotCount\":"<<(readable?count:-1)<<'}';}
    out<<"],\"registry\":[";first=true;int emitted=0;for(int32_t index=0;player&&index<g_Engine->GetObjectCount()&&emitted<64;++index){const auto value=g_Engine->GetObjectByIndex(index);if(!value||!g_Engine->IsObjectLive(value)||!g_Engine->IsA(value,"InventoryComponent"))continue;uintptr_t slots=0;int32_t count=0;const bool readable=ReadSlots(value,slots,count);const std::string type=g_Engine->GetObjectClassName(value);const bool owned=PlayerOwnsInventory(value,*player),interesting=owned||type.find("Loadout")!=std::string::npos||type.find("Personal")!=std::string::npos||(readable&&(count==6||count==40));if(!interesting)continue;const auto outer=Mem::ReadPtr(value+Offsets::UObject_Outer);if(!first)out<<',';first=false;++emitted;out<<"{\"class\":\""<<Escape(type)<<"\",\"outerClass\":\""<<(outer&&g_Engine->IsObjectLive(outer)?Escape(g_Engine->GetObjectClassName(outer)):"")<<"\",\"targetOwned\":"<<(owned?"true":"false")<<",\"slotLayoutReadable\":"<<(readable?"true":"false")<<",\"slotCount\":"<<(readable?count:-1)<<'}';}out<<"]}";
}
bool PersistPlayerInventoryDisplaySnapshot(const std::string&characterGuid,const StorageSnapshot&snapshot,std::int64_t capturedAt,std::int64_t*storedCapturedAt=nullptr){
    if(!playerInventorySnapshotStore)return false;static const char*ids[]={"0000000000000001","0000000000000002","0000000000000005","0000000000000003","0000000000000004"};PlayerInventoryDisplaySnapshot saved;saved.characterGuid=characterGuid;saved.capturedAt=capturedAt;
    for(const char*id:ids)if(const auto*container=SnapshotFindChest(snapshot,id)){PlayerInventorySnapshotContainer stored;stored.id=id;stored.totalSlots=container->totalSlots;stored.hotbarStart=container->hotbarStart;stored.hotbarCount=container->hotbarCount;for(const auto&item:container->items)if(!IsMirroredHeldLeftItem(*container,item))stored.items.push_back({item.id,item.key,item.name,item.icon,DisplaySlot(item),item.quantity,item.maxStack,item.basePowerLevel,item.powerLevel,item.equippable,item.canUnequip,item.masterwork});saved.containers.push_back(std::move(stored));}
    if(saved.containers.size()!=5)return false;const bool persisted=playerInventorySnapshotStore->Replace(saved);if(!persisted){playerInventorySnapshotPersistenceCompatibility=-1;return false;}if(storedCapturedAt){PlayerInventoryDisplaySnapshot stored;if(playerInventorySnapshotStore->Find(characterGuid,stored))*storedCapturedAt=stored.capturedAt;}return true;
}
std::string GetAdminPlayerInventoryJson(const std::string&characterGuid,const std::string&administratorCharacterGuid){
    const auto snapshot=BuildPlayerInventorySnapshot(characterGuid,administratorCharacterGuid);if(!snapshot)return {};
    const bool online=snapshot->targetFound&&snapshot->targetSpawned&&snapshot->targetController;const bool takeAvailable=online&&characterGuid!=administratorCharacterGuid&&snapshot->adminFound&&snapshot->adminSpawned&&!snapshot->adminDead&&snapshot->adminInventory;
    const char*takeReason=!online?"The selected character must be online":characterGuid==administratorCharacterGuid?"This is already your linked character's inventory":!snapshot->adminFound?"Linked administrator character was not found in the live player list":!snapshot->adminSpawned?"Linked administrator character is still spawning":snapshot->adminDead?"Linked administrator character cannot receive items while dead":!snapshot->adminInventory?"Linked administrator inventory is unavailable":"";
    const char*containerIds[]={"0000000000000001","0000000000000002","0000000000000005","0000000000000003","0000000000000004"};
    std::int64_t capturedAt=std::int64_t(UnixNow());bool snapshotAvailable=online,snapshotPersisted=false;std::vector<SnapshotChest> storedChests;
    if(online&&playerInventorySnapshotStore){snapshotPersisted=PersistPlayerInventoryDisplaySnapshot(characterGuid,*snapshot,capturedAt,&capturedAt);
    }else if(!online&&playerInventorySnapshotStore){
        PlayerInventoryDisplaySnapshot saved;if(playerInventorySnapshotStore->Find(characterGuid,saved)){capturedAt=saved.capturedAt;snapshotAvailable=true;snapshotPersisted=true;for(const auto&container:saved.containers){SnapshotChest restored;restored.id=container.id;restored.totalSlots=container.totalSlots;restored.hotbarStart=container.hotbarStart;restored.hotbarCount=container.hotbarCount;for(const auto&item:container.items){SnapshotItem restoredItem;restoredItem.id=item.id;restoredItem.key=item.key;restoredItem.name=item.name;restoredItem.icon=item.icon;restoredItem.slot=item.slot;restoredItem.displaySlot=item.slot;restoredItem.quantity=item.quantity;restoredItem.maxStack=item.maxStack;restoredItem.basePowerLevel=item.basePowerLevel;restoredItem.powerLevel=item.powerLevel;restoredItem.equippable=item.equippable;restoredItem.canUnequip=item.canUnequip;restoredItem.masterwork=item.masterwork;restored.items.push_back(std::move(restoredItem));}storedChests.push_back(std::move(restored));}}
    }
    auto selectedChest=[&](const char*id)->const SnapshotChest*{if(online)return SnapshotFindChest(*snapshot,id);for(const auto&container:storedChests)if(container.id==id)return &container;return nullptr;};
    std::unordered_map<std::string,std::string> displayNames;for(const char*id:containerIds)if(const auto*known=selectedChest(id))for(const auto&item:known->items)displayNames.emplace(item.key,item.name);
    auto snapshotName=[&](const std::string&key){const auto found=displayNames.find(key);if(found!=displayNames.end())return found->second;std::string value=key;if(value.rfind("ITEM_",0)==0)value.erase(0,5);return CleanAssetName(value);};
    auto writeContainer=[&](std::ostringstream&out,const SnapshotChest*container,const char*id,const char*label,int expected,int columns){
        StorageAutofillTemplate autofill;const bool autofillEnabled=std::string(id)!="0000000000000002"&&autofillStore&&autofillStore->Find(characterGuid,id,autofill);
        auto savedAt=[&](int slot)->const StorageAutofillItem*{if(!autofillEnabled)return nullptr;const auto found=std::find_if(autofill.items.begin(),autofill.items.end(),[&](const auto&item){return item.slot==slot;});return found==autofill.items.end()?nullptr:&*found;};
        auto writeItem=[&](const SnapshotItem&item){const int slot=DisplaySlot(item);out<<"{\"id\":\""<<item.id<<"\",\"catalogId\":\""<<CatalogId(item.key)<<"\",\"slot\":"<<slot<<",\"revision\":\""<<item.id<<"\",\"name\":\""<<Escape(item.name)<<"\",\"quantity\":"<<item.quantity<<",\"maxStack\":"<<item.maxStack<<",\"icon\":\""<<Escape(item.icon)<<"\",\"equippable\":"<<(item.equippable?"true":"false")<<",\"canUnequip\":"<<(item.canUnequip?"true":"false")<<",\"held\":"<<(item.held?"true":"false")<<",\"basePowerLevel\":"<<item.basePowerLevel<<",\"powerLevel\":"<<item.powerLevel<<",\"masterwork\":"<<(item.masterwork?"true":"false");if(const auto*saved=savedAt(slot))out<<",\"autofillSnapshotState\":\""<<(saved->key==item.key?"matched":"foreign")<<"\",\"autofillSnapshotName\":\""<<Escape(snapshotName(saved->key))<<"\",\"autofillSnapshotQuantity\":"<<saved->quantity;out<<"}";};
        out<<"{\"id\":\""<<id<<"\",\"label\":\""<<label<<"\",\"available\":"<<(container?"true":"false")<<",\"autofillEnabled\":"<<(autofillEnabled?"true":"false")<<",\"autofillCapturedAt\":"<<(autofillEnabled?std::to_string(autofill.capturedAt):"null")<<",\"totalSlots\":"<<(container?container->totalSlots:expected)<<",\"gridColumns\":"<<columns<<",\"hotbarStart\":"<<(container?container->hotbarStart:-1)<<",\"hotbarCount\":"<<(container?container->hotbarCount:0)<<",\"items\":[";bool firstItem=true;if(container)for(const auto&item:container->items)if(!IsMirroredHeldLeftItem(*container,item)){if(!firstItem)out<<',';firstItem=false;writeItem(item);}out<<"],\"slots\":[";for(int slot=0;slot<(container?container->totalSlots:expected);++slot){if(slot)out<<',';const SnapshotItem*item=nullptr;if(container)for(const auto&candidate:container->items)if(DisplaySlot(candidate)==slot&&!IsMirroredHeldLeftItem(*container,candidate)){item=&candidate;break;}if(!item)out<<"null";else writeItem(*item);}out<<"]}";
    };
    auto writeLayoutDiagnostics=[&](std::ostringstream&out){const PlayerInfo*target=nullptr;const auto players=g_Engine->GetAllPlayers();for(const auto&player:players)if(player.characterGuid==characterGuid){target=&player;break;}WritePlayerInventoryLayoutDiagnostics(out,target);};
    const auto*inventory=selectedChest("0000000000000001"),*equipment=selectedChest("0000000000000002"),*personal=selectedChest("0000000000000005"),*runes=selectedChest("0000000000000003"),*ammunition=selectedChest("0000000000000004");std::ostringstream out;out<<"{\"compatibility\":true,\"pageCompatibility\":"<<(playerInventoryPagesCompatibility.load()==1?"true":"false")<<",\"equipmentCompatibility\":"<<(equipmentSlotCompatibility.load()==1?"true":"false")<<",\"personalInventoryCompatibility\":"<<(personalInventoryCompatibility.load()==1?"true":"false")<<",\"snapshotPersistenceCompatibility\":"<<playerInventorySnapshotPersistenceCompatibility.load()<<",\"online\":"<<(online?"true":"false")<<",\"stale\":"<<(!online?"true":"false")<<",\"snapshotAvailable\":"<<(snapshotAvailable?"true":"false")<<",\"snapshotPersisted\":"<<(snapshotPersisted?"true":"false")<<",\"snapshotCapturedAt\":"<<(snapshotAvailable?std::to_string(capturedAt):"null")<<",\"mutationAvailable\":"<<(online?"true":"false")<<",\"takeAvailable\":"<<(takeAvailable?"true":"false")<<",\"takeUnavailableReason\":\""<<Escape(takeReason)<<"\",\"unavailableReason\":\""<<(!snapshotAvailable?"No saved inventory snapshot is available for this character":online&&!inventory?"The character inventory layout is unavailable for this game build":"")<<"\",\"inventory\":";writeContainer(out,inventory,"0000000000000001","Inventory and hotbar",40,8);out<<",\"equipment\":";writeContainer(out,equipment,"0000000000000002","Equipment",8,8);out<<",\"personal\":";writeContainer(out,personal,"0000000000000005","Personal chest",personal?personal->totalSlots:32,8);out<<",\"runes\":";writeContainer(out,runes,"0000000000000003","Rune pouch",8,8);out<<",\"ammunition\":";writeContainer(out,ammunition,"0000000000000004","Ammunition pouch",8,8);out<<",\"layoutDiagnostics\":";writeLayoutDiagnostics(out);out<<'}';return out.str();
}
void InvalidateAdminStorageSnapshots(){++snapshotInvalidations;storageGeneration.fetch_add(1);}
PerformanceTickSnapshot TakePerformanceTickSnapshot(){PerformanceTickSnapshot result;result.compatibility=dispatcherCompatibility.load();result.frames=performanceFrames.exchange(0);const auto engine=performanceMaximumFrameMicros.exchange(0),zanik=performanceMaximumZanikMicros.exchange(0);result.maximumEngineTickMilliseconds=result.frames?double(engine)/1000.0:-1;result.maximumZanikMilliseconds=result.frames?double(zanik)/1000.0:-1;result.hitches=performanceHitches.exchange(0);return result;}
void UpdateAdminStorageMutationPlayers(const std::vector<PlayerInfo>&players){
    std::atomic_store(&observedMutationPlayers,std::make_shared<const std::vector<PlayerInfo>>(players));std::vector<std::string> newlyOnline;std::unordered_set<std::string> current;for(const auto&player:players)if(player.spawned&&player.controllerPtr&&!player.characterGuid.empty())current.insert(player.characterGuid);
    {std::lock_guard<std::mutex> lock(playerInventorySnapshotOnlineMutex);for(const auto&guid:current)if(!playerInventorySnapshotOnline.count(guid))newlyOnline.push_back(guid);playerInventorySnapshotOnline=std::move(current);}
    for(const auto&guid:newlyOnline){const auto snapshot=BuildPlayerInventorySnapshot(guid,"");if(snapshot&&snapshot->targetFound&&snapshot->targetSpawned&&snapshot->targetController)PersistPlayerInventoryDisplaySnapshot(guid,*snapshot,std::int64_t(UnixNow()));}
}
void ObserveAdminStorageProcessEvent(void*,void*function,void*params){
    if(!function||!params)return;const auto address=reinterpret_cast<uintptr_t>(function),complete=autofillCompleteBuildingRpc.load(std::memory_order_acquire),partial=autofillPartialBuildingRpc.load(std::memory_order_acquire);
    if(autofillBuildingCaptureCompatibility.load(std::memory_order_acquire)==1&&(address==complete||address==partial)){
        const auto started=std::chrono::steady_clock::now();++autofillBuildingCaptureCalls;FUEArray inventories{};const auto offset=address==complete?autofillCompleteBuildingInventoriesOffset:autofillPartialBuildingInventoriesOffset;std::memcpy(&inventories,reinterpret_cast<const unsigned char*>(params)+offset,sizeof(inventories));
        if(inventories.Num<1||inventories.Num>128||inventories.Max<inventories.Num||!inventories.Data||!Mem::Readable(inventories.Data,std::size_t(inventories.Num)*sizeof(uintptr_t)))++autofillBuildingCaptureDrops;
        else for(int32_t index=0;index<inventories.Num;++index){uintptr_t inventory=0;std::memcpy(&inventory,reinterpret_cast<const unsigned char*>(inventories.Data)+std::size_t(index)*sizeof(uintptr_t),sizeof(inventory));if(!inventory||!g_Engine||!g_Engine->IsObjectLive(inventory)||!g_Engine->IsA(inventory,"InventoryComponent")){++autofillBuildingCaptureDrops;continue;}++autofillBuildingCaptureInventories;if(!PublishObservedMutation({ObservedMutationKind::BuildingConsume,inventory,0,0,0,0,true,false,MonotonicMicros()},started))++autofillBuildingCaptureDrops;}
        const auto micros=std::uint64_t(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-started).count());UpdateMaximum(autofillBuildingCaptureMaximumMicros,micros);return;
    }
    if(autofillRpcCaptureCompatibility.load(std::memory_order_acquire)!=1)return;const auto move=autofillMoveRpc.load(std::memory_order_acquire),splitPut=autofillSplitPutRpc.load(std::memory_order_acquire),split=autofillSplitRpc.load(std::memory_order_acquire);ObservedMutationKind kind;uintptr_t source=0,destination=0;
    if(address==move){kind=ObservedMutationKind::RpcMove;std::memcpy(&source,reinterpret_cast<const unsigned char*>(params)+autofillMoveSourceOffset,sizeof(source));std::memcpy(&destination,reinterpret_cast<const unsigned char*>(params)+autofillMoveDestinationOffset,sizeof(destination));++autofillRpcFullStackCalls;}
    else if(address==splitPut){kind=ObservedMutationKind::RpcSplit;std::memcpy(&source,reinterpret_cast<const unsigned char*>(params)+autofillSplitPutSourceOffset,sizeof(source));std::memcpy(&destination,reinterpret_cast<const unsigned char*>(params)+autofillSplitPutDestinationOffset,sizeof(destination));++autofillRpcSplitStackCalls;}
    else if(address==split){kind=ObservedMutationKind::RpcSplit;std::memcpy(&source,reinterpret_cast<const unsigned char*>(params)+autofillSplitSourceOffset,sizeof(source));destination=source;++autofillRpcSplitStackCalls;}
    else return;
    ++autofillRpcCaptureCalls;if(!source||!destination){++autofillRpcCaptureDrops;return;}const auto started=std::chrono::steady_clock::now();if(!PublishObservedMutation({kind,source,destination,0,0,0,true,false,MonotonicMicros()},started))++autofillRpcCaptureDrops;
}
bool ResolveAdminInventoryMutationCallables(uintptr_t moveEntry,uintptr_t removeEntry,uintptr_t&moveCallable,uintptr_t&removeCallable){
    moveCallable=0;removeCallable=0;if(nativeMutationCaptureCompatibility.load(std::memory_order_acquire)!=1||moveEntry!=observedMoveEntry.load(std::memory_order_acquire)||removeEntry!=observedRemoveEntry.load(std::memory_order_acquire)||!originalObservedMoveItem||!originalObservedRemoveFromSlot)return false;moveCallable=reinterpret_cast<uintptr_t>(originalObservedMoveItem);removeCallable=reinterpret_cast<uintptr_t>(originalObservedRemoveFromSlot);return true;
}
void DrainAdminStorageMutationTraces(){
    const auto drainStartedAtMicros=MonotonicMicros();while(!recentExactAutofillMutations.empty()&&drainStartedAtMicros>recentExactAutofillMutations.front().observedAtMicros&&drainStartedAtMicros-recentExactAutofillMutations.front().observedAtMicros>AutofillExactRetentionMicroseconds)recentExactAutofillMutations.pop_front();
    auto rememberExact=[&](uintptr_t inventory,std::uint64_t observedAtMicros){if(!inventory||!observedAtMicros||!g_Engine||!g_Engine->IsObjectLive(inventory))return;const auto identity=g_Engine->GetObjectIdentity(inventory);if(!identity)return;const auto duplicate=std::find_if(recentExactAutofillMutations.begin(),recentExactAutofillMutations.end(),[&](const auto&value){return value.inventory==inventory&&value.identity==identity;});if(duplicate!=recentExactAutofillMutations.end())recentExactAutofillMutations.erase(duplicate);recentExactAutofillMutations.push_back({inventory,identity,observedAtMicros});};
    for(const auto&slot:observedMutationRing){if(slot.state.load(std::memory_order_acquire)!=2)continue;const auto&event=slot.event;if(!event.succeeded)continue;if(event.sourceSlot>=0)rememberExact(event.source,event.observedAtMicros);if(event.destinationSlot>=0)rememberExact(event.destination,event.observedAtMicros);}
    bool invalidateSnapshots=false;auto players=std::atomic_load(&observedMutationPlayers);for(auto&slot:observedMutationRing){if(slot.state.load(std::memory_order_acquire)!=2)continue;const auto event=slot.event;slot.state.store(0,std::memory_order_release);if(event.kind==ObservedMutationKind::BuildingDestroyed){CleanupDestroyedStorageAutofill(event.source,event.identity);continue;}const auto drainedAtMicros=event.observedAtMicros?MonotonicMicros():0;if(drainedAtMicros>=event.observedAtMicros&&event.observedAtMicros)RecordAutofillLatency(autofillCaptureToDrainLastMicros,autofillCaptureToDrainMaximumMicros,drainedAtMicros-event.observedAtMicros);uintptr_t sourceOuter=0,destinationOuter=0;if(g_Engine&&event.source&&g_Engine->IsObjectLive(event.source))sourceOuter=Mem::ReadPtr(event.source+Offsets::UObject_Outer);if(g_Engine&&event.destination&&g_Engine->IsObjectLive(event.destination))destinationOuter=Mem::ReadPtr(event.destination+Offsets::UObject_Outer);
        const PlayerInfo*player=nullptr;for(const auto&candidate:*players)if((event.controller&&candidate.controllerPtr==event.controller)||(sourceOuter&&(candidate.controllerPtr==sourceOuter||candidate.pawnPtr==sourceOuter))||(destinationOuter&&(candidate.controllerPtr==destinationOuter||candidate.pawnPtr==destinationOuter))){player=&candidate;break;}
        const auto isGrave=[&](uintptr_t outer){return outer&&g_Engine&&g_Engine->IsObjectLive(outer)&&(g_Engine->IsA(outer,"Gravestone")||g_Engine->GetObjectName(outer).find("Grave")!=std::string::npos);};
        const auto isStorage=[&](uintptr_t inventory,uintptr_t outer){if(!g_Engine)return false;if(inventory&&g_Engine->IsObjectLive(inventory)&&g_Engine->IsA(inventory,"LinkedInventoryComponent"))return true;if(!outer||!g_Engine->IsObjectLive(outer))return false;const auto name=g_Engine->GetObjectName(outer);return name.rfind("BP_BaseBuilding_",0)==0||name.find("Chest")!=std::string::npos||name.find("Storage")!=std::string::npos;};
        const bool grave=isGrave(sourceOuter)||isGrave(destinationOuter),sourceStorage=!grave&&isStorage(event.source,sourceOuter),destinationStorage=!grave&&isStorage(event.destination,destinationOuter),storage=sourceStorage||destinationStorage;const char*feature=grave?"gravestones":(storage?"storage":"inventory");const char*operation=event.kind==ObservedMutationKind::Move?(event.succeeded?"in-game item move":"failed in-game item move"):event.kind==ObservedMutationKind::Remove?(event.succeeded?"in-game item removal":"failed in-game item removal"):event.kind==ObservedMutationKind::RpcSplit?"authoritative split-stack withdrawal":event.kind==ObservedMutationKind::BuildingConsume?"authoritative linked-building consumption":"authoritative full-stack withdrawal";
        if(event.succeeded&&(storage||player)&&autofillStore){
            invalidateSnapshots=true;
            auto signal=[&](uintptr_t inventory,const std::string&ownerGuid={},const std::string&chestId={}){if(!inventory||!g_Engine||!g_Engine->IsObjectLive(inventory))return;const auto identity=g_Engine->GetObjectIdentity(inventory);if(!identity)return;const auto duplicate=std::find_if(autofillMutationTriggers.begin(),autofillMutationTriggers.end(),[&](const auto&value){return value.inventory==inventory&&value.identity==identity&&value.ownerGuid==ownerGuid&&value.chestId==chestId;});if(duplicate!=autofillMutationTriggers.end()){if(event.observedAtMicros&&(!duplicate->observedAtMicros||event.observedAtMicros<duplicate->observedAtMicros))duplicate->observedAtMicros=event.observedAtMicros;if(drainedAtMicros&&(!duplicate->drainedAtMicros||drainedAtMicros<duplicate->drainedAtMicros))duplicate->drainedAtMicros=drainedAtMicros;return;}if(autofillMutationTriggers.size()>=AutofillMutationTriggerCapacity){++autofillPriorityDrops;return;}autofillMutationTriggers.push_back({inventory,identity,event.observedAtMicros,drainedAtMicros,ownerGuid,chestId});++autofillMutationSignals;};
            if(sourceStorage)signal(event.source);if(destinationStorage)signal(event.destination);
            if(player&&!player->characterGuid.empty()){
                auto signalPlayer=[&](uintptr_t inventory,int rawSlot){if(!inventory||!g_Engine->IsObjectLive(inventory)||!PlayerOwnsInventory(inventory,*player))return;const auto identity=g_Engine->GetObjectIdentity(inventory);const bool rpcWithoutSlot=rawSlot<0&&(event.kind==ObservedMutationKind::RpcMove||event.kind==ObservedMutationKind::RpcSplit);if(rpcWithoutSlot&&event.observedAtMicros&&std::any_of(recentExactAutofillMutations.begin(),recentExactAutofillMutations.end(),[&](const auto&value){const auto difference=value.observedAtMicros>event.observedAtMicros?value.observedAtMicros-event.observedAtMicros:event.observedAtMicros-value.observedAtMicros;return value.inventory==inventory&&value.identity==identity&&difference<=AutofillRpcCorrelationMicroseconds;}))return;auto signalPage=[&](const char*id,std::vector<int>slots){if(slots.empty()||(rawSlot>=0&&std::find(slots.begin(),slots.end(),rawSlot)==slots.end()))return;StorageAutofillTemplate desired;if(autofillStore->Find(player->characterGuid,id,desired))signal(inventory,player->characterGuid,id);};if(g_Engine->IsA(inventory,"PersonalInventoryComponent")){uintptr_t data=0;int32_t count=0;if(ReadSlots(inventory,data,count))signalPage("0000000000000005",SequentialSlots(0,count));return;}auto items=PagedSlots(inventory,1),quick=PagedSlots(inventory,0);if(!items.empty()&&!quick.empty()){quick.insert(quick.end(),items.begin(),items.end());signalPage("0000000000000001",std::move(quick));}signalPage("0000000000000003",PagedSlots(inventory,2));signalPage("0000000000000004",PagedSlots(inventory,3));};
                signalPlayer(event.source,event.sourceSlot);signalPlayer(event.destination,event.destinationSlot);
            }
        }
        if(event.timing){if(player&&!player->characterGuid.empty()){nativeMutationAttributed.fetch_add(1,std::memory_order_relaxed);PerformanceTrace::Record(feature,operation,event.nativeMicros,player->characterGuid.c_str(),'g',"game",PerformanceTrace::TimingScope::NativeGame);if(event.captureMicros>=100)PerformanceTrace::Record("diagnostics","inventory mutation trace capture",event.captureMicros,player->characterGuid.c_str(),'g',"game");}else{nativeMutationUnattributed.fetch_add(1,std::memory_order_relaxed);PerformanceTrace::Record(feature,operation,event.nativeMicros,nullptr,0,"game",PerformanceTrace::TimingScope::NativeGame);}}
    }if(invalidateSnapshots)InvalidateAdminStorageSnapshots();
}
bool HasPendingAdminStorageAutofillMutation(){return !autofillMutationTriggers.empty();}
AdminStorageAutofillLatencyDiagnostics GetAdminStorageAutofillLatencyDiagnostics(){
    AdminStorageAutofillLatencyDiagnostics value;
    value.samples=autofillLatencySamples.load();
    value.captureToDrainLastMicroseconds=autofillCaptureToDrainLastMicros.load();value.captureToDrainMaximumMicroseconds=autofillCaptureToDrainMaximumMicros.load();
    value.drainToReconcileLastMicroseconds=autofillDrainToReconcileLastMicros.load();value.drainToReconcileMaximumMicroseconds=autofillDrainToReconcileMaximumMicros.load();
    value.reconcileToDispatchLastMicroseconds=autofillReconcileToDispatchLastMicros.load();value.reconcileToDispatchMaximumMicroseconds=autofillReconcileToDispatchMaximumMicros.load();
    value.dispatchToRefillLastMicroseconds=autofillDispatchToRefillLastMicros.load();value.dispatchToRefillMaximumMicroseconds=autofillDispatchToRefillMaximumMicros.load();
    value.captureToRefillLastMicroseconds=autofillCaptureToRefillLastMicros.load();value.captureToRefillMaximumMicroseconds=autofillCaptureToRefillMaximumMicros.load();
    return value;
}
AdminStorageAutofillBuildingCaptureDiagnostics GetAdminStorageAutofillBuildingCaptureDiagnostics(){
    AdminStorageAutofillBuildingCaptureDiagnostics value;value.compatibility=autofillBuildingCaptureCompatibility.load();value.calls=autofillBuildingCaptureCalls.load();value.inventories=autofillBuildingCaptureInventories.load();value.drops=autofillBuildingCaptureDrops.load();value.maximumMicroseconds=autofillBuildingCaptureMaximumMicros.load();return value;
}
AdminStorageAutofillDestructionDiagnostics GetAdminStorageAutofillDestructionDiagnostics(){
    AdminStorageAutofillDestructionDiagnostics value;value.compatibility=removalTrackingCompatibility.load()==1&&autofillPersistenceCompatibility.load()==1&&autofillDestructionCompatibility.load()!=-1?1:-1;value.signals=autofillDestructionSignals.load();value.drops=autofillDestructionDrops.load();value.deletedSnapshots=autofillDestroyedSnapshots.load();value.failures=autofillDestructionFailures.load();value.maximumMicroseconds=autofillDestructionMaximumMicros.load();return value;
}
int GetAdminPlayerInventoryPagesCompatibility(){return playerInventoryPagesCompatibility.load();}
int GetAdminPersonalInventoryCompatibility(){return personalInventoryCompatibility.load();}
int GetAdminEquipmentSlotCompatibility(){return equipmentSlotCompatibility.load();}
int GetAdminHeldItemEquipmentCompatibility(){return heldItemEquipmentCompatibility.load();}
AdminStorageDiagnostics GetAdminStorageDiagnostics(){AdminStorageDiagnostics d;d.compatibility=compatibility.load();d.productionCompatibility=productionCompatibility.load();d.candidateOverride=candidateOverride.load();d.dispatcherCompatibility=dispatcherCompatibility.load();d.mutationCompatibility=compatibility.load();d.nativeMutationCaptureCompatibility=nativeMutationCaptureCompatibility.load();d.batchCompatibility=batchCompatibility.load();d.organizationCompatibility=compatibility.load();d.renameCompatibility=renameCompatibility.load();d.ownershipCompatibility=ownershipCompatibility.load()==1&&autofillPersistenceCompatibility.load()==1&&!familyCircuitBroken[size_t(OperationFamily::Ownership)].load()?1:-1;d.removalTrackingCompatibility=removalTrackingCompatibility.load();d.catalogCompatibility=catalogCompatibility.load();d.unlockCatalogCompatibility=unlockCatalogCompatibility.load();d.planCatalogCompatibility=planCatalogCompatibility.load();d.planCatalogItems=PlanItemCatalog().keys.size();d.buildingTaxonomyCompatibility=buildingTaxonomyCompatibility.load();d.buildingRequirementsCompatibility=batchCompatibility.load()==1?1:-1;d.buildingIconsCompatibility=buildingIconsCompatibility.load();d.craftingRecipeCompatibility=craftingRecipeCompatibility.load();const auto& catalog=AuthoritativeBuildingCatalog();d.expectedBuildingPieces=catalog.expectedPieces;d.resolvedBuildingPieces=resolvedBuildingPieces.load();d.unavailableBuildingPieces=unavailableBuildingPieces.load();d.buildingTabs=catalog.tabs.size();for(const auto&tab:catalog.tabs){d.buildingSections+=tab.sections.size();for(const auto&section:tab.sections)d.buildingReferences+=section.pieces.size();}d.queuedRequests=queued.load();d.acceptedMutations=accepted.load();d.rejectedRequests=rejected.load();d.staleRequests=stale.load();d.maximumExecutionMicroseconds=maximumMicros.load();d.tickCalls=tickCalls.load();d.maximumTickMicroseconds=maximumTickMicros.load();d.slowTickCalls=slowTickCalls.load();d.maximumRecipeTickMicroseconds=maximumRecipeTickMicros.load();d.maximumCustomNameTickMicroseconds=maximumCustomNameTickMicros.load();d.maximumStorageTickMicroseconds=maximumStorageTickMicros.load();d.maximumGravestoneTickMicroseconds=maximumGravestoneTickMicros.load();d.snapshotCompatibility=g_Engine?1:-1;const auto playerInventorySnapshotHealth=playerInventorySnapshotStore?playerInventorySnapshotStore->Diagnostics():PlayerInventorySnapshotDiagnostics{};d.playerInventorySnapshotPersistenceCompatibility=playerInventorySnapshotPersistenceCompatibility.load();d.playerInventorySnapshots=playerInventorySnapshotHealth.snapshots;d.playerInventorySnapshotMalformedRecords=playerInventorySnapshotHealth.malformedRecords;d.singleItemCompatibility=familyCircuitBroken[size_t(OperationFamily::SingleItem)]?-1:compatibility.load();d.equipmentCompatibility=familyCircuitBroken[size_t(OperationFamily::Equipment)]?-1:compatibility.load();d.slotMovementCompatibility=familyCircuitBroken[size_t(OperationFamily::SlotMovement)]?-1:compatibility.load();d.batchAdditionCompatibility=familyCircuitBroken[size_t(OperationFamily::BatchAddition)]?-1:batchCompatibility.load();d.batchMovementCompatibility=familyCircuitBroken[size_t(OperationFamily::BatchMovement)]?-1:compatibility.load();d.deletionCompatibility=familyCircuitBroken[size_t(OperationFamily::Deletion)]?-1:compatibility.load();d.renameCompatibility=familyCircuitBroken[size_t(OperationFamily::Rename)]?-1:renameCompatibility.load();d.rollbackCompatibility=familyCircuitBroken[size_t(OperationFamily::Rollback)]?-1:compatibility.load();d.snapshotGenerations=snapshotGenerations.load();d.snapshotCacheHits=snapshotHits.load();d.snapshotCacheMisses=snapshotMisses.load();d.snapshotInvalidations=snapshotInvalidations.load();d.maximumSnapshotPreparationMicroseconds=maximumSnapshotMicros.load();d.queueDepth=pendingCount.load();d.queueDrops=queueDrops.load();d.runningJobs=runningJobs.load();d.completedJobs=completedJobs.load();d.failedJobs=failedJobs.load();d.staleIdentities=staleIdentities.load();d.worldChanges=worldChanges.load();d.rollbackAttempts=rollbackAttempts.load();d.rollbackFailures=rollbackFailures.load();d.singleItemMaximumMicroseconds=familyMaximum[size_t(OperationFamily::SingleItem)].load();d.equipmentMaximumMicroseconds=familyMaximum[size_t(OperationFamily::Equipment)].load();d.slotMovementMaximumMicroseconds=familyMaximum[size_t(OperationFamily::SlotMovement)].load();d.batchAdditionMaximumMicroseconds=familyMaximum[size_t(OperationFamily::BatchAddition)].load();d.batchMovementMaximumMicroseconds=familyMaximum[size_t(OperationFamily::BatchMovement)].load();d.deletionMaximumMicroseconds=familyMaximum[size_t(OperationFamily::Deletion)].load();d.renameMaximumMicroseconds=familyMaximum[size_t(OperationFamily::Rename)].load();d.ownershipMaximumMicroseconds=familyMaximum[size_t(OperationFamily::Ownership)].load();d.ownershipTransfers=ownershipTransfers.load();d.ownershipFailures=ownershipFailures.load();d.ownershipSnapshotMigrations=ownershipSnapshotMigrations.load();d.rollbackMaximumMicroseconds=familyMaximum[size_t(OperationFamily::Rollback)].load();d.nativeMutationCalls=nativeMutationCalls.load();d.nativeMutationAttributed=nativeMutationAttributed.load();d.nativeMutationUnattributed=nativeMutationUnattributed.load();d.nativeMutationDrops=nativeMutationDrops.load();d.nativeMutationMaximumMicroseconds=nativeMutationMaximumMicros.load();d.nativeMutationCaptureMaximumMicroseconds=nativeMutationCaptureMaximumMicros.load();d.autofillRpcCaptureCompatibility=autofillRpcCaptureCompatibility.load();d.autofillRpcCaptureCalls=autofillRpcCaptureCalls.load();d.autofillRpcFullStackCalls=autofillRpcFullStackCalls.load();d.autofillRpcSplitStackCalls=autofillRpcSplitStackCalls.load();d.autofillRpcCaptureDrops=autofillRpcCaptureDrops.load();d.autofillRpcCaptureMaximumMicroseconds=autofillRpcCaptureMaximumMicros.load();d.autofillNormalizationCompatibility=autofillNormalizationCompatibility.load();d.autofillNormalizationFailures=autofillNormalizationFailures.load();d.autofillReplicationWakeCompatibility=autofillReplicationWakeCompatibility.load();d.autofillReplicationWakeCalls=autofillReplicationWakeCalls.load();d.autofillReplicationWakeFailures=autofillReplicationWakeFailures.load();d.autofillReplicationWakeMaximumMicroseconds=autofillReplicationWakeMaximumMicros.load();const auto autofillHealth=autofillStore?autofillStore->Diagnostics():StorageAutofillDiagnostics{};d.autofillPersistenceCompatibility=autofillPersistenceCompatibility.load();d.autofillCompatibility=compatibility.load()==1&&d.autofillPersistenceCompatibility==1&&d.autofillNormalizationCompatibility==1&&!familyCircuitBroken[size_t(OperationFamily::Autofill)].load()?1:-1;d.autofillTemplates=autofillHealth.templates;d.autofillChecks=autofillChecks.load();d.autofillQueued=autofillQueued.load();d.autofillItemsAdded=autofillItemsAdded.load();d.autofillMutationSignals=autofillMutationSignals.load();d.autofillPriorityChecks=autofillPriorityChecks.load();d.autofillPriorityMatches=autofillPriorityMatches.load();d.autofillPriorityDrops=autofillPriorityDrops.load();d.autofillContinuationRequeues=autofillContinuationRequeues.load();d.autofillContinuationWaits=autofillContinuationWaits.load();d.autofillContinuationSettled=autofillContinuationSettled.load();d.autofillContinuationAborted=autofillContinuationAborted.load();d.autofillTargetedRefreshes=autofillTargetedRefreshes.load();d.autofillTargetedRefreshFallbacks=autofillTargetedRefreshFallbacks.load();d.autofillTargetedRefreshMaximumMicroseconds=autofillTargetedRefreshMaximumMicros.load();d.autofillProtectedSkips=autofillProtectedSkips.load();d.autofillFailures=autofillFailures.load();d.autofillMalformedRecords=autofillHealth.malformedRecords;for(size_t i=0;i<size_t(OperationFamily::Count);++i)if(familyCircuitBroken[i])d.circuitBreakers|=1u<<i;return d;}
std::uint64_t GetAdminStorageAutofillNormalizations(){return autofillNormalizations.load();}
std::uint64_t GetAdminStorageAutofillMaximumMicroseconds(){return familyMaximum[size_t(OperationFamily::Autofill)].load();}
int GetAdminStorageItemInstanceCompatibility(){return itemInstanceCompatibility.load();}
std::string GetAdminStorageCatalogJson(){
    if(compatibility.load()!=1||!g_Engine)return {};using GetDisplayStringFn=const FString*(*)(const void*);auto display=reinterpret_cast<GetDisplayStringFn>(g_Engine->GetModuleBase()+DisplayTextOffset());
    const std::string itemRoot=RuntimePaths::WebRoot()+"/items/";std::ostringstream out;out<<"{\"items\":[";bool first=true;int count=0;const int32_t objects=g_Engine->GetObjectCount();
    for(int32_t i=0;i<objects;++i){uintptr_t data=g_Engine->GetObjectByIndex(i);if(!data||!g_Engine->IsA(data,"ItemData"))continue;std::string key=g_Engine->GetObjectName(data);if(!AllowedCatalogKey(key))continue;std::string lower=key;std::transform(lower.begin(),lower.end(),lower.begin(),[](unsigned char c){return char(std::tolower(c));});if(lower.find("test")!=std::string::npos||lower.find("debug")!=std::string::npos||lower.find("developer")!=std::string::npos||lower.find("deprecated")!=std::string::npos||lower.find("_old")!=std::string::npos)continue;std::string icon=itemRoot+key+".png";struct stat st{};if(stat(icon.c_str(),&st)!=0||st.st_size<1)continue;std::string name;const FString* text=display(reinterpret_cast<const void*>(data+0x68));if(text&&Mem::Readable(text,sizeof(FString)))name=g_Engine->ReadFString(reinterpret_cast<uintptr_t>(text));if(name.empty())continue;int maxStack=MaxStackSize(data);auto meta=CatalogMetadata(key,name);const int basePower=BasePowerLevel(data);const bool masterwork=itemInstanceCompatibility.load()==1&&basePower>0&&maxStack==1&&IsMasterworkItemData(data);if(!first)out<<',';first=false;++count;out<<"{\"id\":\""<<CatalogId(key)<<"\",\"name\":\""<<Escape(name)<<"\",\"icon\":\"/assets/items/"<<Escape(key)<<".png\",\"maxStack\":"<<maxStack<<",\"stackable\":"<<(maxStack>1?"true":"false")<<",\"category\":\""<<Escape(meta.category)<<"\",\"categoryOrder\":"<<meta.categoryOrder<<",\"tier\":"<<(meta.tier?std::to_string(meta.tier):"null")<<",\"basePowerLevel\":"<<(masterwork?std::to_string(basePower):"null")<<",\"masterwork\":"<<(masterwork?"true":"false")<<",\"maximumPowerLevel\":"<<(masterwork?"9":"null")<<",\"recipeOrder\":"<<meta.recipeOrder<<",\"aliases\":[\""<<Escape(meta.aliases)<<"\"]}";}
    out<<"],\"count\":"<<count<<'}';return out.str();
}
std::string GetAdminStorageRecipesJson(){
    if(compatibility.load()!=1||batchCompatibility.load()!=1||!g_Engine)return {};
    auto records=RecipeRecords();std::unordered_set<std::string> present;for(const auto&r:records)if(r.type=="building")present.insert(r.key);
    std::ostringstream out;out<<"{\"recipes\":[";bool first=true;
    for(const auto&r:records){if(!first)out<<',';first=false;out<<"{\"id\":\""<<r.id<<"\",\"key\":\""<<Escape(r.key)<<"\",\"name\":\""<<Escape(r.name)<<"\",\"type\":\""<<r.type<<"\",\"category\":\""<<Escape(r.category)<<"\",\"station\":\""<<Escape(r.station)<<"\",\"available\":"<<(r.available?"true":"false")<<",\"unavailableReason\":"<<(r.unavailableReason.empty()?"null":"\""+Escape(r.unavailableReason)+"\"")<<",\"tier\":"<<(r.tier?std::to_string(r.tier):"null")<<",\"order\":"<<r.order<<",\"icon\":\""<<Escape(r.icon)<<"\",\"aliases\":[\""<<Escape(CleanAssetName(r.key))<<"\"],\"ingredients\":[";bool fi=true;for(const auto&i:r.ingredients){if(!fi)out<<',';fi=false;std::string key=g_Engine->GetObjectName(i.data);out<<"{\"id\":\""<<CatalogId(key)<<"\",\"name\":\""<<Escape(ItemDisplayName(i.data))<<"\",\"icon\":\"/assets/items/"<<Escape(key)<<".png\",\"quantity\":"<<i.quantity<<'}';}out<<"]}";}
    const auto&catalog=AuthoritativeBuildingCatalog();out<<"],\"buildingTabs\":[";bool ft=true;
    for(const auto&tab:catalog.tabs){if(!ft)out<<',';ft=false;out<<"{\"key\":\""<<Escape(tab.key)<<"\",\"label\":\""<<Escape(tab.label)<<"\",\"order\":"<<tab.order<<",\"sections\":[";bool fs=true;for(const auto&section:tab.sections){if(!fs)out<<',';fs=false;out<<"{\"key\":\""<<Escape(section.key)<<"\",\"label\":\""<<Escape(section.label)<<"\",\"order\":"<<section.order<<",\"favourites\":"<<(section.favourites?"true":"false")<<",\"recipeIds\":[";bool fr=true;for(const auto&key:section.pieces){if(!present.count(key))continue;if(!fr)out<<',';fr=false;out<<'\"'<<CatalogId("building:"+key)<<'\"';}out<<"]}";}out<<"]}";}
    out<<"],\"count\":"<<records.size()<<",\"buildingCounts\":{\"expected\":"<<catalog.expectedPieces<<",\"resolved\":"<<resolvedBuildingPieces.load()<<",\"unavailable\":"<<unavailableBuildingPieces.load()<<",\"references\":"<<catalog.expectedReferences<<"}}";return out.str();
}
}

