#include "dom_engine.h"
#include "../utils/logger.h"
#include "../utils/memory.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <memory>
#include <pthread.h>
#include <sstream>
#include <sys/mman.h>
#include <unordered_map>
#include <unordered_set>
#include <unistd.h>

namespace DomEngine { namespace {
struct Entry{std::string id,name,key;uintptr_t object=0;};
struct Target{std::string guid;uintptr_t controller=0,pawn=0,state=0,component=0;};
struct Scan{Target target;size_t cursor=0;bool objectivePending=false;std::vector<QuestProgressInfo> quests;};
using ProcessEventFn=void(*)(void*,void*,void*);
std::vector<Entry> catalog;std::vector<Target> targets;std::unordered_map<std::string,PlayerQuestSnapshot> snapshots;std::unordered_map<std::string,uintptr_t> componentCache;
pthread_mutex_t targetMutex=PTHREAD_MUTEX_INITIALIZER,snapshotMutex=PTHREAD_MUTEX_INITIALIZER;
struct Lock{pthread_mutex_t*p;explicit Lock(pthread_mutex_t&v):p(&v){pthread_mutex_lock(p);}~Lock(){pthread_mutex_unlock(p);}};
using GetDisplayStringFn=const FString*(*)(const void*);using FTextDestructorFn=void(*)(void*);
std::unique_ptr<Scan> active;ProcessEventFn processEvent=nullptr,originalProcessEvent=nullptr;uintptr_t getState=0,getCurrentObjective=0,serverNotifyTracked=0,clientNotifyTracked=0;GetDisplayStringFn displayText=nullptr;FTextDestructorFn destroyText=nullptr;std::size_t questParamOffset=0,stateReturnOffset=8,objectiveQuestOffset=0,objectiveReturnOffset=8,trackedNotifyQuestOffset=0;std::string fingerprint;
std::atomic<int> enabled{0},compatibility{0},catalogCompat{0},componentCompat{0},queryCompat{0},objectiveCompat{0},trackedQuestCompat{0};
std::atomic<std::uint64_t> resolved{0},completed{0},failures{0},changes{0},persistenceFailures{0},cacheHits{0},cacheMisses{0},invalidIdentities{0},trackedQuestFailures{0},trackedQuestCaptures{0},lastQueryUs{0},maxQueryUs{0},slowQueries{0};std::atomic<std::int64_t> lastScan{0};
std::unordered_map<uintptr_t,uintptr_t> trackedQuestByComponent;std::unordered_set<uintptr_t> knownQuestComponents;pthread_mutex_t trackedQuestMutex=PTHREAD_MUTEX_INITIALIZER;
std::chrono::steady_clock::time_point nextCycle{},nextResolve{},nextQuery{};constexpr int QueryIntervalMs=20;
std::int64_t Now(){return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();}
std::vector<std::string> Split(const std::string&v,char s){std::vector<std::string>o;size_t p=0;for(;;){size_t a=v.find(s,p);o.push_back(v.substr(p,a==std::string::npos?a:a-p));if(a==std::string::npos)return o;p=a+1;}}
std::string Key(std::string v){std::string o;for(unsigned char c:v)if(std::isalnum(c))o+=char(std::tolower(c));for(const char*prefix:{"default","da","bp","quest","data"})if(o.rfind(prefix,0)==0)o.erase(0,std::char_traits<char>::length(prefix));return o;}
void Max(std::atomic<std::uint64_t>&m,std::uint64_t v){auto p=m.load();while(v>p&&!m.compare_exchange_weak(p,v)){}}
uintptr_t FindComponent(const Target&t){auto c=componentCache.find(t.guid);if(c!=componentCache.end()&&g_Engine->IsObjectLive(c->second)&&g_Engine->IsA(c->second,"QuestProgressComponent")){++cacheHits;return c->second;}++cacheMisses;for(int32_t i=0;i<g_Engine->GetObjectCount();++i){uintptr_t x=g_Engine->GetObjectByIndex(i);if(!x||!g_Engine->IsObjectLive(x)||!g_Engine->IsA(x,"QuestProgressComponent"))continue;uintptr_t outer=x;for(int d=0;d<7&&outer;++d){outer=Mem::ReadPtr(outer+Offsets::UObject_Outer);if(outer&&(outer==t.controller||outer==t.pawn||outer==t.state)){componentCache[t.guid]=x;return x;}}}return 0;}
bool ResolveObjects(){std::unordered_map<std::string,std::vector<std::pair<uintptr_t,std::string>>> objects;std::vector<std::string> diagnostic;for(int32_t i=0;i<g_Engine->GetObjectCount();++i){uintptr_t o=g_Engine->GetObjectByIndex(i);if(!o||!g_Engine->IsObjectLive(o))continue;std::string name=g_Engine->GetObjectName(o),key=Key(name);if(key.empty())continue;std::string cls=g_Engine->GetObjectClassName(o);objects[key].push_back({o,cls});if(diagnostic.size()<40&&(name.find("Quest")!=std::string::npos||cls.find("Quest")!=std::string::npos))diagnostic.push_back(name+"["+cls+"]");}std::uint64_t count=0;for(auto&e:catalog){e.object=0;auto found=objects.find(e.key);if(found==objects.end()||found->second.size()!=1)continue;const auto&candidate=found->second.front();if(candidate.second.find("Quest")!=std::string::npos||g_Engine->IsA(candidate.first,"QuestData")){e.object=candidate.first;++count;}}resolved=count;if(count==catalog.size()){catalogCompat=1;componentCompat=1;compatibility=1;nextCycle=std::chrono::steady_clock::now();return true;}catalogCompat=0;compatibility=0;LogMessage("Quest catalog pending: resolved "+std::to_string(count)+" of "+std::to_string(catalog.size())+" objects");if(count==0&&!diagnostic.empty()){std::string sample;for(const auto&v:diagnostic)sample+=(sample.empty()?"":" | ")+v;LogMessage("Quest object sample: "+sample);}return false;}
std::string Status(uint8_t state){return state==2?"completed":state==1?"in_progress":"not_started";}
uint8_t Query(uintptr_t component,uintptr_t quest){alignas(8)unsigned char p[32]{};std::memcpy(p+questParamOffset,&quest,sizeof(quest));processEvent(reinterpret_cast<void*>(component),reinterpret_cast<void*>(getState),p);return p[stateReturnOffset];}
std::string QueryObjective(uintptr_t component,uintptr_t quest){
    if(objectiveCompat.load()!=1)return {};
    alignas(8)unsigned char p[48]{};
    std::memcpy(p+objectiveQuestOffset,&quest,sizeof(quest));
    processEvent(reinterpret_cast<void*>(component),reinterpret_cast<void*>(getCurrentObjective),p);
    // A quest may legitimately be in progress while having no current objective
    // (Cook's Assistant does this during login).  The reflected call then leaves
    // a default FText whose TextData shared-reference pointer is null. Unreal's
    // FTextInspector::GetDisplayString requires a constructed FText and
    // dereferences TextData, so never pass the default/null value to it.
    const auto text= reinterpret_cast<uintptr_t>(p+objectiveReturnOffset);
    const auto textData=Mem::ReadPtr(text);
    if(!textData)return {};
    std::string value;
    const FString* shown=displayText(reinterpret_cast<const void*>(text));
    if(shown&&Mem::Readable(shown,sizeof(FString)))value=g_Engine->ReadFString(reinterpret_cast<uintptr_t>(shown));
    destroyText(reinterpret_cast<void*>(text));
    return value;
}
bool LoadTextRuntime(uintptr_t moduleBase,uintptr_t moduleEnd){const char*raw=std::getenv("DW_COMPATIBILITY_FILE");std::ifstream in(raw&&*raw?raw:"/home/container/zanik/compatibility.tsv");const std::string sha=FileSha256("/proc/self/exe");std::string line;while(std::getline(in,line)){if(line.empty()||line[0]=='#')continue;auto f=Split(line,'\t');if(f.size()<49||f[0]!=sha)continue;try{uintptr_t display=std::stoull(f[4],nullptr,0),destroy=std::stoull(f.back(),nullptr,0);if(moduleBase+display>=moduleEnd||moduleBase+destroy>=moduleEnd)return false;displayText=reinterpret_cast<GetDisplayStringFn>(moduleBase+display);destroyText=reinterpret_cast<FTextDestructorFn>(moduleBase+destroy);return true;}catch(...){return false;}}return false;}
bool InitializeObjectiveQuery(uintptr_t moduleBase,uintptr_t moduleEnd){getCurrentObjective=g_Engine->FindFunction("QuestProgressComponent","GetCurrentObjectiveText");if(!getCurrentObjective||!LoadTextRuntime(moduleBase,moduleEnd))return false;auto fields=g_Engine->GetStructPropertyOffsets(getCurrentObjective);bool input=false,result=false;for(const auto&f:fields){if(f.first=="ReturnValue"&&f.second+16<=48){objectiveReturnOffset=f.second;result=true;}else if((f.first=="Quest"||f.first=="QuestData"||f.first=="InQuest"||f.first=="InQuestData")&&f.second+sizeof(uintptr_t)<=48){objectiveQuestOffset=f.second;input=true;}}if((input||result)&&(!input||!result||objectiveQuestOffset==objectiveReturnOffset))return false;objectiveCompat=1;return true;}
bool Online(const Target&t){Lock l(targetMutex);return std::any_of(targets.begin(),targets.end(),[&](const auto&v){return v.guid==t.guid&&v.controller==t.controller;});}
bool TrackedId(uintptr_t object,std::string&id){
    if(!object||!g_Engine->IsObjectLive(object)||!g_Engine->IsA(object,"QuestData"))return false;
    const auto found=std::find_if(catalog.begin(),catalog.end(),[&](const Entry&e){return e.object==object;});
    if(found==catalog.end())return false;id=found->id;return true;
}
void WriteAbsoluteJump(unsigned char*destination,uintptr_t target){destination[0]=0x48;destination[1]=0xB8;std::memcpy(destination+2,&target,sizeof(target));destination[10]=0xFF;destination[11]=0xE0;}
void HookProcessEvent(void*object,void*function,void*params){
    const auto functionAddress=reinterpret_cast<uintptr_t>(function);bool trackedNotify=functionAddress==serverNotifyTracked||functionAddress==clientNotifyTracked;
    if(!trackedNotify&&object&&function&&g_Engine){
        bool known=false;{Lock lock(trackedQuestMutex);known=knownQuestComponents.count(reinterpret_cast<uintptr_t>(object))!=0;}
        if(known){const std::string name=g_Engine->GetObjectName(functionAddress);if(name=="Server_NotifyQuestTracked"){serverNotifyTracked=functionAddress;trackedNotify=true;}else if(name=="Client_NotifyTrackedQuestLoaded"){clientNotifyTracked=functionAddress;trackedNotify=true;}}
    }
    if(object&&params&&trackedNotify&&g_Engine&&
       g_Engine->IsObjectLive(reinterpret_cast<uintptr_t>(object))&&g_Engine->IsA(reinterpret_cast<uintptr_t>(object),"QuestProgressComponent")){
        const uintptr_t quest=Mem::ReadPtr(reinterpret_cast<uintptr_t>(params)+trackedNotifyQuestOffset);
        if(!quest||(g_Engine->IsObjectLive(quest)&&g_Engine->IsA(quest,"QuestData"))){
            Lock lock(trackedQuestMutex);trackedQuestByComponent[reinterpret_cast<uintptr_t>(object)]=quest;++trackedQuestCaptures;
        }else ++trackedQuestFailures;
    }
    originalProcessEvent(object,function,params);
}
bool ValidateTrackedNotify(uintptr_t function){
    auto fields=g_Engine->GetStructPropertyOffsets(function);if(fields.empty())return true;
    for(const auto&field:fields)if((field.first=="QuestData"||field.first=="Quest"||field.first=="InQuestData")&&field.second>=0&&field.second+int32_t(sizeof(uintptr_t))<=32){trackedNotifyQuestOffset=size_t(field.second);return true;}
    // The pinned Linux server build retains the RPC and its one-pointer ABI,
    // but strips/renames the parameter field metadata. Windows reflection for
    // the same game build confirms QuestData is the first parameter. The hook
    // still rejects anything that is not a live QuestData object.
    trackedNotifyQuestOffset=0;return true;
}
uintptr_t FindUniqueFunctionByName(const char* name){
    uintptr_t match=0;
    for(int32_t i=0;i<g_Engine->GetObjectCount();++i){
        const uintptr_t object=g_Engine->GetObjectByIndex(i);
        if(!object||g_Engine->GetObjectClassName(object)!="Function"||g_Engine->GetObjectName(object)!=name)continue;
        if(match&&match!=object)return 0;
        match=object;
    }
    return match;
}
bool InstallTrackedQuestCapture(uintptr_t moduleBase,uintptr_t moduleEnd,uintptr_t processOffset){
    serverNotifyTracked=g_Engine->FindFunction("QuestProgressComponent","Server_NotifyQuestTracked");
    clientNotifyTracked=g_Engine->FindFunction("QuestProgressComponent","Client_NotifyTrackedQuestLoaded");
    // Linux retains the RPC UFunctions but can strip their owning class from
    // the UObject outer chain. Exact function names remain authoritative and
    // unique, so fall back to a bounded unique-name lookup.
    if(!serverNotifyTracked)serverNotifyTracked=FindUniqueFunctionByName("Server_NotifyQuestTracked");
    if(!clientNotifyTracked)clientNotifyTracked=FindUniqueFunctionByName("Client_NotifyTrackedQuestLoaded");
    if((serverNotifyTracked&&!ValidateTrackedNotify(serverNotifyTracked))||
       (clientNotifyTracked&&!ValidateTrackedNotify(clientNotifyTracked))){trackedQuestCompat=-2;return false;}
    constexpr size_t PrologueOffset=14,HookBytes=13;const unsigned char expected[HookBytes]={0x55,0x48,0x89,0xE5,0x41,0x57,0x41,0x56,0x41,0x55,0x41,0x54,0x53};
    if(moduleBase+processOffset+PrologueOffset+HookBytes>moduleEnd)return false;auto*target=reinterpret_cast<unsigned char*>(moduleBase+processOffset+PrologueOffset);
    if(std::memcmp(target,expected,sizeof(expected))!=0){trackedQuestCompat=-3;return false;}
    auto*trampoline=static_cast<unsigned char*>(mmap(nullptr,64,PROT_READ|PROT_WRITE|PROT_EXEC,MAP_PRIVATE|MAP_ANONYMOUS,-1,0));
    if(trampoline==MAP_FAILED){trackedQuestCompat=-4;return false;}std::memcpy(trampoline,target,HookBytes);WriteAbsoluteJump(trampoline+HookBytes,reinterpret_cast<uintptr_t>(target+HookBytes));originalProcessEvent=reinterpret_cast<ProcessEventFn>(trampoline);
    const long pageSize=sysconf(_SC_PAGESIZE);const uintptr_t page=reinterpret_cast<uintptr_t>(target)&~(uintptr_t(pageSize)-1);
    if(pageSize<=0||mprotect(reinterpret_cast<void*>(page),size_t(pageSize),PROT_READ|PROT_WRITE|PROT_EXEC)!=0){originalProcessEvent=nullptr;munmap(trampoline,64);trackedQuestCompat=-4;return false;}
    unsigned char patch[HookBytes];std::memset(patch,0x90,sizeof(patch));WriteAbsoluteJump(patch,reinterpret_cast<uintptr_t>(&HookProcessEvent));std::memcpy(target,patch,sizeof(patch));__builtin___clear_cache(reinterpret_cast<char*>(target),reinterpret_cast<char*>(target+HookBytes));mprotect(reinterpret_cast<void*>(page),size_t(pageSize),PROT_READ|PROT_EXEC);
    trackedQuestCompat=0;LogMessage("Tracked quest capture installed for server and login notifications"+(serverNotifyTracked||clientNotifyTracked?std::string{}:"; notification functions will resolve when the component becomes live"));return true;
}
bool ReadTrackedQuest(uintptr_t component,std::string&id){
    id.clear();
    {
        Lock lock(trackedQuestMutex);auto captured=trackedQuestByComponent.find(component);
        if(captured!=trackedQuestByComponent.end()){
            if(!captured->second){trackedQuestCompat=1;return true;}
            if(!TrackedId(captured->second,id)){++trackedQuestFailures;return false;}
            trackedQuestCompat=1;return true;
        }
    }
    int32_t offset=g_Engine->FindPropertyOffset(component,"TrackedQuestMap");
    if(offset>=0&&offset<=0x1000){
        // UE5 TMap<EQuestType,UQuestData*>: the sparse element is an aligned
        // byte enum key, an object pointer at +8, then two 32-bit set links.
        struct Header{uintptr_t data;int32_t num,max;uintptr_t allocation;int32_t allocationNum,allocationMax;
            int32_t firstFree,numFree;uintptr_t hash;int32_t hashSize,padding;};
        static_assert(sizeof(Header)==56,"unexpected TMap header layout");Header before{},after{};
        if(!Mem::Read(component+uintptr_t(offset),before)||before.num<0||before.max<before.num||before.max>8||
           before.allocationNum!=before.num||before.allocationMax<before.allocationNum||before.numFree<0||before.numFree>before.num||
           (before.num&&(!before.data||!before.allocation||!Mem::Readable(reinterpret_cast<void*>(before.data),size_t(before.num)*24)||
            !Mem::Readable(reinterpret_cast<void*>(before.allocation),size_t((before.num+31)/32)*sizeof(std::uint32_t))))){++trackedQuestFailures;return false;}
        std::unordered_map<std::uint8_t,std::string> tracked;
        for(int32_t i=0;i<before.num;++i){std::uint32_t bits=0;if(!Mem::Read(before.allocation+uintptr_t(i/32)*sizeof(bits),bits)){++trackedQuestFailures;return false;}if(!(bits&(std::uint32_t(1)<<unsigned(i%32))))continue;const uintptr_t entry=before.data+uintptr_t(i)*24;const std::uint8_t type=Mem::ReadU8(entry);const uintptr_t object=Mem::ReadPtr(entry+8);std::string questId;if(type<1||type>3||!TrackedId(object,questId)||!tracked.emplace(type,std::move(questId)).second){++trackedQuestFailures;return false;}}
        if(!Mem::Read(component+uintptr_t(offset),after)||std::memcmp(&before,&after,sizeof(before))!=0||tracked.size()!=size_t(before.num-before.numFree)){++trackedQuestFailures;return false;}
        for(const std::uint8_t type:{std::uint8_t(2),std::uint8_t(1),std::uint8_t(3)}){auto found=tracked.find(type);if(found!=tracked.end()){id=found->second;break;}}
        trackedQuestCompat=1;return true;
    }
    offset=g_Engine->FindPropertyOffset(component,"TrackedSecondaryQuest");
    if(offset<0||offset>0x1000){if(!originalProcessEvent)trackedQuestCompat=-1;++trackedQuestFailures;return false;}
    const uintptr_t first=Mem::ReadPtr(component+uintptr_t(offset)),second=Mem::ReadPtr(component+uintptr_t(offset));
    if(first!=second){++trackedQuestFailures;return false;}
    if(!first){trackedQuestCompat=1;return true;}
    if(!TrackedId(first,id)){++trackedQuestFailures;return false;}
    trackedQuestCompat=1;return true;
}
}
bool InitializePlayerQuestTracking(const std::string&path,uintptr_t moduleBase,uintptr_t moduleEnd){const char*toggle=std::getenv("DW_QUEST_TRACKING_ENABLED");if(toggle&&(std::string(toggle)=="0"||std::string(toggle)=="false")){enabled=0;compatibility=-8;return false;}enabled=1;compatibility=-7;std::ifstream in(path);if(!in){catalogCompat=-1;compatibility=-1;return false;}std::string line;size_t expected=0;while(std::getline(in,line)){if(!line.empty()&&line.back()=='\r')line.pop_back();if(line.rfind("#",0)==0){auto a=line.find("fingerprint=");if(a!=std::string::npos)fingerprint=line.substr(a+12,64);a=line.find("count=");if(a!=std::string::npos)try{expected=std::stoull(line.substr(a+6));}catch(...){}continue;}auto f=Split(line,'\t');if(f.size()!=3||f[0].empty()||f[1].empty()||f[2].empty())continue;catalog.push_back({f[0],f[1],Key(f[2]),0});}if(catalog.empty()||catalog.size()!=expected||fingerprint.size()!=64){catalogCompat=-1;compatibility=-2;return false;}uintptr_t a=0,b=0,pe=0,c=0,d=0;if(!GetAdminTeleportOffsets(a,b,pe,c,d)||moduleBase+pe>=moduleEnd){queryCompat=-1;compatibility=-3;return false;}processEvent=reinterpret_cast<ProcessEventFn>(moduleBase+pe);getState=g_Engine->FindFunction("QuestProgressComponent","GetQuestState");if(!getState){queryCompat=-1;compatibility=-4;return false;}auto fields=g_Engine->GetStructPropertyOffsets(getState);if(!fields.empty()){bool input=false,result=false;for(const auto&f:fields){if(f.first=="ReturnValue"&&f.second<32){stateReturnOffset=f.second;result=true;}else if((f.first=="Quest"||f.first=="QuestData"||f.first=="InQuest"||f.first=="InQuestData")&&f.second+sizeof(uintptr_t)<=32){questParamOffset=f.second;input=true;}}if((input||result)&&(!input||!result||questParamOffset==stateReturnOffset)){queryCompat=-1;compatibility=-5;return false;}if(!input&&!result)LogMessage("Quest GetQuestState fields stripped; using fingerprint-bound ABI and behavioral state validation");}queryCompat=1;if(!InitializeObjectiveQuery(moduleBase,moduleEnd)){objectiveCompat=-1;LogMessage("Quest current-objective decoding compatibility unavailable; status tracking remains enabled");}if(!InstallTrackedQuestCapture(moduleBase,moduleEnd,pe))LogMessage("Tracked quest notification capture compatibility unavailable; reflected-property fallback remains enabled");nextResolve=std::chrono::steady_clock::now();ResolveObjects();return compatibility.load()==1;}
void UpdatePlayerQuestTargets(const std::vector<PlayerInfo>&players){std::vector<Target> fresh;std::unordered_set<uintptr_t> components;for(const auto&p:players){if(p.characterGuid.empty()||!p.controllerPtr){if(!p.characterGuid.empty())++invalidIdentities;continue;}Target t{p.characterGuid,p.controllerPtr,p.pawnPtr,p.playerStatePtr,0};t.component=FindComponent(t);if(t.component)components.insert(t.component);fresh.push_back(t);}{Lock l(trackedQuestMutex);knownQuestComponents=std::move(components);}{Lock l(targetMutex);targets=std::move(fresh);}}
void RefreshPlayerQuestTrackingCompatibility(){if(!enabled||!g_Engine||compatibility.load()<0||compatibility.load()==1)return;auto now=std::chrono::steady_clock::now();if(now<nextResolve)return;ResolveObjects();nextResolve=now+std::chrono::seconds(10);}
void ProcessPlayerQuestTracking(){
    if(!enabled||!g_Engine||compatibility.load()!=1)return;
    auto now=std::chrono::steady_clock::now();
    if(active&&!Online(active->target))active.reset();
    if(!active){
        if(now<nextCycle)return;std::vector<Target> current;{Lock l(targetMutex);current=targets;}
        if(current.empty()){nextCycle=now+std::chrono::seconds(2);return;}
        static size_t cursor=0;if(cursor>=current.size())cursor=0;Target t=current[cursor++];
        if(!t.component||!g_Engine->IsObjectLive(t.component)){componentCompat=0;++failures;nextCycle=now+std::chrono::seconds(1);return;}
        componentCompat=1;active=std::make_unique<Scan>();active->target=t;active->quests.reserve(catalog.size());nextQuery=now;
    }
    if(active->cursor<catalog.size()){
        if(now<nextQuery)return;nextQuery=now+std::chrono::milliseconds(QueryIntervalMs);auto&e=catalog[active->cursor];
        if(!e.object||!g_Engine->IsObjectLive(e.object)){compatibility=-6;active.reset();++failures;return;}
        auto started=std::chrono::steady_clock::now();
        if(active->objectivePending){active->quests.back().objective=QueryObjective(active->target.component,e.object);active->objectivePending=false;++active->cursor;}
        else {uint8_t state=Query(active->target.component,e.object);if(state>2){compatibility=-9;queryCompat=-1;active.reset();++failures;return;}active->quests.push_back({e.id,Status(state),"",0});if(state==1&&objectiveCompat.load()==1)active->objectivePending=true;else ++active->cursor;}
        auto us=std::uint64_t(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-started).count());lastQueryUs=us;Max(maxQueryUs,us);if(us>=1000)++slowQueries;
    }
    if(active&&active->cursor==catalog.size()){
        PlayerQuestSnapshot s;s.characterGuid=active->target.guid;s.quests=active->quests;s.catalogFingerprint=fingerprint;s.capturedAt=Now();s.complete=true;s.trackedQuestAvailable=ReadTrackedQuest(active->target.component,s.trackedQuestId);
        {Lock l(snapshotMutex);auto i=snapshots.find(s.characterGuid);bool changed=i==snapshots.end()||i->second.quests.size()!=s.quests.size()||i->second.trackedQuestAvailable!=s.trackedQuestAvailable||i->second.trackedQuestId!=s.trackedQuestId||!std::equal(i->second.quests.begin(),i->second.quests.end(),s.quests.begin(),[](const auto&a,const auto&b){return a.id==b.id&&a.status==b.status&&a.objective==b.objective;});if(changed){snapshots[s.characterGuid]=std::move(s);++changes;}}
        lastScan=Now();++completed;active.reset();nextCycle=now+std::chrono::seconds(2);
    }
}
std::vector<PlayerQuestSnapshot> ReadPlayerQuestSnapshots(){Lock l(snapshotMutex);std::vector<PlayerQuestSnapshot>o;for(const auto&p:snapshots)o.push_back(p.second);return o;}
PlayerQuestDiagnostics GetPlayerQuestDiagnostics(){PlayerQuestDiagnostics d;d.enabled=enabled;d.compatibility=compatibility;d.catalogCompatibility=catalogCompat;d.componentCompatibility=componentCompat;d.queryCompatibility=queryCompat;d.objectiveCompatibility=objectiveCompat;d.trackedQuestCompatibility=trackedQuestCompat;d.catalogEntries=catalog.size();d.resolvedEntries=resolved;{Lock l(snapshotMutex);d.snapshots=snapshots.size();}d.scansCompleted=completed;d.queryFailures=failures;d.snapshotChanges=changes;d.persistenceFailures=persistenceFailures;d.cacheHits=cacheHits;d.cacheMisses=cacheMisses;d.invalidIdentities=invalidIdentities;d.trackedQuestFailures=trackedQuestFailures;d.trackedQuestCaptures=trackedQuestCaptures;d.lastScan=lastScan;d.lastQueryMicroseconds=lastQueryUs;d.maximumQueryMicroseconds=maxQueryUs;d.slowQueries=slowQueries;return d;}
std::string GetPlayerQuestCatalogFingerprint(){return compatibility.load()==1?fingerprint:std::string();}
void RecordPlayerQuestPersistenceFailure(){++persistenceFailures;}
}
