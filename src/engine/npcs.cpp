#include "dom_engine.h"
#include "../utils/memory.h"
#include "../utils/logger.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <pthread.h>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace DomEngine { namespace {
struct NpcIdentity {
    std::string stateKey,id,type,spawnType,aiClass,nativeGuid;
    double respawn=0;
    FVector3d spawn{};
};
struct Death { std::chrono::steady_clock::time_point at; double gameSeconds=0; uintptr_t victim=0;std::uint64_t victimIdentity=0;std::int64_t gameDeadline=0; };
struct Candidate { uintptr_t actor=0;size_t node=0;double distance2=0;FVector3d location{}; };
std::vector<NpcIdentity> catalog;
std::unordered_map<std::string,size_t> byStateKey;
std::unordered_map<std::string,size_t> byNativeGuid;
std::unordered_map<std::string,std::vector<size_t>> byActorClass;
std::unordered_map<uintptr_t,size_t> actorToNode;
std::unordered_map<std::string,Death> deaths;
std::vector<NpcStateInfo> states;
pthread_mutex_t npcMutex=PTHREAD_MUTEX_INITIALIZER;
std::atomic<int> enabled{0},catalogCompat{0},registryCompat{0},identityCompat{0},transformCompat{0};
std::atomic<int> lifecycleCompat{0},respawnCompat{0},dungeonCompat{0},loadedActors{0},matchedLive{0},deadCount{0},readyCount{0},inactiveCount{0},unloadedCount{0},loadedSpawnPointCount{0};
std::atomic<int> authoritativeTimerCompat{0},authoritativeTimerCount{0};
std::atomic<std::uint64_t> ambiguous{0},rejected{0};
std::atomic<std::int64_t> lastSample{0};
std::atomic<int> scanActive{0},scanProgress{0},scanTotal{0};
std::atomic<std::uint64_t> completedScans{0},lastScanMs{0},maxSliceUs{0},slowSliceCount{0};
std::atomic<std::uint64_t> lastGenerationWorkUs{0},lastFinalizeUs{0},maxFinalizeUs{0};
std::atomic<std::uint64_t> lastLiveRefreshUs{0},maxLiveRefreshUs{0};
std::atomic<std::uint64_t> classCacheHits{0},classCacheMisses{0},invalidObjectSlots{0},changedObjectSlots{0};
std::atomic<int> lastDutyBasisPoints{0};
double currentRate=0;
int scanCursor=0;
std::vector<Candidate> scanCandidates;
std::unordered_set<size_t> scanLoadedSpawnPoints;
std::unordered_map<uintptr_t,std::uint8_t> classKinds;
int scanSeen=0;
std::uint64_t scanWorkUs=0;
std::chrono::steady_clock::time_point scanStarted{},nextScan{};
std::chrono::steady_clock::time_point nextLiveRefresh{};
constexpr int ScanIdleMilliseconds=500;
uintptr_t scanAiDirector=0;
enum ClassKind : std::uint8_t { ClassNone=0,ClassAi=1,ClassSpawn=2,ClassCharacter=4,ClassDirector=8 };

std::string IdentityPath(){const char* p=std::getenv("DW_NPC_IDENTITIES");return p&&*p?p:"/home/container/zanik/npc-identities.tsv";}
std::string StripClass(std::string s){
    if(s.size()>2&&s.compare(s.size()-2,2,"_C")==0)s.resize(s.size()-2);
    return s;
}
std::string NormalizeGuid(std::string value){
    value.erase(std::remove(value.begin(),value.end(),'-'),value.end());
    std::transform(value.begin(),value.end(),value.begin(),[](unsigned char c){return static_cast<char>(std::toupper(c));});
    if(value.size()!=32||!std::all_of(value.begin(),value.end(),[](unsigned char c){return std::isxdigit(c)!=0;}))return {};
    return value;
}
bool ReadLocation(uintptr_t actor,FVector3d& out){
    if(!actor||!Mem::Readable(reinterpret_cast<void*>(actor+Offsets::Actor_RootComponent),sizeof(uintptr_t)))return false;
    uintptr_t root=Mem::ReadPtr(actor+Offsets::Actor_RootComponent);
    if(!root||!Mem::Readable(reinterpret_cast<void*>(root+Offsets::SceneComp_RelativeLocation),sizeof(out)))return false;
    std::memcpy(&out,reinterpret_cast<void*>(root+Offsets::SceneComp_RelativeLocation),sizeof(out));
    return std::isfinite(out.X)&&std::isfinite(out.Y)&&std::isfinite(out.Z)&&std::abs(out.X)<1e8&&std::abs(out.Y)<1e8;
}
std::uint8_t Classify(uintptr_t object){
    if(!object||!Mem::Readable(reinterpret_cast<void*>(object+Offsets::UObject_Class),sizeof(uintptr_t)))return ClassNone;
    const uintptr_t klass=Mem::ReadPtr(object+Offsets::UObject_Class);if(!klass)return ClassNone;
    auto found=classKinds.find(klass);
    if(found!=classKinds.end()){++classCacheHits;return found->second;}
    ++classCacheMisses;std::uint8_t kind=ClassNone;
    if(g_Engine->IsA(object,"DominionAICharacter"))kind|=ClassAi|ClassCharacter;
    else{
        if(g_Engine->IsA(object,"AISpawnPoint"))kind|=ClassSpawn;
        if(g_Engine->IsA(object,"Character"))kind|=ClassCharacter;
        if(g_Engine->IsA(object,"AiDirector"))kind|=ClassDirector;
    }
    classKinds.emplace(klass,kind);return kind;
}
bool LoadCatalog(){
    std::ifstream in(IdentityPath());if(!in)return false;
    std::string line;if(!std::getline(in,line))return false;if(!line.empty()&&line.back()=='\r')line.pop_back();
    std::istringstream header(line);std::string marker,schema,countText,catalogFingerprint;
    if(!std::getline(header,marker,'\t')||marker!="# zanik-npc-identities"||!std::getline(header,schema,'\t')||(schema!="1"&&schema!="2")||!std::getline(header,countText,'\t')||!std::getline(header,catalogFingerprint,'\t')||catalogFingerprint.size()!=64)return false;
    size_t expected=0;try{expected=std::stoull(countText);}catch(...){return false;}if(!expected||expected>10000)return false;
    std::unordered_set<std::string> ids;
    while(std::getline(in,line)){
        std::istringstream row(line);NpcIdentity n;std::string duration,x,y,z,source;
        if(!std::getline(row,n.stateKey,'\t')||!std::getline(row,n.id,'\t')||!std::getline(row,n.type,'\t')||
           !std::getline(row,n.spawnType,'\t')||!std::getline(row,n.aiClass,'\t')||!std::getline(row,duration,'\t')||
           !std::getline(row,x,'\t')||!std::getline(row,y,'\t')||!std::getline(row,z,'\t'))continue;
        if(schema=="2"){if(!std::getline(row,n.nativeGuid,'\t'))continue;n.nativeGuid=NormalizeGuid(n.nativeGuid);}
        try{n.respawn=std::stod(duration);n.spawn={std::stod(x),std::stod(y),std::stod(z)};}catch(...){++rejected;continue;}
        if(n.id.size()!=20||n.stateKey.empty()||n.aiClass.empty()||!ids.insert(n.id).second||byStateKey.count(n.stateKey)){++rejected;continue;}
        n.aiClass=StripClass(n.aiClass);if(!n.nativeGuid.empty()&&byNativeGuid.count(n.nativeGuid)){++rejected;continue;}
        byStateKey[n.stateKey]=catalog.size();byActorClass[n.aiClass].push_back(catalog.size());if(!n.nativeGuid.empty())byNativeGuid[n.nativeGuid]=catalog.size();catalog.push_back(std::move(n));
    }
    return catalog.size()==expected;
}

bool ReadAuthoritativeRespawnTimers(uintptr_t director,std::unordered_map<size_t,std::int64_t>& out){
    out.clear();if(!director||!g_Engine->IsObjectLive(director)||!g_Engine->IsA(director,"AiDirector")||byNativeGuid.empty())return false;
    const int32_t offset=g_Engine->FindPropertyOffset(director,"RespawnTimerSpawnPointIds");
    if(offset<0||offset>0x2000)return false;
    // Current-build native layout: TMap's sparse element is 40 bytes (FGuid
    // key at +0, FAIRespawnTimer value at +16, then set bookkeeping). Read the
    // allocation bitmap as well as the array bounds so free sparse slots can
    // never be mistaken for live timer entries.
    struct Header{uintptr_t data;int32_t num,max;uintptr_t allocation;int32_t allocationNum,allocationMax;
        int32_t firstFree,numFree;uintptr_t hash;int32_t hashSize,padding;};
    static_assert(sizeof(Header)==56,"unexpected TMap header layout");
    Header before{},after{};
    if(!Mem::Read(director+uintptr_t(offset),before)||before.num<0||before.max<before.num||before.max>10000||
       before.allocationNum!=before.num||before.allocationMax<before.allocationNum||before.numFree<0||before.numFree>before.num||
       (before.num&&(!before.data||!before.allocation||
        !Mem::Readable(reinterpret_cast<void*>(before.data),size_t(before.num)*40)||
        !Mem::Readable(reinterpret_cast<void*>(before.allocation),size_t((before.num+31)/32)*sizeof(std::uint32_t)))))return false;
    for(int32_t i=0;i<before.num;++i){
        std::uint32_t bits=0;if(!Mem::Read(before.allocation+uintptr_t(i/32)*sizeof(bits),bits))return false;
        if(!(bits&(std::uint32_t(1)<<unsigned(i%32))))continue;
        const uintptr_t entry=before.data+uintptr_t(i)*40;std::string guid=g_Engine->ReadGuid(entry);
        auto known=byNativeGuid.find(guid);if(known==byNativeGuid.end())continue;
        std::int64_t deadline=0;if(!Mem::Read(entry+16,deadline)||deadline<=0||!out.emplace(known->second,deadline).second)return false;
    }
    if(!Mem::Read(director+uintptr_t(offset),after)||std::memcmp(&before,&after,sizeof(before))!=0||
       out.size()>size_t(before.num-before.numFree))return false;
    return true;
}

bool ValidateTransformLayout(){
    int readableActors=0;
    for(int32_t i=0;i<g_Engine->GetObjectCount();++i){
        uintptr_t actor=g_Engine->GetObjectByIndex(i);if(!actor||!g_Engine->IsObjectLive(actor)||!g_Engine->IsA(actor,"Actor"))continue;
        FVector3d location{};if(!ReadLocation(actor,location))continue;
        ++readableActors;if(readableActors>=16)return true;
    }
    return readableActors>0;
}

void RefreshCachedLiveActors(){
    auto now=std::chrono::steady_clock::now();if(now<nextLiveRefresh)return;
    nextLiveRefresh=now+std::chrono::seconds(2);
    std::vector<std::pair<uintptr_t,size_t>> cached;
    pthread_mutex_lock(&npcMutex);cached.assign(actorToNode.begin(),actorToNode.end());pthread_mutex_unlock(&npcMutex);
    struct Update{size_t node;bool live;double x,y;};std::vector<Update> updates;updates.reserve(cached.size());
    auto started=std::chrono::steady_clock::now();
    for(const auto& entry:cached){
        if(updates.size()>=128||std::chrono::steady_clock::now()-started>=std::chrono::microseconds(400))break;
        FVector3d loc{};double x=0,y=0;bool live=g_Engine->IsObjectLive(entry.first)&&ReadLocation(entry.first,loc)&&g_Engine->ProjectWorldToMap(loc,x,y);
        updates.push_back({entry.second,live,x,y});
    }
    pthread_mutex_lock(&npcMutex);
    for(const auto& update:updates){
        if(update.node>=states.size()||deaths.count(catalog[update.node].id))continue;
        auto& state=states[update.node];
        if(update.live){state.state="live";state.hasMapLocation=true;state.mapX=update.x;state.mapY=update.y;}
        else{state.state="unloaded";state.hasMapLocation=false;}
    }
    pthread_mutex_unlock(&npcMutex);
    const auto elapsed=static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now()-started).count());
    lastLiveRefreshUs=elapsed;auto prior=maxLiveRefreshUs.load();
    while(elapsed>prior&&!maxLiveRefreshUs.compare_exchange_weak(prior,elapsed)){}
}
}

bool InitializeNpcTracking(uintptr_t,uintptr_t){
    const char* toggle=std::getenv("DW_NPC_TRACKING_ENABLED");
    if(toggle&&(std::string(toggle)=="0"||std::string(toggle)=="false")){enabled=0;return false;}
    enabled=1;
    if(!g_Engine||!g_Engine->IsInitialized()||!LoadCatalog()){catalogCompat=-1;return false;}
    catalogCompat=1;identityCompat=1;transformCompat=g_Engine->IsMapProjectionAvailable()&&ValidateTransformLayout()?1:-1;
    if(transformCompat.load()!=1){catalogCompat=-1;LogMessage("NPC transform layout validation failed; NPC tracking disabled");return false;}
    lifecycleCompat=NativeKillHookState()==1?1:-1;respawnCompat=lifecycleCompat.load();dungeonCompat=1;
    LogMessage("NPC catalog loaded nodes="+std::to_string(catalog.size()));return true;
}

const char* ResolveNpcTrackedDeath(bool actorAssigned,bool replacementActor,bool timerElapsed){
    if(replacementActor||(actorAssigned&&timerElapsed))return "clear";
    return timerElapsed?"ready":"dead";
}

void RefreshNpcStates(){
    if(!enabled||catalogCompat.load()!=1||!g_Engine)return;
    RefreshCachedLiveActors();
    auto sliceStarted=std::chrono::steady_clock::now();
    if(!scanActive.load()){
        if(sliceStarted<nextScan)return;
        scanCursor=0;scanSeen=0;scanWorkUs=0;scanCandidates.clear();scanLoadedSpawnPoints.clear();scanAiDirector=0;scanTotal=std::max(0,g_Engine->GetObjectCount());
        scanProgress=0;scanStarted=sliceStarted;scanActive=1;
    }
    // This runs on the same worker as the tiny HTTP server. Keep each pass
    // comfortably below the shared one-millisecond hard ceiling; the next
    // loop resumes at scanCursor after servicing pending sockets.
    constexpr int MaxObjectsPerSlice=128;
    constexpr auto SliceBudget=std::chrono::microseconds(400);
    int processed=0;
    while(scanCursor<scanTotal.load()&&processed<MaxObjectsPerSlice&&
          std::chrono::steady_clock::now()-sliceStarted<SliceBudget){
        int i=scanCursor++;++processed;
        uintptr_t actor=g_Engine->GetObjectByIndex(i);if(!actor)continue;
        if(!g_Engine->IsObjectLive(actor)){++invalidObjectSlots;continue;}
        const std::uint64_t identity=g_Engine->GetObjectIdentity(actor);if(!identity){++invalidObjectSlots;continue;}
        const std::uint8_t kind=Classify(actor);const bool isAi=(kind&ClassAi)!=0;
        if(kind&ClassDirector){scanAiDirector=actor;continue;}
        if((kind&(ClassAi|ClassSpawn|ClassCharacter))&&g_Engine->GetObjectIdentity(actor)!=identity){++changedObjectSlots;continue;}
        if(!isAi&&(kind&ClassSpawn)){
            auto placed=byStateKey.find(g_Engine->GetObjectName(actor));
            if(placed!=byStateKey.end()&&catalog[placed->second].spawnType!=catalog[placed->second].aiClass)
                scanLoadedSpawnPoints.insert(placed->second);
            continue;
        }
        if(!isAi&&!(kind&ClassCharacter))continue;
        std::string klass=StripClass(g_Engine->GetObjectClassName(actor));
        if(!isAi){
            if(!byActorClass.count(klass))continue;
            auto direct=byStateKey.find(g_Engine->GetObjectName(actor));
            if(direct==byStateKey.end()||catalog[direct->second].spawnType!=catalog[direct->second].aiClass)continue;
            FVector3d loc{};if(ReadLocation(actor,loc)&&!g_Engine->GetDungeonAtLocation(actor,loc))
                scanCandidates.push_back({actor,direct->second,0,loc});
            continue;
        }
        ++scanSeen;FVector3d loc{};if(!ReadLocation(actor,loc)){++rejected;continue;}
        std::vector<std::pair<size_t,double>> actorCandidates;double bestD=1e100,second=1e100;
        auto classIt=byActorClass.find(klass);if(classIt==byActorClass.end()){++rejected;continue;}
        for(size_t n:classIt->second){
            double dx=loc.X-catalog[n].spawn.X,dy=loc.Y-catalog[n].spawn.Y,dz=loc.Z-catalog[n].spawn.Z;
            double d=dx*dx+dy*dy+dz*dz;
            if(d<=200000.0*200000.0)actorCandidates.push_back({n,d});
            if(d<bestD){second=bestD;bestD=d;}else if(d<second)second=d;
        }
        // A two-kilometre leash catches normal movement without assigning a
        // transient/summoned actor to an unrelated persistent placement.
        if(actorCandidates.empty()){++rejected;continue;}
        if(std::abs(second-bestD)<1.0){++ambiguous;continue;}
        for(const auto& option:actorCandidates)scanCandidates.push_back({actor,option.first,option.second,loc});
    }
    scanProgress=scanCursor;
    auto sliceUs=static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-sliceStarted).count());
    scanWorkUs+=sliceUs;
    if(sliceUs>=1000)++slowSliceCount;
    auto prior=maxSliceUs.load();while(sliceUs>prior&&!maxSliceUs.compare_exchange_weak(prior,sliceUs)){}
    if(scanCursor<scanTotal.load())return;
    const auto finalizeStarted=std::chrono::steady_clock::now();
    std::sort(scanCandidates.begin(),scanCandidates.end(),[](const auto&a,const auto&b){return a.distance2<b.distance2;});
    std::unordered_set<size_t> usedNodes;std::unordered_set<uintptr_t> usedActors;
    std::vector<NpcStateInfo> next(catalog.size());std::unordered_map<uintptr_t,size_t> nextActors;
    for(size_t n=0;n<catalog.size();++n){
        next[n].id=catalog[n].id;
        next[n].state=scanLoadedSpawnPoints.count(n)?"inactive":"unloaded";
    }
    for(const auto& c:scanCandidates){
        if(usedNodes.count(c.node)||usedActors.count(c.actor))continue;
        double x=0,y=0;if(!g_Engine->ProjectWorldToMap(c.location,x,y)){++rejected;continue;}
        usedNodes.insert(c.node);usedActors.insert(c.actor);
        next[c.node].state="live";next[c.node].hasMapLocation=true;next[c.node].mapX=x;next[c.node].mapY=y;
        nextActors[c.actor]=c.node;
    }
    std::vector<ResourceStateInfo> resourceStates;double rate=0;std::int64_t ignored=0;
    if(!ReadResourceStates(resourceStates,rate,ignored)||rate<=0)rate=0;
    auto now=std::chrono::steady_clock::now();std::int64_t gameNow=0;double authoritativeRate=0;
    const bool haveGameClock=ReadAuthoritativeGameTime(gameNow,authoritativeRate);
    std::unordered_map<size_t,std::int64_t> authoritativeTimers;
    const bool haveAuthoritativeTimers=ReadAuthoritativeRespawnTimers(scanAiDirector,authoritativeTimers);
    authoritativeTimerCompat=haveAuthoritativeTimers?1:-1;authoritativeTimerCount=static_cast<int>(authoritativeTimers.size());
    int dead=0,ready=0,inactive=0,unloaded=0;
    pthread_mutex_lock(&npcMutex);
    actorToNode.swap(nextActors);
    for(auto it=deaths.begin();it!=deaths.end();){
        auto ni=std::find_if(catalog.begin(),catalog.end(),[&](const auto& n){return n.id==it->first;});
        if(ni==catalog.end()){it=deaths.erase(it);continue;}
        size_t index=static_cast<size_t>(ni-catalog.begin());
        // A killed AI commonly remains a valid UObject/corpse for a while.
        // The game's global AiDirector keeps the spawn GUID exhausted until
        // its absolute in-game respawn deadline matures, then replenishes that
        // GUID independently of whether its World Partition cell is loaded.
        // Before that deadline only a different actor proves replacement;
        // afterwards the same reused actor may validly be the revived AI.
        uintptr_t assignedActor=0;
        for(const auto& live:actorToNode)if(live.second==index){assignedActor=live.first;break;}
        const bool replacementActor=assignedActor&&(assignedActor!=it->second.victim||
            (it->second.victimIdentity&&g_Engine->GetObjectIdentity(assignedActor)!=it->second.victimIdentity));
        if(it->second.gameSeconds<=0){it=deaths.erase(it);continue;}
        const double fallbackRate=rate>0?rate:authoritativeRate;
        const bool timerElapsed=it->second.gameDeadline>0&&haveGameClock?
            gameNow>=it->second.gameDeadline:
            fallbackRate>0&&std::chrono::duration<double>(now-it->second.at).count()*fallbackRate>=it->second.gameSeconds;
        const char* resolution=ResolveNpcTrackedDeath(assignedActor!=0,replacementActor,timerElapsed);
        if(std::strcmp(resolution,"clear")==0){it=deaths.erase(it);continue;}
        if(std::strcmp(resolution,"ready")==0){next[index].state="ready";next[index].remainingSeconds=0;++ready;++it;continue;}
        next[index].state="dead";
        if(it->second.gameDeadline>0&&haveGameClock&&authoritativeRate>0)
            next[index].remainingSeconds=std::max(0.0,(it->second.gameDeadline-gameNow)/10000000.0/authoritativeRate);
        else if(fallbackRate>0){double elapsed=std::chrono::duration<double>(now-it->second.at).count()*fallbackRate;next[index].remainingSeconds=std::max(0.0,(it->second.gameSeconds-elapsed)/fallbackRate);}
        else next[index].remainingSeconds=it->second.gameSeconds;
        ++dead;++it;
    }
    if(haveAuthoritativeTimers&&haveGameClock){
        for(const auto& timer:authoritativeTimers){
            if(timer.first>=next.size())continue;
            auto& state=next[timer.first];state.state=timer.second<=gameNow?"ready":"dead";state.hasMapLocation=false;
            if(timer.second<=gameNow){state.remainingSeconds=0;++ready;}
            else{state.remainingSeconds=authoritativeRate>0?(timer.second-gameNow)/10000000.0/authoritativeRate:0;++dead;}
        }
    }
    dead=ready=0;
    for(const auto& state:next){if(state.state=="dead")++dead;else if(state.state=="ready")++ready;}
    states.swap(next);currentRate=rate>0?rate:authoritativeRate;
    pthread_mutex_unlock(&npcMutex);
    for(const auto&s:states){if(s.state=="inactive")++inactive;else if(s.state=="unloaded")++unloaded;}
    loadedActors=scanSeen;matchedLive=static_cast<int>(usedNodes.size());deadCount=dead;readyCount=ready;inactiveCount=inactive;unloadedCount=unloaded;
    loadedSpawnPointCount=static_cast<int>(scanLoadedSpawnPoints.size());
    registryCompat=1;lastSample=static_cast<std::int64_t>(std::time(nullptr));
    lastScanMs=static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now()-scanStarted).count());
    const auto finalizeUs=static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now()-finalizeStarted).count());
    lastFinalizeUs=finalizeUs;auto maxFinalize=maxFinalizeUs.load();
    while(finalizeUs>maxFinalize&&!maxFinalizeUs.compare_exchange_weak(maxFinalize,finalizeUs)){}
    scanWorkUs+=finalizeUs;lastGenerationWorkUs=scanWorkUs;
    const auto wallUs=static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now()-scanStarted).count());
    lastDutyBasisPoints=wallUs?static_cast<int>(std::min<std::uint64_t>(10000,scanWorkUs*10000/wallUs)):0;
    // A short idle interval avoids perpetual object-array traversal while
    // keeping newly streamed NPC discovery comfortably below map refresh time.
    ++completedScans;scanActive=0;nextScan=std::chrono::steady_clock::now()+std::chrono::milliseconds(ScanIdleMilliseconds);
}

void RecordNpcDeath(uintptr_t actor){
    if(!enabled||!actor)return;
    pthread_mutex_lock(&npcMutex);auto it=actorToNode.find(actor);
    if(it!=actorToNode.end()&&catalog[it->second].respawn>0){
        const size_t node=it->second;
        std::int64_t gameTicks=0;double rate=0;ReadAuthoritativeGameTime(gameTicks,rate);
        const auto deadline=gameTicks>0?gameTicks+static_cast<std::int64_t>(std::llround(catalog[node].respawn*10000000.0)):0;
        deaths[catalog[node].id]={std::chrono::steady_clock::now(),catalog[node].respawn,actor,g_Engine->GetObjectIdentity(actor),deadline};
        if(node<states.size()){
            states[node].state="dead";
            states[node].hasMapLocation=false; // serializer/UI falls back to catalogued spawn
            states[node].remainingSeconds=currentRate>0?catalog[node].respawn/currentRate:0;
        }
        ++deadCount;
    }
    pthread_mutex_unlock(&npcMutex);
}
bool NpcScanInProgress(){return scanActive.load()!=0;}
bool ReadNpcStates(std::vector<NpcStateInfo>& out,double& rate,std::int64_t& updatedAt){
    if(!enabled||catalogCompat.load()!=1||lastSample.load()==0)return false;
    pthread_mutex_lock(&npcMutex);out=states;rate=currentRate;pthread_mutex_unlock(&npcMutex);updatedAt=lastSample.load();return true;
}
NpcDiagnostics GetNpcDiagnostics(){return {enabled.load(),catalogCompat.load(),registryCompat.load(),identityCompat.load(),
    transformCompat.load(),lifecycleCompat.load(),respawnCompat.load(),dungeonCompat.load(),static_cast<int>(catalog.size()),
    loadedActors.load(),matchedLive.load(),deadCount.load(),readyCount.load(),inactiveCount.load(),unloadedCount.load(),loadedSpawnPointCount.load(),authoritativeTimerCompat.load(),authoritativeTimerCount.load(),ambiguous.load(),rejected.load(),lastSample.load(),
    scanActive.load(),scanProgress.load(),scanTotal.load(),completedScans.load(),lastScanMs.load(),maxSliceUs.load(),slowSliceCount.load(),
    lastGenerationWorkUs.load(),lastFinalizeUs.load(),maxFinalizeUs.load(),lastLiveRefreshUs.load(),maxLiveRefreshUs.load(),
    classCacheHits.load(),classCacheMisses.load(),invalidObjectSlots.load(),changedObjectSlots.load(),
    lastDutyBasisPoints.load(),ScanIdleMilliseconds};}
}
