#include "dom_engine.h"
#include "../utils/logger.h"
#include "../utils/memory.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <fstream>
#include <memory>
#include <pthread.h>
#include <sstream>
#include <unordered_map>

namespace DomEngine { namespace {
struct CatalogEntry { std::string id,kind,internal; uintptr_t object=0; };
struct Target { std::string guid; uintptr_t controller=0,pawn=0,state=0,progress=0; };
struct Scan { Target target; uintptr_t progress=0; size_t cursor=0; std::vector<std::string> unlocked; };
struct CachedProgress { uintptr_t controller=0,progress=0; };
using ProcessEventFn=void(*)(void*,void*,void*);
std::vector<CatalogEntry> catalog;
std::vector<Target> targets;
std::unordered_map<std::string,PlayerRecipeSnapshot> snapshots;
std::unordered_map<std::string,CachedProgress> progressCache;
pthread_mutex_t targetMutex=PTHREAD_MUTEX_INITIALIZER,snapshotMutex=PTHREAD_MUTEX_INITIALIZER;
struct Lock{pthread_mutex_t*value;explicit Lock(pthread_mutex_t&v):value(&v){pthread_mutex_lock(value);}~Lock(){pthread_mutex_unlock(value);}};
std::unique_ptr<Scan> active;
ProcessEventFn processEvent=nullptr;
uintptr_t recipeQuery=0,buildingQuery=0;
std::string fingerprint;
std::atomic<int> compatibility{-7},catalogCompatibility{0},layoutCompatibility{0},queryCompatibility{0};
std::atomic<std::uint64_t> resolved{0},completed{0},failures{0};
std::atomic<std::uint64_t> maximumProgressLookupUs{0},maximumSingleQueryUs{0},slowQueries{0};
std::atomic<std::int64_t> lastScan{0};
std::chrono::steady_clock::time_point nextCycle{};
std::chrono::steady_clock::time_point nextResolve{};
std::chrono::steady_clock::time_point nextQuery{};
constexpr int QueryIntervalMilliseconds=0;

void RecordMaximum(std::atomic<std::uint64_t>&maximum,std::uint64_t value){
    auto prior=maximum.load();while(value>prior&&!maximum.compare_exchange_weak(prior,value)){}
}

bool ValidateQueryLayout(uintptr_t function,const char* inputName){
    int input=-1,result=-1;for(const auto& field:g_Engine->GetStructPropertyOffsets(function)){
        if(field.first==inputName)input=field.second;else if(field.first=="ReturnValue")result=field.second;
    }
    // Native UFunction parameters are not exposed through ChildProperties in
    // the stripped Linux server build. When either field is exposed, require
    // the complete extracted layout. When neither is enumerable, the exact
    // build fingerprint, exact UFunction names, runtime object types, and the
    // first complete non-empty scan form the compatibility proof.
    if(input==0&&result==8)return true;
    if(input<0&&result<0){LogMessage(std::string("Player recipe query fields stripped for ")+inputName+"; using fingerprint-bound ABI and behavioral scan validation");return true;}
    return false;
}

std::vector<std::string> Split(const std::string& value,char separator){std::vector<std::string> out;size_t start=0;while(true){size_t at=value.find(separator,start);out.push_back(value.substr(start,at==std::string::npos?at:at-start));if(at==std::string::npos)break;start=at+1;}return out;}
std::int64_t Now(){return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();}
uintptr_t FindProgress(const Target&t){
    if(!g_Engine||!t.controller||!g_Engine->IsObjectLive(t.controller))return 0;
    auto cached=progressCache.find(t.guid);
    if(cached!=progressCache.end()&&cached->second.controller==t.controller&&
       g_Engine->IsObjectLive(cached->second.progress)&&g_Engine->IsA(cached->second.progress,"ProgressComponent"))
        return cached->second.progress;
    int32_t offset=g_Engine->FindPropertyOffset(t.controller,"BP_Components_Progress");
    if(offset>=0&&offset<=0x3000){uintptr_t value=Mem::ReadPtr(t.controller+uintptr_t(offset));if(value&&g_Engine->IsObjectLive(value)&&g_Engine->IsA(value,"ProgressComponent")){progressCache[t.guid]={t.controller,value};return value;}}
    for(int32_t i=0;i<g_Engine->GetObjectCount();++i){
        uintptr_t component=g_Engine->GetObjectByIndex(i);if(!component||!g_Engine->IsObjectLive(component)||!g_Engine->IsA(component,"ProgressComponent"))continue;
        uintptr_t outer=component;for(int depth=0;depth<6&&outer;++depth){outer=Mem::ReadPtr(outer+Offsets::UObject_Outer);if(outer&&(outer==t.controller||outer==t.pawn||outer==t.state)){progressCache[t.guid]={t.controller,component};return component;}}
    }
    return 0;
}
bool Query(uintptr_t progress,const CatalogEntry&entry){alignas(8) struct { void* data; std::uint8_t result; std::uint8_t padding[7]; } params{};params.data=reinterpret_cast<void*>(entry.object);processEvent(reinterpret_cast<void*>(progress),reinterpret_cast<void*>(entry.kind=="building"?buildingQuery:recipeQuery),&params);return params.result!=0;}
bool TargetStillOnline(const Target&t){Lock lock(targetMutex);return std::any_of(targets.begin(),targets.end(),[&](const Target&v){return v.guid==t.guid&&v.controller==t.controller;});}
bool ResolveCatalogObjects(){
    std::unordered_map<std::string,uintptr_t> recipes,buildings;
    for(int32_t i=0;i<g_Engine->GetObjectCount();++i){uintptr_t object=g_Engine->GetObjectByIndex(i);if(!object||!g_Engine->IsObjectLive(object))continue;if(g_Engine->IsA(object,"RecipeData"))recipes.emplace(g_Engine->GetObjectName(object),object);else if(g_Engine->IsA(object,"BuildingPieceData"))buildings.emplace(g_Engine->GetObjectName(object),object);}
    std::uint64_t count=0;for(auto&entry:catalog){const auto&source=entry.kind=="building"?buildings:recipes;auto found=source.find(entry.internal);entry.object=found==source.end()?0:found->second;if(entry.object)++count;}resolved=count;
    if(count==catalog.size()){catalogCompatibility=1;layoutCompatibility=1;compatibility=1;nextCycle=std::chrono::steady_clock::now();LogMessage("Player recipe catalog resolved all "+std::to_string(count)+" runtime objects");return true;}
    catalogCompatibility=0;compatibility=0;LogMessage("Player recipe catalog pending: resolved "+std::to_string(count)+" of "+std::to_string(catalog.size())+" runtime objects (loaded recipes="+std::to_string(recipes.size())+", buildings="+std::to_string(buildings.size())+")");return false;
}
}

bool InitializePlayerRecipeTracking(const std::string& path,uintptr_t moduleBase,uintptr_t moduleEnd){
    // The engine tick hook is installed before this worker-side initialization
    // completes. Keep tick processing fail-closed so it cannot race initialization
    // and repeat the full catalog object scan on the game thread.
    compatibility=-7;
    catalog.clear();fingerprint.clear();std::ifstream in(path);if(!in){compatibility=-1;catalogCompatibility=-1;LogMessage("Player recipe runtime catalog is missing: "+path);return false;}std::string line;size_t expected=0;while(std::getline(in,line)){if(!line.empty()&&line.back()=='\r')line.pop_back();if(line.rfind("#",0)==0){auto at=line.find("fingerprint=");if(at!=std::string::npos){auto end=line.find(' ',at);fingerprint=line.substr(at+12,end==std::string::npos?end:end-at-12);}at=line.find("count=");if(at!=std::string::npos)try{expected=std::stoull(line.substr(at+6));}catch(...){}continue;}auto fields=Split(line,'\t');if(fields.size()!=3||fields[0].size()!=16||(fields[1]!="crafting"&&fields[1]!="building")||fields[2].empty())continue;catalog.push_back({fields[0],fields[1],fields[2],0});}
    if(catalog.empty()||catalog.size()!=expected||fingerprint.size()!=64){compatibility=-2;catalogCompatibility=-1;return false;}catalogCompatibility=1;
    uintptr_t processOffset=0,a=0,b=0,c=0,d=0;if(!GetAdminTeleportOffsets(a,b,processOffset,c,d)||moduleBase+processOffset>=moduleEnd){compatibility=-3;queryCompatibility=-1;return false;}processEvent=reinterpret_cast<ProcessEventFn>(moduleBase+processOffset);recipeQuery=g_Engine->FindFunction("ProgressComponent","IsRecipeUnlocked");buildingQuery=g_Engine->FindFunction("ProgressComponent","IsBuildingUnlocked");if(!recipeQuery||!buildingQuery){compatibility=-4;queryCompatibility=-1;return false;}if(!ValidateQueryLayout(recipeQuery,"RecipeData")||!ValidateQueryLayout(buildingQuery,"BuildingData")){compatibility=-5;queryCompatibility=-1;layoutCompatibility=-1;LogMessage("Player recipe query parameter layout changed; tracking disabled");return false;}queryCompatibility=1;
    nextResolve=std::chrono::steady_clock::now();ResolveCatalogObjects();LogMessage(compatibility.load()==1?"Player recipe tracking initialized with "+std::to_string(catalog.size())+" recipes":"Player recipe tracking waiting for runtime recipe objects");return true;
}
void UpdatePlayerRecipeTargets(const std::vector<PlayerInfo>& players){std::vector<Target> fresh;for(const auto&p:players)if(!p.characterGuid.empty()&&p.controllerPtr){Target target{p.characterGuid,p.controllerPtr,p.pawnPtr,p.playerStatePtr,0};const auto started=std::chrono::steady_clock::now();target.progress=FindProgress(target);RecordMaximum(maximumProgressLookupUs,static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-started).count()));fresh.push_back(std::move(target));}Lock lock(targetMutex);targets=std::move(fresh);}
void RefreshPlayerRecipeTrackingCompatibility(){if(!g_Engine||compatibility.load()<0||compatibility.load()==1)return;const auto now=std::chrono::steady_clock::now();if(now<nextResolve)return;ResolveCatalogObjects();nextResolve=now+std::chrono::seconds(10);}
void ProcessPlayerRecipeTracking(){
    if(!g_Engine||compatibility.load()!=1)return;
    const auto now=std::chrono::steady_clock::now();
    if(active&&!TargetStillOnline(active->target))active.reset();
    if(!active){if(now<nextCycle)return;std::vector<Target> current;{Lock lock(targetMutex);current=targets;}if(current.empty()){nextCycle=now+std::chrono::seconds(2);return;}static size_t targetCursor=0;if(targetCursor>=current.size())targetCursor=0;Target target=current[targetCursor++];if(!target.progress||!g_Engine->IsObjectLive(target.progress)||!g_Engine->IsA(target.progress,"ProgressComponent")){layoutCompatibility=0;++failures;nextCycle=now+std::chrono::seconds(1);return;}layoutCompatibility=1;active=std::make_unique<Scan>();active->target=target;active->progress=target.progress;active->unlocked.reserve(catalog.size());nextQuery=now;}
    if(!active||!g_Engine->IsObjectLive(active->progress)){active.reset();++failures;return;}
    if(active->cursor<catalog.size()){
        if(now<nextQuery)return;const auto batchStarted=std::chrono::steady_clock::now();int queries=0;
        while(active&&active->cursor<catalog.size()&&queries<8){const auto&entry=catalog[active->cursor];if(!entry.object||!g_Engine->IsObjectLive(entry.object)){active.reset();++failures;compatibility=-6;return;}const auto queryStarted=std::chrono::steady_clock::now();const bool unlocked=Query(active->progress,entry);const auto queryUs=static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-queryStarted).count());RecordMaximum(maximumSingleQueryUs,queryUs);if(queryUs>=1000)++slowQueries;if(unlocked)active->unlocked.push_back(entry.id);++active->cursor;++queries;if(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-batchStarted).count()>=250)break;}
    }
    if(active&&active->cursor==catalog.size()){std::sort(active->unlocked.begin(),active->unlocked.end());if(active->unlocked.empty()){++failures;active.reset();nextCycle=now+std::chrono::seconds(2);return;}PlayerRecipeSnapshot snapshot{active->target.guid,active->unlocked,fingerprint,Now(),true};{Lock lock(snapshotMutex);auto prior=snapshots.find(snapshot.characterGuid);if(prior==snapshots.end()||prior->second.catalogFingerprint!=snapshot.catalogFingerprint||snapshot.unlockedIds.size()>=prior->second.unlockedIds.size()||!std::includes(prior->second.unlockedIds.begin(),prior->second.unlockedIds.end(),snapshot.unlockedIds.begin(),snapshot.unlockedIds.end()))snapshots[snapshot.characterGuid]=std::move(snapshot);}lastScan=Now();++completed;active.reset();nextCycle=now+std::chrono::seconds(2);}
}
std::vector<PlayerRecipeSnapshot> ReadPlayerRecipeSnapshots(){Lock lock(snapshotMutex);std::vector<PlayerRecipeSnapshot> out;out.reserve(snapshots.size());for(const auto&pair:snapshots)out.push_back(pair.second);return out;}
PlayerRecipeDiagnostics GetPlayerRecipeDiagnostics(){PlayerRecipeDiagnostics d;d.compatibility=compatibility;d.catalogCompatibility=catalogCompatibility;d.progressLayoutCompatibility=layoutCompatibility;d.queryCompatibility=queryCompatibility;d.catalogEntries=catalog.size();d.resolvedEntries=resolved;{Lock lock(snapshotMutex);d.snapshots=snapshots.size();}d.scansCompleted=completed;d.queryFailures=failures;d.lastScan=lastScan;d.maximumProgressLookupMicroseconds=maximumProgressLookupUs;d.maximumSingleQueryMicroseconds=maximumSingleQueryUs;d.slowQueries=slowQueries;d.queryIntervalMilliseconds=QueryIntervalMilliseconds;return d;}
std::string GetPlayerRecipeCatalogFingerprint(){return compatibility.load()==1?fingerprint:std::string();}
}
