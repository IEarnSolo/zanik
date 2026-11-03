#include "dom_engine.h"
#include "../utils/memory.h"
#include "../utils/logger.h"
#include "../performance_trace.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cctype>
#include <cstring>
#include <fstream>
#include <iterator>
#include <memory>
#include <pthread.h>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

namespace DomEngine { namespace {
enum class EntryKind { Material, Perk, Spell };
struct Entry { EntryKind kind;uintptr_t object=0;std::string key,canonical,name,icon; };
struct Target {std::string guid,controllerName;uintptr_t controller=0,pawn=0,state=0,progress=0,perks=0;bool regionsComplete=false;std::uint32_t regionBitmap=0;std::vector<ActivityRegionInfo> regions;};
struct Scan {Target target;size_t cursor=0;bool materialsComplete=true,unlocksComplete=true;std::vector<ActivityMaterialInfo> materials;std::vector<ActivityUnlockInfo> unlocks;};
using ProcessEventFn=void(*)(void*,void*,void*);
std::atomic<int> regionCompatibility{0},materialCompatibility{0},unlockCompatibility{0};
std::atomic<std::uint64_t> snapshots{0},dropped{0};
std::atomic<std::uint64_t> materialCatalogEntries{0},resolvedMaterialEntries{0},perkCatalogEntries{0},spellCatalogEntries{0},resolvedUnlockEntries{0};
std::atomic<std::uint64_t> queryCalls{0},maximumQueryUs{0},slowQueries{0},maximumBatchUs{0};
std::atomic<std::uint64_t> regionHookCalls{0},regionHookQueueDrops{0},regionHookMaximumUs{0},regionHookSlowCalls{0};
const std::unordered_map<std::uint8_t,std::string> regionNames={
    {1,"Temple Woods"},{2,"Bramblemead Valley"},{3,"Whispering Swamp"},{4,"Fractured Plains"},
    {5,"Stormtouched Highlands"},{6,"Bloodblight Swamp"},{7,"Bleakfields Valley"},{8,"Hope's Fall"},
    {9,"Lake of Lost Souls"},{10,"Coalridge Pass"},{11,"Silverthorn Keep"},{12,"Dragons' Run"},
    {13,"Witchwillow Range"},{14,"Emberwood"},{15,"Forgotten Temple"},{16,"The Approach"},
    {17,"The Library"},{18,"The Courtyard"},{19,"The Menagerie"},{20,"The Garrison"},
    {21,"The Nexus"},{22,"The Pastures"},{23,"The Cathedral"},{24,"The Bastion"},
    {25,"The Grand Hall"},{26,"Alcarrid Oasis"},{27,"Dunes of Uzzer"},{28,"Manafem Plains"},
    {29,"The Burning Spire"},{30,"Rasthin's Rest"},{31,"Scorned Wilderness"},
};
std::vector<Entry> catalog;std::vector<Target> targets;
std::unordered_map<std::string,PlayerActivityDiscoverySnapshot> completedSnapshots;
std::unordered_map<std::string,uintptr_t> progressCache,perkCache,regionCache;
std::unordered_set<std::uint64_t> regionFallbackScannedControllers;
pthread_mutex_t targetMutex=PTHREAD_MUTEX_INITIALIZER,snapshotMutex=PTHREAD_MUTEX_INITIALIZER;
struct Lock{pthread_mutex_t*p;explicit Lock(pthread_mutex_t&v):p(&v){pthread_mutex_lock(p);}~Lock(){pthread_mutex_unlock(p);}};
std::unique_ptr<Scan> active;ProcessEventFn processEvent=nullptr;
uintptr_t hasPickedUpItem=0,isPerkUnlocked=0,getRevealedMapFogComponent=0;
std::atomic<uintptr_t> regionRevealFunction{0};
std::size_t regionRevealRegionOffset=0,regionRevealAgainOffset=8;
std::uint64_t regionResolutionControllerIdentity=0;
unsigned regionResolutionAttempts=0;
std::string regionServerLogPath;
std::streamoff regionServerLogOffset=0;
bool regionServerLogPrimed=false;
std::atomic<bool> regionServerLogReady{false};
struct RawRegionReveal{uintptr_t object=0,region=0;std::uint64_t hookMicros=0;bool again=false;};
constexpr std::size_t RegionRevealCapacity=64;
std::array<RawRegionReveal,RegionRevealCapacity> regionRevealQueue{};
std::atomic<std::uint64_t> regionRevealWrite{0},regionRevealRead{0};
std::string materialFingerprint,unlockFingerprint;
std::chrono::steady_clock::time_point nextMaterialResolve{};

uintptr_t ComponentFor(const Target&t,const char*type,const std::initializer_list<const char*>names,std::unordered_map<std::string,uintptr_t>&cache){auto known=cache.find(t.guid);if(known!=cache.end()&&g_Engine->IsObjectLive(known->second)&&g_Engine->IsA(known->second,type))return known->second;for(const char*name:names){int32_t offset=g_Engine->FindPropertyOffset(t.controller,name);if(offset<0||offset>0x3000)continue;uintptr_t value=Mem::ReadPtr(t.controller+uintptr_t(offset));if(value&&g_Engine->IsObjectLive(value)&&g_Engine->IsA(value,type)){cache[t.guid]=value;return value;}}for(int32_t i=0;i<g_Engine->GetObjectCount();++i){uintptr_t value=g_Engine->GetObjectByIndex(i);if(!value||!g_Engine->IsObjectLive(value)||!g_Engine->IsA(value,type))continue;uintptr_t outer=value;for(int depth=0;depth<7&&outer;++depth){outer=Mem::ReadPtr(outer+Offsets::UObject_Outer);if(outer&&(outer==t.controller||outer==t.pawn||outer==t.state)){cache[t.guid]=value;return value;}}}return 0;}
uintptr_t ProgressFor(const Target&t){return ComponentFor(t,"ProgressComponent",{"BP_Components_Progress","ProgressComponent"},progressCache);}
uintptr_t PerksFor(const Target&t){return ComponentFor(t,"SkillPerkComponent",{"BP_Components_SkillPerk","BP_Components_SkillPerks","SkillPerkComponent"},perkCache);}
uintptr_t RegionComponentFor(const Target&t){auto known=regionCache.find(t.guid);if(known!=regionCache.end()&&g_Engine->IsObjectLive(known->second)&&g_Engine->IsA(known->second,"RevealedMapFogComponent"))return known->second;if(getRevealedMapFogComponent&&processEvent){alignas(8)struct{void*returnValue;}params{};processEvent(reinterpret_cast<void*>(t.controller),reinterpret_cast<void*>(getRevealedMapFogComponent),&params);uintptr_t value=reinterpret_cast<uintptr_t>(params.returnValue);if(value&&g_Engine->IsObjectLive(value)&&g_Engine->IsA(value,"RevealedMapFogComponent")){regionCache[t.guid]=value;return value;}}for(const char*property:{"BP_RevealedMapFogComponent","RevealedMapFogComponent","RevealedMapFogComponentRef"}){int32_t offset=g_Engine->FindPropertyOffset(t.controller,property);if(offset<0||offset>0x3000)continue;uintptr_t value=Mem::ReadPtr(t.controller+uintptr_t(offset));if(value&&g_Engine->IsObjectLive(value)&&g_Engine->IsA(value,"RevealedMapFogComponent")){regionCache[t.guid]=value;return value;}}const auto controllerIdentity=g_Engine->GetObjectIdentity(t.controller);if(!controllerIdentity||!regionFallbackScannedControllers.insert(controllerIdentity).second)return 0;for(int32_t i=0;i<g_Engine->GetObjectCount();++i){uintptr_t value=g_Engine->GetObjectByIndex(i);if(!value||!g_Engine->IsObjectLive(value)||!g_Engine->IsA(value,"RevealedMapFogComponent"))continue;uintptr_t outer=value;for(int depth=0;depth<7&&outer;++depth){outer=Mem::ReadPtr(outer+Offsets::UObject_Outer);if(outer&&(outer==t.controller||outer==t.pawn||outer==t.state)){regionCache[t.guid]=value;return value;}}}return 0;}
bool ReadRegions(const Target&p,std::uint32_t&bitmap,std::vector<ActivityRegionInfo>&out){uintptr_t component=RegionComponentFor(p);if(!component)return false;int32_t bitmapOffset=g_Engine->FindPropertyOffset(component,"RevealedMapRegionsBitmap");if(bitmapOffset<0||bitmapOffset>0x1000||!Mem::Read(component+uintptr_t(bitmapOffset),bitmap))return false;for(unsigned i=0;i<32;++i)if(bitmap&(1u<<i)){auto found=regionNames.find(static_cast<std::uint8_t>(i));if(found!=regionNames.end())out.push_back({static_cast<std::uint8_t>(i),found->second});}std::uint32_t confirmed=0;return Mem::Read(component+uintptr_t(bitmapOffset),confirmed)&&confirmed==bitmap;}
bool Query(uintptr_t component,uintptr_t function,uintptr_t object){alignas(8)struct{void*data;std::uint8_t result;std::uint8_t padding[7];}params{};params.data=reinterpret_cast<void*>(object);processEvent(reinterpret_cast<void*>(component),reinterpret_cast<void*>(function),&params);return params.result!=0;}
void RecordMaximum(std::atomic<std::uint64_t>&maximum,std::uint64_t value){auto prior=maximum.load();while(value>prior&&!maximum.compare_exchange_weak(prior,value)){};}
bool ValidateQuery(uintptr_t function,const std::vector<std::string>&names){return PlayerActivityQueryLayoutValid(g_Engine->GetStructPropertyOffsets(function),names);}
uintptr_t FindUniqueFunctionByName(const char*name){uintptr_t match=0;for(int32_t i=0;i<g_Engine->GetObjectCount();++i){uintptr_t object=g_Engine->GetObjectByIndex(i);if(!object||g_Engine->GetObjectClassName(object)!="Function"||g_Engine->GetObjectName(object)!=name)continue;if(match&&match!=object)return 0;match=object;}return match;}
bool ResolveRegionRevealFunction(){
    uintptr_t function=g_Engine->FindFunction("RevealedMapFogComponent","Client_OnRegionRevealed");
    if(!function)function=FindUniqueFunctionByName("Client_OnRegionRevealed");
    if(!function)return false;
    std::size_t regionOffset=0,againOffset=8;const auto fields=g_Engine->GetStructPropertyOffsets(function);
    if(!fields.empty()){
        bool foundRegion=false,foundAgain=false;
        for(const auto&field:fields){
            if(field.first=="RevealedRegion"&&field.second>=0&&field.second+int32_t(sizeof(uintptr_t))<=32){regionOffset=std::size_t(field.second);foundRegion=true;}
            else if(field.first=="bIsReReveal"&&field.second>=0&&field.second<32){againOffset=std::size_t(field.second);foundAgain=true;}
        }
        if(!foundRegion||!foundAgain)return false;
    }
    regionRevealRegionOffset=regionOffset;regionRevealAgainOffset=againOffset;
    regionRevealFunction.store(function,std::memory_order_release);regionCompatibility=1;
    LogMessage("Player activity region reveal capture resolved after the map-fog component became live");
    return true;
}
void PrimeRegionServerLog(){
    std::ifstream input(regionServerLogPath,std::ios::binary|std::ios::ate);
    if(!input){regionServerLogReady=false;return;}
    const auto end=input.tellg();regionServerLogOffset=end<0?0:std::streamoff(end);
    regionServerLogPrimed=true;regionServerLogReady=true;
}
std::vector<NativeRegionDiscovery> ReadRegionServerLog(const std::vector<Target>&current,std::int64_t capturedAt){
    std::vector<NativeRegionDiscovery> out;
    if(!regionServerLogPrimed){PrimeRegionServerLog();return out;}
    std::ifstream input(regionServerLogPath,std::ios::binary);
    if(!input){regionServerLogReady=false;return out;}
    input.seekg(0,std::ios::end);const auto end=input.tellg();
    if(end<0){regionServerLogReady=false;return out;}
    if(std::streamoff(end)<regionServerLogOffset)regionServerLogOffset=0;
    input.seekg(regionServerLogOffset);std::string line;
    while(std::getline(input,line)){
        std::string region,controller;std::uint8_t id=0;
        if(!PlayerActivityParseRegionRevealLine(line,region,id,controller))continue;
        auto named=regionNames.find(id);if(named==regionNames.end()||named->second!=region)continue;
        auto target=std::find_if(current.begin(),current.end(),[&](const Target&value){return value.controllerName==controller;});
        if(target==current.end()){++dropped;continue;}
        out.push_back({target->guid,region,id,capturedAt});
    }
    regionServerLogOffset=std::streamoff(end);regionServerLogReady=true;return out;
}
std::vector<std::string> Split(const std::string&value,char separator){std::vector<std::string> out;size_t start=0;while(true){size_t at=value.find(separator,start);out.push_back(value.substr(start,at==std::string::npos?at:at-start));if(at==std::string::npos)break;start=at+1;}return out;}
bool LoadMaterialCatalog(const std::string&path){std::ifstream input(path);std::string line;size_t expected=0;materialFingerprint.clear();std::unordered_set<std::string> keys;if(!input)return false;while(std::getline(input,line)){if(!line.empty()&&line.back()=='\r')line.pop_back();if(line.rfind("#",0)==0){auto count=line.find("count=");if(count!=std::string::npos)try{expected=std::stoull(line.substr(count+6));}catch(...){}auto fingerprint=line.find("fingerprint=");if(fingerprint!=std::string::npos)materialFingerprint=line.substr(fingerprint+12,64);continue;}auto fields=Split(line,'\t');if(fields.size()!=3||fields[0].empty()||fields[0].size()>256||fields[1].empty()||fields[1].size()>512||fields[2].rfind("/assets/items/",0)!=0||!keys.insert(fields[0]).second)return false;catalog.push_back({EntryKind::Material,0,fields[0],fields[0],fields[1],fields[2]});}return expected&&keys.size()==expected&&materialFingerprint.size()==64;}
bool LoadUnlockCatalog(const std::string&path){std::ifstream input(path);std::string line;size_t expected=0;unlockFingerprint.clear();std::unordered_set<std::string> keys;if(!input)return false;while(std::getline(input,line)){if(!line.empty()&&line.back()=='\r')line.pop_back();if(line.rfind("#",0)==0){auto count=line.find("count=");if(count!=std::string::npos)try{expected=std::stoull(line.substr(count+6));}catch(...){}auto fingerprint=line.find("fingerprint=");if(fingerprint!=std::string::npos)unlockFingerprint=line.substr(fingerprint+12,64);continue;}auto fields=Split(line,'\t');if(fields.size()!=6||fields[0].empty()||fields[0].size()>256||(fields[1]!="perk"&&fields[1]!="spell")||fields[2].empty()||fields[2].size()>256||fields[3].empty()||fields[3].size()>512||fields[4].rfind("/assets/spells-and-perks/",0)!=0||!keys.insert(fields[0]).second)return false;catalog.push_back({fields[1]=="spell"?EntryKind::Spell:EntryKind::Perk,0,fields[0],fields[2],fields[3],fields[4]});}return expected&&keys.size()==expected&&unlockFingerprint.size()==64;}
void ResolveCatalogObjects(){std::unordered_map<std::string,uintptr_t> items,perks;for(int32_t i=0;i<g_Engine->GetObjectCount();++i){uintptr_t object=g_Engine->GetObjectByIndex(i);if(!object||!g_Engine->IsObjectLive(object))continue;std::string key=g_Engine->GetObjectName(object);if(key.empty()||key.rfind("Default__",0)==0)continue;if(g_Engine->IsA(object,"ItemData"))items.emplace(key,object);else if(g_Engine->IsA(object,"SkillPerkData"))perks.emplace(key,object);}std::uint64_t materials=0,unlocks=0;for(auto&entry:catalog){const auto&loaded=entry.kind==EntryKind::Material?items:perks;auto found=loaded.find(entry.key);entry.object=found==loaded.end()?0:found->second;if(entry.object){if(entry.kind==EntryKind::Material)++materials;else ++unlocks;}}resolvedMaterialEntries=materials;resolvedUnlockEntries=unlocks;nextMaterialResolve=std::chrono::steady_clock::now()+std::chrono::seconds(5);}
bool Online(const Target&t){Lock lock(targetMutex);return std::any_of(targets.begin(),targets.end(),[&](const auto&v){return v.guid==t.guid&&v.controller==t.controller;});}
}

int PlayerActivitySourceCompatibility(bool queryFound,bool queryValid,bool catalogReady){return !queryFound?-2:!queryValid?-3:!catalogReady?-4:1;}
bool PlayerActivityDiscoveryReady(int materialStatus,int unlockStatus){return materialStatus==1||unlockStatus==1;}
bool PlayerActivityQueryLayoutValid(const std::vector<std::pair<std::string,int32_t>>&fields,const std::vector<std::string>&inputNames){int input=-1,result=-1;for(const auto&field:fields){if(field.first=="ReturnValue")result=field.second;else if(std::find(inputNames.begin(),inputNames.end(),field.first)!=inputNames.end())input=field.second;}return (input==0&&result==8)||(input<0&&result<0);}

bool PlayerActivityParseRegionRevealLine(const std::string&line,std::string&regionName,std::uint8_t&regionId,std::string&controllerName){
    constexpr const char* marker="LogMapRegions: Revealing map region [";
    const auto start=line.find(marker);if(start==std::string::npos)return false;
    const auto nameStart=start+std::char_traits<char>::length(marker),nameEnd=line.find("] (Region ",nameStart);
    if(nameEnd==std::string::npos||nameEnd==nameStart)return false;
    const auto idStart=nameEnd+10,idEnd=line.find(") for player [",idStart);
    if(idEnd==std::string::npos||idEnd==idStart)return false;
    unsigned parsed=0;for(auto i=idStart;i<idEnd;++i){if(line[i]<'0'||line[i]>'9')return false;parsed=parsed*10+unsigned(line[i]-'0');if(parsed>255)return false;}
    const auto controllerStart=idEnd+14,controllerEnd=line.find(']',controllerStart);
    if(controllerEnd==std::string::npos||controllerEnd==controllerStart)return false;
    regionName=line.substr(nameStart,nameEnd-nameStart);controllerName=line.substr(controllerStart,controllerEnd-controllerStart);regionId=std::uint8_t(parsed);return true;
}

bool InitializePlayerActivityDiscovery(const std::string&materialCatalogPath,const std::string&unlockCatalogPath,const std::string&serverLogPath,uintptr_t moduleBase,uintptr_t moduleEnd){
    uintptr_t a=0,b=0,processOffset=0,c=0,d=0;
    if(!GetAdminTeleportOffsets(a,b,processOffset,c,d)||moduleBase+processOffset>=moduleEnd){
        materialCompatibility=-10;unlockCompatibility=-10;return false;
    }
    processEvent=reinterpret_cast<ProcessEventFn>(moduleBase+processOffset);
    regionServerLogPath=serverLogPath;PrimeRegionServerLog();
    hasPickedUpItem=g_Engine->FindFunction("ProgressComponent","HasPickedUpItem");
    isPerkUnlocked=g_Engine->FindFunction("SkillPerkComponent","IsPerkUnlocked");
    getRevealedMapFogComponent=g_Engine->FindFunction("DominionPlayerController","GetRevealedMapFogComponent");
    if(getRevealedMapFogComponent){const auto fields=g_Engine->GetStructPropertyOffsets(getRevealedMapFogComponent);if(!fields.empty()&&std::none_of(fields.begin(),fields.end(),[](const auto&field){return field.first=="ReturnValue"&&field.second==0;}))getRevealedMapFogComponent=0;}
    const bool captureReady=EnsurePlayerProcessEventCapture(moduleBase,moduleEnd);
    if(captureReady)ResolveRegionRevealFunction();
    regionCompatibility=regionServerLogReady.load()?1:(regionRevealFunction.load(std::memory_order_acquire)?1:(captureReady?0:-2));
    const bool materialQueryFound=hasPickedUpItem!=0;
    const bool materialQueryValid=materialQueryFound&&ValidateQuery(hasPickedUpItem,{"ItemData","Item","InItemData"});
    const bool unlockQueriesFound=isPerkUnlocked!=0;
    const bool unlockQueriesValid=unlockQueriesFound&&ValidateQuery(isPerkUnlocked,{"InSkillPerkData","SkillPerkData","PerkData"});

    catalog.clear();const bool materialCatalogReady=LoadMaterialCatalog(materialCatalogPath);const bool unlockCatalogReady=LoadUnlockCatalog(unlockCatalogPath);
    std::sort(catalog.begin(),catalog.end(),[](const auto&x,const auto&y){return int(x.kind)<int(y.kind)||(x.kind==y.kind&&x.key<y.key);});
    const auto perks=std::count_if(catalog.begin(),catalog.end(),[](const auto&e){return e.kind==EntryKind::Perk;});
    const auto spells=std::count_if(catalog.begin(),catalog.end(),[](const auto&e){return e.kind==EntryKind::Spell;});
    materialCatalogEntries=std::count_if(catalog.begin(),catalog.end(),[](const auto&e){return e.kind==EntryKind::Material;});perkCatalogEntries=perks;spellCatalogEntries=spells;
    materialCompatibility=PlayerActivitySourceCompatibility(materialQueryFound,materialQueryValid,materialCatalogReady);
    unlockCompatibility=PlayerActivitySourceCompatibility(unlockQueriesFound,unlockQueriesValid,unlockCatalogReady&&perks>0&&spells>0);
    if(PlayerActivityDiscoveryReady(materialCompatibility.load(),unlockCompatibility.load()))ResolveCatalogObjects();
    LogMessage("Player activity discovery catalog loaded "+std::to_string(materialCatalogEntries.load())+" authoritative first-pickup materials ("+std::to_string(resolvedMaterialEntries.load())+" currently loaded), "+std::to_string(perks)+" perks/spell upgrades ("+std::to_string(resolvedUnlockEntries.load())+" loaded), and "+std::to_string(spells)+" canonical spells; material compatibility "+std::to_string(materialCompatibility.load())+", unlock compatibility "+std::to_string(unlockCompatibility.load()));
    return PlayerActivityDiscoveryReady(materialCompatibility.load(),unlockCompatibility.load());
}
void RefreshPlayerActivityDiscoveryCompatibility(){if(g_Engine&&PlayerActivityDiscoveryReady(materialCompatibility.load(),unlockCompatibility.load())&&std::chrono::steady_clock::now()>=nextMaterialResolve)ResolveCatalogObjects();}
void UpdatePlayerActivityDiscoveryTargets(const std::vector<PlayerInfo>&players){
    if(!regionRevealFunction.load(std::memory_order_acquire)&&!players.empty()){
        const auto identity=g_Engine->GetObjectIdentity(players.front().controllerPtr);
        if(identity!=regionResolutionControllerIdentity){regionResolutionControllerIdentity=identity;regionResolutionAttempts=0;}
        // The dedicated server does not materialize Client_OnRegionRevealed until
        // a player's map-fog component is loaded. Retry only three times for each
        // controller lifetime, rather than scanning the object registry forever.
        if(identity&&regionResolutionAttempts<3){++regionResolutionAttempts;ResolveRegionRevealFunction();}
    }
    std::vector<Target> fresh;const bool nativeCapture=regionRevealFunction.load(std::memory_order_acquire)!=0,logCapture=regionServerLogReady.load();
    for(const auto&p:players){if(p.characterGuid.empty()||!p.controllerPtr)continue;Target t;t.guid=p.characterGuid;t.controllerName=g_Engine->GetObjectName(p.controllerPtr);t.controller=p.controllerPtr;t.pawn=p.pawnPtr;t.state=p.playerStatePtr;t.progress=ProgressFor(t);t.perks=PerksFor(t);t.regionsComplete=ReadRegions(t,t.regionBitmap,t.regions);regionCompatibility=t.regionsComplete||nativeCapture||logCapture?1:-1;if(!t.regionsComplete&&!nativeCapture&&!logCapture)++dropped;fresh.push_back(std::move(t));}Lock lock(targetMutex);targets=std::move(fresh);
}
void ProcessPlayerActivityDiscoveryTracking(){if(!g_Engine||!processEvent||catalog.empty())return;if(active&&!Online(active->target))active.reset();if(!active){std::vector<Target> current;{Lock lock(targetMutex);current=targets;}if(current.empty())return;static size_t targetCursor=0;if(targetCursor>=current.size())targetCursor=0;active=std::make_unique<Scan>();active->target=current[targetCursor++];active->materialsComplete=bool(active->target.progress)&&materialCompatibility.load()==1;active->unlocksComplete=bool(active->target.perks)&&unlockCompatibility.load()==1;active->materials.reserve(256);active->unlocks.reserve(128);}const auto started=std::chrono::steady_clock::now();int queries=0;while(active&&active->cursor<catalog.size()&&queries<8){const auto&e=catalog[active->cursor++];++queries;bool unlocked=false;if(e.kind==EntryKind::Material){if(active->materialsComplete){if(!g_Engine->IsObjectLive(active->target.progress)||!g_Engine->IsA(active->target.progress,"ProgressComponent"))active->materialsComplete=false;else if(e.object&&!g_Engine->IsObjectLive(e.object))active->materialsComplete=false;else if(e.object){const auto queryStarted=std::chrono::steady_clock::now();unlocked=Query(active->target.progress,hasPickedUpItem,e.object);const auto us=static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-queryStarted).count());++queryCalls;RecordMaximum(maximumQueryUs,us);if(us>=1000)++slowQueries;}if(unlocked)active->materials.push_back({e.key,e.name,e.icon});}}else if(active->unlocksComplete){if(!g_Engine->IsObjectLive(active->target.perks)||!g_Engine->IsA(active->target.perks,"SkillPerkComponent"))active->unlocksComplete=false;else if(e.object&&!g_Engine->IsObjectLive(e.object))active->unlocksComplete=false;else if(e.object){const auto queryStarted=std::chrono::steady_clock::now();unlocked=Query(active->target.perks,isPerkUnlocked,e.object);const auto us=static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-queryStarted).count());++queryCalls;RecordMaximum(maximumQueryUs,us);if(us>=1000)++slowQueries;}if(unlocked)active->unlocks.push_back({e.kind==EntryKind::Perk?"perk":"spell",e.canonical,e.name,e.icon});}if(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-started).count()>=250)break;}RecordMaximum(maximumBatchUs,static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-started).count()));if(active&&active->cursor==catalog.size()){PlayerActivityDiscoverySnapshot snapshot;snapshot.characterGuid=active->target.guid;snapshot.regionsComplete=active->target.regionsComplete;snapshot.regionBitmap=active->target.regionBitmap;snapshot.regions=active->target.regions;snapshot.materialsComplete=active->materialsComplete;snapshot.materials=std::move(active->materials);snapshot.materialFingerprint=materialFingerprint;snapshot.unlocksComplete=active->unlocksComplete;snapshot.unlocks=std::move(active->unlocks);snapshot.unlockFingerprint=unlockFingerprint;snapshot.capturedAt=std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();if(!snapshot.materialsComplete||!snapshot.unlocksComplete)++dropped;{Lock lock(snapshotMutex);completedSnapshots[snapshot.characterGuid]=std::move(snapshot);}++snapshots;active.reset();}}
std::vector<PlayerActivityDiscoverySnapshot> ReadPlayerActivityDiscoveries(){Lock lock(snapshotMutex);std::vector<PlayerActivityDiscoverySnapshot> out;out.reserve(completedSnapshots.size());for(const auto&pair:completedSnapshots)out.push_back(pair.second);return out;}
void ObservePlayerActivityProcessEvent(void*object,void*function,void*params){
    if(!object||!params||reinterpret_cast<uintptr_t>(function)!=regionRevealFunction.load(std::memory_order_acquire))return;
    const auto started=std::chrono::steady_clock::now();++regionHookCalls;
    RawRegionReveal event;event.object=reinterpret_cast<uintptr_t>(object);
    event.region=Mem::ReadPtr(reinterpret_cast<uintptr_t>(params)+regionRevealRegionOffset);
    event.again=Mem::ReadU8(reinterpret_cast<uintptr_t>(params)+regionRevealAgainOffset)!=0;
    if(!event.region){const auto us=static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-started).count());RecordMaximum(regionHookMaximumUs,us);if(us>=1000)++regionHookSlowCalls;return;}
    const std::uint64_t write=regionRevealWrite.load(std::memory_order_relaxed);
    if(write-regionRevealRead.load(std::memory_order_acquire)>=RegionRevealCapacity){++dropped;++regionHookQueueDrops;return;}
    const auto us=static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-started).count());event.hookMicros=us;
    regionRevealQueue[write%RegionRevealCapacity]=event;
    regionRevealWrite.store(write+1,std::memory_order_release);
    RecordMaximum(regionHookMaximumUs,us);if(us>=1000)++regionHookSlowCalls;
}
std::vector<NativeRegionDiscovery> DrainNativeRegionDiscoveries(){
    std::vector<RawRegionReveal> raw;std::uint64_t read=regionRevealRead.load(std::memory_order_relaxed),write=regionRevealWrite.load(std::memory_order_acquire);
    while(read<write){raw.push_back(regionRevealQueue[read%RegionRevealCapacity]);++read;}regionRevealRead.store(read,std::memory_order_release);
    std::vector<Target> current;{Lock lock(targetMutex);current=targets;}
    std::vector<NativeRegionDiscovery> out;const auto capturedAt=std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    for(const auto&event:raw){if(event.again||!g_Engine||!g_Engine->IsObjectLive(event.region)||!g_Engine->IsA(event.region,"MapRegion"))continue;int32_t offset=g_Engine->FindPropertyOffset(event.region,"RegionID");if(offset<0||offset>0x1000)continue;const std::uint8_t id=Mem::ReadU8(event.region+uintptr_t(offset));auto name=regionNames.find(id);if(name==regionNames.end())continue;std::string guid;uintptr_t owner=event.object;for(int depth=0;owner&&depth<8&&guid.empty();++depth){auto target=std::find_if(current.begin(),current.end(),[&](const Target&candidate){return owner==candidate.controller||owner==candidate.pawn||owner==candidate.state;});if(target!=current.end()){guid=target->guid;break;}uintptr_t next=Mem::ReadPtr(owner+Offsets::UObject_Outer);if(next==owner)break;owner=next;}if(guid.empty()){++dropped;continue;}PerformanceTrace::Record("activity","region discovered",event.hookMicros,guid.c_str(),'g',"game");out.push_back({std::move(guid),name->second,id,capturedAt});}
    // The Linux dedicated server strips the client map-fog RPC and component
    // reflection metadata, but emits this authoritative line exactly once at
    // discovery time. The reader starts at EOF, so old regions are never
    // backfilled. It also uses the existing two-second activity cycle and does
    // not inspect the Unreal object registry.
    auto logged=ReadRegionServerLog(current,capturedAt);out.insert(out.end(),std::make_move_iterator(logged.begin()),std::make_move_iterator(logged.end()));
    return out;
}
PlayerActivityDiscoveryDiagnostics GetPlayerActivityDiscoveryDiagnostics(){return {regionCompatibility.load(),materialCompatibility.load(),unlockCompatibility.load(),materialCatalogEntries.load(),resolvedMaterialEntries.load(),perkCatalogEntries.load(),spellCatalogEntries.load(),resolvedUnlockEntries.load(),snapshots.load(),dropped.load(),queryCalls.load(),maximumQueryUs.load(),slowQueries.load(),maximumBatchUs.load(),regionHookCalls.load(),regionRevealWrite.load()-regionRevealRead.load(),regionHookQueueDrops.load(),regionHookMaximumUs.load(),regionHookSlowCalls.load()};}
}
