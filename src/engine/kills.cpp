#include "dom_engine.h"
#include "../utils/logger.h"
#include "../utils/memory.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <pthread.h>
#include <sys/mman.h>
#include <unistd.h>
#include <unordered_map>
#include <unordered_set>
#include <fstream>
#include <sstream>

namespace DomEngine {
namespace {

// .sym address for UDominionAISubsystem::OnAIKilled. The main image's lowest
// PT_LOAD begins at ELF VMA 0x200000, while .sym addresses have 0x200000
// removed, so this value is also the offset from Engine::GetModuleBase().
constexpr size_t HookBytes = 14;
constexpr size_t QueueCapacity = 256;
constexpr auto DuplicateWindow = std::chrono::seconds(5);
struct BuildCompatibility {
    std::string sha256;
    uintptr_t onAIKilled = 0, textOffset = 0, fTextDisplayString = 0, mapRegionAtLocation = 0;
    uintptr_t mapRegionNameOffset = 0;
    uintptr_t dungeonAtLocation = 0;
    uintptr_t dungeonNameOffset = 0;
    uintptr_t dungeonEntranceOffset = 0;
    double mapMinX = 0, mapMinY = 0, mapWorldSize = 0;
    std::string mapAssetVersion;
    uintptr_t registryGetInstance = 0, registryGetLodestones = 0, lodestoneGetLocation = 0;
    uintptr_t teleportTargetNames = 0, teleportTargetTransform = 0;
    uintptr_t adminPlayerTick = 0, actorTeleportTo = 0, processEvent = 0;
    uintptr_t lodestoneTryTeleport = 0, lodestoneCanTeleport = 0;
    size_t textSize = 0;
};
BuildCompatibility activeBuild;

bool LoadCompatibility(const std::string& sha, BuildCompatibility& out) {
    const char* raw = std::getenv("DW_COMPATIBILITY_FILE");
    std::ifstream input(raw && *raw ? raw : "/home/container/zanik/compatibility.tsv");
    std::string line;
    while (std::getline(input, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream fields(line);
        BuildCompatibility candidate;
        std::string onAI, text, size, display, region, regionName, dungeon, dungeonName, dungeonEntrance;
        std::string mapMinX, mapMinY, mapWorldSize, registryGetInstance, registryGetLodestones,
                    lodestoneGetLocation, labelText, labelRotation, labelFont,
                    teleportTargetNames, teleportTargetTransform, adminPlayerTick, actorTeleportTo, processEvent,
                    lodestoneTryTeleport, lodestoneCanTeleport;
        if (!std::getline(fields, candidate.sha256, '\t') ||
            !std::getline(fields, onAI, '\t') || !std::getline(fields, text, '\t') ||
            !std::getline(fields, size, '\t') || !std::getline(fields, display, '\t') ||
            !std::getline(fields, region, '\t') || !std::getline(fields, regionName, '\t') ||
            !std::getline(fields, dungeon, '\t') || !std::getline(fields, dungeonName, '\t') ||
            !std::getline(fields, dungeonEntrance, '\t')) continue;
        try {
            candidate.onAIKilled = std::stoull(onAI, nullptr, 0);
            candidate.textOffset = std::stoull(text, nullptr, 0);
            candidate.textSize = std::stoull(size, nullptr, 0);
            candidate.fTextDisplayString = std::stoull(display, nullptr, 0);
            candidate.mapRegionAtLocation = std::stoull(region, nullptr, 0);
            candidate.mapRegionNameOffset = std::stoull(regionName, nullptr, 0);
            candidate.dungeonAtLocation = std::stoull(dungeon, nullptr, 0);
            candidate.dungeonNameOffset = std::stoull(dungeonName, nullptr, 0);
            candidate.dungeonEntranceOffset = std::stoull(dungeonEntrance, nullptr, 0);
            if (std::getline(fields, mapMinX, '\t') && std::getline(fields, mapMinY, '\t') &&
                std::getline(fields, mapWorldSize, '\t') && std::getline(fields, candidate.mapAssetVersion, '\t')) {
                candidate.mapMinX = std::stod(mapMinX);
                candidate.mapMinY = std::stod(mapMinY);
                candidate.mapWorldSize = std::stod(mapWorldSize);
                if(std::getline(fields,registryGetInstance,'\t') &&
                   std::getline(fields,registryGetLodestones,'\t') &&
                   std::getline(fields,lodestoneGetLocation,'\t')) {
                    candidate.registryGetInstance=std::stoull(registryGetInstance,nullptr,0);
                    candidate.registryGetLodestones=std::stoull(registryGetLodestones,nullptr,0);
                    candidate.lodestoneGetLocation=std::stoull(lodestoneGetLocation,nullptr,0);
                    if(std::getline(fields,labelText,'\t') && std::getline(fields,labelRotation,'\t') &&
                       std::getline(fields,labelFont,'\t') && std::getline(fields,teleportTargetNames,'\t') &&
                       std::getline(fields,teleportTargetTransform,'\t')) {
                        candidate.teleportTargetNames=std::stoull(teleportTargetNames,nullptr,0);
                        candidate.teleportTargetTransform=std::stoull(teleportTargetTransform,nullptr,0);
                        if(std::getline(fields,adminPlayerTick,'\t') && std::getline(fields,actorTeleportTo,'\t') &&
                           std::getline(fields,processEvent,'\t')){
                            candidate.adminPlayerTick=std::stoull(adminPlayerTick,nullptr,0);
                            candidate.actorTeleportTo=std::stoull(actorTeleportTo,nullptr,0);
                            candidate.processEvent=std::stoull(processEvent,nullptr,0);
                            if(std::getline(fields,lodestoneTryTeleport,'\t') && std::getline(fields,lodestoneCanTeleport,'\t')){
                                candidate.lodestoneTryTeleport=std::stoull(lodestoneTryTeleport,nullptr,0);
                                candidate.lodestoneCanTeleport=std::stoull(lodestoneCanTeleport,nullptr,0);
                            }
                        }
                    }
                }
            }
        } catch (...) { continue; }
        if (candidate.sha256 == sha && candidate.onAIKilled && candidate.textOffset &&
            candidate.textSize && candidate.fTextDisplayString && candidate.mapRegionAtLocation &&
            candidate.mapRegionNameOffset && candidate.dungeonAtLocation && candidate.dungeonNameOffset &&
            candidate.dungeonEntranceOffset) {
            out = candidate; return true;
        }
    }
    return false;
}

using OnAIKilledFn = void (*)(void*, uintptr_t, uintptr_t, const void*);
OnAIKilledFn originalOnAIKilled = nullptr;
pthread_mutex_t queueMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t playerCacheMutex = PTHREAD_MUTEX_INITIALIZER;
std::deque<NativeKillEvent> eventQueue;
std::unordered_map<uintptr_t,std::string> playerIdentityCache;
std::unordered_map<uintptr_t, std::pair<std::uint64_t,
    std::chrono::steady_clock::time_point>> recentVictims;
std::atomic<int> hookState{0};
std::atomic<std::uint64_t> rawFatalEvents{0}, acceptedKills{0}, duplicateEvents{0};
std::atomic<std::uint64_t> unresolvedAttackers{0}, rejectedNonPlayerDeaths{0};
std::atomic<std::uint64_t> droppedQueueEvents{0};
std::atomic<std::uint64_t> maximumHookMicroseconds{0},slowHookEvents{0};
std::atomic<int> regionLookupState{0};
std::atomic<int> lodestoneCompatibility{0},lodestoneRegistryFound{0},lodestoneRegistryCount{0},lodestoneValidMarkers{0};
std::atomic<std::uint64_t> lodestoneRejectedRecords{0};
std::atomic<std::int64_t> lodestoneLastSuccessfulCollection{0};
std::atomic<int> nodestoneCompatibility{0},nodestoneActorsFound{0},nodestoneValidMarkers{0};
std::atomic<std::uint64_t> nodestoneRejectedRecords{0};
std::atomic<std::int64_t> nodestoneLastSuccessfulCollection{0};
std::atomic<int> labelComponents{0},labelTextProperties{0},labelReadableTexts{0},labelActorOwners{0},labelValidAnchors{0};
std::atomic<uintptr_t> cachedDungeonRegistry{0};

uint32_t RotateRight(uint32_t value, unsigned bits) {
    return (value >> bits) | (value << (32 - bits));
}

struct HookDurationSample {
    std::chrono::steady_clock::time_point started=std::chrono::steady_clock::now();
    bool finished=false;
    void Finish(){
        if(finished)return;finished=true;
        auto elapsed=static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now()-started).count());
        auto maximum=maximumHookMicroseconds.load(std::memory_order_relaxed);
        while(elapsed>maximum&&!maximumHookMicroseconds.compare_exchange_weak(
            maximum,elapsed,std::memory_order_relaxed)){}
        if(elapsed>=1000)++slowHookEvents;
    }
    ~HookDurationSample(){Finish();}
};

std::string ResolveCachedPlayer(uintptr_t actor){
    if(!actor)return {};
    pthread_mutex_lock(&playerCacheMutex);
    std::string result;
    for(int depth=0;actor&&depth<6;++depth){
        auto found=playerIdentityCache.find(actor);
        if(found!=playerIdentityCache.end()){result=found->second;break;}
        if(!Mem::Readable(reinterpret_cast<void*>(actor),Offsets::Actor_Owner+sizeof(uintptr_t)))break;
        uintptr_t next=Mem::ReadPtr(actor+Offsets::Actor_Owner);
        if(!next||next==actor)next=Mem::ReadPtr(actor+Offsets::UObject_Outer);
        actor=next;
    }
    pthread_mutex_unlock(&playerCacheMutex);
    return result;
}

// Self-contained SHA-256 keeps the executable check independent of container tools.
std::string ComputeFileSha256(const char* path) {
    static constexpr uint32_t k[64] = {
        0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
        0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
        0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
        0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
        0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
        0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
        0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
        0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2};
    uint32_t hash[8] = {0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,
                        0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};
    FILE* file = std::fopen(path, "rb");
    if (!file) return {};
    std::array<unsigned char,64> block{};
    uint64_t total=0;
    auto process = [&](const unsigned char* bytes) {
        uint32_t w[64];
        for (int i=0;i<16;++i) w[i]=(uint32_t(bytes[i*4])<<24)|(uint32_t(bytes[i*4+1])<<16)|
            (uint32_t(bytes[i*4+2])<<8)|bytes[i*4+3];
        for (int i=16;i<64;++i) {
            uint32_t s0=RotateRight(w[i-15],7)^RotateRight(w[i-15],18)^(w[i-15]>>3);
            uint32_t s1=RotateRight(w[i-2],17)^RotateRight(w[i-2],19)^(w[i-2]>>10);
            w[i]=w[i-16]+s0+w[i-7]+s1;
        }
        uint32_t a=hash[0],b=hash[1],c=hash[2],d=hash[3],e=hash[4],f=hash[5],g=hash[6],h=hash[7];
        for (int i=0;i<64;++i) {
            uint32_t s1=RotateRight(e,6)^RotateRight(e,11)^RotateRight(e,25);
            uint32_t t1=h+s1+((e&f)^((~e)&g))+k[i]+w[i];
            uint32_t s0=RotateRight(a,2)^RotateRight(a,13)^RotateRight(a,22);
            uint32_t t2=s0+((a&b)^(a&c)^(b&c));
            h=g;g=f;f=e;e=d+t1;d=c;c=b;b=a;a=t1+t2;
        }
        hash[0]+=a;hash[1]+=b;hash[2]+=c;hash[3]+=d;
        hash[4]+=e;hash[5]+=f;hash[6]+=g;hash[7]+=h;
    };
    size_t used;
    while ((used=std::fread(block.data(),1,block.size(),file))==block.size()) {
        process(block.data()); total+=block.size();
    }
    total+=used; block[used++]=0x80;
    if (used>56) { std::fill(block.begin()+used,block.end(),0);process(block.data());used=0; }
    std::fill(block.begin()+used,block.begin()+56,0);
    uint64_t bits=total*8;
    for(int i=0;i<8;++i) block[63-i]=static_cast<unsigned char>(bits>>(i*8));
    process(block.data()); std::fclose(file);
    char out[65]{};
    for(int i=0;i<8;++i) std::snprintf(out+i*8,9,"%08x",hash[i]);
    return out;
}

void WriteAbsoluteJump(unsigned char* destination, uintptr_t target) {
    destination[0]=0x48;destination[1]=0xB8;
    std::memcpy(destination+2,&target,sizeof(target));
    destination[10]=0xFF;destination[11]=0xE0;
}

void HookOnAIKilled(void* subsystem, uintptr_t enemy, uintptr_t killer, const void* event) {
    HookDurationSample duration;
    ++rawFatalEvents;
    RecordNpcDeath(enemy);
    if (!g_Engine || !enemy || !g_Engine->IsA(enemy,"DominionAICharacter")) {
        ++rejectedNonPlayerDeaths;
        duration.Finish();
        originalOnAIKilled(subsystem,enemy,killer,event);
        return;
    }
    std::string guid;
    if (killer) guid=ResolveCachedPlayer(killer);
    if (guid.empty()) {
        if (killer) ++unresolvedAttackers; else ++rejectedNonPlayerDeaths;
        duration.Finish();
        originalOnAIKilled(subsystem,enemy,killer,event);
        return;
    }
    auto now=std::chrono::steady_clock::now();
    std::string internalEnemy=g_Engine->GetObjectClassName(enemy);
    if(internalEnemy.rfind("Default__",0)==0)internalEnemy.erase(0,9);
    if(internalEnemy.size()>2&&internalEnemy.compare(internalEnemy.size()-2,2,"_C")==0)
        internalEnemy.erase(internalEnemy.size()-2);
    if(internalEnemy.rfind("BP_",0)==0)internalEnemy.erase(0,3);
    std::replace(internalEnemy.begin(),internalEnemy.end(),'_',' ');
    std::string enemyName=g_Engine->GetEnemyDisplayName(enemy);
    pthread_mutex_lock(&queueMutex);
    auto& prior=recentVictims[enemy];
    if (prior.first && now-prior.second<DuplicateWindow) {
        ++duplicateEvents;pthread_mutex_unlock(&queueMutex);
        duration.Finish();
        originalOnAIKilled(subsystem,enemy,killer,event);return;
    }
    ++prior.first;prior.second=now;
    if(eventQueue.size()==QueueCapacity){eventQueue.pop_front();++droppedQueueEvents;}
    eventQueue.push_back({enemy,prior.first,std::move(internalEnemy),std::move(enemyName),std::move(guid)});
    ++acceptedKills;
    pthread_mutex_unlock(&queueMutex);
    duration.Finish();
    originalOnAIKilled(subsystem,enemy,killer,event);
}

}

std::string FileSha256(const char* path) { return path ? ComputeFileSha256(path) : std::string{}; }

bool InstallNativeKillHook(uintptr_t moduleBase, uintptr_t moduleEnd) {
    if(originalOnAIKilled)return true;
    if(!moduleBase||moduleEnd<=moduleBase){hookState=-1;return false;}
    std::string executableSha = FileSha256("/proc/self/exe");
    if(!LoadCompatibility(executableSha,activeBuild)){hookState=-2;return false;}
    const unsigned char signature[]={
        0x55,0x41,0x57,0x41,0x56,0x41,0x55,0x41,0x54,0x53,0x48,0x83,0xEC,0x18,
        0x48,0x89,0x4C,0x24,0x10,0x8B,0x6F,0x50,0x8B,0x47,0x5C,0x8D,0x48,0x01};
    auto* target=reinterpret_cast<unsigned char*>(moduleBase+activeBuild.onAIKilled);
    if(!Mem::Readable(target,sizeof(signature))||std::memcmp(target,signature,sizeof(signature))!=0){hookState=-3;return false;}
    uintptr_t textStart=moduleBase+activeBuild.textOffset;
    if(textStart<activeBuild.textOffset||textStart+activeBuild.textSize>moduleEnd||
       !Mem::Readable(reinterpret_cast<void*>(textStart),activeBuild.textSize)){hookState=-4;return false;}
    size_t matches=0;
    for(uintptr_t address=textStart;address+sizeof(signature)<=textStart+activeBuild.textSize;++address){
        if(std::memcmp(reinterpret_cast<void*>(address),signature,sizeof(signature))==0)++matches;
    }
    if(matches!=1){hookState=-4;return false;}
    auto* trampoline=static_cast<unsigned char*>(mmap(nullptr,64,PROT_READ|PROT_WRITE|PROT_EXEC,
        MAP_PRIVATE|MAP_ANONYMOUS,-1,0));
    if(trampoline==MAP_FAILED){hookState=-5;return false;}
    std::memcpy(trampoline,target,HookBytes);
    WriteAbsoluteJump(trampoline+HookBytes,reinterpret_cast<uintptr_t>(target+HookBytes));
    originalOnAIKilled=reinterpret_cast<OnAIKilledFn>(trampoline);
    long pageSize=sysconf(_SC_PAGESIZE);
    uintptr_t page=reinterpret_cast<uintptr_t>(target)&~(uintptr_t(pageSize)-1);
    if(mprotect(reinterpret_cast<void*>(page),size_t(pageSize),PROT_READ|PROT_WRITE|PROT_EXEC)!=0){
        originalOnAIKilled=nullptr;munmap(trampoline,64);hookState=-6;return false;
    }
    unsigned char patch[HookBytes];std::memset(patch,0x90,sizeof(patch));
    WriteAbsoluteJump(patch,reinterpret_cast<uintptr_t>(&HookOnAIKilled));
    std::memcpy(target,patch,sizeof(patch));
    __builtin___clear_cache(reinterpret_cast<char*>(target),reinterpret_cast<char*>(target+sizeof(patch)));
    mprotect(reinterpret_cast<void*>(page),size_t(pageSize),PROT_READ|PROT_EXEC);
    hookState=1;return true;
}

void UpdateKillPlayerCache(const std::vector<PlayerInfo>& players){
    std::unordered_map<uintptr_t,std::string> next;
    next.reserve(players.size()*3);
    for(const auto& player:players){
        if(player.characterGuid.empty())continue;
        if(player.pawnPtr)next[player.pawnPtr]=player.characterGuid;
        if(player.controllerPtr)next[player.controllerPtr]=player.characterGuid;
        if(player.playerStatePtr)next[player.playerStatePtr]=player.characterGuid;
    }
    pthread_mutex_lock(&playerCacheMutex);
    playerIdentityCache.swap(next);
    pthread_mutex_unlock(&playerCacheMutex);
}

std::vector<NativeKillEvent> DrainNativeKillEvents(){
    pthread_mutex_lock(&queueMutex);
    std::vector<NativeKillEvent> result(eventQueue.begin(),eventQueue.end());eventQueue.clear();
    pthread_mutex_unlock(&queueMutex);return result;
}

int NativeKillHookState(){return hookState.load();}
KillDiagnostics GetKillDiagnostics(){return {hookState.load(),regionLookupState.load(),rawFatalEvents.load(),acceptedKills.load(),
    duplicateEvents.load(),unresolvedAttackers.load(),rejectedNonPlayerDeaths.load(),droppedQueueEvents.load(),
    maximumHookMicroseconds.load(),slowHookEvents.load()};}

LodestoneDiagnostics GetLodestoneDiagnostics(){
    return {lodestoneCompatibility.load(),lodestoneRegistryFound.load(),lodestoneRegistryCount.load(),
            lodestoneValidMarkers.load(),lodestoneRejectedRecords.load(),
            lodestoneLastSuccessfulCollection.load()};
}

NodestoneDiagnostics GetNodestoneDiagnostics(){
    return {nodestoneCompatibility.load(),nodestoneActorsFound.load(),nodestoneValidMarkers.load(),
            nodestoneRejectedRecords.load(),nodestoneLastSuccessfulCollection.load()};
}

bool ReviewedNodestoneLocation(const std::string& suffix,std::string& display){
    static const std::unordered_map<std::string,std::string> locations={
        {"Aquifer","Aquifer"},{"Bastion","Bastion"},{"Courtyard","Courtyard"},
        {"Garrison","Garrison"},{"Library","Library"},{"Menagerie","Menagerie"},
        {"Mines","Mines"},{"Pastures","Pastures"},{"Temple","Temple"}
    };
    const auto found=locations.find(suffix);if(found==locations.end())return false;
    display=found->second;return true;
}

bool ReadNodestones(std::vector<NodestoneInfo>& out){
    out.clear();nodestoneActorsFound=0;nodestoneValidMarkers=0;
    if(!g_Engine||!g_Engine->IsInitialized()||!g_Engine->IsMapProjectionAvailable()){
        nodestoneCompatibility=-1;return false;
    }
    if(!activeBuild.teleportTargetNames||!activeBuild.teleportTargetTransform){nodestoneCompatibility=-2;return false;}
    std::unordered_set<std::string> seen;
    const int32_t count=g_Engine->GetObjectCount();
    uintptr_t subsystem=0;
    for(int32_t i=0;i<count;++i){
        const uintptr_t object=g_Engine->GetObjectByIndex(i);
        if(object&&g_Engine->IsA(object,"TeleportationSubsystem")&&
           g_Engine->GetObjectName(object).rfind("Default__",0)!=0)subsystem=object;
    }
    if(!subsystem||!Mem::Readable(reinterpret_cast<void*>(subsystem+0x30),0x50)){
        nodestoneCompatibility=-3;return false;
    }
    const uintptr_t entries=Mem::ReadPtr(subsystem+0x30);
    const int32_t slots=Mem::ReadI32(subsystem+0x38);
    if(!entries||slots<0||slots>4096||!Mem::Readable(reinterpret_cast<void*>(entries),static_cast<size_t>(slots)*0x80)){
        nodestoneCompatibility=-4;return false;
    }
    using GetTransformFn=bool (*)(uintptr_t,const void*,void*);
    auto getTransform=reinterpret_cast<GetTransformFn>(g_Engine->GetModuleBase()+activeBuild.teleportTargetTransform);
    const std::uint64_t subsystemIdentity=g_Engine->GetObjectIdentity(subsystem);
    struct Target { std::string name; FVector3d location; std::uint32_t nativeName[2]{}; };
    std::vector<Target> targets;
    std::unordered_set<std::string> targetNames;
    for(int32_t i=0;i<slots;++i){
        const uintptr_t entry=entries+static_cast<uintptr_t>(i)*0x80;
        const uint32_t comparisonIndex=static_cast<uint32_t>(Mem::ReadI32(entry));
        const uint32_t number=static_cast<uint32_t>(Mem::ReadI32(entry+4));
        const std::string targetName=g_Engine->NameToString(comparisonIndex,number);
        if(targetName.empty())continue;
        alignas(16) unsigned char transform[0x60]{};
        if(!getTransform(subsystem,reinterpret_cast<const void*>(entry),transform))continue;
        FVector3d location{};std::memcpy(&location,transform+0x20,sizeof(location));
        Target target;target.name=targetName;target.location=location;
        target.nativeName[0]=comparisonIndex;target.nativeName[1]=number;
        targets.push_back(target);targetNames.insert(targetName);
    }
    for(const auto& target:targets){
        const std::string nexusPrefix="DR_Nexus_";
        std::string suffix;
        const bool nexus=target.name.rfind(nexusPrefix,0)==0;
        if(nexus){
            suffix=target.name.substr(nexusPrefix.size());
            if(!targetNames.count("DR_"+suffix))continue;
        }else if(target.name.rfind("DR_",0)==0){
            suffix=target.name.substr(3);
            if(!targetNames.count(nexusPrefix+suffix))continue;
        }else continue;
        std::string location;if(!ReviewedNodestoneLocation(suffix,location)){++nodestoneRejectedRecords;continue;}
        ++nodestoneActorsFound;
        double x=0,y=0;
        if(!std::isfinite(target.location.X)||!std::isfinite(target.location.Y)||!std::isfinite(target.location.Z)||
           !g_Engine->ProjectWorldToMap(target.location,x,y)){++nodestoneRejectedRecords;continue;}
        std::uint64_t hash=1469598103934665603ULL;
        for(unsigned char c:target.name){hash^=c;hash*=1099511628211ULL;}
        char id[17]{};std::snprintf(id,sizeof(id),"%016llx",static_cast<unsigned long long>(hash));
        if(seen.insert(id).second){
            NodestoneInfo info;info.id=id;info.mapX=x;info.mapY=y;info.worldLocation=target.location;
            info.nexusEndpoint=nexus;info.displayLocation=nexus?"The Nexus \xC2\xB7 "+location+" route":location;
            info.nativeTargetName[0]=target.nativeName[0];info.nativeTargetName[1]=target.nativeName[1];
            info.nativeSubsystem=subsystem;info.nativeSubsystemIdentity=subsystemIdentity;
            out.push_back(std::move(info));
        }
    }
    nodestoneValidMarkers=static_cast<int>(out.size());
    nodestoneLastSuccessfulCollection=std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    nodestoneCompatibility=1;
    // Native functions and their child properties can finish registering just
    // after the core engine becomes usable. Retry the independent mutation
    // gate here on this HTTP worker; never scan reflection from the game tick.
    RefreshAdminNodestoneTeleportCompatibility();
    return true;
}

bool ReadLodestones(std::vector<LodestoneInfo>& out){
    out.clear();
    lodestoneRegistryFound=0;lodestoneRegistryCount=0;lodestoneValidMarkers=0;
    if(!g_Engine||!g_Engine->IsInitialized()||!g_Engine->IsMapProjectionAvailable()||
       !activeBuild.registryGetLodestones||!activeBuild.lodestoneGetLocation){
        lodestoneCompatibility=-1;return false;
    }
    uintptr_t world=g_Engine->GetWorld();if(!world){lodestoneCompatibility=-8;return false;}
    using GetRegistryFn=uintptr_t (*)(uintptr_t);
    uintptr_t registry=reinterpret_cast<GetRegistryFn>(
        g_Engine->GetModuleBase()+activeBuild.registryGetInstance)(world);
    uintptr_t scriptStruct=0;
    int32_t count=g_Engine->GetObjectCount();
    for(int32_t i=0;i<count;++i){
        uintptr_t object=g_Engine->GetObjectByIndex(i);if(!object)continue;
        std::string name=g_Engine->GetObjectName(object);
        if(!scriptStruct&&name=="LodestoneData"&&g_Engine->GetObjectClassName(object)=="ScriptStruct")scriptStruct=object;
        if(scriptStruct)break;
    }
    if(!scriptStruct||!Mem::Readable(reinterpret_cast<void*>(scriptStruct+Offsets::UStruct_PropertiesSize),4)){
        lodestoneCompatibility=-3;return false;
    }
    int32_t elementSize=Mem::ReadI32(scriptStruct+Offsets::UStruct_PropertiesSize);
    if(elementSize<16||elementSize>512||(elementSize&7)){lodestoneCompatibility=-4;return false;}
    // The fingerprint-bound GetLodestones prologue proves the registry's
    // internal TArray is at +0x2F0 and FLodestoneData is 0x40 bytes. Reading
    // it directly avoids copying a game-owned TArray across allocator ABIs.
    constexpr uintptr_t RegistryLodestonesOffset=0x2F0;
    constexpr int32_t VerifiedElementSize=0x40;
    auto validRegistry=[&](uintptr_t candidate,FUEArray* result=nullptr){
        if(!candidate||!g_Engine->IsA(candidate,"GameplayObjectRegistry")||
           g_Engine->GetObjectName(candidate).rfind("Default__",0)==0||
           !Mem::Readable(reinterpret_cast<void*>(candidate+RegistryLodestonesOffset),sizeof(FUEArray)))return false;
        FUEArray a{};std::memcpy(&a,reinterpret_cast<void*>(candidate+RegistryLodestonesOffset),sizeof(a));
        if(a.Num<0||a.Num>4096||a.Max<a.Num||
           (a.Num&&(!a.Data||!Mem::Readable(a.Data,static_cast<size_t>(a.Num)*VerifiedElementSize))))return false;
        if(result)*result=a;return true;
    };
    using GetLocationFn=void (*)(FVector3d*,uintptr_t,uintptr_t);
    auto getLocation=reinterpret_cast<GetLocationFn>(g_Engine->GetModuleBase()+activeBuild.lodestoneGetLocation);
    // A login transition can leave the singleton getter pointing at a
    // structurally valid but stale registry. Score every live registry by the
    // number of records whose game-resolved locations project successfully.
    // This selects the authoritative registry instead of trusting its count.
    uintptr_t best=0;FUEArray selected{};int32_t bestValid=-1;
    std::unordered_set<uintptr_t> candidates;
    if(registry)candidates.insert(registry);
    for(int32_t i=0;i<count;++i){
        uintptr_t candidate=g_Engine->GetObjectByIndex(i);
        if(candidate&&g_Engine->IsA(candidate,"GameplayObjectRegistry"))candidates.insert(candidate);
    }
    for(uintptr_t candidate:candidates){
        FUEArray a{};if(!validRegistry(candidate,&a))continue;int32_t valid=0;
        for(int32_t i=0;i<a.Num;++i){
            uintptr_t entry=reinterpret_cast<uintptr_t>(a.Data)+static_cast<uintptr_t>(i)*VerifiedElementSize;
            FVector3d location{};getLocation(&location,entry,world);double x=0,y=0;
            if(std::isfinite(location.X)&&std::isfinite(location.Y)&&std::isfinite(location.Z)&&
               g_Engine->ProjectWorldToMap(location,x,y))++valid;
        }
        if(valid>bestValid||(valid==bestValid&&a.Num>selected.Num)){
            best=candidate;selected=a;bestValid=valid;
        }
    }
    registry=best;
    if(!registry){lodestoneCompatibility=-2;return false;}
    lodestoneRegistryFound=1;
    if(elementSize!=VerifiedElementSize||
       !Mem::Readable(reinterpret_cast<void*>(registry+RegistryLodestonesOffset),sizeof(FUEArray))){
        lodestoneCompatibility=-6;return false;
    }
    FUEArray array=selected;
    lodestoneRegistryCount=array.Num;
    if(array.Num<0||array.Num>4096||array.Max<array.Num||
       (array.Num&&(!array.Data||!Mem::Readable(array.Data,static_cast<size_t>(array.Num)*VerifiedElementSize)))){
        lodestoneCompatibility=-7;return false;
    }
    // Never publish a partial snapshot. The browser retains its previous
    // complete marker set while this transient registry state is unavailable.
    if(bestValid!=array.Num){lodestoneCompatibility=-9;return false;}
    std::unordered_set<std::string> seen;
    for(int32_t i=0;i<array.Num;++i){
        uintptr_t entry=reinterpret_cast<uintptr_t>(array.Data)+static_cast<uintptr_t>(i)*VerifiedElementSize;
        FVector3d location{};getLocation(&location,entry,world);
        double x=0,y=0;
        if(!std::isfinite(location.X)||!std::isfinite(location.Y)||
           !std::isfinite(location.Z)||!g_Engine->ProjectWorldToMap(location,x,y)){
            ++lodestoneRejectedRecords;continue;
        }
        // Player-built lodestones are immovable. Their validated world position is
        // a stable opaque identity without exposing the registry's internal GUID.
        std::uint64_t hash=1469598103934665603ULL;
        const auto* bytes=reinterpret_cast<const unsigned char*>(&location);
        for(size_t b=0;b<sizeof(location);++b){hash^=bytes[b];hash*=1099511628211ULL;}
        char id[17]{};std::snprintf(id,sizeof(id),"%016llx",static_cast<unsigned long long>(hash));
        if(!seen.insert(id).second){++lodestoneRejectedRecords;continue;}
        LodestoneInfo info;info.id=id;info.mapX=x;info.mapY=y;info.worldLocation=location;info.nativeEntry=entry;
        // FLodestoneData is fingerprint-bound to 0x40 bytes. GetLodestone()
        // compares its FGuid at +0x30, while
        // ULodestoneTeleportationComponent::GetTeleportationRespawnLocation
        // copies the safe player arrival vector from +0x18.
        if(!Mem::Readable(reinterpret_cast<void*>(entry+0x18),0x28)){++lodestoneRejectedRecords;continue;}
        std::memcpy(&info.arrivalLocation,reinterpret_cast<void*>(entry+0x18),sizeof(info.arrivalLocation));
        std::memcpy(info.nativeGuid,reinterpret_cast<void*>(entry+0x30),sizeof(info.nativeGuid));
        if(!std::isfinite(info.arrivalLocation.X)||!std::isfinite(info.arrivalLocation.Y)||
           !std::isfinite(info.arrivalLocation.Z)||
           !(info.nativeGuid[0]|info.nativeGuid[1]|info.nativeGuid[2]|info.nativeGuid[3])){
            ++lodestoneRejectedRecords;continue;
        }
        out.push_back(std::move(info));
    }
    lodestoneValidMarkers=static_cast<int>(out.size());
    lodestoneLastSuccessfulCollection=std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    lodestoneCompatibility=1;return true;
}

bool GetAdminTeleportOffsets(uintptr_t& playerTick,uintptr_t& actorTeleportTo,uintptr_t& processEvent,
                             uintptr_t& lodestoneTryTeleport,uintptr_t& lodestoneCanTeleport){
    playerTick=activeBuild.adminPlayerTick;actorTeleportTo=activeBuild.actorTeleportTo;
    processEvent=activeBuild.processEvent;
    lodestoneTryTeleport=activeBuild.lodestoneTryTeleport;
    lodestoneCanTeleport=activeBuild.lodestoneCanTeleport;
    return playerTick&&actorTeleportTo&&processEvent&&lodestoneTryTeleport&&lodestoneCanTeleport;
}

bool ReadMapLabels(std::vector<MapLabelInfo>& out){
    out.clear();
    labelComponents=0;labelTextProperties=0;labelReadableTexts=0;labelActorOwners=0;labelValidAnchors=0;
    if(!g_Engine||!g_Engine->IsInitialized()||!g_Engine->IsMapProjectionAvailable()||
       !activeBuild.fTextDisplayString)return false;
    using GetDisplayStringFn=const FString* (*)(const void*);
    auto getDisplayString=reinterpret_cast<GetDisplayStringFn>(
        g_Engine->GetModuleBase()+activeBuild.fTextDisplayString);
    std::unordered_set<std::string> seen;
    int32_t count=g_Engine->GetObjectCount();
    for(int32_t i=0;i<count;++i){
        uintptr_t component=g_Engine->GetObjectByIndex(i);
        if(!component||g_Engine->GetObjectName(component).rfind("Default__",0)==0||
           !g_Engine->IsA(component,"MapRegionIconComponent"))continue;
        ++labelComponents;
        constexpr uintptr_t LabelTextOffset=0x7C0;
        constexpr uintptr_t LabelRotationOffset=0x7D0;
        constexpr uintptr_t LabelFontSizeOffset=0x7D4;
        constexpr uintptr_t LetterTintOffset=0x7D8;
        constexpr uintptr_t LetterTintOpacityOffset=0x7E8;
        constexpr uintptr_t LetterOpacityOffset=0x7EC;
        constexpr uintptr_t LetterContrastOffset=0x7F0;
        constexpr uintptr_t GlowTintOffset=0x7F4;
        constexpr uintptr_t GlowTintOpacityOffset=0x804;
        constexpr uintptr_t GlowOpacityOffset=0x808;
        constexpr uintptr_t GlowRangeOffset=0x80C;
        constexpr uintptr_t GlowBrightnessOffset=0x810;
        constexpr uintptr_t VolumeToFlatnessOffset=0x814;
        constexpr uintptr_t RegularToBoldOffset=0x818;
        if(!Mem::Readable(reinterpret_cast<void*>(component+LabelTextOffset),24))continue;
        ++labelTextProperties;
        const FString* display=getDisplayString(reinterpret_cast<const void*>(component+LabelTextOffset));
        if(!display||!Mem::Readable(display,sizeof(FString)))continue;
        std::string text=g_Engine->ReadFString(reinterpret_cast<uintptr_t>(display));
        if(text.empty()||text.size()>128)continue;
        ++labelReadableTexts;
        uintptr_t actor=Mem::ReadPtr(component+Offsets::UObject_Outer);
        if(!actor||!Mem::Readable(reinterpret_cast<void*>(actor+Offsets::Actor_RootComponent),sizeof(uintptr_t)))continue;
        ++labelActorOwners;
        uintptr_t root=Mem::ReadPtr(actor+Offsets::Actor_RootComponent);
        if(!root||!Mem::Readable(reinterpret_cast<void*>(root+Offsets::SceneComp_RelativeLocation),sizeof(FVector3d)))continue;
        FVector3d location{};std::memcpy(&location,reinterpret_cast<void*>(root+Offsets::SceneComp_RelativeLocation),sizeof(location));
        double x=0,y=0;if(!g_Engine->ProjectWorldToMap(location,x,y))continue;
        ++labelValidAnchors;
        double rotation=0;int fontSize=0;
        rotation=Mem::ReadFloat(component+LabelRotationOffset);
        float nativeFontSize=Mem::ReadFloat(component+LabelFontSizeOffset);
        if(std::isfinite(nativeFontSize)&&nativeFontSize>=0&&nativeFontSize<=512)
            fontSize=static_cast<int>(std::lround(nativeFontSize));
        std::string key=text+":"+std::to_string(static_cast<int>(x*100000))+":"+
                        std::to_string(static_cast<int>(y*100000));
        if(!seen.insert(key).second)continue;
        std::uint64_t hash=1469598103934665603ULL;
        for(unsigned char c:key){hash^=c;hash*=1099511628211ULL;}
        char id[17]{};std::snprintf(id,sizeof(id),"%016llx",static_cast<unsigned long long>(hash));
        MapLabelInfo label{id,std::move(text),x,y,rotation,fontSize};
        label.letterR=Mem::ReadFloat(component+LetterTintOffset);
        label.letterG=Mem::ReadFloat(component+LetterTintOffset+4);
        label.letterB=Mem::ReadFloat(component+LetterTintOffset+8);
        label.letterA=Mem::ReadFloat(component+LetterTintOffset+12);
        label.letterTintOpacity=Mem::ReadFloat(component+LetterTintOpacityOffset);
        label.letterOpacity=Mem::ReadFloat(component+LetterOpacityOffset);
        label.letterContrast=Mem::ReadFloat(component+LetterContrastOffset);
        label.glowR=Mem::ReadFloat(component+GlowTintOffset);
        label.glowG=Mem::ReadFloat(component+GlowTintOffset+4);
        label.glowB=Mem::ReadFloat(component+GlowTintOffset+8);
        label.glowA=Mem::ReadFloat(component+GlowTintOffset+12);
        label.glowTintOpacity=Mem::ReadFloat(component+GlowTintOpacityOffset);
        label.glowOpacity=Mem::ReadFloat(component+GlowOpacityOffset);
        label.glowRange=Mem::ReadFloat(component+GlowRangeOffset);
        label.glowBrightness=Mem::ReadFloat(component+GlowBrightnessOffset);
        label.volumeToFlatness=Mem::ReadFloat(component+VolumeToFlatnessOffset);
        label.regularToBold=Mem::ReadFloat(component+RegularToBoldOffset);
        out.push_back(std::move(label));
    }
    return !out.empty();
}

MapLabelDiagnostics GetMapLabelDiagnostics(){
    return {labelComponents.load(),labelTextProperties.load(),labelReadableTexts.load(),
            labelActorOwners.load(),labelValidAnchors.load()};
}

std::string Engine::ResolvePlayerCharacterGuid(uintptr_t actor,const std::vector<PlayerInfo>& players)const{
    if(!actor)return {};
    auto direct=[&](uintptr_t candidate){
        for(const auto& player:players)if(!player.characterGuid.empty()&&
            (candidate==player.pawnPtr||candidate==player.controllerPtr||candidate==player.playerStatePtr))
            return player.characterGuid;
        return std::string{};
    };
    std::string guid=direct(actor);if(!guid.empty())return guid;
    for(int depth=0;actor&&depth<6;++depth){
        if(!Mem::Readable(reinterpret_cast<void*>(actor),Offsets::Actor_Owner+sizeof(uintptr_t)))break;
        uintptr_t next=Mem::ReadPtr(actor+Offsets::Actor_Owner);
        if(!next||next==actor)next=Mem::ReadPtr(actor+Offsets::UObject_Outer);
        actor=next;guid=direct(actor);if(!guid.empty())return guid;
    }
    return {};
}

std::string Engine::GetEnemyDisplayName(uintptr_t actor)const{
    using GetDisplayStringFn=const FString* (*)(const void*);
    if(actor&&moduleBase&&Mem::Readable(reinterpret_cast<void*>(actor+0xB10),16)&&
       Mem::ReadPtr(actor+0xB10)){
        auto getDisplayString=reinterpret_cast<GetDisplayStringFn>(
            moduleBase+activeBuild.fTextDisplayString);
        const FString* display=getDisplayString(reinterpret_cast<const void*>(actor+0xB10));
        if(display&&Mem::Readable(display,sizeof(FString))){
            std::string value=ReadFString(reinterpret_cast<uintptr_t>(display));
            if(!value.empty())return value;
        }
    }
    std::string value=GetObjectClassName(actor);
    if(value.rfind("Default__",0)==0)value.erase(0,9);
    if(value.size()>2&&value.compare(value.size()-2,2,"_C")==0)value.erase(value.size()-2);
    if(value.rfind("BP_",0)==0)value.erase(0,3);
    std::replace(value.begin(),value.end(),'_',' ');
    return value.empty()?"Unknown enemy":value;
}

std::string Engine::GetMapRegionName(uintptr_t context,const FVector3d& location)const{
    if(!moduleBase||!context||!activeBuild.mapRegionAtLocation){regionLookupState=-1;return {};}
    using LookupFn=uintptr_t (*)(const void*,const FVector3d&);
    auto lookup=reinterpret_cast<LookupFn>(moduleBase+activeBuild.mapRegionAtLocation);
    uintptr_t region=lookup(reinterpret_cast<const void*>(context),location);
    if(!region)return {};
    if(!Mem::Readable(reinterpret_cast<void*>(region),0x200)||!IsA(region,"MapRegion")){
        regionLookupState=-2;return {};
    }
    uintptr_t offset=activeBuild.mapRegionNameOffset;
    if(offset>0x2000||!Mem::Readable(reinterpret_cast<void*>(region+offset),16)){
        regionLookupState=-3;return {};
    }
    using GetDisplayStringFn=const FString* (*)(const void*);
    auto getDisplayString=reinterpret_cast<GetDisplayStringFn>(moduleBase+activeBuild.fTextDisplayString);
    const FString* display=getDisplayString(reinterpret_cast<const void*>(region+offset));
    if(!display||!Mem::Readable(display,sizeof(FString))){regionLookupState=-4;return {};}
    std::string value=ReadFString(reinterpret_cast<uintptr_t>(display));
    if(!value.empty())regionLookupState=1;
    return value;
}

uintptr_t Engine::GetDungeonAtLocation(uintptr_t context,const FVector3d& location)const{
    if(!moduleBase||!context||!activeBuild.dungeonAtLocation)return 0;
    // The verified native lookup scans AGameplayObjectRegistry::DungeonData at
    // +0x320. Each 0xC0-byte FDungeonData begins with an FBox (min/max vectors).
    // Read the registry data directly so background status polling never calls
    // game-thread-only subsystem code.
    uintptr_t registry=cachedDungeonRegistry.load(std::memory_order_acquire);
    if(registry&&(!IsObjectLive(registry)||!IsA(registry,"GameplayObjectRegistry"))) {
        cachedDungeonRegistry.store(0,std::memory_order_release);registry=0;
    }
    if(!registry){
        int32_t objectCount=GetObjectCount();
        for(int32_t i=0;i<objectCount;++i){
            uintptr_t candidate=GetObjectByIndex(i);
            if(!candidate||!IsObjectLive(candidate)||GetObjectName(candidate).rfind("Default__",0)==0)continue;
            if(IsA(candidate,"GameplayObjectRegistry")){registry=candidate;break;}
        }
        if(registry)cachedDungeonRegistry.store(registry,std::memory_order_release);
    }
    if(!registry||!Mem::Readable(reinterpret_cast<void*>(registry+0x320),sizeof(FUEArray)))return 0;
    FUEArray entries{};std::memcpy(&entries,reinterpret_cast<void*>(registry+0x320),sizeof(entries));
    constexpr uintptr_t EntrySize=0xC0;
    if(!entries.Data||entries.Num<=0||entries.Num>4096||
       !Mem::Readable(entries.Data,EntrySize*static_cast<size_t>(entries.Num)))return 0;
    for(int32_t i=0;i<entries.Num;++i){
        uintptr_t entry=reinterpret_cast<uintptr_t>(entries.Data)+EntrySize*static_cast<uintptr_t>(i);
        double bounds[6]{};std::memcpy(bounds,reinterpret_cast<void*>(entry),sizeof(bounds));
        if(location.X>bounds[0]&&location.Y>bounds[1]&&location.Z>bounds[2]&&
           location.X<bounds[3]&&location.Y<bounds[4]&&location.Z<bounds[5])return entry;
    }
    return 0;
}

std::string Engine::GetDungeonName(uintptr_t dungeon)const{
    if(!dungeon||!activeBuild.dungeonNameOffset)return {};
    uintptr_t text=dungeon+activeBuild.dungeonNameOffset;
    if(!Mem::Readable(reinterpret_cast<void*>(text),24))return {};
    using GetDisplayStringFn=const FString* (*)(const void*);
    auto getDisplayString=reinterpret_cast<GetDisplayStringFn>(moduleBase+activeBuild.fTextDisplayString);
    const FString* display=getDisplayString(reinterpret_cast<const void*>(text));
    if(!display||!Mem::Readable(display,sizeof(FString)))return {};
    return ReadFString(reinterpret_cast<uintptr_t>(display));
}

std::string Engine::GetDungeonRegionName(uintptr_t context,uintptr_t dungeon)const{
    if(!context||!dungeon||!activeBuild.dungeonEntranceOffset)return {};
    uintptr_t address=dungeon+activeBuild.dungeonEntranceOffset;
    if(!Mem::Readable(reinterpret_cast<void*>(address),sizeof(FVector3d)))return {};
    FVector3d entrance{};std::memcpy(&entrance,reinterpret_cast<void*>(address),sizeof(entrance));
    return GetMapRegionName(context,entrance);
}

bool Engine::GetDungeonEntrance(uintptr_t dungeon,FVector3d& entrance)const{
    if(!dungeon||!activeBuild.dungeonEntranceOffset)return false;
    uintptr_t address=dungeon+activeBuild.dungeonEntranceOffset;
    if(!Mem::Readable(reinterpret_cast<void*>(address),sizeof(entrance)))return false;
    std::memcpy(&entrance,reinterpret_cast<void*>(address),sizeof(entrance));
    return std::isfinite(entrance.X)&&std::isfinite(entrance.Y)&&std::isfinite(entrance.Z);
}

bool Engine::ProjectWorldToMap(const FVector3d& location,double& x,double& y)const{
    if(activeBuild.mapWorldSize<=0||activeBuild.mapAssetVersion.empty())return false;
    x=(location.X-activeBuild.mapMinX)/activeBuild.mapWorldSize;
    y=(location.Y-activeBuild.mapMinY)/activeBuild.mapWorldSize;
    return std::isfinite(x)&&std::isfinite(y)&&x>=0&&x<=1&&y>=0&&y<=1;
}

bool Engine::IsMapProjectionAvailable()const{
    return activeBuild.mapWorldSize>0&&!activeBuild.mapAssetVersion.empty()&&activeBuild.mapAssetVersion!="-";
}

std::string Engine::GetMapAssetVersion()const{return activeBuild.mapAssetVersion;}

}
