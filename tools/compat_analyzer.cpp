#include <algorithm>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

struct SymbolRecord { std::uint64_t address, unused; std::uint32_t nameOffset; } __attribute__((packed));
struct ElfHeader { unsigned char ident[16]; std::uint16_t type,machine; std::uint32_t version; std::uint64_t entry,phoff,shoff; std::uint32_t flags; std::uint16_t ehsize,phentsize,phnum; } __attribute__((packed));
struct ProgramHeader { std::uint32_t type,flags; std::uint64_t offset,vaddr,paddr,filesz,memsz,align; } __attribute__((packed));

static std::string Sha256(const std::string& path) {
    std::string command="sha256sum \""+path+"\"";
    FILE* pipe=popen(command.c_str(),"r"); char value[65]{};
    if(!pipe||std::fread(value,1,64,pipe)!=64){if(pipe)pclose(pipe);return {};}
    pclose(pipe); return std::string(value,64);
}

int main(int argc,char** argv){
    const bool dumpNativeOffsets=argc==4&&std::string(argv[3])=="--native-offsets";
    if(argc!=3&&!dumpNativeOffsets){std::cerr<<"usage: zanik-compat-analyzer BINARY SYM [--native-offsets]\n";return 2;}
    std::ifstream sf(argv[2],std::ios::binary); std::uint32_t count=0; sf.read((char*)&count,4);
    if(!sf||count<1||count>30000000){std::cerr<<"invalid symbol file\n";return 3;}
    std::vector<SymbolRecord> records(count); sf.read((char*)records.data(),records.size()*sizeof(SymbolRecord));
    std::string names((std::istreambuf_iterator<char>(sf)),{});
    const std::string killed="UDominionAISubsystem::OnAIKilled(ADominionAICharacter*, AActor*, FDominionDamageEvent const&)";
    const std::string death="ADominionAICharacter::OnDeath()";
    const std::string display="FTextInspector::GetDisplayString(FText const&)";
    const std::string mapRegion="DominionFunctionLibrary::GetMapRegionAtLocation(UObject const&, UE::Math::TVector<double> const&)";
    const std::string regionNameGetter="AMapRegion::execGetRegionName(UObject*, FFrame&, void*)";
    const std::string dungeon="DominionFunctionLibrary::GetDungeonAtLocation(UObject const&, UE::Math::TVector<double> const&)";
    const std::string registryInstance="AGameplayObjectRegistry::GetInstance(UWorld*)";
    const std::string registryLodestones="AGameplayObjectRegistry::GetLodestones() const";
    const std::string lodestoneLocation="FLodestoneData::GetLodestoneLocation(UWorld*) const";
    const std::string nodestoneBeginPlay="ASwitchableTeleporter::BeginPlay()";
    const std::string nodestoneSubsystem="ASwitchableTeleporter::GetTeleportationSubsystem() const";
    const std::string teleportTargetNames="UTeleportationSubsystem::GetTargetLocationNames() const";
    const std::string teleportTargetTransform="UTeleportationSubsystem::GetTargetLocationTransform(FName const&, UE::Math::TTransform<double>&)";
    const std::string labelTextGetter="UMapRegionIconComponent::execGetLabelText(UObject*, FFrame&, void*)";
    const std::string labelRotationGetter="UMapRegionIconComponent::execGetLabelRotation(UObject*, FFrame&, void*)";
    const std::string labelFontGetter="UMapRegionIconComponent::execGetFontSizeOverride(UObject*, FFrame&, void*)";
    const std::string resourceUnavailable="ABaseInteractableResource::SetUnavailableDueToInteraction(ADominionPlayerCharacter*)";
    const std::string resourceSetAmount="UResourceRespawnComponent::SetResourceAmount(int)";
    const std::string resourceRespawn="UResourceRespawnComponent::OnRespawnTimerElapsed()";
    const std::string resourceInterval="UResourceRespawnComponent::GetRespawnIntervalInGameTimeAdjustedForPlayerAmount(IInGameTimeProvider&) const";
    const std::string gameTime="UInGameTimeFunctionLibrary::GetInGameTime(UObject const*)";
    const std::string playerTick="ADominionPlayerCharacter::TickActor(float, ELevelTick, FActorTickFunction&)";
    const std::string actorTeleport="AActor::TeleportTo(UE::Math::TVector<double> const&, UE::Math::TRotator<double> const&, bool, bool)";
    const std::string processEvent="UObject::ProcessEvent(UFunction*, void*)";
    const std::string questGetState="UQuestProgressComponent::execGetQuestState(UObject*, FFrame&, void*)";
    const std::string questCurrentObjective="UQuestProgressComponent::execGetCurrentObjectiveText(UObject*, FFrame&, void*)";
    const std::string lodestoneTryTeleport="UBaseTeleportationComponent::TryTeleport(FGuid const&, FGuid const&)";
    const std::string lodestoneCanTeleport="UBaseTeleportationComponent::CanTeleport(FGuid const&, FGuid const&) const";
    const std::string delayedTeleport="ADominionPlayerCharacter::TriggerDelayedTeleport(FPlayerTeleportParams const&, TArray<UE::Math::TVector<double>, TSizedDefaultAllocator<32> >, TFunction<void (bool)>)";
    const std::string delayedTeleportWrapper="ADominionPlayerCharacter::BP_TriggerDelayedTeleport(FPlayerTeleportParams const&)";
    const std::string teleportWithParams="UTeleportationSubsystem::TeleportWithParams(ADominionPlayerCharacter*, FPlayerTeleportParams&, FTeleportToNamedLocationResult)";
    const std::string teleportSuccess="UBaseTeleportationComponent::Client_OnTeleportSuccessful_Implementation(FVector_NetQuantize)";
    const std::string kismetLineTraceConstruct="Z_Construct_UFunction_UKismetSystemLibrary_LineTraceSingle()";
    const std::string kismetLineTraceExec="UKismetSystemLibrary::execLineTraceSingle(UObject*, FFrame&, void*)";
    const std::string storageOwner="UOwnershipComponent::execGetOwningCharacterGuid(UObject*, FFrame&, void*)";
    const std::string storagePrivate="UOwnershipComponent::execIsPrivatelyOwned(UObject*, FFrame&, void*)";
    const std::string storageLinkable="ULinkedInventoryComponent::execIsLinkable(UObject*, FFrame&, void*)";
    const std::string storageItems="UInventoryComponent::GetAllItems(TArray<UItem*, TSizedDefaultAllocator<32> >&) const";
    const std::string storageItemData="UItem::execBP_GetItemData(UObject*, FFrame&, void*)";
    const std::string storageSetStackSize="UItem::SetStackSize(int)";
    const std::string storageAddItemByData="UInventoryComponent::AddItemByData(UItemData const*, int, float, FGameplayTagContainer const&)";
    const std::string storageCanAddItemByData="UInventoryComponent::CanAddItemByData(UItemData const*, int)";
    const std::string storageMoveItem="UInventoryComponent::MoveItem(int, UInventoryComponent*, int, ADominionPlayerController*, int)";
    const std::string storageRemoveFromSlot="UInventoryComponent::RemoveFromSlot(int, int, ADominionPlayerController*)";
    const std::string storageStackCount="UItemData::GetNumberOfStacksRequiredForCount(int) const";
    const std::string storageCategory="UItemData::execGetCategory(UObject*, FFrame&, void*)";
    const std::string storageBasePower="UItemData::execGetBasePowerLevel(UObject*, FFrame&, void*)";
    const std::string storageCategoryIcon="UItemData::execGetCategoryClassIcon(UObject*, FFrame&, void*)";
    const std::string storageEmptySlots="UInventoryComponent::GetNumEmptySlots() const";
    const std::string storageFilledSlots="UInventoryComponent::GetNumFilledSlots() const";
    const std::string storageMoveItemInternal="UInventoryComponent::MoveItemInternal(int, UInventoryComponent*, int, int, ADominionPlayerController*)";
    const std::string renameableGetCustomName="IRenameable::Execute_GetCustomName(UObject*)";
    const std::string renameableGetSetterId="IRenameable::Execute_GetSetterID(UObject*)";
    const std::string renameableSetCustomName="IRenameable::Execute_SetCustomName(UObject*, FText const&, FUniqueNetIdRepl const&)";
    const std::string fTextFromString="FText::FromString(char const*)";
    const std::string uniqueNetIdDestructor="FUniqueNetIdRepl::~FUniqueNetIdRepl()";
    const std::string fTextDestructor="FText::~FText()";
    const std::string storageCanAddItems="UInventoryComponent::CanAddItemsByData(TArray<FItemDataContainer, TSizedDefaultAllocator<32> > const&)";
    const std::string storageAddItems="UInventoryComponent::AddItemsByData(TArray<UItemData*, TSizedDefaultAllocator<32> > const&, TArray<int, TSizedDefaultAllocator<32> > const&, float, FGameplayTagContainer const&)";
    const std::string buildingRequirements="UBuildingPieceData::GetScaledResourceRequirements(UObject const*) const";
    const std::string memoryFree="FMemory::Free(void*)";
    const std::string worldTick="AActor::TickActor(float, ELevelTick, FActorTickFunction&)";
    const std::string buildingEndPlay="ABaseBuildingActor::EndPlay(EEndPlayReason::Type)";
    const std::string graveGetInventory="AGravestone::GetInventory() const";
    const std::string graveGetOwner="AGravestone::GetOwningCharacterGuid() const";
    const std::string graveCanAccess="AGravestone::CanPlayerAccessGravestone(ADominionPlayerController const*) const";
    const std::string graveRemove="AGravestone::Remove()";
    const std::string graveMoveAll="UInventoryComponent::MoveAllItems(UInventoryComponent*)";
    const std::string graveRegistry="AGameplayObjectRegistry::GetGravestones() const";
    const std::string graveRespawnActor="USpudState::RespawnActor(FSpudSpawnedActorData const&, FSpudClassMetadata const&, ULevel*)";
    const std::string graveRestoreFromLevel="USpudState::RestoreRuntimeSpawnedActorsFromLevel(UWorld const&, TFunction<bool (UE::Math::TBox<double> const&)> const&, FString const&)";
    const std::string graveRegisterStreaming="UWorldPartitionSubsystem::RegisterStreamingSourceProvider(IWorldPartitionStreamingSourceProvider*)";
    const std::string graveUnregisterStreaming="UWorldPartitionSubsystem::UnregisterStreamingSourceProvider(IWorldPartitionStreamingSourceProvider*)";
    const std::string graveStreamingComplete="UWorldPartitionSubsystem::IsStreamingCompleted(IWorldPartitionStreamingSourceProvider const*) const";
    const std::string graveProviderSources="IWorldPartitionStreamingSourceProvider::GetStreamingSources(TArray<FWorldPartitionStreamingSource, TSizedDefaultAllocator<32> >&) const";
    const std::string gravePlayerSources="APlayerController::GetStreamingSources(TArray<FWorldPartitionStreamingSource, TSizedDefaultAllocator<32> >&) const";
    const std::string graveStreamingSourceCopy="FWorldPartitionStreamingSource::FWorldPartitionStreamingSource(FWorldPartitionStreamingSource const&)";
    const std::string graveStreamingSourceDestroy="FWorldPartitionStreamingSource::~FWorldPartitionStreamingSource()";
    std::uint64_t killedAt=0,deathAt=0,deathEnd=0,displayAt=0,mapRegionAt=0,regionNameGetterAt=0,dungeonAt=0;
    std::uint64_t registryInstanceAt=0,registryLodestonesAt=0,lodestoneLocationAt=0; int killedNames=0;
    std::uint64_t nodestoneBeginPlayAt=0,nodestoneSubsystemAt=0;
    std::uint64_t teleportTargetNamesAt=0,teleportTargetTransformAt=0;
    std::uint64_t labelTextGetterAt=0,labelRotationGetterAt=0,labelFontGetterAt=0;
    std::uint64_t resourceUnavailableAt=0,resourceSetAmountAt=0,resourceRespawnAt=0,resourceIntervalAt=0,gameTimeAt=0,foliageDepictableAt=0;
    std::uint64_t playerTickAt=0,actorTeleportAt=0,processEventAt=0,lodestoneTryTeleportAt=0,lodestoneCanTeleportAt=0;
    std::uint64_t questGetStateAt=0,questCurrentObjectiveAt=0;
    std::uint64_t delayedTeleportAt=0,delayedTeleportWrapperAt=0,teleportWithParamsAt=0,teleportSuccessAt=0,buildingEndPlayAt=0;
    std::uint64_t kismetLineTraceConstructAt=0,kismetLineTraceExecAt=0;
    std::uint64_t storageOwnerAt=0,storagePrivateAt=0,storageLinkableAt=0,storageItemsAt=0,storageItemDataAt=0,storageSetStackSizeAt=0;
    std::uint64_t storageAddItemByDataAt=0,storageCanAddItemByDataAt=0,storageMoveItemAt=0,storageRemoveFromSlotAt=0;
    std::uint64_t storageStackCountAt=0,storageCategoryAt=0,storageBasePowerAt=0,storageCategoryIconAt=0,worldTickAt=0;
    std::uint64_t storageCanAddItemsAt=0,storageAddItemsAt=0,buildingRequirementsAt=0,memoryFreeAt=0;
    std::uint64_t storageEmptySlotsAt=0,storageFilledSlotsAt=0;
    std::uint64_t storageMoveItemInternalAt=0,renameableGetCustomNameAt=0,renameableGetSetterIdAt=0,renameableSetCustomNameAt=0,fTextFromStringAt=0,uniqueNetIdDestructorAt=0,fTextDestructorAt=0;
    std::uint64_t graveGetInventoryAt=0,graveGetOwnerAt=0,graveCanAccessAt=0,graveRemoveAt=0,graveMoveAllAt=0,graveRegistryAt=0;
    std::uint64_t graveRespawnActorAt=0,graveRestoreFromLevelAt=0,graveRegisterStreamingAt=0,graveUnregisterStreamingAt=0,graveStreamingCompleteAt=0,graveProviderSourcesAt=0,gravePlayerSourcesAt=0,graveStreamingSourceCopyAt=0,graveStreamingSourceDestroyAt=0;
    auto nameOf=[&](const SymbolRecord&r){if(r.nameOffset>=names.size())return std::string();auto e=names.find('\n',r.nameOffset);return names.substr(r.nameOffset,e-r.nameOffset);};
    for(const auto&r:records){auto n=nameOf(r);if(n==killed){killedAt=killedAt?std::min(killedAt,r.address):r.address;++killedNames;}if(n==death){deathAt=deathAt?std::min(deathAt,r.address):r.address;deathEnd=std::max(deathEnd,r.address+16);}if(n==display)displayAt=displayAt?std::min(displayAt,r.address):r.address;if(n==mapRegion)mapRegionAt=mapRegionAt?std::min(mapRegionAt,r.address):r.address;if(n==regionNameGetter)regionNameGetterAt=regionNameGetterAt?std::min(regionNameGetterAt,r.address):r.address;if(n==dungeon)dungeonAt=dungeonAt?std::min(dungeonAt,r.address):r.address;if(n==registryInstance)registryInstanceAt=registryInstanceAt?std::min(registryInstanceAt,r.address):r.address;if(n==registryLodestones)registryLodestonesAt=registryLodestonesAt?std::min(registryLodestonesAt,r.address):r.address;if(n==lodestoneLocation)lodestoneLocationAt=lodestoneLocationAt?std::min(lodestoneLocationAt,r.address):r.address;if(n==nodestoneBeginPlay)nodestoneBeginPlayAt=nodestoneBeginPlayAt?std::min(nodestoneBeginPlayAt,r.address):r.address;if(n==nodestoneSubsystem)nodestoneSubsystemAt=nodestoneSubsystemAt?std::min(nodestoneSubsystemAt,r.address):r.address;if(n==teleportTargetNames)teleportTargetNamesAt=teleportTargetNamesAt?std::min(teleportTargetNamesAt,r.address):r.address;if(n==teleportTargetTransform)teleportTargetTransformAt=teleportTargetTransformAt?std::min(teleportTargetTransformAt,r.address):r.address;if(n==labelTextGetter)labelTextGetterAt=labelTextGetterAt?std::min(labelTextGetterAt,r.address):r.address;if(n==labelRotationGetter)labelRotationGetterAt=labelRotationGetterAt?std::min(labelRotationGetterAt,r.address):r.address;if(n==labelFontGetter)labelFontGetterAt=labelFontGetterAt?std::min(labelFontGetterAt,r.address):r.address;if(n==resourceUnavailable)resourceUnavailableAt=resourceUnavailableAt?std::min(resourceUnavailableAt,r.address):r.address;if(n==resourceSetAmount)resourceSetAmountAt=resourceSetAmountAt?std::min(resourceSetAmountAt,r.address):r.address;if(n==resourceRespawn)resourceRespawnAt=resourceRespawnAt?std::min(resourceRespawnAt,r.address):r.address;if(n==resourceInterval)resourceIntervalAt=resourceIntervalAt?std::min(resourceIntervalAt,r.address):r.address;if(n==gameTime)gameTimeAt=gameTimeAt?std::min(gameTimeAt,r.address):r.address;if(n.find("AWorldActor::GetFoliageDepictableComponent_Implementation()")!=std::string::npos)foliageDepictableAt=foliageDepictableAt?std::min(foliageDepictableAt,r.address):r.address;}
    for(const auto&r:records){auto n=nameOf(r);if(n==playerTick)playerTickAt=playerTickAt?std::min(playerTickAt,r.address):r.address;if(n==actorTeleport)actorTeleportAt=actorTeleportAt?std::min(actorTeleportAt,r.address):r.address;if(n==processEvent)processEventAt=processEventAt?std::min(processEventAt,r.address):r.address;if(n==lodestoneTryTeleport)lodestoneTryTeleportAt=lodestoneTryTeleportAt?std::min(lodestoneTryTeleportAt,r.address):r.address;if(n==lodestoneCanTeleport)lodestoneCanTeleportAt=lodestoneCanTeleportAt?std::min(lodestoneCanTeleportAt,r.address):r.address;if(n==storageOwner)storageOwnerAt=storageOwnerAt?std::min(storageOwnerAt,r.address):r.address;if(n==storagePrivate)storagePrivateAt=storagePrivateAt?std::min(storagePrivateAt,r.address):r.address;if(n==storageLinkable)storageLinkableAt=storageLinkableAt?std::min(storageLinkableAt,r.address):r.address;if(n==storageItems)storageItemsAt=storageItemsAt?std::min(storageItemsAt,r.address):r.address;if(n==storageItemData)storageItemDataAt=storageItemDataAt?std::min(storageItemDataAt,r.address):r.address;if(n==storageSetStackSize)storageSetStackSizeAt=storageSetStackSizeAt?std::min(storageSetStackSizeAt,r.address):r.address;if(n==storageAddItemByData)storageAddItemByDataAt=storageAddItemByDataAt?std::min(storageAddItemByDataAt,r.address):r.address;if(n==storageCanAddItemByData)storageCanAddItemByDataAt=storageCanAddItemByDataAt?std::min(storageCanAddItemByDataAt,r.address):r.address;if(n==storageMoveItem)storageMoveItemAt=storageMoveItemAt?std::min(storageMoveItemAt,r.address):r.address;if(n==storageRemoveFromSlot)storageRemoveFromSlotAt=storageRemoveFromSlotAt?std::min(storageRemoveFromSlotAt,r.address):r.address;if(n==storageStackCount)storageStackCountAt=storageStackCountAt?std::min(storageStackCountAt,r.address):r.address;if(n==storageCategory)storageCategoryAt=storageCategoryAt?std::min(storageCategoryAt,r.address):r.address;if(n==storageBasePower)storageBasePowerAt=storageBasePowerAt?std::min(storageBasePowerAt,r.address):r.address;if(n==storageCategoryIcon)storageCategoryIconAt=storageCategoryIconAt?std::min(storageCategoryIconAt,r.address):r.address;if(n==worldTick)worldTickAt=worldTickAt?std::min(worldTickAt,r.address):r.address;}
    for(const auto&r:records){auto n=nameOf(r);if(n==delayedTeleport)delayedTeleportAt=delayedTeleportAt?std::min(delayedTeleportAt,r.address):r.address;if(n==delayedTeleportWrapper)delayedTeleportWrapperAt=delayedTeleportWrapperAt?std::min(delayedTeleportWrapperAt,r.address):r.address;if(n==teleportWithParams)teleportWithParamsAt=teleportWithParamsAt?std::min(teleportWithParamsAt,r.address):r.address;if(n==teleportSuccess)teleportSuccessAt=teleportSuccessAt?std::min(teleportSuccessAt,r.address):r.address;if(n==buildingEndPlay)buildingEndPlayAt=buildingEndPlayAt?std::min(buildingEndPlayAt,r.address):r.address;}
    for(const auto&r:records){
        const auto n=nameOf(r);
        if(n==kismetLineTraceConstruct)kismetLineTraceConstructAt=kismetLineTraceConstructAt?std::min(kismetLineTraceConstructAt,r.address):r.address;
        if(n==kismetLineTraceExec)kismetLineTraceExecAt=kismetLineTraceExecAt?std::min(kismetLineTraceExecAt,r.address):r.address;
    }
    for(const auto&r:records){auto n=nameOf(r);if(n==questGetState)questGetStateAt=questGetStateAt?std::min(questGetStateAt,r.address):r.address;if(n==questCurrentObjective)questCurrentObjectiveAt=questCurrentObjectiveAt?std::min(questCurrentObjectiveAt,r.address):r.address;}
    if(!killedAt||!deathAt||!displayAt||!mapRegionAt||!regionNameGetterAt||!dungeonAt||killedNames<1){std::cerr<<"required exact symbols missing\n";return 4;}
    if(!questGetStateAt||!questCurrentObjectiveAt){std::cerr<<"quest progress query symbols missing\n";return 4;}
    if(!registryInstanceAt||!registryLodestonesAt||!lodestoneLocationAt){std::cerr<<"lodestone registry symbols missing\n";return 4;}
    if(!nodestoneBeginPlayAt||!nodestoneSubsystemAt){std::cerr<<"nodestone teleporter symbols missing\n";return 4;}
    if(!teleportTargetNamesAt||!teleportTargetTransformAt){std::cerr<<"teleport destination registry symbols missing\n";return 4;}
    if(!labelTextGetterAt||!labelRotationGetterAt||!labelFontGetterAt){std::cerr<<"map label getter symbols missing\n";return 4;}
    if(!resourceUnavailableAt||!resourceSetAmountAt||!resourceRespawnAt||!resourceIntervalAt||!gameTimeAt||!foliageDepictableAt){std::cerr<<"resource tracking symbols missing\n";return 4;}
    if(!playerTickAt||!actorTeleportAt||!processEventAt||!lodestoneTryTeleportAt||!lodestoneCanTeleportAt){std::cerr<<"admin teleport symbols missing\n";return 4;}
    if(!delayedTeleportAt||!delayedTeleportWrapperAt||!teleportWithParamsAt||!teleportSuccessAt){std::cerr<<"admin delayed teleport, wrapper, effect, or parameter pipeline symbols missing\n";return 4;}
    if(!kismetLineTraceConstructAt||!kismetLineTraceExecAt){std::cerr<<"storage teleport ground-trace reflection symbols missing\n";return 4;}
    if(!storageOwnerAt||!storagePrivateAt||!storageLinkableAt||!storageItemsAt||!storageItemDataAt||!storageSetStackSizeAt){std::cerr<<"storage symbols missing\n";return 4;}
    if(!storageAddItemByDataAt||!storageCanAddItemByDataAt||!storageMoveItemAt||!storageRemoveFromSlotAt||!storageStackCountAt||!worldTickAt){std::cerr<<"storage mutation symbols missing\n";return 4;}
    if(!buildingEndPlayAt){std::cerr<<"storage removal tracking symbol missing\n";return 4;}
    for(const auto&r:records){auto n=nameOf(r);if(n==storageCanAddItems)storageCanAddItemsAt=storageCanAddItemsAt?std::min(storageCanAddItemsAt,r.address):r.address;if(n==storageAddItems)storageAddItemsAt=storageAddItemsAt?std::min(storageAddItemsAt,r.address):r.address;if(n==buildingRequirements)buildingRequirementsAt=buildingRequirementsAt?std::min(buildingRequirementsAt,r.address):r.address;if(n==memoryFree)memoryFreeAt=memoryFreeAt?std::min(memoryFreeAt,r.address):r.address;if(n==storageEmptySlots)storageEmptySlotsAt=storageEmptySlotsAt?std::min(storageEmptySlotsAt,r.address):r.address;if(n==storageFilledSlots)storageFilledSlotsAt=storageFilledSlotsAt?std::min(storageFilledSlotsAt,r.address):r.address;if(n==storageMoveItemInternal)storageMoveItemInternalAt=storageMoveItemInternalAt?std::min(storageMoveItemInternalAt,r.address):r.address;if(n==renameableGetCustomName)renameableGetCustomNameAt=renameableGetCustomNameAt?std::min(renameableGetCustomNameAt,r.address):r.address;if(n==renameableGetSetterId)renameableGetSetterIdAt=renameableGetSetterIdAt?std::min(renameableGetSetterIdAt,r.address):r.address;if(n==renameableSetCustomName)renameableSetCustomNameAt=renameableSetCustomNameAt?std::min(renameableSetCustomNameAt,r.address):r.address;if(n==fTextFromString)fTextFromStringAt=fTextFromStringAt?std::min(fTextFromStringAt,r.address):r.address;if(n==uniqueNetIdDestructor)uniqueNetIdDestructorAt=uniqueNetIdDestructorAt?std::min(uniqueNetIdDestructorAt,r.address):r.address;if(n==fTextDestructor)fTextDestructorAt=fTextDestructorAt?std::min(fTextDestructorAt,r.address):r.address;}
    for(const auto&r:records){auto n=nameOf(r);if(n==graveGetInventory)graveGetInventoryAt=graveGetInventoryAt?std::min(graveGetInventoryAt,r.address):r.address;if(n==graveGetOwner)graveGetOwnerAt=graveGetOwnerAt?std::min(graveGetOwnerAt,r.address):r.address;if(n==graveCanAccess)graveCanAccessAt=graveCanAccessAt?std::min(graveCanAccessAt,r.address):r.address;if(n==graveRemove)graveRemoveAt=graveRemoveAt?std::min(graveRemoveAt,r.address):r.address;if(n==graveMoveAll)graveMoveAllAt=graveMoveAllAt?std::min(graveMoveAllAt,r.address):r.address;if(n==graveRegistry)graveRegistryAt=graveRegistryAt?std::min(graveRegistryAt,r.address):r.address;if(n==graveRespawnActor)graveRespawnActorAt=graveRespawnActorAt?std::min(graveRespawnActorAt,r.address):r.address;if(n==graveRestoreFromLevel)graveRestoreFromLevelAt=graveRestoreFromLevelAt?std::min(graveRestoreFromLevelAt,r.address):r.address;if(n==graveRegisterStreaming)graveRegisterStreamingAt=graveRegisterStreamingAt?std::min(graveRegisterStreamingAt,r.address):r.address;if(n==graveUnregisterStreaming)graveUnregisterStreamingAt=graveUnregisterStreamingAt?std::min(graveUnregisterStreamingAt,r.address):r.address;if(n==graveStreamingComplete)graveStreamingCompleteAt=graveStreamingCompleteAt?std::min(graveStreamingCompleteAt,r.address):r.address;if(n==graveProviderSources)graveProviderSourcesAt=graveProviderSourcesAt?std::min(graveProviderSourcesAt,r.address):r.address;if(n==gravePlayerSources)gravePlayerSourcesAt=gravePlayerSourcesAt?std::min(gravePlayerSourcesAt,r.address):r.address;if(n==graveStreamingSourceCopy)graveStreamingSourceCopyAt=graveStreamingSourceCopyAt?std::min(graveStreamingSourceCopyAt,r.address):r.address;if(n==graveStreamingSourceDestroy)graveStreamingSourceDestroyAt=graveStreamingSourceDestroyAt?std::min(graveStreamingSourceDestroyAt,r.address):r.address;}
    if(!storageCategoryAt||!storageBasePowerAt||!storageCategoryIconAt){std::cerr<<"storage catalog metadata symbols missing\n";return 4;}
    if(!storageCanAddItemsAt||!storageAddItemsAt||!buildingRequirementsAt||!memoryFreeAt){std::cerr<<"recipe batch, building requirement, or allocator symbols missing\n";return 4;}
    if(!storageEmptySlotsAt||!storageFilledSlotsAt){std::cerr<<"storage slot count symbols missing\n";return 4;}
    if(!storageMoveItemInternalAt||!renameableGetCustomNameAt||!renameableGetSetterIdAt||!renameableSetCustomNameAt||!fTextFromStringAt||!uniqueNetIdDestructorAt||!fTextDestructorAt){std::cerr<<"storage organization or rename symbols missing\n";return 4;}
    if(!graveGetInventoryAt||!graveGetOwnerAt||!graveCanAccessAt||!graveRemoveAt||!graveMoveAllAt||!graveRegistryAt){std::cerr<<"gravestone ownership, inventory, registry, transfer, or removal symbols missing\n";return 4;}
    if(!graveRespawnActorAt||!graveRestoreFromLevelAt||!graveRegisterStreamingAt||!graveUnregisterStreamingAt||!graveStreamingCompleteAt||!graveProviderSourcesAt||!gravePlayerSourcesAt||!graveStreamingSourceCopyAt||!graveStreamingSourceDestroyAt){std::cerr<<"gravestone SPUD restore or World Partition streaming symbols missing\n";return 4;}
    std::ifstream bf(argv[1],std::ios::binary);std::vector<unsigned char> binary((std::istreambuf_iterator<char>(bf)),{});
    if(binary.size()<sizeof(ElfHeader)){std::cerr<<"invalid ELF\n";return 5;} auto* eh=(ElfHeader*)binary.data();
    if(std::memcmp(eh->ident,"\177ELF",4)||eh->phoff+std::uint64_t(eh->phnum)*eh->phentsize>binary.size()){std::cerr<<"invalid ELF headers\n";return 5;}
    std::uint64_t baseVma=~0ULL,textOffset=0,textSize=0,textFile=0;
    for(unsigned i=0;i<eh->phnum;++i){auto*p=(ProgramHeader*)(binary.data()+eh->phoff+i*eh->phentsize);if(p->type==1)baseVma=std::min(baseVma,p->vaddr);}
    for(unsigned i=0;i<eh->phnum;++i){auto*p=(ProgramHeader*)(binary.data()+eh->phoff+i*eh->phentsize);if(p->type==1&&(p->flags&1)){textOffset=p->vaddr-baseVma;textSize=p->filesz;textFile=p->offset;}}
    auto fileAt=[&](std::uint64_t sym)->std::uint64_t{std::uint64_t v=sym+baseVma;for(unsigned i=0;i<eh->phnum;++i){auto*p=(ProgramHeader*)(binary.data()+eh->phoff+i*eh->phentsize);if(p->type==1&&v>=p->vaddr&&v<p->vaddr+p->filesz)return p->offset+(v-p->vaddr);}return UINT64_MAX;};
    const unsigned char sig[]={0x55,0x41,0x57,0x41,0x56,0x41,0x55,0x41,0x54,0x53,0x48,0x83,0xEC,0x18,0x48,0x89,0x4C,0x24,0x10,0x8B,0x6F,0x50,0x8B,0x47,0x5C,0x8D,0x48,0x01};
    auto targetFile=fileAt(killedAt);if(targetFile==~0ULL||targetFile+sizeof(sig)>binary.size()||std::memcmp(binary.data()+targetFile,sig,sizeof(sig))){std::cerr<<"fatal hook signature mismatch\n";return 6;}
    size_t matches=0;for(size_t i=textFile;i+sizeof(sig)<=textFile+textSize;++i)if(!std::memcmp(binary.data()+i,sig,sizeof(sig)))++matches;
    if(matches!=1){std::cerr<<"fatal hook signature is not unique\n";return 7;}
    auto deathFile=fileAt(deathAt);auto deathLimit=fileAt(deathEnd);bool caller=false;
    if(deathFile!=~0ULL&&deathLimit!=~0ULL)for(size_t i=deathFile;i+5<=deathLimit;++i)if(binary[i]==0xE8){std::int32_t d;std::memcpy(&d,binary.data()+i+1,4);std::uint64_t callSym=(i-textFile+textOffset)+5+d;if(callSym==killedAt){caller=true;break;}}
    if(!caller){std::cerr<<"OnDeath no longer directly calls OnAIKilled\n";return 8;}
    auto getterFile=fileAt(regionNameGetterAt);std::uint32_t regionNameOffset=0;int getterCalls=0;
    if(getterFile==~0ULL||getterFile+64>binary.size()){std::cerr<<"region name getter outside ELF\n";return 9;}
    for(size_t i=getterFile;i+5<getterFile+64;++i)if(binary[i]==0xE8){
        std::int32_t d;std::memcpy(&d,binary.data()+i+1,4);std::int64_t target=std::int64_t(i)+5+d;
        if(target>=0&&std::uint64_t(target)+8<=binary.size()&&binary[target]==0x48&&binary[target+1]==0x8d&&binary[target+2]==0x87&&binary[target+7]==0xc3){
            std::memcpy(&regionNameOffset,binary.data()+target+3,4);++getterCalls;
        }
    }
    if(getterCalls!=1||regionNameOffset<0x100||regionNameOffset>0x2000){std::cerr<<"region name getter layout changed\n";return 9;}
    auto reflectedOffset=[&](const std::string& property,std::uint32_t minimum,std::uint32_t maximum){
        std::uint32_t result=0;int fields=0;
        for(size_t stringFile=0;stringFile<binary.size();++stringFile){
            auto found=std::search(binary.begin()+stringFile,binary.end(),property.begin(),property.end());
            if(found==binary.end())break;stringFile=static_cast<size_t>(found-binary.begin());
            if(stringFile+property.size()>=binary.size()||binary[stringFile+property.size()]!=0)continue;
            std::uint64_t stringVma=0;
            for(unsigned i=0;i<eh->phnum;++i){auto*p=(ProgramHeader*)(binary.data()+eh->phoff+i*eh->phentsize);if(p->type==1&&stringFile>=p->offset&&stringFile<p->offset+p->filesz){stringVma=p->vaddr+(stringFile-p->offset);break;}}
            if(!stringVma)continue;
            for(size_t descriptor=0;descriptor+56<=binary.size();++descriptor){
                std::uint64_t pointer=0;std::memcpy(&pointer,binary.data()+descriptor,8);if(pointer!=stringVma)continue;
                std::uint32_t encoded=0;std::memcpy(&encoded,binary.data()+descriptor+48,4);
                std::uint32_t offset=encoded>>16;if((encoded&0xffff)==1&&offset>=minimum&&offset<maximum){result=offset;++fields;}
            }
        }
        return fields==1?result:0;
    };
    std::uint32_t dungeonNameOffset=reflectedOffset("UserVisibleName",0x30,0xC0);
    std::uint32_t dungeonEntranceOffset=reflectedOffset("EntranceLocation",0x30,0xC0);
    if(dungeonNameOffset!=0x88||dungeonEntranceOffset!=0x38){std::cerr<<"dungeon field layout changed\n";return 9;}
    for(auto address:{registryInstanceAt,registryLodestonesAt,lodestoneLocationAt}){
        auto at=fileAt(address);if(at==~0ULL||at+8>binary.size()){
            std::cerr<<"lodestone function outside ELF\n";return 9;
        }
        bool any=false;for(size_t i=0;i<8;++i)any|=binary[at+i]!=0;
        if(!any){std::cerr<<"invalid lodestone function prologue\n";return 9;}
    }
    const unsigned char lodestoneArrayLayout[]={0x41,0x57,0x41,0x56,0x53,0x48,0x83,0xec,0x10,0x48,0x89,0xf8,0x80,0xbe,0x30,0x03,0x00,0x00,0x00};
    auto registryGetterFile=fileAt(registryLodestonesAt);
    if(registryGetterFile==~0ULL||registryGetterFile+sizeof(lodestoneArrayLayout)>binary.size()||
       std::memcmp(binary.data()+registryGetterFile,lodestoneArrayLayout,sizeof(lodestoneArrayLayout))){
        std::cerr<<"lodestone registry layout signature changed\n";return 9;
    }
    for(auto address:{nodestoneBeginPlayAt,nodestoneSubsystemAt}){
        auto at=fileAt(address);if(at==~0ULL||at+8>binary.size()){
            std::cerr<<"nodestone function outside ELF\n";return 9;
        }
        bool any=false;for(size_t i=0;i<8;++i)any|=binary[at+i]!=0;
        if(!any){std::cerr<<"invalid nodestone function prologue\n";return 9;}
    }
    const unsigned char labelRotationLayout[]={0x48,0x8b,0x46,0x20,0x48,0x83,0xf8,0x01,0x48,0x83,0xd8,0xff,0x48,0x89,0x46,0x20,0xf3,0x0f,0x10,0x87,0xd0,0x07,0x00,0x00};
    const unsigned char labelFontLayout[]={0x48,0x8b,0x46,0x20,0x48,0x83,0xf8,0x01,0x48,0x83,0xd8,0xff,0x48,0x89,0x46,0x20,0xf3,0x0f,0x10,0x87,0xd4,0x07,0x00,0x00};
    const unsigned char labelTextLayout[]={0x41,0x56,0x53,0x48,0x83,0xec,0x18};
    for(const auto& check:{std::make_pair(labelRotationGetterAt,std::make_pair(labelRotationLayout,sizeof(labelRotationLayout))),std::make_pair(labelFontGetterAt,std::make_pair(labelFontLayout,sizeof(labelFontLayout))),std::make_pair(labelTextGetterAt,std::make_pair(labelTextLayout,sizeof(labelTextLayout)))}){
        auto at=fileAt(check.first);if(at==~0ULL||at+check.second.second>binary.size()||
           std::memcmp(binary.data()+at,check.second.first,check.second.second)){
            std::cerr<<"map label getter layout changed\n";return 9;
        }
    }
    const unsigned char unavailableLayout[]={0x41,0x57,0x41,0x56,0x41,0x54,0x53,0x50,0x80,0xbf,0x70,0x01,0x00,0x00,0x03,0x0f,0x85,0x01,0x01,0x00,0x00,0x48,0x89,0xfb,0x80,0xbf,0x44,0x04,0x00,0x00,0x00};
    const unsigned char setAmountLayout[]={0x41,0x56,0x53,0x50,0x48,0x89,0xfb,0x8b,0x87,0xdc,0x00,0x00,0x00,0x39,0xc6};
    const unsigned char respawnLayout[]={0x55,0x41,0x57,0x41,0x56,0x41,0x54,0x53,0x48,0x83,0xec,0x10,0x8b,0x87,0xd8,0x00,0x00,0x00,0x8b,0xaf,0xdc,0x00,0x00,0x00,0x2b,0xaf,0x70,0x01,0x00,0x00,0x39,0xc5};
    for(const auto& check:{std::make_pair(resourceUnavailableAt,std::make_pair(unavailableLayout,sizeof(unavailableLayout))),std::make_pair(resourceSetAmountAt,std::make_pair(setAmountLayout,sizeof(setAmountLayout))),std::make_pair(resourceRespawnAt,std::make_pair(respawnLayout,sizeof(respawnLayout)))}){
        auto at=fileAt(check.first);if(at==~0ULL||at+check.second.second>binary.size()||
           std::memcmp(binary.data()+at,check.second.first,check.second.second)){
            std::cerr<<"resource hook signature changed\n";return 10;
        }
        size_t found=0;for(size_t i=textFile;i+check.second.second<=textFile+textSize;++i)
            if(!std::memcmp(binary.data()+i,check.second.first,check.second.second))++found;
        if(found!=1){std::cerr<<"resource hook signature is not unique\n";return 10;}
    }
    const unsigned char playerTickLayout[]={0x55,0x41,0x57,0x41,0x56,0x53,0x48,0x83,0xec,0x28,0xf3,0x0f,0x11,0x44,0x24,0x24};
    auto playerTickFile=fileAt(playerTickAt);
    if(playerTickFile==~0ULL||playerTickFile+sizeof(playerTickLayout)>binary.size()||std::memcmp(binary.data()+playerTickFile,playerTickLayout,sizeof(playerTickLayout))){std::cerr<<"admin player tick signature changed\n";return 11;}
    size_t playerTickMatches=0;for(size_t i=textFile;i+sizeof(playerTickLayout)<=textFile+textSize;++i)if(!std::memcmp(binary.data()+i,playerTickLayout,sizeof(playerTickLayout)))++playerTickMatches;
    if(playerTickMatches!=1){std::cerr<<"admin player tick signature is not unique\n";return 11;}
    const unsigned char tryTeleportLayout[]={0x55,0x41,0x57,0x41,0x56,0x41,0x55,0x41,0x54,0x53,0x48,0x81,0xec,0x08,0x01,0x00,0x00,0x49,0x89,0xd6,0x49,0x89,0xf7,0x48,0x89,0xfb,0x48,0x8b,0x07,0xff,0x90,0x80,0x05,0x00,0x00};
    const unsigned char canTeleportLayout[]={0x55,0x41,0x57,0x41,0x56,0x41,0x55,0x41,0x54,0x53,0x48,0x83,0xec,0x48,0x49,0x89,0xd7,0x49,0x89,0xf6,0x48,0x89,0xfb,0xf3,0x0f,0x6f,0x06,0x66,0x0f,0x38,0x17,0xc0};
    for(const auto& check:{std::make_pair(lodestoneTryTeleportAt,std::make_pair(tryTeleportLayout,sizeof(tryTeleportLayout))),std::make_pair(lodestoneCanTeleportAt,std::make_pair(canTeleportLayout,sizeof(canTeleportLayout)))}){
        auto at=fileAt(check.first);if(at==~0ULL||at+check.second.second>binary.size()||std::memcmp(binary.data()+at,check.second.first,check.second.second)){std::cerr<<"native lodestone teleport signature changed\n";return 11;}
        size_t found=0;for(size_t i=textFile;i+check.second.second<=textFile+textSize;++i)if(!std::memcmp(binary.data()+i,check.second.first,check.second.second))++found;
        if(found!=1){std::cerr<<"native lodestone teleport signature is not unique\n";return 11;}
    }
    for(auto address:{actorTeleportAt,processEventAt}){auto at=fileAt(address);if(at==~0ULL||at+8>binary.size()){std::cerr<<"admin teleport function outside ELF\n";return 11;}bool any=false;for(size_t i=0;i<8;++i)any|=binary[at+i]!=0;if(!any){std::cerr<<"invalid admin teleport function prologue\n";return 11;}}
    for(auto address:{delayedTeleportAt,delayedTeleportWrapperAt,teleportWithParamsAt,teleportSuccessAt}){auto at=fileAt(address);if(at==~0ULL||at+16>binary.size()){std::cerr<<"admin delayed teleport pipeline outside ELF\n";return 11;}bool any=false;for(size_t i=0;i<16;++i)any|=binary[at+i]!=0;if(!any){std::cerr<<"invalid admin delayed teleport pipeline function\n";return 11;}}
    if(!(delayedTeleportAt<delayedTeleportWrapperAt&&delayedTeleportWrapperAt<teleportWithParamsAt)){std::cerr<<"admin delayed teleport call-family relationship changed\n";return 11;}
    const unsigned char delayedLayout[]={0x41,0x57,0x41,0x56,0x41,0x55,0x41,0x54,0x53,0x48,0x81,0xec,0x10,0x01,0x00,0x00};
    auto delayedFile=fileAt(delayedTeleportAt);bool delayedSignature=false;if(delayedFile!=~0ULL&&delayedFile>=32)for(size_t i=delayedFile-32;i+sizeof(delayedLayout)<=delayedFile+16;++i)if(!std::memcmp(binary.data()+i,delayedLayout,sizeof(delayedLayout))){delayedSignature=true;break;}
    if(!delayedSignature)std::cerr<<"warning: admin delayed teleport capture signature changed; runtime teleport gate must validate independently\n";
    const unsigned char ownerLayout[]={0x48,0x8b,0x46,0x20,0x48,0x83,0xf8,0x01,0x48,0x83,0xd8,0xff,0x48,0x89,0x46,0x20,0x0f,0x10,0x87,0x58,0x01,0x00,0x00};
    const unsigned char privateLayout[]={0x48,0x8b,0x46,0x20,0x48,0x83,0xf8,0x01,0x48,0x83,0xd8,0xff,0x48,0x89,0x46,0x20,0x0f,0xb6,0x87,0x69,0x01,0x00,0x00};
    const unsigned char linkableLayout[]={0x48,0x8b,0x46,0x20,0x48,0x83,0xf8,0x01,0x48,0x83,0xd8,0xff,0x48,0x89,0x46,0x20,0x0f,0xb6,0x87,0xf8,0x00,0x00,0x00};
    const unsigned char itemsLayout[]={0x55,0x41,0x57,0x41,0x56,0x41,0x55,0x41,0x54,0x53,0x50,0x4c,0x63,0xbf,0x70,0x01,0x00,0x00};
    const unsigned char itemDataLayout[]={0x53,0x48,0x89,0xd3,0x48,0x8b,0x46,0x20,0x48,0x83,0xf8,0x01,0x48,0x83,0xd8,0xff};
    for(const auto& check:{std::make_pair(storageOwnerAt,std::make_pair(ownerLayout,sizeof(ownerLayout))),std::make_pair(storagePrivateAt,std::make_pair(privateLayout,sizeof(privateLayout))),std::make_pair(storageLinkableAt,std::make_pair(linkableLayout,sizeof(linkableLayout))),std::make_pair(storageItemsAt,std::make_pair(itemsLayout,sizeof(itemsLayout))),std::make_pair(storageItemDataAt,std::make_pair(itemDataLayout,sizeof(itemDataLayout)))}){
        auto at=fileAt(check.first);if(at==~0ULL||at+check.second.second>binary.size()||std::memcmp(binary.data()+at,check.second.first,check.second.second)){std::cerr<<"storage layout signature changed\n";return 12;}
    }
    const unsigned char stackCountLayout[]={0x8b,0x43,0x58,0x39,0xe8,0x74,0x3d,0x89,0x43,0x5c,0x89,0x6b,0x58};
    auto stackFile=fileAt(storageSetStackSizeAt);bool stackLayout=false;
    if(stackFile!=~0ULL)for(size_t i=stackFile;i+sizeof(stackCountLayout)<=binary.size()&&i<stackFile+0x500;++i)if(!std::memcmp(binary.data()+i,stackCountLayout,sizeof(stackCountLayout))){stackLayout=true;break;}
    if(!stackLayout){std::cerr<<"storage item count layout changed\n";return 12;}
    {auto at=fileAt(buildingEndPlayAt);const unsigned char layout[]={0x55,0x41,0x57,0x41,0x56,0x41,0x55,0x41,0x54,0x53,0x50,0x89,0xf5,0x49,0x89,0xfe};bool found=false;if(at!=~0ULL&&at>=32)for(size_t i=at-32;i+sizeof(layout)<=at+16;++i)if(!std::memcmp(binary.data()+i,layout,sizeof(layout))){found=true;break;}if(!found){std::cerr<<"storage removal hook signature changed\n";return 12;}}
    const unsigned char storageAddLayout[]={0x55,0x41,0x57,0x41,0x56,0x41,0x54,0x53,0x48,0x83,0xec,0x30};
    const unsigned char storageMoveLayout[]={0x55,0x53,0x50,0x4c,0x89,0xc0,0x48,0x89,0xfb};
    const unsigned char storageRemoveLayout[]={0x55,0x41,0x57,0x41,0x56,0x53,0x48,0x83,0xec,0x28};
    for(const auto& check:{std::make_pair(storageAddItemByDataAt,std::make_pair(storageAddLayout,sizeof(storageAddLayout))),std::make_pair(storageMoveItemAt,std::make_pair(storageMoveLayout,sizeof(storageMoveLayout))),std::make_pair(storageRemoveFromSlotAt,std::make_pair(storageRemoveLayout,sizeof(storageRemoveLayout)))}){auto at=fileAt(check.first);if(at==~0ULL||at+check.second.second>binary.size()||std::memcmp(binary.data()+at,check.second.first,check.second.second)){std::cerr<<"storage mutation signature changed\n";return 12;}}
    const unsigned char stackVirtualLayout[]={0x55,0x41,0x56,0x53,0x89,0xf3,0x49,0x89,0xfe,0x48,0x8b,0x07,0xff,0x90,0xd8,0x02,0x00,0x00};auto stackVirtualFile=fileAt(storageStackCountAt);if(stackVirtualFile==~0ULL||stackVirtualFile+sizeof(stackVirtualLayout)>binary.size()||std::memcmp(binary.data()+stackVirtualFile,stackVirtualLayout,sizeof(stackVirtualLayout))){std::cerr<<"storage max stack virtual layout changed\n";return 12;}
    const unsigned char worldTickLayout[]={0x53,0x48,0x83,0xec,0x10,0x8b,0x47,0x08,0xa8,0x10,0x0f,0x85,0x92,0x00,0x00,0x00,0x48,0x8b,0x47,0x20,0x48,0x85,0xc0};
    auto worldTickFile=fileAt(worldTickAt);if(worldTickFile==~0ULL||worldTickFile+sizeof(worldTickLayout)>binary.size()||std::memcmp(binary.data()+worldTickFile,worldTickLayout,sizeof(worldTickLayout))){std::cerr<<"storage world tick signature changed\n";return 12;}
    size_t worldTickMatches=0;for(size_t i=textFile;i+sizeof(worldTickLayout)<=textFile+textSize;++i)if(!std::memcmp(binary.data()+i,worldTickLayout,sizeof(worldTickLayout)))++worldTickMatches;
    if(worldTickMatches!=1){std::cerr<<"storage world tick signature is not unique\n";return 12;}
    for(auto address:{storageCategoryAt,storageBasePowerAt,storageCategoryIconAt}){auto at=fileAt(address);if(at==~0ULL||at+16>binary.size()){std::cerr<<"storage catalog metadata function outside ELF\n";return 12;}bool any=false;for(size_t i=0;i<16;++i)any|=binary[at+i]!=0;if(!any){std::cerr<<"storage catalog metadata function invalid\n";return 12;}}
    const unsigned char addItemsLayout[]={0x55,0x41,0x57,0x41,0x56,0x41,0x55,0x41,0x54};
    const unsigned char buildingRequirementsLayout[]={0x41,0x57,0x41,0x56,0x41,0x54,0x53};
    for(const auto& check:{std::make_pair(storageAddItemsAt,std::make_pair(addItemsLayout,sizeof(addItemsLayout))),std::make_pair(buildingRequirementsAt,std::make_pair(buildingRequirementsLayout,sizeof(buildingRequirementsLayout)))}){auto at=fileAt(check.first);if(at==~0ULL||at+check.second.second>binary.size()||std::memcmp(binary.data()+at,check.second.first,check.second.second)){std::cerr<<"recipe batch signature changed\n";return 12;}}
    auto canBatchFile=fileAt(storageCanAddItemsAt);if(canBatchFile==~0ULL||canBatchFile+16>binary.size()){std::cerr<<"recipe batch capacity function outside ELF\n";return 12;}
    auto memoryFreeFile=fileAt(memoryFreeAt);if(memoryFreeFile==~0ULL||memoryFreeFile>=binary.size()){std::cerr<<"recipe result allocator function outside ELF\n";return 12;}
    for(auto address:{storageEmptySlotsAt,storageFilledSlotsAt}){auto at=fileAt(address);if(at==~0ULL||at+16>binary.size()){std::cerr<<"storage slot count function outside ELF\n";return 12;}bool any=false;for(size_t i=0;i<16;++i)any|=binary[at+i]!=0;if(!any){std::cerr<<"storage slot count function invalid\n";return 12;}}
    for(auto address:{storageMoveItemInternalAt,renameableGetCustomNameAt,renameableGetSetterIdAt,renameableSetCustomNameAt,fTextFromStringAt,uniqueNetIdDestructorAt,fTextDestructorAt}){auto at=fileAt(address);if(at==~0ULL||at+16>binary.size()){std::cerr<<"storage organization function outside ELF\n";return 12;}bool any=false;for(size_t i=0;i<16;++i)any|=binary[at+i]!=0;if(!any){std::cerr<<"storage organization function invalid\n";return 12;}}
    for(auto address:{graveGetInventoryAt,graveGetOwnerAt,graveCanAccessAt,graveRemoveAt,graveMoveAllAt,graveRegistryAt}){auto at=fileAt(address);if(at==~0ULL||at+16>binary.size()){std::cerr<<"gravestone function outside ELF\n";return 12;}bool any=false;for(size_t i=0;i<16;++i)any|=binary[at+i]!=0;if(!any){std::cerr<<"gravestone function invalid\n";return 12;}}
    for(auto address:{graveRestoreFromLevelAt,graveRegisterStreamingAt,graveUnregisterStreamingAt,graveStreamingCompleteAt,graveProviderSourcesAt,gravePlayerSourcesAt,graveStreamingSourceCopyAt,graveStreamingSourceDestroyAt}){auto at=fileAt(address);if(at==~0ULL||at+16>binary.size()){std::cerr<<"gravestone restore or streaming function outside ELF\n";return 12;}bool any=false;for(size_t i=0;i<16;++i)any|=binary[at+i]!=0;if(!any){std::cerr<<"gravestone restore or streaming function invalid\n";return 12;}}
    const unsigned char graveInventoryLayout[]={0x53,0x48,0x83,0xec,0x10,0x48,0x89,0xfb};
    const unsigned char graveOwnerLayout[]={0x48,0x8d,0x87,0x10,0x04,0x00,0x00,0xc3};
    const unsigned char graveRespawnLayout[]={0x55,0x41,0x57,0x41,0x56,0x41,0x55,0x41,0x54,0x53,0x48,0x81,0xec,0x28,0x02,0x00,0x00};
    for(const auto&check:{std::make_pair(graveGetInventoryAt,std::make_pair(graveInventoryLayout,sizeof(graveInventoryLayout))),std::make_pair(graveGetOwnerAt,std::make_pair(graveOwnerLayout,sizeof(graveOwnerLayout))),std::make_pair(graveRespawnActorAt,std::make_pair(graveRespawnLayout,sizeof(graveRespawnLayout)))}){auto at=fileAt(check.first);if(at==~0ULL||at+check.second.second>binary.size()||std::memcmp(binary.data()+at,check.second.first,check.second.second)){std::cerr<<"gravestone ABI or SPUD respawn signature changed\n";return 12;}}
    const unsigned char graveRegistryLayout[]={0x55,0x41,0x56,0x53,0x48,0x83,0xec,0x10,0x48,0x89,0xf8,0x80,0xbe,0x30,0x03,0x00,0x00,0x00};
    auto graveRegistryFile=fileAt(graveRegistryAt);if(graveRegistryFile==~0ULL||graveRegistryFile+0x60>binary.size()||std::memcmp(binary.data()+graveRegistryFile,graveRegistryLayout,sizeof(graveRegistryLayout))){std::cerr<<"gravestone registry getter ABI changed\n";return 12;}
    bool graveRegistryOffset=false,graveRecordSize=false;for(size_t i=graveRegistryFile;i<graveRegistryFile+0x60;++i){if(i+7<=binary.size()&&binary[i]==0x48&&binary[i+1]==0x8b&&binary[i+2]==0x9e&&binary[i+3]==0xe0&&binary[i+4]==0x02&&binary[i+5]==0x00&&binary[i+6]==0x00)graveRegistryOffset=true;if(i+5<=binary.size()&&binary[i]==0xbf&&binary[i+1]==0x68&&binary[i+2]==0x00&&binary[i+3]==0x00&&binary[i+4]==0x00)graveRecordSize=true;}if(!graveRegistryOffset||!graveRecordSize){std::cerr<<"gravestone registry offset or record size changed\n";return 12;}
    const unsigned char getCustomNameLayout[]={0x41,0x56,0x53,0x48,0x83,0xec,0x28};
    const unsigned char getSetterIdLayout[]={0x41,0x57,0x41,0x56,0x41,0x55,0x41,0x54,0x53,0x48,0x81,0xec,0xa0,0x00,0x00,0x00};
    const unsigned char setCustomNameLayout[]={0x55,0x41,0x57,0x41,0x56,0x41,0x55,0x41,0x54,0x53,0x48,0x83,0xec,0x78};
    for(const auto&check:{std::make_pair(renameableGetCustomNameAt,std::make_pair(getCustomNameLayout,sizeof(getCustomNameLayout))),std::make_pair(renameableGetSetterIdAt,std::make_pair(getSetterIdLayout,sizeof(getSetterIdLayout))),std::make_pair(renameableSetCustomNameAt,std::make_pair(setCustomNameLayout,sizeof(setCustomNameLayout)))}){auto at=fileAt(check.first);if(at==~0ULL||at+check.second.second>binary.size()||std::memcmp(binary.data()+at,check.second.first,check.second.second)){std::cerr<<"storage rename ABI signature changed\n";return 12;}}
    // Layout evidence used by the runtime catalog: consumed FItemDataContainer array at
    // URecipeData +0x60/+0x68, output array at +0x90/+0x98, and raw building
    // requirements at UBuildingPieceData +0x100/+0x108. Require all three immediate
    // pairs to remain referenced by the exact build before enabling Recipe mode.
    auto hasImmediatePair=[&](std::uint32_t a,std::uint32_t b){unsigned hitsA=0,hitsB=0;for(size_t i=textFile;i+4<=textFile+textSize;++i){std::uint32_t v=0;std::memcpy(&v,binary.data()+i,4);hitsA+=v==a;hitsB+=v==b;}return hitsA&&hitsB;};
    if(!hasImmediatePair(0x60,0x68)||!hasImmediatePair(0x90,0x98)||!hasImmediatePair(0x100,0x108)){std::cerr<<"recipe object layout evidence changed\n";return 12;}
    auto hash=Sha256(argv[1]);if(hash.empty())return 9;
    if(dumpNativeOffsets){
        auto field=[&](const char*name,std::uint64_t value,bool&first){if(!first)std::cout<<',';first=false;std::cout<<'\"'<<name<<"\":\"0x"<<std::hex<<value<<"\"";};
        bool first=true;std::cout<<'{';field("storageAddItemByData",storageAddItemByDataAt,first);field("storageCanAddItemByData",storageCanAddItemByDataAt,first);field("storageMoveItem",storageMoveItemAt,first);field("storageRemoveFromSlot",storageRemoveFromSlotAt,first);field("storageCanAddItems",storageCanAddItemsAt,first);field("storageAddItems",storageAddItemsAt,first);field("buildingRequirements",buildingRequirementsAt,first);field("memoryFree",memoryFreeAt,first);field("worldTick",worldTickAt,first);field("buildingEndPlay",buildingEndPlayAt,first);field("renameableGetCustomName",renameableGetCustomNameAt,first);field("renameableGetSetterId",renameableGetSetterIdAt,first);field("renameableSetCustomName",renameableSetCustomNameAt,first);field("fTextFromString",fTextFromStringAt,first);field("uniqueNetIdDestructor",uniqueNetIdDestructorAt,first);field("fTextDestructor",fTextDestructorAt,first);field("graveGetInventory",graveGetInventoryAt,first);field("graveGetOwner",graveGetOwnerAt,first);field("graveCanAccess",graveCanAccessAt,first);field("graveRemove",graveRemoveAt,first);field("graveMoveAll",graveMoveAllAt,first);field("graveRegistry",graveRegistryAt,first);field("graveRespawnActor",graveRespawnActorAt,first);field("graveRestoreFromLevel",graveRestoreFromLevelAt,first);field("graveRegisterStreaming",graveRegisterStreamingAt,first);field("graveUnregisterStreaming",graveUnregisterStreamingAt,first);field("graveStreamingComplete",graveStreamingCompleteAt,first);field("graveProviderSources",graveProviderSourcesAt,first);field("gravePlayerSources",gravePlayerSourcesAt,first);field("graveStreamingSourceCopy",graveStreamingSourceCopyAt,first);field("graveStreamingSourceDestroy",graveStreamingSourceDestroyAt,first);std::cout<<"}\n";return 0;
    }
    const bool knownMap=hash=="d26de80fd01f4b73b726aed855a20377996b7c140ff33f1ec40150c0dd73b259"||
                        hash=="d12145525d88f3828ebeac0aec3865f5b16ba4771861efe65108754a89bbb090"||
                        hash=="445f8b65047d208adb04a3cb03bc7706f069306d814172ae29caec16c15bec7e"||
                        hash=="ef39023c04e1bde785907e13b9b64d72600455f1633bbfe231f639d31d98cfac"||
                        hash=="49512c0ee933c3ee4dda5fb5c2f0a25e66d7f26e34795c45b91b5c1d12b61b0c"||
                        hash=="b5017cf67bc6127dbec443e00fed1f818e82afd5d6d17f912d959a1dd87d5687";
    std::cout<<hash<<'\t'<<"0x"<<std::hex<<killedAt<<'\t'<<"0x"<<textOffset<<'\t'<<"0x"<<textSize<<'\t'<<"0x"<<displayAt<<'\t'<<"0x"<<mapRegionAt<<'\t'<<"0x"<<regionNameOffset<<'\t'<<"0x"<<dungeonAt<<'\t'<<"0x"<<dungeonNameOffset<<'\t'<<"0x"<<dungeonEntranceOffset
             <<std::dec<<'\t'<<(knownMap?-11075:0)<<'\t'<<(knownMap?-117685:0)<<'\t'<<(knownMap?420000:0)<<'\t'<<(knownMap?"world-map-1.0.0.2":"-")
             <<'\t'<<"0x"<<std::hex<<registryInstanceAt<<'\t'<<"0x"<<registryLodestonesAt<<'\t'<<"0x"<<lodestoneLocationAt
             <<'\t'<<"0x"<<labelTextGetterAt<<'\t'<<"0x"<<labelRotationGetterAt<<'\t'<<"0x"<<labelFontGetterAt
             <<'\t'<<"0x"<<teleportTargetNamesAt<<'\t'<<"0x"<<teleportTargetTransformAt
             <<'\t'<<"0x"<<playerTickAt<<'\t'<<"0x"<<actorTeleportAt<<'\t'<<"0x"<<processEventAt
             <<'\t'<<"0x"<<lodestoneTryTeleportAt<<'\t'<<"0x"<<lodestoneCanTeleportAt
             <<'\t'<<"0x"<<storageOwnerAt<<'\t'<<"0x"<<storagePrivateAt<<'\t'<<"0x"<<storageLinkableAt<<'\t'<<"0x"<<storageItemsAt<<'\t'<<"0x"<<storageItemDataAt<<'\t'<<"0x"<<storageSetStackSizeAt
             <<'\t'<<"0x158\t0x169\t0xF8\t0x168\t0x170\t0xB0\t0x58\t0x68"
             <<'\t'<<"0x"<<storageAddItemByDataAt<<'\t'<<"0x"<<storageCanAddItemByDataAt<<'\t'<<"0x"<<storageMoveItemAt<<'\t'<<"0x"<<storageRemoveFromSlotAt<<'\t'<<"0x"<<storageStackCountAt
             <<'\t'<<"0x"<<storageMoveItemInternalAt<<'\t'<<"0x"<<renameableGetCustomNameAt<<'\t'<<"0x"<<fTextDestructorAt<<"\n";
}
