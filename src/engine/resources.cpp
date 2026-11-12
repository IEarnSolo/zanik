#include "dom_engine.h"
#include "../utils/memory.h"
#include "../performance_trace.h"
#include "../utils/logger.h"
#include "../utils/runtime_paths.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <pthread.h>
#include <sstream>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace DomEngine { namespace {

struct Identity { std::string id, type, profile; };
std::unordered_map<std::string,Identity> identities;
std::unordered_map<std::string,ResourceStateInfo> states;
std::unordered_map<std::string,bool> priorAvailability;
pthread_mutex_t stateMutex=PTHREAD_MUTEX_INITIALIZER;
std::atomic<int> compatibility{0},catalogNodes{0},loadedComponents{0},matchedNodes{0},persistedNodes{0},unresolvedNodes{0};
std::atomic<int> actionableCatalogNodes{0},foliageCatalogNodes{0},matchedActionableNodes{0},matchedFoliageNodes{0};
std::atomic<std::uint64_t> rejected{0},collections{0},respawns{0},rawHookCalls{0},unresolvedHookCalls{0};
std::atomic<std::uint64_t> hookQueueDrops{0},maximumHookMicroseconds{0},slowHookCalls{0};
std::atomic<std::uint64_t> scanSlices{0},scanObjects{0},maximumScanMicroseconds{0},slowScanSlices{0};
std::atomic<std::int64_t> lastSuccess{0};
std::atomic<double> tickRate{0};
std::atomic<std::int64_t> latestGameTicks{0};
std::int64_t sampleTicks=0,updatedAt=0;
std::chrono::steady_clock::time_point sampleWall{};
uintptr_t getGameTimeOffset=0;
uintptr_t getTimeProviderOffset=0;
uintptr_t getRespawnIntervalOffset=0;
uintptr_t getFoliageDepictableOffset=0;
uintptr_t setUnavailableOffset=0;
uintptr_t setResourceAmountOffset=0;
uintptr_t respawnElapsedOffset=0;
bool timeProviderNeedsSubsystem=false;
uintptr_t cachedTimeSubsystem=0;
constexpr size_t ResourceHookBytes=15;
using SetUnavailableFn=void(*)(uintptr_t,uintptr_t);
SetUnavailableFn originalSetUnavailable=nullptr;
using SetResourceAmountFn=void(*)(uintptr_t,int);
SetResourceAmountFn originalSetResourceAmount=nullptr;
using RespawnElapsedFn=void(*)(uintptr_t);
RespawnElapsedFn originalRespawnElapsed=nullptr;
struct DepletionEvent { std::int64_t changed=0, deadline=0; };
std::unordered_map<std::string,DepletionEvent> eventDepleted;
std::unordered_set<std::string> loggedRuntimeIntervals;
std::unordered_map<std::string,std::pair<int,std::int64_t>> persisted;
time_t saveMtime=0;

enum class RawResourceEventKind:std::uint8_t{ActorDepleted,ComponentDepleted,ComponentRespawned};
struct RawResourceEvent{RawResourceEventKind kind{};uintptr_t object=0,player=0;std::uint64_t identity=0,playerIdentity=0,hookMicros=0;};
constexpr std::size_t RawResourceEventCapacity=256;
std::array<RawResourceEvent,RawResourceEventCapacity> rawResourceEvents{};
std::atomic<std::uint64_t> rawResourceWrite{0},rawResourceRead{0};
struct ResourceScanCycle{
    bool active=false;int cursor=0,count=0,loaded=0,matched=0,matchedActionable=0,matchedFoliage=0,unresolved=0;
    std::int64_t nowTicks=0;std::unordered_map<std::string,ResourceStateInfo> next;
};
ResourceScanCycle resourceScan;
std::chrono::steady_clock::time_point nextResourceReconciliation{};

std::uint64_t RecordHookDuration(const std::chrono::steady_clock::time_point&started){
    const auto elapsed=static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now()-started).count());auto prior=maximumHookMicroseconds.load(std::memory_order_relaxed);
    while(elapsed>prior&&!maximumHookMicroseconds.compare_exchange_weak(prior,elapsed,std::memory_order_relaxed)){}
    if(elapsed>=1000)++slowHookCalls;return elapsed;
}
void QueueRawResourceEvent(RawResourceEventKind kind,uintptr_t object,std::uint64_t hookMicros,uintptr_t player=0){
    if(!object||!g_Engine)return;
    const auto write=rawResourceWrite.load(std::memory_order_relaxed);
    if(write-rawResourceRead.load(std::memory_order_acquire)>=RawResourceEventCapacity){++hookQueueDrops;return;}
    const auto identity=g_Engine->GetObjectIdentity(object);if(!identity){++unresolvedHookCalls;return;}
    rawResourceEvents[write%RawResourceEventCapacity]={kind,object,player,identity,player?g_Engine->GetObjectIdentity(player):0,hookMicros};
    rawResourceWrite.store(write+1,std::memory_order_release);
}

uintptr_t ResolveTimeProvider(uintptr_t worldContext){
    using ProviderFn=uintptr_t(*)(uintptr_t);
    if(!getTimeProviderOffset||!g_Engine)return 0;
    uintptr_t receiver=worldContext;
    if(timeProviderNeedsSubsystem){
        if(!cachedTimeSubsystem||!Mem::Readable(reinterpret_cast<void*>(cachedTimeSubsystem),0x30)||
           !g_Engine->IsA(cachedTimeSubsystem,"InGameTimeSubsystem")){
            cachedTimeSubsystem=0;
        }
        receiver=cachedTimeSubsystem;
    }
    return receiver?reinterpret_cast<ProviderFn>(g_Engine->GetModuleBase()+getTimeProviderOffset)(receiver):0;
}

std::string IdentityPath(){return RuntimePaths::Environment("DW_RESOURCE_IDENTITIES",RuntimePaths::WebFile("resource-identities.tsv"));}
std::string SavePath(){return RuntimePaths::Environment("DW_WORLD_SAVE",RuntimePaths::GameSavedRoot()+"/SaveGames/Emberlands TEST.sav");}
std::string NormalizeGuid(std::string s){
    for(char& c:s)c=static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    size_t colon=s.find(':');
    std::string head=s.substr(0,colon),hex;
    for(char c:head){if(c=='-')continue;if(!std::isxdigit(static_cast<unsigned char>(c)))return s;hex.push_back(c);}
    if(hex.size()!=32)return s;
    std::string canonical=hex.substr(0,8)+"-"+hex.substr(8,8)+"-"+hex.substr(16,8)+"-"+hex.substr(24,8);
    return colon==std::string::npos?canonical:canonical+s.substr(colon);
}

double ProfileHours(const std::string& profile){
    auto h=profile.rfind('_');
    if(h==std::string::npos||profile.back()!='H')return 0;
    try{return std::stod(profile.substr(h+1,profile.size()-h-2));}catch(...){return 0;}
}

bool LoadIdentities(){
    std::ifstream in(IdentityPath());if(!in)return false;
    std::string line;std::getline(in,line);int count=0;
    while(std::getline(in,line)){
        std::istringstream row(line);std::string key,id,type,profile,icon;
        if(!std::getline(row,key,'\t')||!std::getline(row,id,'\t')||!std::getline(row,type,'\t')||
           !std::getline(row,profile,'\t'))continue;
        identities.emplace(NormalizeGuid(key),Identity{id,type,profile});++count;
        if(type=="irit"||type=="corpse-cotton")++actionableCatalogNodes;
        else if(type=="desert-weed"||type=="barrel-cactus"||type=="bunny-cactus"||type=="fibrous-pipe-cactus")++foliageCatalogNodes;
    }
    catalogNodes=count;return count>0;
}

struct R { const std::vector<unsigned char>* b;size_t p=0,e=0;R(const std::vector<unsigned char>&v,size_t a,size_t z):b(&v),p(a),e(z){}bool take(void*out,size_t n){if(p+n>e)return false;std::memcpy(out,b->data()+p,n);p+=n;return true;}template<class T>bool num(T&v){return take(&v,sizeof(v));}bool str(std::string&s){int32_t n;if(!num(n)||n<0||n>100000||p+static_cast<size_t>(n)>e)return false;s.assign(reinterpret_cast<const char*>(b->data()+p),n?n-1:0);p+=n;return true;}bool chunk(std::string&m,R&c){char x[4];uint32_t n;if(!take(x,4)||!num(n)||p+n>e)return false;m.assign(x,4);c=R(*b,p,p+n);p+=n;return true;}};
struct PropDef{uint32_t name,prefix;uint16_t type;};
struct ClassDef{std::vector<PropDef> props;};
std::string GuidBytes(const unsigned char* p){char s[37];uint32_t a,b,c,d;std::memcpy(&a,p,4);std::memcpy(&b,p+4,4);std::memcpy(&c,p+8,4);std::memcpy(&d,p+12,4);std::snprintf(s,sizeof(s),"%08X-%08X-%08X-%08X",a,b,c,d);return s;}
bool StringArray(R&r,std::vector<std::string>&out){int32_t n;if(!r.num(n)||n<0||n>1000000)return false;out.resize(n);for(auto&s:out)if(!r.str(s))return false;return true;}
void ParseMeta(R r,std::vector<std::string>&properties,std::vector<ClassDef>&defs){std::string m;R c(*r.b,0,0);while(r.chunk(m,c)){if(m=="PNIX")StringArray(c,properties);else if(m=="CLST"){std::string cm;R d(*r.b,0,0);while(c.chunk(cm,d)){if(cm=="CDVE"){unsigned char version=0;if(!d.num(version))continue;R nested(*d.b,0,0);if(!d.chunk(cm,nested))continue;d=nested;}if(cm=="CDEF"){std::string name;uint16_t n;if(!d.str(name)||!d.num(n))continue;ClassDef cd;for(int i=0;i<n;++i){PropDef p{};if(!d.num(p.name)||!d.num(p.prefix)||!d.num(p.type))break;cd.props.push_back(p);}defs.push_back(std::move(cd));}}}}}
bool ParseNestedResource(R r,uint32_t classId,const std::vector<std::string>&names,const std::vector<ClassDef>&defs,int&quantity,std::int64_t&changed){
    if(classId>=defs.size())return false;int32_t n=0;if(!r.num(n)||n<0||n>10000)return false;std::vector<uint32_t> offsets(n);for(auto&o:offsets)if(!r.num(o))return false;int32_t size=0;if(!r.num(size)||size<0||r.p+size>r.e)return false;const unsigned char*data=r.b->data()+r.p;const auto&pd=defs[classId].props;
    for(int i=0;i<n&&i<(int)pd.size();++i){size_t a=offsets[i];if(a==0xffffffff||a>(size_t)size||pd[i].name>=names.size())continue;size_t z=size;for(int j=i+1;j<n;++j)if(offsets[j]!=0xffffffff&&offsets[j]>=a){z=offsets[j];break;}if(a>z||z>(size_t)size)continue;const std::string&name=names[pd[i].name];if(name=="ResourcesAvailable"&&z-a>=4)std::memcpy(&quantity,data+a,4);else if(name=="PersistedTimeOfLastResourceChange"&&z-a>=8)std::memcpy(&changed,data+a,8);}
    return quantity>=0;
}
void ParseProps(const std::string&actor,uint32_t classId,R r,const std::vector<std::string>&names,const std::vector<ClassDef>&defs){
    if(classId>=defs.size())return;int32_t n;if(!r.num(n)||n<0||n>10000)return;std::vector<uint32_t> offsets(n);for(auto&o:offsets)if(!r.num(o))return;int32_t size;if(!r.num(size)||size<0||r.p+size>r.e)return;const unsigned char* data=r.b->data()+r.p;
    int quantity=-1,index=-1;std::int64_t changed=0;std::string guid;
    const auto& pd=defs[classId].props;for(int i=0;i<n&&i<(int)pd.size();++i){size_t a=offsets[i];if(a==0xffffffff||a>(size_t)size||pd[i].name>=names.size())continue;size_t z=size;for(int j=i+1;j<n;++j)if(offsets[j]!=0xffffffff&&offsets[j]>=a){z=offsets[j];break;}if(a>z||z>(size_t)size)continue;const std::string&name=names[pd[i].name];const unsigned char*v=data+a;size_t len=z-a;if(name=="RespawnComponent"&&len>=12){R nested(*r.b,r.p+a,r.p+z);uint32_t nestedClass=0;if(!nested.num(nestedClass))continue;ParseNestedResource(nested,nestedClass,names,defs,quantity,changed);}else if(name=="ResourcesAvailable"&&len>=4)std::memcpy(&quantity,v,4);else if(name=="PersistedTimeOfLastResourceChange"&&len>=8)std::memcpy(&changed,v,8);else if(name=="Index"&&len>=4)std::memcpy(&index,v,4);else if(name=="ISMCGuid"&&len>=16)guid=GuidBytes(v);}
    if(quantity!=0)return; // Missing/default and positive values are available.
    std::string key=NormalizeGuid(actor);if(!guid.empty()&&index>=0)key=guid+":"+std::to_string(index);
    auto it=identities.find(key);if(it!=identities.end())persisted[it->second.id]={quantity,changed};
}
void ParseActors(R r,int version,const std::vector<std::string>&names,const std::vector<ClassDef>&defs){std::string m;R a(*r.b,0,0);while(r.chunk(m,a))if(m=="NOBJ"){uint32_t classId;if(!a.num(classId))continue;std::string actor;if(!a.str(actor))continue;int32_t versions;if(!a.num(versions)||versions<0||versions>10000)continue;a.p+=static_cast<size_t>(versions)*4+8;if(a.p>a.e)continue;std::string cm;R c(*a.b,0,0);while(a.chunk(cm,c))if(cm=="PROP")ParseProps(actor,classId,c,names,defs);}}
void RefreshPersisted(){struct stat st{};std::string path=SavePath();if(stat(path.c_str(),&st)!=0||st.st_mtime==saveMtime)return;std::ifstream in(path,std::ios::binary);if(!in)return;std::vector<unsigned char>b((std::istreambuf_iterator<char>(in)),{});if(b.size()<16)return;R root(b,0,b.size()),save(b,0,0);std::string magic;if(!root.chunk(magic,save)||magic!="SAVE")return;int version=8;std::unordered_map<std::string,std::pair<int,std::int64_t>> old;persisted.swap(old);std::string m;R c(b,0,0);while(save.chunk(m,c)){if(m=="INFO"){uint16_t v;if(c.num(v))version=v;}else if(m=="LVLS"){std::string lm;R level(b,0,0);while(c.chunk(lm,level))if(lm=="LEVL"){std::string levelName;if(!level.str(levelName))continue;if(version>=4)level.p+=8;if(version<=6)level.p+=8;std::vector<std::string>names;std::vector<ClassDef>defs;R actors(b,0,0);bool haveActors=false;std::string x;R y(b,0,0);while(level.chunk(x,y)){if(x=="META")ParseMeta(y,names,defs);else if(x=="LATS"){actors=y;haveActors=true;}}if(haveActors)ParseActors(actors,version,names,defs);}}}saveMtime=st.st_mtime;persistedNodes=static_cast<int>(persisted.size());}

std::string KeyForActor(uintptr_t actor){
    if(!actor)return {};
    std::string actorName=g_Engine->GetObjectName(actor);
    auto direct=identities.find(NormalizeGuid(actorName));if(direct!=identities.end())return direct->first;
    // AWorldActor::GetFoliageDepictableComponent_Implementation() is the
    // authoritative bridge from a transient actor representation back to its
    // persistent foliage instance.  FFoliageItemIdentifier is copied into the
    // returned component at +0xe8: FGuid ISMCGuid followed by int32 Index at
    // +0xf8 (verified from SetFoliageDepiction in this exact executable).
    using GetDepictableFn=uintptr_t(*)(uintptr_t);
    uintptr_t base=g_Engine->GetModuleBase();
    uintptr_t identityObject=reinterpret_cast<GetDepictableFn>(base+getFoliageDepictableOffset)(actor);
    constexpr int guidOffset=0xe8,indexOffset=0xf8;
    if(!identityObject||!Mem::Readable(reinterpret_cast<void*>(identityObject+guidOffset),16)||
       !Mem::Readable(reinterpret_cast<void*>(identityObject+indexOffset),4)){
        static std::atomic<int> samples{0};if(samples.fetch_add(1)<12)LogMessage("Resource identity unresolved actor="+actorName+
            " nativeDepictable="+std::to_string(identityObject));
        return {};
    }
    std::string guid=NormalizeGuid(g_Engine->ReadGuid(identityObject+guidOffset));
    int index=Mem::ReadI32(identityObject+indexOffset);
    std::string key=guid+":"+std::to_string(index);
    if(!identities.count(key)){static std::atomic<int> samples{0};if(samples.fetch_add(1)<12)
        LogMessage("Resource identity absent key="+key+" actor="+actorName);return {};}
    return key;
}
std::string KeyFor(uintptr_t component){return KeyForActor(Mem::ReadPtr(component+Offsets::UObject_Outer));}

void WriteJump(unsigned char* at,uintptr_t destination){at[0]=0xff;at[1]=0x25;std::memset(at+2,0,4);std::memcpy(at+6,&destination,8);}
void CaptureCollection(uintptr_t actor){
    std::string key=KeyForActor(actor);if(key.empty()){++unresolvedHookCalls;return;}auto meta=identities.find(key);if(meta==identities.end()){++unresolvedHookCalls;return;}
    uintptr_t component=Mem::ReadPtr(actor+0x408);if(!component||!Mem::Readable(reinterpret_cast<void*>(component+0x178),8)){++unresolvedHookCalls;return;}
    std::int64_t changed=0;std::memcpy(&changed,reinterpret_cast<void*>(component+0x178),8);
    using IntervalFn=std::int64_t(*)(uintptr_t,uintptr_t);
    uintptr_t base=g_Engine->GetModuleBase();uintptr_t provider=ResolveTimeProvider(component);
    std::int64_t interval=provider?reinterpret_cast<IntervalFn>(base+getRespawnIntervalOffset)(component,provider):0;
    if(changed<=0||interval<=0){++unresolvedHookCalls;return;}
    pthread_mutex_lock(&stateMutex);bool was=eventDepleted.count(meta->second.id)>0;eventDepleted[meta->second.id]={changed,changed+interval};const double rate=tickRate.load();const auto current=latestGameTicks.load();const double remaining=rate>0&&current>0?std::max(0.0,double(changed+interval-current)/10000000.0/rate):0;const ResourceStateInfo state{meta->second.id,false,remaining,"live-event"};states[meta->second.id]=state;if(resourceScan.active)resourceScan.next[meta->second.id]=state;updatedAt=std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();pthread_mutex_unlock(&stateMutex);
    if(!was)++collections;LogMessage("Resource collection id="+meta->second.id+" type="+meta->second.type+" actor="+g_Engine->GetObjectName(actor));
}
void CaptureCollectionComponent(uintptr_t component){
    if(!component){++unresolvedHookCalls;return;}
    std::string key=KeyFor(component);auto meta=identities.find(key);
    if(meta==identities.end()){++unresolvedHookCalls;return;}
    if(!Mem::Readable(reinterpret_cast<void*>(component+0x178),8)){++unresolvedHookCalls;return;}
    std::int64_t changed=0;std::memcpy(&changed,reinterpret_cast<void*>(component+0x178),8);
    using IntervalFn=std::int64_t(*)(uintptr_t,uintptr_t);
    uintptr_t base=g_Engine->GetModuleBase();uintptr_t provider=ResolveTimeProvider(component);
    std::int64_t interval=provider?reinterpret_cast<IntervalFn>(base+getRespawnIntervalOffset)(component,provider):0;
    if(changed<=0||interval<=0){++unresolvedHookCalls;return;}
    pthread_mutex_lock(&stateMutex);bool was=eventDepleted.count(meta->second.id)>0;eventDepleted[meta->second.id]={changed,changed+interval};const double rate=tickRate.load();const auto current=latestGameTicks.load();const double remaining=rate>0&&current>0?std::max(0.0,double(changed+interval-current)/10000000.0/rate):0;const ResourceStateInfo state{meta->second.id,false,remaining,"live-event"};states[meta->second.id]=state;if(resourceScan.active)resourceScan.next[meta->second.id]=state;updatedAt=std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();pthread_mutex_unlock(&stateMutex);
    if(!was)++collections;
    if(!was)LogMessage("Resource amount depleted id="+meta->second.id+" type="+meta->second.type+
        " actor="+g_Engine->GetObjectName(Mem::ReadPtr(component+Offsets::UObject_Outer)));
}
void HookSetUnavailable(uintptr_t actor,uintptr_t player){
    ++rawHookCalls;
    bool before=actor&&Mem::Readable(reinterpret_cast<void*>(actor+0x444),1)&&Mem::ReadU8(actor+0x444)!=0;
    originalSetUnavailable(actor,player);
    const auto started=std::chrono::steady_clock::now();
    bool after=actor&&Mem::Readable(reinterpret_cast<void*>(actor+0x444),1)&&Mem::ReadU8(actor+0x444)!=0;
    const auto elapsed=RecordHookDuration(started);if(!before&&after)QueueRawResourceEvent(RawResourceEventKind::ActorDepleted,actor,elapsed,player);
}
bool InstallSetUnavailableHook(uintptr_t moduleBase,uintptr_t moduleEnd){
    const uintptr_t offset=setUnavailableOffset;constexpr size_t bytes=15;
    const unsigned char sig[]={0x41,0x57,0x41,0x56,0x41,0x54,0x53,0x50,0x80,0xbf,0x70,0x01,0x00,0x00,0x03};
    if(moduleBase+offset+sizeof(sig)>moduleEnd||std::memcmp(reinterpret_cast<void*>(moduleBase+offset),sig,sizeof(sig)))return false;
    auto* target=reinterpret_cast<unsigned char*>(moduleBase+offset);auto* trampoline=static_cast<unsigned char*>(mmap(nullptr,64,PROT_READ|PROT_WRITE|PROT_EXEC,MAP_PRIVATE|MAP_ANONYMOUS,-1,0));if(trampoline==MAP_FAILED)return false;
    std::memcpy(trampoline,target,bytes);WriteJump(trampoline+bytes,reinterpret_cast<uintptr_t>(target+bytes));originalSetUnavailable=reinterpret_cast<SetUnavailableFn>(trampoline);
    long ps=sysconf(_SC_PAGESIZE);uintptr_t page=reinterpret_cast<uintptr_t>(target)&~(uintptr_t(ps)-1);if(mprotect(reinterpret_cast<void*>(page),size_t(ps),PROT_READ|PROT_WRITE|PROT_EXEC)!=0)return false;
    unsigned char patch[bytes];std::memset(patch,0x90,sizeof(patch));WriteJump(patch,reinterpret_cast<uintptr_t>(&HookSetUnavailable));std::memcpy(target,patch,sizeof(patch));__builtin___clear_cache(reinterpret_cast<char*>(target),reinterpret_cast<char*>(target+bytes));mprotect(reinterpret_cast<void*>(page),size_t(ps),PROT_READ|PROT_EXEC);return true;
}

void HookSetResourceAmount(uintptr_t component,int amount){
    ++rawHookCalls;
    int before=component&&Mem::Readable(reinterpret_cast<void*>(component+0x170),4)?Mem::ReadI32(component+0x170):-1;
    originalSetResourceAmount(component,amount);
    const auto started=std::chrono::steady_clock::now();
    int after=component&&Mem::Readable(reinterpret_cast<void*>(component+0x170),4)?Mem::ReadI32(component+0x170):-1;
    const auto elapsed=RecordHookDuration(started);if(before>0&&after<=0)QueueRawResourceEvent(RawResourceEventKind::ComponentDepleted,component,elapsed);
}
bool InstallSetResourceAmountHook(uintptr_t moduleBase,uintptr_t moduleEnd){
    const uintptr_t offset=setResourceAmountOffset;constexpr size_t bytes=15;
    const unsigned char sig[]={0x41,0x56,0x53,0x50,0x48,0x89,0xfb,0x8b,0x87,0xdc,0x00,0x00,0x00,0x39,0xc6};
    if(moduleBase+offset+sizeof(sig)>moduleEnd||std::memcmp(reinterpret_cast<void*>(moduleBase+offset),sig,sizeof(sig)))return false;
    auto* target=reinterpret_cast<unsigned char*>(moduleBase+offset);auto* trampoline=static_cast<unsigned char*>(mmap(nullptr,64,PROT_READ|PROT_WRITE|PROT_EXEC,MAP_PRIVATE|MAP_ANONYMOUS,-1,0));if(trampoline==MAP_FAILED)return false;
    std::memcpy(trampoline,target,bytes);WriteJump(trampoline+bytes,reinterpret_cast<uintptr_t>(target+bytes));originalSetResourceAmount=reinterpret_cast<SetResourceAmountFn>(trampoline);
    long ps=sysconf(_SC_PAGESIZE);uintptr_t page=reinterpret_cast<uintptr_t>(target)&~(uintptr_t(ps)-1);if(mprotect(reinterpret_cast<void*>(page),size_t(ps),PROT_READ|PROT_WRITE|PROT_EXEC)!=0)return false;
    unsigned char patch[bytes];std::memset(patch,0x90,sizeof(patch));WriteJump(patch,reinterpret_cast<uintptr_t>(&HookSetResourceAmount));std::memcpy(target,patch,sizeof(patch));__builtin___clear_cache(reinterpret_cast<char*>(target),reinterpret_cast<char*>(target+bytes));mprotect(reinterpret_cast<void*>(page),size_t(ps),PROT_READ|PROT_EXEC);return true;
}

void HookRespawnElapsed(uintptr_t component){
    ++rawHookCalls;
    int before=component&&Mem::Readable(reinterpret_cast<void*>(component+0x170),4)?Mem::ReadI32(component+0x170):-1;
    originalRespawnElapsed(component);
    const auto started=std::chrono::steady_clock::now();
    int after=component&&Mem::Readable(reinterpret_cast<void*>(component+0x170),4)?Mem::ReadI32(component+0x170):-1;
    const auto elapsed=RecordHookDuration(started);if(before<=0&&after>0)QueueRawResourceEvent(RawResourceEventKind::ComponentRespawned,component,elapsed);
}
bool InstallRespawnElapsedHook(uintptr_t moduleBase,uintptr_t moduleEnd){
    const uintptr_t offset=respawnElapsedOffset;constexpr size_t bytes=18;
    const unsigned char sig[]={0x55,0x41,0x57,0x41,0x56,0x41,0x54,0x53,0x48,0x83,0xec,0x10,0x8b,0x87,0xd8,0x00,0x00,0x00};
    if(moduleBase+offset+sizeof(sig)>moduleEnd||std::memcmp(reinterpret_cast<void*>(moduleBase+offset),sig,sizeof(sig)))return false;
    auto* target=reinterpret_cast<unsigned char*>(moduleBase+offset);auto* trampoline=static_cast<unsigned char*>(mmap(nullptr,64,PROT_READ|PROT_WRITE|PROT_EXEC,MAP_PRIVATE|MAP_ANONYMOUS,-1,0));if(trampoline==MAP_FAILED)return false;
    std::memcpy(trampoline,target,bytes);WriteJump(trampoline+bytes,reinterpret_cast<uintptr_t>(target+bytes));originalRespawnElapsed=reinterpret_cast<RespawnElapsedFn>(trampoline);
    long ps=sysconf(_SC_PAGESIZE);uintptr_t page=reinterpret_cast<uintptr_t>(target)&~(uintptr_t(ps)-1);if(mprotect(reinterpret_cast<void*>(page),size_t(ps),PROT_READ|PROT_WRITE|PROT_EXEC)!=0)return false;
    unsigned char patch[bytes];std::memset(patch,0x90,sizeof(patch));WriteJump(patch,reinterpret_cast<uintptr_t>(&HookRespawnElapsed));std::memcpy(target,patch,sizeof(patch));__builtin___clear_cache(reinterpret_cast<char*>(target),reinterpret_cast<char*>(target+bytes));mprotect(reinterpret_cast<void*>(page),size_t(ps),PROT_READ|PROT_EXEC);return true;
}

}

double UpdateMeasuredGameClockRate(double priorRate,std::int64_t elapsedGameTicks,double elapsedRealSeconds){
    if(elapsedGameTicks<0||!std::isfinite(elapsedRealSeconds)||elapsedRealSeconds<=0)return priorRate;
    const double measured=elapsedGameTicks/10000000.0/elapsedRealSeconds;
    if(!std::isfinite(measured)||measured>=10000.0)return priorRate;
    if(measured<0.001)return 0;
    return priorRate>0?priorRate*0.85+measured*0.15:measured;
}

bool InitializeResourceTracking(uintptr_t moduleBase,uintptr_t moduleEnd){
    if(!LoadIdentities()){compatibility=-1;return false;}
    // This entry is accepted only for the exact analyzed executable. The
    // updater/analyzer must add a new fingerprint before a later build uses it.
    const char* exe=std::getenv("DW_ORIGINAL_EXE");
    const std::string executable=exe&&*exe?exe:RuntimePaths::ServerRoot()+"/RSDragonwilds/Binaries/Linux/RSDragonwildsServer-Linux-Shipping.zanik-original";
    std::string hash=FileSha256(executable.c_str());
    if(hash=="d26de80fd01f4b73b726aed855a20377996b7c140ff33f1ec40150c0dd73b259"){
        timeProviderNeedsSubsystem=false;
        getGameTimeOffset=0x0A731740;getTimeProviderOffset=0x0A73A560;
        getRespawnIntervalOffset=0x0AA6D280;getFoliageDepictableOffset=0x0AA8EA90;
        setUnavailableOffset=0x0AA4CC50;setResourceAmountOffset=0x0AA4BD70;respawnElapsedOffset=0x0AA6DD50;
    }else if(hash=="d12145525d88f3828ebeac0aec3865f5b16ba4771861efe65108754a89bbb090"){
        timeProviderNeedsSubsystem=true;
        getGameTimeOffset=0x0A732D20;getTimeProviderOffset=0x0A7E6A90;
        getRespawnIntervalOffset=0x0AA70A90;getFoliageDepictableOffset=0x0AA922B0;
        setUnavailableOffset=0x0AA50460;setResourceAmountOffset=0x0AA4F580;respawnElapsedOffset=0x0AA71560;
    }else if(hash=="445f8b65047d208adb04a3cb03bc7706f069306d814172ae29caec16c15bec7e"||hash=="ef39023c04e1bde785907e13b9b64d72600455f1633bbfe231f639d31d98cfac"){
        timeProviderNeedsSubsystem=true;
        getGameTimeOffset=0x0A732D10;getTimeProviderOffset=0x0A7E71F0;
        getRespawnIntervalOffset=0x0AA711F0;getFoliageDepictableOffset=0x0AA92A10;
        setUnavailableOffset=0x0AA50BC0;setResourceAmountOffset=0x0AA4FCE0;respawnElapsedOffset=0x0AA71CC0;
    }else if(hash=="49512c0ee933c3ee4dda5fb5c2f0a25e66d7f26e34795c45b91b5c1d12b61b0c"){
        timeProviderNeedsSubsystem=true;
        getGameTimeOffset=0x0A77BEE0;getTimeProviderOffset=0x0A839B70;
        getRespawnIntervalOffset=0x0AACCEA0;getFoliageDepictableOffset=0x0AAEEB20;
        setUnavailableOffset=0x0AAABF70;setResourceAmountOffset=0x0AAAB090;respawnElapsedOffset=0x0AACD970;
    }else if(hash=="b5017cf67bc6127dbec443e00fed1f818e82afd5d6d17f912d959a1dd87d5687"){
        timeProviderNeedsSubsystem=true;
        getGameTimeOffset=0x0A77BFF0;getTimeProviderOffset=0x0A839C80;
        getRespawnIntervalOffset=0x0AACD130;getFoliageDepictableOffset=0x0AAEEDB0;
        setUnavailableOffset=0x0AAAC200;setResourceAmountOffset=0x0AAAB320;respawnElapsedOffset=0x0AACDC00;
    }else if(hash=="45cd13cf24658153f4c53777816ffa38ac893a471ebdf5d9c9768f7e4e74c966"){
        timeProviderNeedsSubsystem=true;
        getGameTimeOffset=0x0A77C8E0;getTimeProviderOffset=0x0A83A570;
        getRespawnIntervalOffset=0x0AACDA50;getFoliageDepictableOffset=0x0AAEF6D0;
        setUnavailableOffset=0x0AAACB20;setResourceAmountOffset=0x0AAABC40;respawnElapsedOffset=0x0AACE520;
    }else if(hash=="a350b998ea38955345619b7adde31aa9e29db629e9e9aa32287013957eb41901"){
        timeProviderNeedsSubsystem=true;
        getGameTimeOffset=0x0A7A9950;getTimeProviderOffset=0x0A8509F0;
        getRespawnIntervalOffset=0x0AAEE990;getFoliageDepictableOffset=0x0AB0C920;
        setUnavailableOffset=0x0AAC30C0;setResourceAmountOffset=0x0AAEDE10;respawnElapsedOffset=0x0AAEF460;
    }else if(hash=="1b779665120e8d0df8c5d481969cdb2d1a4df52441cd5569ad2309ec62da1d26"){
        timeProviderNeedsSubsystem=true;
        getGameTimeOffset=0x0A7A99F0;getTimeProviderOffset=0x0A850A90;
        getRespawnIntervalOffset=0x0AAEE900;getFoliageDepictableOffset=0x0AB0C890;
        setUnavailableOffset=0x0AAC3030;setResourceAmountOffset=0x0AAEDD80;respawnElapsedOffset=0x0AAEF3D0;
    }else{compatibility=-2;return false;}
    if(moduleBase+getGameTimeOffset<moduleBase||moduleBase+getGameTimeOffset>=moduleEnd){compatibility=-3;return false;}
    if(!InstallSetUnavailableHook(moduleBase,moduleEnd)||!InstallSetResourceAmountHook(moduleBase,moduleEnd)||!InstallRespawnElapsedHook(moduleBase,moduleEnd)){compatibility=-4;return false;}
    compatibility=1;return true;
}

bool RefreshResourceStates(){
    if(compatibility.load()!=1||!g_Engine||!g_Engine->IsInitialized())return false;
    auto read=rawResourceRead.load(std::memory_order_relaxed);const auto write=rawResourceWrite.load(std::memory_order_acquire);
    while(read<write){
        const auto event=rawResourceEvents[read%RawResourceEventCapacity];++read;
        if(!event.object||g_Engine->GetObjectIdentity(event.object)!=event.identity){++unresolvedHookCalls;continue;}
        std::string playerGuid;if(event.player&&event.playerIdentity&&g_Engine->GetObjectIdentity(event.player)==event.playerIdentity)for(const auto& player:g_Engine->GetAllPlayers())if(player.pawnPtr==event.player||player.controllerPtr==event.player||player.playerStatePtr==event.player){playerGuid=player.characterGuid;break;}const char* operation=event.kind==RawResourceEventKind::ActorDepleted?"resource collected":event.kind==RawResourceEventKind::ComponentDepleted?"resource depleted":"resource respawned";PerformanceTrace::Record("resources",operation,event.hookMicros,playerGuid.empty()?nullptr:playerGuid.c_str(),playerGuid.empty()?0:'g',"game");
        if(event.kind==RawResourceEventKind::ActorDepleted)CaptureCollection(event.object);
        else if(event.kind==RawResourceEventKind::ComponentDepleted)CaptureCollectionComponent(event.object);
        else{std::string key=KeyFor(event.object);auto meta=identities.find(key);if(meta==identities.end()){++unresolvedHookCalls;continue;}pthread_mutex_lock(&stateMutex);bool erased=eventDepleted.erase(meta->second.id)>0;states.erase(meta->second.id);if(resourceScan.active)resourceScan.next.erase(meta->second.id);updatedAt=std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();pthread_mutex_unlock(&stateMutex);if(erased)++respawns;}
    }
    rawResourceRead.store(read,std::memory_order_release);
    using GetGameTimeFn=std::int64_t(*)(uintptr_t);
    auto getTime=reinterpret_cast<GetGameTimeFn>(g_Engine->GetModuleBase()+getGameTimeOffset);
    std::int64_t nowTicks=getTime(g_Engine->GetWorld());
    auto wall=std::chrono::steady_clock::now();
    latestGameTicks=nowTicks;
    if(!sampleTicks){sampleTicks=nowTicks;sampleWall=wall;}
    else if(nowTicks>=sampleTicks){
        double seconds=std::chrono::duration<double>(wall-sampleWall).count();
        double priorRate=tickRate.load();constexpr double minimum=4.0;
        if(seconds>=minimum){
            tickRate=UpdateMeasuredGameClockRate(priorRate,nowTicks-sampleTicks,seconds);
            sampleTicks=nowTicks;sampleWall=wall;
        }
    }else{sampleTicks=nowTicks;sampleWall=wall;tickRate=0;}
    if(!resourceScan.active){
    if(nextResourceReconciliation.time_since_epoch().count()!=0&&wall<nextResourceReconciliation)return false;
    resourceScan={};resourceScan.active=true;resourceScan.nowTicks=nowTicks;resourceScan.count=g_Engine->GetObjectCount();resourceScan.next.reserve(states.size()+eventDepleted.size()+64);
    RefreshPersisted();
    for(const auto& saved:persisted){
        auto meta=std::find_if(identities.begin(),identities.end(),[&](const auto& x){return x.second.id==saved.first;});
        if(meta==identities.end())continue;double remaining=0;double hours=ProfileHours(meta->second.profile);
        const double rate=tickRate.load();if(saved.second.second>0&&nowTicks>=saved.second.second){double game=hours*3600.0-(nowTicks-saved.second.second)/10000000.0;remaining=rate>0?std::max(0.0,game/rate):0;}
        resourceScan.next[saved.first]={saved.first,false,remaining,"persisted"};
    }
    pthread_mutex_lock(&stateMutex);
    auto captured=eventDepleted;
    pthread_mutex_unlock(&stateMutex);
    for(const auto& event:captured){
        auto meta=std::find_if(identities.begin(),identities.end(),[&](const auto& x){return x.second.id==event.first;});if(meta==identities.end())continue;
        const double rate=tickRate.load();double remaining=0;if(event.second.deadline>0){double game=(event.second.deadline-nowTicks)/10000000.0;remaining=rate>0?std::max(0.0,game/rate):0;}
        resourceScan.next[event.first]={event.first,false,remaining,"live-event"};
    }
    }
    const auto sliceStarted=std::chrono::steady_clock::now(),sliceDeadline=sliceStarted+std::chrono::microseconds(300);++scanSlices;
    int inspected=0;
    for(;resourceScan.cursor<resourceScan.count&&inspected<512&&std::chrono::steady_clock::now()<sliceDeadline;++resourceScan.cursor,++inspected){
        const int i=resourceScan.cursor;
        uintptr_t c=g_Engine->GetObjectByIndex(i);if(!c||!g_Engine->IsA(c,"ResourceRespawnComponent")||
           g_Engine->GetObjectName(c).rfind("Default__",0)==0){if(timeProviderNeedsSubsystem&&!cachedTimeSubsystem&&c&&g_Engine->IsA(c,"InGameTimeSubsystem")&&g_Engine->GetObjectName(c).rfind("Default__",0)!=0)cachedTimeSubsystem=c;continue;}
        uintptr_t owner=Mem::ReadPtr(c+Offsets::UObject_Outer);
        if(!owner||g_Engine->GetObjectName(owner).rfind("Default__",0)==0)continue;
        ++resourceScan.loaded;std::string key=KeyFor(c);if(key.empty()){++resourceScan.unresolved;continue;}
        auto meta=identities.find(key);if(meta==identities.end()){++resourceScan.unresolved;continue;}++resourceScan.matched;
        if(meta->second.type=="irit"||meta->second.type=="corpse-cotton")++resourceScan.matchedActionable;
        else if(meta->second.type=="desert-weed"||meta->second.type=="barrel-cactus"||meta->second.type=="bunny-cactus"||meta->second.type=="fibrous-pipe-cactus")++resourceScan.matchedFoliage;
        if(!Mem::Readable(reinterpret_cast<void*>(c+0x170),0x18)){++rejected;continue;}
        int quantity=Mem::ReadI32(c+0x170);std::int64_t changed=0;std::memcpy(&changed,reinterpret_cast<void*>(c+0x178),8);
        static std::atomic<int> liveSamples{0};if(liveSamples.fetch_add(1)<30)LogMessage("Resource live match id="+meta->second.id+
            " type="+meta->second.type+" quantity="+std::to_string(quantity)+" changed="+std::to_string(changed)+
            " actor="+g_Engine->GetObjectName(owner));
        // Some legacy resource blueprints have no data-table row, but the
        // native resolver still supplies a runtime fallback interval. Sample
        // that authoritative value while the component is healthy instead of
        // inferring a duration from another resource with the same item type.
        if(meta->second.profile.empty()&&!loggedRuntimeIntervals.count(meta->second.type)){
            using IntervalFn=std::int64_t(*)(uintptr_t,uintptr_t);
            uintptr_t base=g_Engine->GetModuleBase();uintptr_t provider=ResolveTimeProvider(c);
            std::int64_t interval=provider?reinterpret_cast<IntervalFn>(base+getRespawnIntervalOffset)(c,provider):0;
            int replenishment=Mem::Readable(reinterpret_cast<void*>(c+0xd8),4)?Mem::ReadI32(c+0xd8):-1;
            int maximum=Mem::Readable(reinterpret_cast<void*>(c+0xdc),4)?Mem::ReadI32(c+0xdc):-1;
            loggedRuntimeIntervals.insert(meta->second.type);
            LogMessage("Resource runtime interval type="+meta->second.type+
                " ticks="+std::to_string(interval)+" gameHours="+
                std::to_string(interval>0?double(interval)/10000000.0/3600.0:0.0)+
                " replenishment="+std::to_string(replenishment)+" maximum="+std::to_string(maximum)+
                " actor="+g_Engine->GetObjectName(owner));
        }
        if(quantity<=0&&changed>0){
            using IntervalFn=std::int64_t(*)(uintptr_t,uintptr_t);
            uintptr_t base=g_Engine->GetModuleBase();uintptr_t provider=ResolveTimeProvider(c);
            std::int64_t interval=provider?reinterpret_cast<IntervalFn>(base+getRespawnIntervalOffset)(c,provider):0;
            if(interval>0){double game=(changed+interval-resourceScan.nowTicks)/10000000.0;const double rate=tickRate.load();resourceScan.next[meta->second.id]={meta->second.id,false,rate>0?std::max(0.0,game/rate):0,"live-component"};}
            else resourceScan.next[meta->second.id]={meta->second.id,false,0,"live-component"};
        }else{
            resourceScan.next.erase(meta->second.id);
            pthread_mutex_lock(&stateMutex);eventDepleted.erase(meta->second.id);pthread_mutex_unlock(&stateMutex);
        }
    }
    scanObjects.fetch_add(inspected);const auto sliceMicros=static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-sliceStarted).count());auto priorSlice=maximumScanMicroseconds.load();while(sliceMicros>priorSlice&&!maximumScanMicroseconds.compare_exchange_weak(priorSlice,sliceMicros)){}if(sliceMicros>=1000)++slowScanSlices;
    if(resourceScan.cursor<resourceScan.count)return true;
    {
        pthread_mutex_lock(&stateMutex);states.swap(resourceScan.next);
        updatedAt=std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        pthread_mutex_unlock(&stateMutex);
    }
    loadedComponents=resourceScan.loaded;matchedNodes=resourceScan.matched;matchedActionableNodes=resourceScan.matchedActionable;matchedFoliageNodes=resourceScan.matchedFoliage;unresolvedNodes=resourceScan.unresolved;lastSuccess=updatedAt;resourceScan.active=false;nextResourceReconciliation=wall+std::chrono::seconds(30);return false;
}

bool ReadResourceStates(std::vector<ResourceStateInfo>& out,double& rate,std::int64_t& timestamp){
    if(compatibility.load()!=1)return false;
    pthread_mutex_lock(&stateMutex);
    out.clear();for(const auto& item:states)out.push_back(item.second);rate=tickRate.load();timestamp=updatedAt;
    pthread_mutex_unlock(&stateMutex);return true;
}

bool ReadAuthoritativeGameTime(std::int64_t& gameTicks,double& gameSecondsPerRealSecond){
    gameTicks=latestGameTicks.load();gameSecondsPerRealSecond=tickRate.load();
    return compatibility.load()==1&&gameTicks>0;
}

ResourceDiagnostics GetResourceDiagnostics(){return {compatibility.load(),catalogNodes.load(),actionableCatalogNodes.load(),foliageCatalogNodes.load(),loadedComponents.load(),matchedNodes.load(),matchedActionableNodes.load(),matchedFoliageNodes.load(),persistedNodes.load(),unresolvedNodes.load(),rejected.load(),collections.load(),respawns.load(),rawHookCalls.load(),unresolvedHookCalls.load(),rawResourceWrite.load()-rawResourceRead.load(),hookQueueDrops.load(),maximumHookMicroseconds.load(),slowHookCalls.load(),resourceScan.active,scanSlices.load(),scanObjects.load(),maximumScanMicroseconds.load(),slowScanSlices.load(),lastSuccess.load()};}

} // namespace DomEngine
