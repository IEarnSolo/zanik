#pragma once
#include <cstdint>
#include <string>
#include <vector>

struct FString {
    char16_t* Data;
    int32_t Num;   // includes the null terminator
    int32_t Max;
};

struct FUEArray {
    void* Data;
    int32_t Num;
    int32_t Max;
};

struct FVector3d {
    double X, Y, Z;
};

namespace DomEngine {

namespace Offsets {

// Image offsets - Source: RSDWSDK/Dumpspace/OffsetsInfo.json
constexpr uintptr_t GObjects = 0x0DC2E4D0;
constexpr uintptr_t GNames   = 0x0DB72FB8;
constexpr uintptr_t GWorld   = 0x0DD525F8;
// Build 25501739 moved the writable image globals by 0x3000 while retaining
// the Unreal object/name layouts. Both candidates are validated at runtime.
constexpr uintptr_t GObjects_25501739 = 0x0DC39510;
constexpr uintptr_t GNames_25501739   = 0x0DB7DFF8;
constexpr uintptr_t GWorld_25501739   = 0x0DD555F8;
constexpr uintptr_t GObjects_25630937 = 0x0DC3A510;
constexpr uintptr_t GNames_25630937   = 0x0DB7EFF8;
constexpr uintptr_t GWorld_25630937   = 0x0DD565F8;

// TUObjectArray / FUObjectItem - Source: RSDWSDK/CppSDK/SDK/Basic.hpp
constexpr uintptr_t ObjArray_Objects     = 0x00;
constexpr uintptr_t ObjArray_MaxElements = 0x10;
constexpr uintptr_t ObjArray_NumElements = 0x14;
constexpr uintptr_t ObjArray_MaxChunks   = 0x18;
constexpr uintptr_t ObjArray_NumChunks   = 0x1C;
constexpr int32_t   ObjectsPerChunk      = 0x10000;
constexpr uintptr_t ObjectItem_Size      = 0x18;
constexpr uintptr_t ObjectItem_Flags     = 0x08;
constexpr uintptr_t ObjectItem_Serial    = 0x10;

// FNamePool - Source: RSDWSDK/CppSDK/SDK/Basic.hpp (FNamePool, FNameEntry)
constexpr uintptr_t NamePool_CurrentBlock      = 0x00;
constexpr uintptr_t NamePool_CurrentByteCursor = 0x04;
constexpr uintptr_t NamePool_Blocks            = 0x08;
constexpr int32_t   NameBlockOffsetBits        = 16;
constexpr int32_t   NameEntryStride            = 2;
constexpr int32_t   NamePool_MaxBlocks         = 0x2000;

// UObject / UStruct / UClass - Source: RSDWSDK/CppSDK/Assertions.inl
constexpr uintptr_t UObject_Flags     = 0x08;
constexpr uintptr_t UObject_Index     = 0x0C;
constexpr uintptr_t UObject_Class     = 0x10;
constexpr uintptr_t UObject_Name      = 0x18;
constexpr uintptr_t UObject_Outer     = 0x20;
constexpr uintptr_t UStruct_SuperStruct = 0x40;
constexpr uintptr_t UStruct_ChildProperties = 0x50;
constexpr uintptr_t UStruct_PropertiesSize = 0x58;
constexpr uintptr_t UClass_ClassDefaultObject = 0x110;

// UE5 FField / FProperty metadata. These are used only after validating names
// and bounded offsets at runtime, so an incompatible game update disables kill
// attribution instead of reading arbitrary process memory.
constexpr uintptr_t FField_Next = 0x20;
constexpr uintptr_t FField_Name = 0x28;
constexpr uintptr_t FProperty_OffsetInternal = 0x4C;

// UWorld - Source: RSDWSDK/CppSDK/Assertions.inl
constexpr uintptr_t World_PersistentLevel   = 0x30;
constexpr uintptr_t World_AuthorityGameMode = 0x1D8;
constexpr uintptr_t World_GameState         = 0x1E0;

// AGameStateBase - Source: RSDWSDK/CppSDK/Assertions.inl
constexpr uintptr_t GameStateBase_AuthorityGameMode = 0x2C8;
constexpr uintptr_t GameStateBase_PlayerArray       = 0x2D8;

// ADominionGameStateBase - Source: RSDWSDK/CppSDK/SDK/Dominion_classes.hpp
constexpr uintptr_t DomGameState_bIsDedicatedServer  = 0x320;
constexpr uintptr_t DomGameState_bIsCrossplayEnabled = 0x321;
constexpr uintptr_t DomGameState_KickedUsers         = 0x358;
constexpr uintptr_t DomGameState_BannedUsers         = 0x368;
constexpr uintptr_t DomGameState_DisplayNameComponent = 0x3E8;
constexpr uintptr_t DomGameState_WorldHardcoreState  = 0x420;

// UDisplayNameComponent - Source: RSDWSDK/CppSDK/SDK/Dominion_classes.hpp
constexpr uintptr_t DisplayName_OriginalCharacterNames = 0xE0;

// FDomCharacterDisplayName - Source: RSDWSDK/CppSDK/SDK/Dominion_structs.hpp
constexpr uintptr_t CharDisplayName_OwnerGuid     = 0x00; // FDomOwnerGuid
constexpr uintptr_t CharDisplayName_CharacterGuid = 0x98; // FDomCharacterGuid -> FGuid
constexpr uintptr_t CharDisplayName_CharacterName = 0xA8; // FString
constexpr uintptr_t CharDisplayName_Size          = 0xB8;

// APlayerState - Source: RSDWSDK/CppSDK/SDK/Engine_classes.hpp
constexpr uintptr_t PlayerState_Score          = 0x2C0;
constexpr uintptr_t PlayerState_PlayerId       = 0x2C4;
constexpr uintptr_t PlayerState_CompressedPing = 0x2C8;
constexpr uintptr_t PlayerState_Flags          = 0x2CA; // bit 1 spectator, 3 bot, 5 inactive
constexpr uintptr_t PlayerState_StartTime      = 0x2CC;
constexpr uintptr_t PlayerState_UniqueID       = 0x2D0; // FUniqueNetIdRepl, 0x30
constexpr uintptr_t PlayerState_PawnPrivate    = 0x338;
constexpr uintptr_t PlayerState_PlayerName     = 0x350;

// ADominionPlayerState - Source: RSDWSDK/CppSDK/SDK/Dominion_classes.hpp
constexpr uintptr_t DomPlayerState_OwnerGuid     = 0x3D0; // FDomOwnerGuid
constexpr uintptr_t DomPlayerState_CharacterGuid = 0x3D0; // FDomOwnerGuid::CharacterGuid -> FGuid
constexpr uintptr_t DomPlayerState_NetIdPrimary   = 0x3F8; // FDomOwnerGuid::AccountGuid::NetIdPrimary
constexpr uintptr_t DomPlayerState_NetIdSecondary = 0x428; // FDomOwnerGuid::AccountGuid::NetIdSecondary
constexpr uintptr_t DomPlayerState_PlayerColor   = 0x458; // FPlayerColor
constexpr uintptr_t DomPlayerState_PlatformData  = 0x480; // FDomPlatformData

// FPlayerColor - Source: RSDWSDK/CppSDK/SDK/Dominion_structs.hpp
constexpr uintptr_t PlayerColor_Color     = 0x00; // FLinearColor, 4 floats
constexpr uintptr_t PlayerColor_HexString = 0x10; // FString

// FDomPlatformData - Source: RSDWSDK/CppSDK/SDK/Dominion_structs.hpp
constexpr uintptr_t PlatformData_UniqueID     = 0x00; // FUniqueNetIdRepl, 0x30
constexpr uintptr_t PlatformData_DisplayName  = 0x30; // FString
constexpr uintptr_t PlatformData_PlatformName = 0x40; // FString

constexpr uintptr_t NetIdRepl_SharedPtrA  = 0x00;
constexpr uintptr_t NetIdRepl_SharedPtrB  = 0x08;
constexpr uintptr_t NetIdRepl_Size        = 0x30;
constexpr uintptr_t NetId_FirstMember     = 0x18;

// AActor / APawn / AController / ACharacter - Source: RSDWSDK/CppSDK/Assertions.inl
constexpr uintptr_t Actor_Owner          = 0x160;
constexpr uintptr_t Actor_Role           = 0x170;
constexpr uintptr_t Actor_NetDormancy    = 0x171;
constexpr uintptr_t Actor_RootComponent  = 0x1D0;
constexpr uintptr_t Pawn_PlayerState     = 0x2E0;
constexpr uintptr_t Pawn_Controller      = 0x2F0;
constexpr uintptr_t Controller_PlayerState = 0x2C8;
constexpr uintptr_t Controller_Pawn        = 0x300;
constexpr uintptr_t Character_Mesh              = 0x340;
constexpr uintptr_t Character_CharacterMovement = 0x348;

// Source: RSDWSDK/CppSDK/Assertions.inl. Locations and rotations are doubles:
constexpr uintptr_t SceneComp_RelativeLocation = 0x148;
constexpr uintptr_t SceneComp_RelativeRotation = 0x160;
constexpr uintptr_t MovementComp_Velocity      = 0xD8;
constexpr uintptr_t CharMove_MovementMode      = 0x241;

// pointers - Source: RSDWSDK/CppSDK/Assertions.inl. These are real pointers,
constexpr uintptr_t DomPlayerChar_AttributesComponent = 0x1448; // UDominionAttributesComponent*
constexpr uintptr_t DomPlayerChar_HealthComponent = 0x1458; // UPlayerHealthComponent*
constexpr uintptr_t DomPlayerController_SkillComponent = 0x820; // USkillComponent*

// UDominionAttributesComponent - Source: RSDWSDK/CppSDK/Assertions.inl.
constexpr uintptr_t Attributes_FloatAttributes       = 0xD0;
constexpr uintptr_t Attributes_SharedFloatAttributes = 0xE0;
constexpr uintptr_t Attributes_AttributeValues       = 0xF0;
constexpr uintptr_t Attributes_SharedAttributeValues = 0x100;
constexpr uintptr_t FloatAttribute_BaseValue         = 0x38;

// UHealthComponent - Source: RSDWSDK/CppSDK/SDK/Dominion_classes.hpp
constexpr uintptr_t Health_MaxHealth           = 0x138;
constexpr uintptr_t Health_AuthoritativeHealth = 0x13C;
constexpr uintptr_t Health_bCanDie             = 0x140;

// shape - Source: RSDWSDK/CppSDK/SDK/Dominion_classes.hpp
constexpr uintptr_t SurvivalStat_Current    = 0x130;
constexpr uintptr_t SurvivalStat_CurrentMax = 0x134;
constexpr uintptr_t SurvivalStat_DecayBuffer = 0x138;

// USkillComponent - Source: RSDWSDK/CppSDK/SDK/Dominion_classes.hpp
constexpr uintptr_t Skill_Skills = 0x188; // TArray<FSkill>
constexpr uintptr_t FSkill_CurrentXP = 0x00;
constexpr uintptr_t FSkill_SkillData = 0x08;
constexpr uintptr_t FSkill_Size      = 0x10;

// UDedicatedServerSettings - Source: RSDWSDK/CppSDK/SDK/Dominion_classes.hpp
constexpr uintptr_t DediSettings_OwnerId         = 0x28;
constexpr uintptr_t DediSettings_ServerGuid      = 0x68;
constexpr uintptr_t DediSettings_ServerName      = 0x78;
constexpr uintptr_t DediSettings_DefaultWorldName = 0x98;

}

struct SkillInfo {
    std::string skill;
    int32_t currentXP = 0;
};

struct AttributeInfo {
    std::string name;
    float value = 0.0f;
    float baseValue = 0.0f;
    bool shared = false;
};

struct PlayerInfo {
    std::string name;
    int32_t playerId = 0;
    int32_t pingMs = 0;
    float score = 0.0f;
    int32_t startTime = 0;
    bool isSpectator = false;
    bool isBot = false;
    bool isInactive = false;

    std::string characterGuid;
    std::string characterName;
    std::string platform;
    std::string uniqueNetId;
    std::string uniqueNetIdSource;
    std::string colorHex;
    float color[4] = {0, 0, 0, 0};
    bool hasColor = false;

    bool spawned = false;
    std::string pawnClass;
    bool hasLocation = false;
    double x = 0, y = 0, z = 0;
    std::string location;
    bool hasMapLocation = false;
    double mapX = 0, mapY = 0;
    bool mapAtDungeonEntrance = false;
    double pitch = 0, yaw = 0, roll = 0;
    bool hasVelocity = false;
    double vx = 0, vy = 0, vz = 0;
    uint8_t movementMode = 0;

    bool hasHealth = false;
    float health = 0.0f;
    float maxHealth = 0.0f;
    bool canDie = false;
    bool isDead = false;

    bool hasSustenance = false;
    float sustenance = 0.0f;
    float maxSustenance = 0.0f;

    bool hasHydration = false;
    float hydration = 0.0f;
    float maxHydration = 0.0f;

    std::vector<SkillInfo> skills;
    std::vector<AttributeInfo> attributes;

    uintptr_t playerStatePtr = 0;
    uintptr_t pawnPtr = 0;
    uintptr_t controllerPtr = 0;
    uintptr_t dungeonPtr = 0;
};

struct ServerInfo {
    std::string ownerId;
    std::string serverName;
    std::string worldName;
    std::string serverGuid;
    bool isDedicatedServer = false;
    bool crossplayEnabled = false;
    int32_t hardcoreState = -1;
    bool valid = false;
};

struct DiscoveryInfo {
    uintptr_t moduleBase = 0;
    uintptr_t moduleEnd = 0;
    uintptr_t gObjects = 0;
    uintptr_t gNames = 0;
    uintptr_t world = 0;
    std::string gObjectsSource;
    std::string gNamesSource;
    std::string worldSource;
    int32_t objectCount = 0;
};

struct NativeKillEvent {
    uintptr_t victimIdentity = 0;
    std::uint64_t deathGeneration = 0;
    std::string internalEnemy;
    std::string enemy;
    std::string killerCharacterGuid;
};

struct LodestoneInfo {
    std::string id;
    double mapX = 0, mapY = 0;
    // Server-only authoritative destination. Status serialization deliberately
    // publishes only id/mapX/mapY.
    FVector3d worldLocation{};
    FVector3d arrivalLocation{};
    std::uint32_t nativeGuid[4]{};
    // Server-only pointer to the fingerprint-validated FLodestoneData record.
    // It is used only synchronously on the game thread and is never serialized.
    uintptr_t nativeEntry = 0;
};

struct AdminTeleportDiagnostics {
    int compatibility = 0;
    int lodestoneCompatibility = 0;
    int playerCompatibility = 0;
    int groundTraceCompatibility = 0;
    int storagePipelineCompatibility = 0;
    int nativeCaptureCompatibility = 0;
    int nativeTemplateAvailable = 0;
    std::uint64_t queuedRequests = 0;
    std::uint64_t acceptedTeleports = 0;
    std::uint64_t rejectedRequests = 0;
    std::uint64_t maximumExecutionMicroseconds = 0;
    std::uint64_t acceptedStorageTeleports = 0;
    std::uint64_t staleStorageDestinations = 0;
    std::uint64_t unsafeStorageDestinations = 0;
    std::uint64_t tickCalls = 0;
    std::uint64_t maximumTickMicroseconds = 0;
    std::uint64_t slowTickCalls = 0;
};

struct NodestoneInfo {
    std::string id;
    double mapX = 0, mapY = 0;
};

struct NodestoneDiagnostics {
    int compatibility = 0;
    int actorsFound = 0;
    int validMarkers = 0;
    std::uint64_t rejectedRecords = 0;
    std::int64_t lastSuccessfulCollection = 0;
};

struct MapLabelInfo {
    std::string id;
    std::string text;
    double mapX = 0, mapY = 0;
    double rotation = 0;
    int fontSize = 0;
    float letterR = 0, letterG = 0, letterB = 0, letterA = 1;
    float letterTintOpacity = 1, letterOpacity = 1, letterContrast = 1;
    float glowR = 0, glowG = 0, glowB = 0, glowA = 1;
    float glowTintOpacity = 1, glowOpacity = 1, glowRange = 0, glowBrightness = 1;
    float volumeToFlatness = 0, regularToBold = 0;
};
struct MapLabelDiagnostics {
    int components = 0;
    int textProperties = 0;
    int readableTexts = 0;
    int actorOwners = 0;
    int validAnchors = 0;
};

struct LodestoneDiagnostics {
    int compatibility = 0;
    int registryFound = 0;
    int registryCount = 0;
    int validMarkers = 0;
    std::uint64_t rejectedRecords = 0;
    std::int64_t lastSuccessfulCollection = 0;
};

struct ResourceStateInfo {
    std::string id;
    bool available = true;
    double remainingSeconds = 0;
    std::string confidence;
};

struct ResourceDiagnostics {
    int compatibility = 0;
    int catalogNodes = 0;
    int actionableCatalogNodes = 0;
    int foliageCatalogNodes = 0;
    int loadedComponents = 0;
    int matchedLoadedNodes = 0;
    int matchedActionableNodes = 0;
    int matchedFoliageNodes = 0;
    int persistedStateNodes = 0;
    int unresolvedNodes = 0;
    std::uint64_t rejectedRecords = 0;
    std::uint64_t collectionEvents = 0;
    std::uint64_t respawnEvents = 0;
    std::uint64_t rawSetAmountCalls = 0;
    std::uint64_t unresolvedHookCalls = 0;
    std::int64_t lastSuccessfulReconciliation = 0;
};

struct NpcStateInfo {
    std::string id;
    std::string state; // live, dead, inactive, unloaded
    bool hasMapLocation = false;
    double mapX = 0, mapY = 0;
    double remainingSeconds = 0;
};
struct NpcDiagnostics {
    int enabled = 0;
    int catalogCompatibility = 0;
    int registryCompatibility = 0;
    int identityCompatibility = 0;
    int transformCompatibility = 0;
    int lifecycleCompatibility = 0;
    int respawnCompatibility = 0;
    int dungeonExclusionCompatibility = 0;
    int catalogNodes = 0;
    int loadedActors = 0;
    int matchedLiveNodes = 0;
    int deadNodes = 0;
    int inactiveNodes = 0;
    int unloadedNodes = 0;
    int loadedSpawnPoints = 0;
    std::uint64_t ambiguousMatches = 0;
    std::uint64_t rejectedActors = 0;
    std::int64_t lastSample = 0;
    int scanInProgress = 0;
    int scannedObjects = 0;
    int totalObjects = 0;
    std::uint64_t completedScans = 0;
    std::uint64_t lastScanMilliseconds = 0;
    std::uint64_t maximumSliceMicroseconds = 0;
    std::uint64_t lastGenerationWorkMicroseconds = 0;
    std::uint64_t lastFinalizationMicroseconds = 0;
    std::uint64_t maximumFinalizationMicroseconds = 0;
    std::uint64_t lastLiveRefreshMicroseconds = 0;
    std::uint64_t maximumLiveRefreshMicroseconds = 0;
    std::uint64_t classCacheHits = 0;
    std::uint64_t classCacheMisses = 0;
    std::uint64_t invalidObjectSlots = 0;
    std::uint64_t changedObjectSlots = 0;
    int lastDutyCycleBasisPoints = 0;
    int idleMilliseconds = 0;
};

bool ReadLodestones(std::vector<LodestoneInfo>& out);
bool ReadNodestones(std::vector<NodestoneInfo>& out);
bool ReadMapLabels(std::vector<MapLabelInfo>& out);
MapLabelDiagnostics GetMapLabelDiagnostics();
LodestoneDiagnostics GetLodestoneDiagnostics();
bool InitializeAdminTeleport(uintptr_t moduleBase, uintptr_t moduleEnd);
bool QueueAdminTeleport(const std::string& characterGuid, const std::string& publicId,
                        const std::string& destinationId, const std::string& sessionPrefix);
bool QueueAdminPlayerTeleport(const std::string& movingCharacterGuid, const std::string& movingPublicId,
                              const std::string& targetCharacterGuid, const std::string& targetPublicId,
                              const std::string& action, const std::string& sessionPrefix);
bool QueueAdminStorageTeleport(const std::string& characterGuid, const std::string& publicId,
                               const std::string& targetCharacterGuid, const std::string& targetPublicId,
                               const std::string& chestId, const std::string& sessionPrefix);
AdminTeleportDiagnostics GetAdminTeleportDiagnostics();
struct AdminStorageDiagnostics {
    int compatibility = 0;
    int dispatcherCompatibility = 0;
    int mutationCompatibility = 0;
    int batchCompatibility = 0;
    int organizationCompatibility = 0;
    int renameCompatibility = 0;
    int removalTrackingCompatibility = 0;
    int catalogCompatibility = 0;
    int unlockCatalogCompatibility = 0;
    int planCatalogCompatibility = 0;
    std::uint64_t planCatalogItems = 0;
    int buildingTaxonomyCompatibility = 0;
    int buildingRequirementsCompatibility = 0;
    int buildingIconsCompatibility = 0;
    int craftingRecipeCompatibility = 0;
    std::uint64_t expectedBuildingPieces = 0;
    std::uint64_t resolvedBuildingPieces = 0;
    std::uint64_t unavailableBuildingPieces = 0;
    std::uint64_t buildingTabs = 0;
    std::uint64_t buildingSections = 0;
    std::uint64_t buildingReferences = 0;
    std::uint64_t queuedRequests = 0;
    std::uint64_t acceptedMutations = 0;
    std::uint64_t rejectedRequests = 0;
    std::uint64_t staleRequests = 0;
    std::uint64_t maximumExecutionMicroseconds = 0;
    std::uint64_t tickCalls = 0;
    std::uint64_t maximumTickMicroseconds = 0;
    std::uint64_t slowTickCalls = 0;
    std::uint64_t maximumRecipeTickMicroseconds = 0;
    std::uint64_t maximumCustomNameTickMicroseconds = 0;
    std::uint64_t maximumStorageTickMicroseconds = 0;
    std::uint64_t maximumGravestoneTickMicroseconds = 0;
};
bool InitializeAdminStorage(uintptr_t moduleBase, uintptr_t moduleEnd);
bool InitializeAdminStorageHookEarly();
void ProcessAdminStorageRequests();
bool SubmitAdminStorageMutation(const std::string& operation,
                                const std::string& adminCharacterGuid,
                                const std::string& adminPublicId,
                                const std::string& targetCharacterGuid,
                                const std::string& targetPublicId,
                                const std::string& chestId,
                                const std::string& itemId,
                                const std::string& catalogId,
                                int quantity,
                                const std::string& sessionPrefix,
                                std::string& result,
                                const std::string& destinationChestId = {},
                                int destinationSlot = -1,
                                const std::vector<std::string>& itemIds = {},
                                const std::string& customName = {});
AdminStorageDiagnostics GetAdminStorageDiagnostics();
struct AdminGravestoneDiagnostics {
    int compatibility = 0;
    int inventoryCompatibility = 0;
    int registryCompatibility = 0;
    int streamingCompatibility = 0;
    int respawnCaptureCompatibility = 0;
    int persistenceCompatibility = 0;
    std::uint64_t discovered = 0;
    std::uint64_t unresolved = 0;
    std::uint64_t queuedRequests = 0;
    std::uint64_t acceptedMutations = 0;
    std::uint64_t rejectedRequests = 0;
    std::uint64_t staleRequests = 0;
    std::uint64_t maximumExecutionMicroseconds = 0;
};
bool InitializeAdminGravestones(uintptr_t moduleBase, uintptr_t moduleEnd);
void ProcessAdminGravestoneRequests();
std::string GetPlayerGravestonesJson(const std::string& characterGuid, const std::string& ownerName);
bool SubmitAdminGravestoneMutation(const std::string& operation,
                                   const std::string& adminCharacterGuid,
                                   const std::string& adminPublicId,
                                   const std::string& ownerCharacterGuid,
                                   const std::string& ownerPublicId,
                                   const std::string& graveId,
                                   const std::string& itemId,
                                   int quantity,
                                   const std::string& sessionPrefix,
                                   std::string& result);
bool ResolveAdminGravestoneActor(const std::string& ownerCharacterGuid, const std::string& graveId,
                                 uintptr_t& actor, FVector3d& location);
bool ResolveAdminGravestoneLocation(const std::string& ownerCharacterGuid, const std::string& graveId,
                                    uintptr_t& actor, FVector3d& location);
bool QueueAdminGravestoneTeleport(const std::string& characterGuid, const std::string& publicId,
                                  const std::string& ownerCharacterGuid, const std::string& ownerPublicId,
                                  const std::string& graveId, const std::string& sessionPrefix);
AdminGravestoneDiagnostics GetAdminGravestoneDiagnostics();
bool WasAdminStorageBuildingRemoved(uintptr_t actor);
bool ResolveAdminStorageLocation(const std::string& characterGuid, const std::string& chestId,
                                 FVector3d& location);
bool ResolveAdminStorageActor(const std::string& characterGuid, const std::string& chestId,
                              uintptr_t& actor, FVector3d& location);
bool ReadAdminStorageLocations(const std::string& characterGuid, std::vector<FVector3d>& locations);
std::string GetAdminStorageCustomName(uintptr_t actor);
std::string GetAdminStorageCatalogJson();
std::string GetAdminStorageRecipesJson();
bool GetAdminTeleportOffsets(uintptr_t& playerTick, uintptr_t& actorTeleportTo,
                             uintptr_t& processEvent, uintptr_t& lodestoneTryTeleport,
                             uintptr_t& lodestoneCanTeleport);
NodestoneDiagnostics GetNodestoneDiagnostics();
bool InitializeResourceTracking(uintptr_t moduleBase, uintptr_t moduleEnd);
void RefreshResourceStates();
bool ReadResourceStates(std::vector<ResourceStateInfo>& out, double& gameSecondsPerRealSecond,
                        std::int64_t& updatedAt);
ResourceDiagnostics GetResourceDiagnostics();
bool InitializeNpcTracking(uintptr_t moduleBase, uintptr_t moduleEnd);
void RefreshNpcStates();
bool NpcScanInProgress();
bool ReadNpcStates(std::vector<NpcStateInfo>& out, double& gameSecondsPerRealSecond,
                   std::int64_t& updatedAt);
void RecordNpcDeath(uintptr_t actor);
NpcDiagnostics GetNpcDiagnostics();

bool InstallNativeKillHook(uintptr_t moduleBase, uintptr_t moduleEnd);
void UpdateKillPlayerCache(const std::vector<PlayerInfo>& players);
std::vector<NativeKillEvent> DrainNativeKillEvents();
int NativeKillHookState();
struct KillDiagnostics {
    int hookCompatibility = 0;
    int regionCompatibility = 0;
    std::uint64_t rawFatalEvents = 0;
    std::uint64_t acceptedKills = 0;
    std::uint64_t duplicateEvents = 0;
    std::uint64_t unresolvedAttackers = 0;
    std::uint64_t rejectedNonPlayerDeaths = 0;
    std::uint64_t droppedQueueEvents = 0;
    std::uint64_t maximumHookMicroseconds = 0;
    std::uint64_t slowHookEvents = 0;
};
KillDiagnostics GetKillDiagnostics();
struct PlayerDiscoveryDiagnostics {
    std::uint64_t syncAttempts = 0;
    int gameStateValid = 0;
    int playerArrayCount = 0;
    int readablePlayerStates = 0;
    int dominionPlayerStates = 0;
    int characterGuids = 0;
    int characterNames = 0;
    int spawnedPawns = 0;
    int worldLocations = 0;
    int regionLookupAttempts = 0;
    int playersProduced = 0;
};
PlayerDiscoveryDiagnostics GetPlayerDiscoveryDiagnostics();
struct PlayerRecipeSnapshot {
    std::string characterGuid;
    std::vector<std::string> unlockedIds;
    std::string catalogFingerprint;
    std::int64_t capturedAt = 0;
    bool complete = false;
};
struct PlayerRecipeDiagnostics {
    int compatibility = 0;
    int catalogCompatibility = 0;
    int progressLayoutCompatibility = 0;
    int queryCompatibility = 0;
    std::uint64_t catalogEntries = 0;
    std::uint64_t resolvedEntries = 0;
    std::uint64_t snapshots = 0;
    std::uint64_t scansCompleted = 0;
    std::uint64_t queryFailures = 0;
    std::int64_t lastScan = 0;
    std::uint64_t maximumProgressLookupMicroseconds = 0;
    std::uint64_t maximumSingleQueryMicroseconds = 0;
    std::uint64_t slowQueries = 0;
    int queryIntervalMilliseconds = 0;
};
bool InitializePlayerRecipeTracking(const std::string& catalogPath, uintptr_t moduleBase, uintptr_t moduleEnd);
void UpdatePlayerRecipeTargets(const std::vector<PlayerInfo>& players);
void ProcessPlayerRecipeTracking();
std::vector<PlayerRecipeSnapshot> ReadPlayerRecipeSnapshots();
PlayerRecipeDiagnostics GetPlayerRecipeDiagnostics();
std::string GetPlayerRecipeCatalogFingerprint();
struct QuestProgressInfo { std::string id,status,objective;std::int64_t completedObservedAt=0; };
struct PlayerQuestSnapshot { std::string characterGuid;std::vector<QuestProgressInfo> quests;std::string catalogFingerprint;std::int64_t capturedAt=0;bool complete=false; };
struct PlayerQuestDiagnostics { int enabled=0,compatibility=0,catalogCompatibility=0,componentCompatibility=0,queryCompatibility=0,objectiveCompatibility=0;std::uint64_t catalogEntries=0,resolvedEntries=0,snapshots=0,scansCompleted=0,queryFailures=0,snapshotChanges=0,persistenceFailures=0,cacheHits=0,cacheMisses=0,invalidIdentities=0;std::int64_t lastScan=0;std::uint64_t lastQueryMicroseconds=0,maximumQueryMicroseconds=0,slowQueries=0; };
bool InitializePlayerQuestTracking(const std::string&,uintptr_t,uintptr_t);
void RefreshPlayerQuestTrackingCompatibility();
void UpdatePlayerQuestTargets(const std::vector<PlayerInfo>&);
void ProcessPlayerQuestTracking();
std::vector<PlayerQuestSnapshot> ReadPlayerQuestSnapshots();
PlayerQuestDiagnostics GetPlayerQuestDiagnostics();
std::string GetPlayerQuestCatalogFingerprint();
void RecordPlayerQuestPersistenceFailure();
std::string FileSha256(const char* path);

class Engine {
private:
    uintptr_t moduleBase = 0;
    uintptr_t moduleEnd = 0;
    uintptr_t gObjects = 0;
    uintptr_t gNames = 0;
    bool initialized = false;

    std::string gObjectsSource;
    std::string gNamesSource;
    mutable std::string worldSource;
    mutable uintptr_t cachedWorld = 0;
    int attempts = 0;

    bool ShouldLogDiscovery() const;

    bool ResolveModule();
    bool ResolveNamePool();
    bool ResolveObjectArray();
    bool ValidateNamePool(uintptr_t candidate) const;
    bool ValidateObjectArray(uintptr_t candidate) const;
    uintptr_t ScanForObjectArray() const;

public:
    bool Initialize();
    bool IsInitialized() const { return initialized; }

    uintptr_t GetModuleBase() const { return moduleBase; }
    uintptr_t GetModuleEnd() const { return moduleEnd; }
    uintptr_t GetGObjects() const { return gObjects; }
    uintptr_t GetGNames() const { return gNames; }

    int32_t GetObjectCount() const;
    uintptr_t GetObjectByIndex(int32_t index) const;
    bool IsObjectLive(uintptr_t object) const;
    std::uint64_t GetObjectIdentity(uintptr_t object) const;
    std::string NameToString(uint32_t comparisonIndex, uint32_t number) const;
    std::string GetObjectName(uintptr_t object) const;
    std::string GetObjectClassName(uintptr_t object) const;
    bool IsA(uintptr_t object, const char* className) const;
    int32_t FindPropertyOffset(uintptr_t object, const char* propertyName) const;
    bool ReadBoolProperty(uintptr_t object, const char* propertyName, bool& value) const;
    std::vector<std::pair<std::string,int32_t>> GetPropertyOffsets(uintptr_t object) const;
    std::vector<std::pair<std::string,int32_t>> GetStructPropertyOffsets(uintptr_t structure) const;
    std::string ResolvePlayerCharacterGuid(uintptr_t actor,
                                           const std::vector<PlayerInfo>& players) const;
    std::string GetEnemyDisplayName(uintptr_t actor) const;
    std::string GetMapRegionName(uintptr_t context, const FVector3d& location) const;
    uintptr_t GetDungeonAtLocation(uintptr_t context, const FVector3d& location) const;
    std::string GetDungeonName(uintptr_t dungeon) const;
    std::string GetDungeonRegionName(uintptr_t context, uintptr_t dungeon) const;
    bool GetDungeonEntrance(uintptr_t dungeon, FVector3d& entrance) const;
    bool ProjectWorldToMap(const FVector3d& location, double& x, double& y) const;
    bool IsMapProjectionAvailable() const;
    std::string GetMapAssetVersion() const;
    uintptr_t FindObject(const char* className, bool wantDefaultObject) const;
    uintptr_t GetWorld() const;
    uintptr_t GetGameState() const;

    std::string ReadFString(uintptr_t address) const;
    std::string ReadGuid(uintptr_t address) const;
    std::string ReadUniqueNetId(uintptr_t netIdRepl, std::string& outSource) const;
    std::string GetStorageDiagnosticsJson() const;
    std::string GetPlayerStorageJson(const std::string& characterGuid) const;

    std::vector<PlayerInfo> GetAllPlayers() const;

private:
    void ReadAttributeArrays(uintptr_t attributesComponent, uintptr_t attributesOffset,
                             uintptr_t valuesOffset, bool shared,
                             std::vector<AttributeInfo>& out) const;
    void ReadHealthAndAttributes(PlayerInfo& info) const;
    void ReadSkills(PlayerInfo& info) const;

public:

    bool KickPlayer(const std::string& identifier, const std::string& reason,
                    std::string& outError) const;
    uintptr_t FindFunction(const char* className, const char* functionName) const;

    void RememberKick(const std::string& netId, const std::string& name,
                      const std::string& reason) const;
    bool IsKicked(const std::string& netId) const;
    std::vector<std::string> KickedNetIds() const;
    bool ClearKick(const std::string& netId) const;
    void EnforceKicks() const;

    ServerInfo GetServerInfo() const;
    DiscoveryInfo GetDiscoveryInfo() const;
};

extern Engine* g_Engine;

}
