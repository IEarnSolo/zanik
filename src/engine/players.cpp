#include "dom_engine.h"
#include "../utils/logger.h"
#include "../utils/memory.h"

#include <cstring>
#include <atomic>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

namespace DomEngine {

namespace {

std::atomic<std::uint64_t> discoverySyncAttempts{0};
std::atomic<int> discoveryGameStateValid{0};
std::atomic<int> discoveryPlayerArrayCount{0};
std::atomic<int> discoveryReadablePlayerStates{0};
std::atomic<int> discoveryDominionPlayerStates{0};
std::atomic<int> discoveryCharacterGuids{0};
std::atomic<int> discoveryCharacterNames{0};
std::atomic<int> discoverySpawnedPawns{0};
std::atomic<int> discoveryWorldLocations{0};
std::atomic<int> discoveryRegionLookupAttempts{0};
std::atomic<int> discoveryPlayersProduced{0};

struct PawnComponents {
    uintptr_t sustenance = 0;
    uintptr_t hydration = 0;
};

void ReadDoubleTriple(uintptr_t address, double& a, double& b, double& c) {
    if (!Mem::Readable((void*)address, sizeof(double) * 3)) return;
    double values[3] = {};
    memcpy(values, (void*)address, sizeof(values));
    a = values[0];
    b = values[1];
    c = values[2];
}

bool ReadArray(uintptr_t address, FUEArray& out, int32_t maxCount) {
    if (!Mem::Readable((void*)address, sizeof(FUEArray))) return false;
    memcpy(&out, (void*)address, sizeof(out));
    return out.Data && out.Num > 0 && out.Num <= maxCount;
}

uintptr_t FindPossessedPawnForState(const Engine* engine, uintptr_t playerState) {
    if (!engine || !playerState) return 0;
    int32_t count = engine->GetObjectCount();
    // PlayerState::PawnPrivate can be transiently null during login and world
    // streaming.  The authoritative controller relationship is already live
    // in that window, so resolve Controller -> Pawn before the broad scan.
    for (int32_t i = 0; i < count; ++i) {
        uintptr_t controller = engine->GetObjectByIndex(i);
        if (!controller || !engine->IsA(controller, "DominionPlayerControllerBase") ||
            !Mem::Readable((void*)(controller + Offsets::Controller_Pawn), sizeof(uintptr_t))) continue;
        if (Mem::ReadPtr(controller + Offsets::Controller_PlayerState) != playerState) continue;
        uintptr_t pawn = Mem::ReadPtr(controller + Offsets::Controller_Pawn);
        if (pawn && engine->IsA(pawn, "DominionPlayerCharacter")) return pawn;
    }
    for (int32_t i = 0; i < count; ++i) {
        uintptr_t candidate = engine->GetObjectByIndex(i);
        if (!candidate || !engine->IsA(candidate, "DominionPlayerCharacter") ||
            !Mem::Readable((void*)(candidate + Offsets::Pawn_PlayerState), sizeof(uintptr_t))) continue;
        if (Mem::ReadPtr(candidate + Offsets::Pawn_PlayerState) == playerState) return candidate;
    }
    return 0;
}

}

PlayerDiscoveryDiagnostics GetPlayerDiscoveryDiagnostics() {
    return {discoverySyncAttempts.load(), discoveryGameStateValid.load(),
            discoveryPlayerArrayCount.load(), discoveryReadablePlayerStates.load(),
            discoveryDominionPlayerStates.load(), discoveryCharacterGuids.load(),
            discoveryCharacterNames.load(), discoverySpawnedPawns.load(),
            discoveryWorldLocations.load(), discoveryRegionLookupAttempts.load(),
            discoveryPlayersProduced.load()};
}

void Engine::ReadAttributeArrays(uintptr_t attributesComponent, uintptr_t attributesOffset,
                                 uintptr_t valuesOffset, bool shared,
                                 std::vector<AttributeInfo>& out) const {
    FUEArray attributes = {};
    FUEArray values = {};
    if (!ReadArray(attributesComponent + attributesOffset, attributes, 512)) return;
    if (!ReadArray(attributesComponent + valuesOffset, values, 512)) return;
    if (!Mem::Readable(attributes.Data, sizeof(uintptr_t) * (size_t)attributes.Num)) return;
    if (!Mem::Readable(values.Data, sizeof(float) * (size_t)values.Num)) return;

    int32_t count = attributes.Num < values.Num ? attributes.Num : values.Num;
    for (int32_t i = 0; i < count; i++) {
        uintptr_t attribute = ((uintptr_t*)attributes.Data)[i];
        if (!attribute) continue;

        AttributeInfo info;
        info.name = GetObjectClassName(attribute);
        if (info.name.empty()) continue;
        const std::string suffix = "Attribute";
        if (info.name.size() > suffix.size() &&
            info.name.compare(info.name.size() - suffix.size(), suffix.size(), suffix) == 0) {
            info.name.erase(info.name.size() - suffix.size());
        }
        info.value = ((float*)values.Data)[i];
        info.baseValue = Mem::ReadFloat(attribute + Offsets::FloatAttribute_BaseValue);
        info.shared = shared;
        out.push_back(info);
    }
}

void Engine::ReadHealthAndAttributes(PlayerInfo& info) const {
    if (!IsA(info.pawnPtr, "DominionPlayerCharacter")) return;

    uintptr_t health = Mem::ReadPtr(info.pawnPtr + Offsets::DomPlayerChar_HealthComponent);
    if (!health || !Mem::Readable((void*)health, 0x1B8)) return;

    info.canDie = Mem::ReadU8(health + Offsets::Health_bCanDie) != 0;

    uintptr_t attributesComponent =
        Mem::ReadPtr(info.pawnPtr + Offsets::DomPlayerChar_AttributesComponent);
    if (!attributesComponent || !Mem::Readable((void*)attributesComponent, 0x1B8)) return;

    ReadAttributeArrays(attributesComponent, Offsets::Attributes_FloatAttributes,
                        Offsets::Attributes_AttributeValues, false, info.attributes);
    ReadAttributeArrays(attributesComponent, Offsets::Attributes_SharedFloatAttributes,
                        Offsets::Attributes_SharedAttributeValues, true, info.attributes);

    for (const AttributeInfo& attribute : info.attributes) {
        if (attribute.name == "Health") {
            info.health = attribute.value;
            info.hasHealth = true;
        } else if (attribute.name == "MaxHealth") {
            info.maxHealth = attribute.value;
            info.hasHealth = true;
        }
    }
    info.isDead = info.hasHealth && info.health <= 0.0f;
}

void Engine::ReadSkills(PlayerInfo& info) const {
    if (!info.controllerPtr) return;
    if (!IsA(info.controllerPtr, "DominionPlayerControllerBase")) return;

    uintptr_t skills = Mem::ReadPtr(info.controllerPtr + Offsets::DomPlayerController_SkillComponent);
    if (!skills || !Mem::Readable((void*)skills, 0x210)) return;

    FUEArray list = {};
    if (!ReadArray(skills + Offsets::Skill_Skills, list, 256)) return;
    if (!Mem::Readable(list.Data, Offsets::FSkill_Size * (size_t)list.Num)) return;

    for (int32_t i = 0; i < list.Num; i++) {
        uintptr_t entry = (uintptr_t)list.Data + (uintptr_t)i * Offsets::FSkill_Size;
        uintptr_t skillData = Mem::ReadPtr(entry + Offsets::FSkill_SkillData);
        if (!skillData) continue;

        SkillInfo skill;
        skill.skill = GetObjectName(skillData);
        if (skill.skill.empty()) continue;
        skill.currentXP = Mem::ReadI32(entry + Offsets::FSkill_CurrentXP);
        info.skills.push_back(skill);
    }
}

std::vector<PlayerInfo> Engine::GetAllPlayers() const {
    std::vector<PlayerInfo> players;
    discoverySyncAttempts++;
    discoveryGameStateValid = 0;
    discoveryPlayerArrayCount = 0;
    discoveryReadablePlayerStates = 0;
    discoveryDominionPlayerStates = 0;
    discoveryCharacterGuids = 0;
    discoveryCharacterNames = 0;
    discoverySpawnedPawns = 0;
    discoveryWorldLocations = 0;
    discoveryRegionLookupAttempts = 0;
    discoveryPlayersProduced = 0;
    if (!initialized) return players;

    uintptr_t gameState = GetGameState();
    if (!gameState) {
        LogVerbose("GetAllPlayers: no GameState yet");
        return players;
    }
    discoveryGameStateValid = IsA(gameState, "DominionGameStateBase") ? 1 : -1;

    FUEArray playerArray = {};
    uintptr_t arrayAddress = gameState + Offsets::GameStateBase_PlayerArray;
    if (!Mem::Readable((void*)arrayAddress, sizeof(FUEArray))) {
        LogVerbose("GetAllPlayers: PlayerArray unreadable");
        return players;
    }
    memcpy(&playerArray, (void*)arrayAddress, sizeof(playerArray));
    discoveryPlayerArrayCount = playerArray.Num;

    if (!playerArray.Data || playerArray.Num <= 0 || playerArray.Num > 512) return players;
    if (!Mem::Readable(playerArray.Data, sizeof(uintptr_t) * (size_t)playerArray.Num)) {
        return players;
    }

    bool isDominionGameState = IsA(gameState, "DominionGameStateBase");

    std::unordered_map<std::string, std::string> characterNames;
    auto collectCharacterNames = [&](uintptr_t state) {
        if (!state || !IsA(state, "DominionGameStateBase")) return;
        uintptr_t displayNames = Mem::ReadPtr(state + Offsets::DomGameState_DisplayNameComponent);
        if (displayNames) {
            uintptr_t listAddress = displayNames + Offsets::DisplayName_OriginalCharacterNames;
            if (Mem::Readable((void*)listAddress, sizeof(FUEArray))) {
                FUEArray list = {};
                memcpy(&list, (void*)listAddress, sizeof(list));
                if (list.Data && list.Num > 0 && list.Num <= 4096 &&
                    Mem::Readable(list.Data, Offsets::CharDisplayName_Size * (size_t)list.Num)) {
                    for (int32_t i = 0; i < list.Num; i++) {
                        uintptr_t entry = (uintptr_t)list.Data + (uintptr_t)i * Offsets::CharDisplayName_Size;
                        std::string guid = ReadGuid(entry + Offsets::CharDisplayName_CharacterGuid);
                        std::string name = ReadFString(entry + Offsets::CharDisplayName_CharacterName);
                        if (!guid.empty() && !name.empty()) characterNames[guid] = name;
                    }
                }
            }
        }
    };
    if (isDominionGameState) collectCharacterNames(gameState);

    uintptr_t* states = (uintptr_t*)playerArray.Data;
    std::vector<uintptr_t> stateList(states, states + playerArray.Num);

    // During login/world streaming, GWorld can expose a transitional
    // PlayerState array even though the authoritative pawn and controller are
    // already live. Recover only states proven by a two-way live actor
    // relationship; this avoids publishing stale disconnected PlayerStates.
    std::unordered_set<uintptr_t> actorBackedStates;
    int32_t objectCount = GetObjectCount();
    for (int32_t i = 0; i < objectCount; ++i) {
        uintptr_t object = GetObjectByIndex(i);
        if (!object) continue;
        if (IsA(object, "DominionPlayerCharacter")) {
            uintptr_t state = Mem::ReadPtr(object + Offsets::Pawn_PlayerState);
            uintptr_t controller = Mem::ReadPtr(object + Offsets::Pawn_Controller);
            if (state && IsA(state, "DominionPlayerState") &&
                (!controller || Mem::ReadPtr(controller + Offsets::Controller_PlayerState) == state)) {
                actorBackedStates.insert(state);
            }
        } else if (IsA(object, "DominionPlayerControllerBase")) {
            uintptr_t state = Mem::ReadPtr(object + Offsets::Controller_PlayerState);
            uintptr_t pawn = Mem::ReadPtr(object + Offsets::Controller_Pawn);
            if (state && pawn && IsA(state, "DominionPlayerState") &&
                IsA(pawn, "DominionPlayerCharacter") &&
                Mem::ReadPtr(pawn + Offsets::Pawn_Controller) == object) {
                actorBackedStates.insert(state);
            }
        }
    }
    bool listedStateIsLive = false;
    for (uintptr_t state : stateList) {
        if (actorBackedStates.count(state)) { listedStateIsLive = true; break; }
    }
    if (!listedStateIsLive && !actorBackedStates.empty()) {
        stateList.assign(actorBackedStates.begin(), actorBackedStates.end());
        // A recovered state may belong to another live GameState during the
        // transition, so merge every authoritative character-name table.
        for (int32_t i = 0; i < objectCount; ++i) {
            uintptr_t object = GetObjectByIndex(i);
            if (object && IsA(object, "DominionGameStateBase")) collectCharacterNames(object);
        }
    }
    std::unordered_set<uintptr_t> pawnSet;

    for (uintptr_t playerState : stateList) {
        if (!playerState || !Mem::Readable((void*)playerState, 0x370)) continue;
        discoveryReadablePlayerStates++;

        PlayerInfo info;
        info.playerStatePtr = playerState;
        info.name = ReadFString(playerState + Offsets::PlayerState_PlayerName);
        info.playerId = Mem::ReadI32(playerState + Offsets::PlayerState_PlayerId);
        info.score = Mem::ReadFloat(playerState + Offsets::PlayerState_Score);
        info.startTime = Mem::ReadI32(playerState + Offsets::PlayerState_StartTime);
        info.pingMs = (int32_t)Mem::ReadU8(playerState + Offsets::PlayerState_CompressedPing) * 4;

        uint8_t flags = Mem::ReadU8(playerState + Offsets::PlayerState_Flags);
        info.isSpectator = (flags & (1 << 1)) != 0;
        info.isBot = (flags & (1 << 3)) != 0;
        info.isInactive = (flags & (1 << 5)) != 0;

        info.uniqueNetId = ReadUniqueNetId(playerState + Offsets::PlayerState_UniqueID,
                                           info.uniqueNetIdSource);

        if (IsA(playerState, "DominionPlayerState")) {
            discoveryDominionPlayerStates++;
            info.characterGuid = ReadGuid(playerState + Offsets::DomPlayerState_CharacterGuid);
            if (!info.characterGuid.empty()) discoveryCharacterGuids++;

            uintptr_t platformData = playerState + Offsets::DomPlayerState_PlatformData;
            info.platform = ReadFString(platformData + Offsets::PlatformData_PlatformName);
            if (info.uniqueNetId.empty()) {
                info.uniqueNetId = ReadUniqueNetId(platformData + Offsets::PlatformData_UniqueID,
                                                   info.uniqueNetIdSource);
            }

            uintptr_t colorAddress = playerState + Offsets::DomPlayerState_PlayerColor;
            if (Mem::Readable((void*)(colorAddress + Offsets::PlayerColor_Color), sizeof(float) * 4)) {
                memcpy(info.color, (void*)(colorAddress + Offsets::PlayerColor_Color), sizeof(info.color));
                info.hasColor = true;
            }
            info.colorHex = ReadFString(colorAddress + Offsets::PlayerColor_HexString);

            auto nameIt = characterNames.find(info.characterGuid);
            if (nameIt != characterNames.end()) info.characterName = nameIt->second;
            if (!info.characterName.empty()) discoveryCharacterNames++;
        }

        uintptr_t pawn = Mem::ReadPtr(playerState + Offsets::PlayerState_PawnPrivate);
        if (!pawn || !Mem::Readable((void*)pawn, 0x400) ||
            !IsA(pawn, "DominionPlayerCharacter")) {
            pawn = FindPossessedPawnForState(this, playerState);
        }
        if (pawn && Mem::Readable((void*)pawn, 0x400)) {
            discoverySpawnedPawns++;
            info.spawned = true;
            info.pawnPtr = pawn;
            info.pawnClass = GetObjectClassName(pawn);
            info.controllerPtr = Mem::ReadPtr(pawn + Offsets::Pawn_Controller);
            pawnSet.insert(pawn);

            uintptr_t root = Mem::ReadPtr(pawn + Offsets::Actor_RootComponent);
            if (root && Mem::Readable((void*)root, 0x200)) {
                ReadDoubleTriple(root + Offsets::SceneComp_RelativeLocation, info.x, info.y, info.z);
                ReadDoubleTriple(root + Offsets::SceneComp_RelativeRotation,
                                 info.pitch, info.yaw, info.roll);
                info.hasLocation = true;
                discoveryWorldLocations++;
                discoveryRegionLookupAttempts++;
                info.location = GetMapRegionName(pawn, {info.x, info.y, info.z});
                info.dungeonPtr = GetDungeonAtLocation(pawn, {info.x, info.y, info.z});
                FVector3d mapLocation{info.x, info.y, info.z};
                if(info.dungeonPtr){
                    std::string dungeonName=GetDungeonName(info.dungeonPtr);
                    if(!dungeonName.empty()){
                        if(info.location.empty())info.location=GetDungeonRegionName(pawn,info.dungeonPtr);
                        info.location=dungeonName+(info.location.empty()?"":" - "+info.location);
                    }
                    FVector3d entrance{};
                    if(GetDungeonEntrance(info.dungeonPtr,entrance)){
                        mapLocation=entrance;
                        info.mapAtDungeonEntrance=true;
                    }
                }
                info.hasMapLocation=ProjectWorldToMap(mapLocation,info.mapX,info.mapY);
            }

            if (IsA(pawn, "Character")) {
                uintptr_t movement = Mem::ReadPtr(pawn + Offsets::Character_CharacterMovement);
                if (movement && Mem::Readable((void*)movement, 0x300)) {
                    ReadDoubleTriple(movement + Offsets::MovementComp_Velocity,
                                     info.vx, info.vy, info.vz);
                    info.hasVelocity = true;
                    info.movementMode = Mem::ReadU8(movement + Offsets::CharMove_MovementMode);
                }
            }
        }

        players.push_back(info);
        discoveryPlayersProduced++;
    }

    if (pawnSet.empty()) return players;

    std::unordered_map<uintptr_t, PawnComponents> byPawn;
    for (int32_t i = 0; i < objectCount; i++) {
        uintptr_t object = GetObjectByIndex(i);
        if (!object) continue;

        uintptr_t outer = Mem::ReadPtr(object + Offsets::UObject_Outer);
        if (!outer || pawnSet.find(outer) == pawnSet.end()) continue;

        PawnComponents& slots = byPawn[outer];
        if (!slots.sustenance && IsA(object, "SustenanceComponent")) {
            slots.sustenance = object;
        } else if (!slots.hydration && IsA(object, "HydrationComponent")) {
            slots.hydration = object;
        }
    }

    for (PlayerInfo& info : players) {
        if (!info.pawnPtr) continue;

        ReadHealthAndAttributes(info);
        ReadSkills(info);

        auto it = byPawn.find(info.pawnPtr);
        if (it == byPawn.end()) continue;
        const PawnComponents& slots = it->second;

        if (slots.sustenance && Mem::Readable((void*)slots.sustenance, 0x140)) {
            info.hasSustenance = true;
            info.sustenance = Mem::ReadFloat(slots.sustenance + Offsets::SurvivalStat_Current);
            info.maxSustenance = Mem::ReadFloat(slots.sustenance + Offsets::SurvivalStat_CurrentMax);
        }

        if (slots.hydration && Mem::Readable((void*)slots.hydration, 0x140)) {
            info.hasHydration = true;
            info.hydration = Mem::ReadFloat(slots.hydration + Offsets::SurvivalStat_Current);
            info.maxHydration = Mem::ReadFloat(slots.hydration + Offsets::SurvivalStat_CurrentMax);
        }

    }

    return players;
}

}
