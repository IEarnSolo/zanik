#include "dom_engine.h"
#include "../utils/logger.h"
#include "../utils/memory.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <deque>
#include <dirent.h>
#include <fstream>
#include <memory>
#include <pthread.h>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

namespace DomEngine {
namespace {
bool ExpectedSha(const std::string& hash){return hash=="445f8b65047d208adb04a3cb03bc7706f069306d814172ae29caec16c15bec7e"||hash=="ef39023c04e1bde785907e13b9b64d72600455f1633bbfe231f639d31d98cfac";}
constexpr uintptr_t MoveItemOffset=0x0A6139B0;
constexpr uintptr_t RemoveFromSlotOffset=0x0A615490;
constexpr uintptr_t CanAddItemByDataOffset=0x0A612180;
constexpr uintptr_t CanAddItemsByDataOffset=0x0A612E40;
constexpr uintptr_t MoveAllItemsOffset=0x0A613A10;
constexpr uintptr_t RemoveGraveOffset=0x0A60E2B0;
constexpr uintptr_t GetGraveInventoryOffset=0x0A60CE00;
constexpr uintptr_t GetGraveOwnerOffset=0x0A60D430;
constexpr uintptr_t GetGraveRegistryOffset=0x0A60BC20;
constexpr uintptr_t RegisterStreamingProviderOffset=0x08707750;
constexpr uintptr_t UnregisterStreamingProviderOffset=0x087077A0;
constexpr uintptr_t GetStreamingSourcesOffset=0x07E3D200;
constexpr uintptr_t IsStreamingCompletedOffset=0x08707C30;
constexpr uintptr_t RestoreRuntimeSpawnedActorsOffset=0x09DF5E30;
constexpr uintptr_t RespawnActorOffset=0x09DE4FE0;
constexpr uintptr_t RestoreRuntimeSpawnedActorsFromLevelOffset=0x09DEDA60;
constexpr uintptr_t GetStreamingSourceOffset=0x07E2FEA0;
constexpr uintptr_t PlayerGetStreamingSourcesOffset=0x083552D0;
constexpr uintptr_t StreamingSourceCopyOffset=0x0870DA40;
constexpr uintptr_t StreamingSourceDestructorOffset=0x07E422C0;
constexpr size_t QueueCapacity=32;
constexpr uintptr_t RegistryGravesOffset=0x2E0;
constexpr int32_t GraveRecordSize=0x68;
using MoveItemFn=bool(*)(void*,int,void*,int,void*,int);
using RemoveFromSlotFn=bool(*)(void*,int,int,void*);
using CanAddItemByDataFn=bool(*)(void*,void*,int);
using CanAddItemsByDataFn=bool(*)(void*,const FUEArray*);
using MoveAllItemsFn=void(*)(void*,void*);
using RemoveGraveFn=void(*)(void*);
using GetGraveInventoryFn=void*(*)(void*);
using GetMaxStackSizeFn=int(*)(void*);
using RegisterStreamingProviderFn=void(*)(void*,void*);
using UnregisterStreamingProviderFn=bool(*)(void*,void*);
using GetStreamingSourcesFn=bool(*)(void*,void*);
using IsStreamingCompletedFn=bool(*)(void*,void*);
using RestoreRuntimeSpawnedActorsFn=void(*)(void*,const void*,const FUEArray*,const void*);
using GetStreamingSourceFn=bool(*)(void*,void*);
using StreamingSourceCopyFn=void(*)(void*,const void*);
using StreamingSourceDestructorFn=void(*)(void*);
using RespawnActorFn=void*(*)(void*,const void*,const void*,void*);
using RestoreRuntimeSpawnedActorsFromLevelFn=void(*)(void*,void*,const void*,const FString*);
MoveItemFn moveItem=nullptr; RemoveFromSlotFn removeFromSlot=nullptr;
CanAddItemByDataFn canAddItemByData=nullptr; CanAddItemsByDataFn canAddItemsByData=nullptr;
MoveAllItemsFn moveAllItems=nullptr; RemoveGraveFn removeGrave=nullptr;
GetGraveInventoryFn getGraveInventory=nullptr;
RegisterStreamingProviderFn registerStreamingProvider=nullptr;
UnregisterStreamingProviderFn unregisterStreamingProvider=nullptr;
GetStreamingSourcesFn getStreamingSources=nullptr;
IsStreamingCompletedFn isStreamingCompleted=nullptr;
RestoreRuntimeSpawnedActorsFn originalRestoreRuntimeSpawnedActors=nullptr;
GetStreamingSourceFn getStreamingSource=nullptr;
GetStreamingSourcesFn playerGetStreamingSources=nullptr;
StreamingSourceCopyFn copyStreamingSource=nullptr;
StreamingSourceDestructorFn destroyStreamingSource=nullptr;
RespawnActorFn originalRespawnActor=nullptr;
RestoreRuntimeSpawnedActorsFromLevelFn restoreRuntimeSpawnedActorsFromLevel=nullptr;

struct Lock{pthread_mutex_t* value;explicit Lock(pthread_mutex_t&v):value(&v){pthread_mutex_lock(value);}~Lock(){pthread_mutex_unlock(value);}};
struct Grave{uintptr_t actor=0,inventory=0;std::string id,ownerGuid;FVector3d location{};};
struct Request{
    std::string operation,adminGuid,adminId,ownerGuid,ownerId,graveId,itemId,session;
    int quantity=0;bool complete=false,success=false;std::atomic<bool> cancelled{false};std::string result;
    std::chrono::steady_clock::time_point queuedAt=std::chrono::steady_clock::now();
    pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;pthread_cond_t changed=PTHREAD_COND_INITIALIZER;
    ~Request(){pthread_cond_destroy(&changed);pthread_mutex_destroy(&mutex);}
};
pthread_mutex_t queueMutex=PTHREAD_MUTEX_INITIALIZER;std::deque<std::shared_ptr<Request>> requests;
std::atomic<int> compatibility{0};
std::atomic<int> inventoryCompatibility{0},registryCompatibility{0},streamingCompatibility{0},respawnCaptureCompatibility{0},persistenceCompatibility{0};
std::atomic<std::uint64_t> discovered{0},unresolved{0},queued{0},accepted{0},rejected{0},stale{0},maximumMicros{0};
pthread_mutex_t probeMutex=PTHREAD_MUTEX_INITIALIZER;std::unordered_set<std::uint64_t> probedGraves;
pthread_mutex_t savedPathMutex=PTHREAD_MUTEX_INITIALIZER;std::unordered_map<std::string,std::string> savedCellPaths;
std::atomic<bool> spudIndexCaptured{false};
std::unordered_map<std::string,std::chrono::steady_clock::time_point> graveRestoreAttempts;

// IWorldPartitionStreamingSourceProvider has two virtual entry points in this
// build. The engine's non-virtual wrapper constructs a complete, valid
// FWorldPartitionStreamingSource and calls entry 0 to let us supply its
// position. We deliberately leave every other field at the engine default.
alignas(16) unsigned char streamingSourceTemplate[0x160]{};bool haveStreamingSourceTemplate=false;
struct GraveStreamingProvider{void** vtable=nullptr;FVector3d location{};std::string id;std::uint64_t sourceName=0;std::uint64_t calls=0;bool completionLogged=false;};
bool GetGraveStreamingSource(GraveStreamingProvider*self,void*out){if(!self||!out)return false;if(haveStreamingSourceTemplate&&copyStreamingSource&&destroyStreamingSource){destroyStreamingSource(out);copyStreamingSource(out,streamingSourceTemplate);}std::memcpy(out,&self->sourceName,sizeof(self->sourceName));std::memcpy(reinterpret_cast<unsigned char*>(out)+8,&self->location,sizeof(self->location));if(++self->calls==1)LogMessage("Gravestone streaming source queried id="+self->id);return true;}
bool GetGraveStreamingSources(GraveStreamingProvider*self,void*out){return getStreamingSources&&self&&out?getStreamingSources(self,out):false;}
void* graveStreamingVtable[]={reinterpret_cast<void*>(&GetGraveStreamingSource),reinterpret_cast<void*>(&GetGraveStreamingSources)};
std::unordered_map<std::string,std::unique_ptr<GraveStreamingProvider>> graveProviders;
uintptr_t worldPartitionSubsystem=0;
uintptr_t gameplayObjectRegistry=0;

struct SpudBox{double minX=0,minY=0,minZ=0,maxX=0,maxY=0,maxZ=0;unsigned char valid=1,padding[7]{};};
static_assert(sizeof(SpudBox)==0x38,"FBox layout changed");
void WriteAbsoluteJump(unsigned char*at,uintptr_t destination){at[0]=0xff;at[1]=0x25;std::memset(at+2,0,4);std::memcpy(at+6,&destination,8);}

struct GraveRespawnCapture{
    char magic[16];std::uint32_t version,spawnedSize,metadataSize,reserved;
    std::uint64_t actor,level,state,spawnedPointer,metadataPointer,levelDataPointer;double location[3];char owner[64],actorName[128],levelName[128];
    unsigned char spawned[0x200],metadata[0x100],stateData[0x400],levelData[0x380];
};
void WriteGraveRespawnCapture(void*self,void*actor,void*level,const void*spawnedPointer,const void*metadataPointer,const unsigned char*spawned,const unsigned char*metadata){
    static std::atomic<bool> captured{false};bool expected=false;if(!captured.compare_exchange_strong(expected,true))return;
    GraveRespawnCapture value{};std::memcpy(value.magic,"ZANIKGRAVECAP2",14);value.version=2;value.spawnedSize=sizeof(value.spawned);value.metadataSize=sizeof(value.metadata);value.actor=reinterpret_cast<std::uint64_t>(actor);value.level=reinterpret_cast<std::uint64_t>(level);value.state=reinterpret_cast<std::uint64_t>(self);value.spawnedPointer=reinterpret_cast<std::uint64_t>(spawnedPointer);value.metadataPointer=reinterpret_cast<std::uint64_t>(metadataPointer);value.levelDataPointer=value.metadataPointer>=0x40?value.metadataPointer-0x40:0;std::memcpy(value.spawned,spawned,sizeof(value.spawned));std::memcpy(value.metadata,metadata,sizeof(value.metadata));if(self&&Mem::Readable(self,sizeof(value.stateData)))std::memcpy(value.stateData,self,sizeof(value.stateData));if(value.levelDataPointer&&Mem::Readable(reinterpret_cast<void*>(value.levelDataPointer),sizeof(value.levelData)))std::memcpy(value.levelData,reinterpret_cast<void*>(value.levelDataPointer),sizeof(value.levelData));
    const uintptr_t actorAddress=reinterpret_cast<uintptr_t>(actor),root=Mem::ReadPtr(actorAddress+Offsets::Actor_RootComponent);if(root&&Mem::Readable(reinterpret_cast<void*>(root+Offsets::SceneComp_RelativeLocation),sizeof(value.location)))std::memcpy(value.location,reinterpret_cast<void*>(root+Offsets::SceneComp_RelativeLocation),sizeof(value.location));
    std::string owner=g_Engine?g_Engine->ReadGuid(actorAddress+0x410):std::string(),actorName=g_Engine?g_Engine->GetObjectName(actorAddress):std::string(),levelName=(g_Engine&&level)?g_Engine->GetObjectName(reinterpret_cast<uintptr_t>(level)):std::string();std::snprintf(value.owner,sizeof(value.owner),"%s",owner.c_str());std::snprintf(value.actorName,sizeof(value.actorName),"%s",actorName.c_str());std::snprintf(value.levelName,sizeof(value.levelName),"%s",levelName.c_str());
    const char*path="/home/container/zanik-data/grave-respawn-capture.bin";int fd=open(path,O_WRONLY|O_CREAT|O_TRUNC,0600);if(fd>=0){const unsigned char*bytes=reinterpret_cast<const unsigned char*>(&value);size_t left=sizeof(value);while(left){ssize_t wrote=write(fd,bytes+(sizeof(value)-left),left);if(wrote<=0)break;left-=size_t(wrote);}fsync(fd);close(fd);LogMessage("Captured native gravestone respawn arguments actor="+actorName+" level="+levelName+" owner="+owner);}else LogMessage("Unable to write native gravestone respawn capture");
}
void* HookRespawnActor(void*self,const void*spawned,const void*metadata,void*level){
    unsigned char spawnedCopy[0x200]{},metadataCopy[0x100]{};const bool readable=spawned&&metadata&&Mem::Readable(spawned,sizeof(spawnedCopy))&&Mem::Readable(metadata,sizeof(metadataCopy));if(readable){std::memcpy(spawnedCopy,spawned,sizeof(spawnedCopy));std::memcpy(metadataCopy,metadata,sizeof(metadataCopy));}
    void*actor=originalRespawnActor?originalRespawnActor(self,spawned,metadata,level):nullptr;uintptr_t address=reinterpret_cast<uintptr_t>(actor);if(readable&&address&&g_Engine&&g_Engine->IsObjectLive(address)&&g_Engine->IsA(address,"Gravestone"))WriteGraveRespawnCapture(self,actor,level,spawned,metadata,spawnedCopy,metadataCopy);return actor;
}
bool InstallRespawnActorCaptureHook(uintptr_t moduleBase,uintptr_t moduleEnd){
    if(originalRespawnActor)return true;constexpr size_t bytes=17;const unsigned char sig[]={0x55,0x41,0x57,0x41,0x56,0x41,0x55,0x41,0x54,0x53,0x48,0x81,0xec,0x28,0x02,0x00,0x00};if(moduleBase+RespawnActorOffset+bytes>moduleEnd||std::memcmp(reinterpret_cast<void*>(moduleBase+RespawnActorOffset),sig,sizeof(sig)))return false;auto*target=reinterpret_cast<unsigned char*>(moduleBase+RespawnActorOffset);auto*trampoline=static_cast<unsigned char*>(mmap(nullptr,64,PROT_READ|PROT_WRITE|PROT_EXEC,MAP_PRIVATE|MAP_ANONYMOUS,-1,0));if(trampoline==MAP_FAILED)return false;std::memcpy(trampoline,target,bytes);WriteAbsoluteJump(trampoline+bytes,reinterpret_cast<uintptr_t>(target+bytes));originalRespawnActor=reinterpret_cast<RespawnActorFn>(trampoline);long ps=sysconf(_SC_PAGESIZE);uintptr_t page=reinterpret_cast<uintptr_t>(target)&~(uintptr_t(ps)-1);if(mprotect(reinterpret_cast<void*>(page),size_t(ps),PROT_READ|PROT_WRITE|PROT_EXEC)!=0){originalRespawnActor=nullptr;munmap(trampoline,64);return false;}unsigned char patch[bytes];std::memset(patch,0x90,sizeof(patch));WriteAbsoluteJump(patch,reinterpret_cast<uintptr_t>(&HookRespawnActor));std::memcpy(target,patch,sizeof(patch));__builtin___clear_cache(reinterpret_cast<char*>(target),reinterpret_cast<char*>(target+bytes));mprotect(reinterpret_cast<void*>(page),size_t(ps),PROT_READ|PROT_EXEC);return true;
}

std::string Escape(const std::string&s){std::string o;for(unsigned char c:s){if(c=='"'||c=='\\'){o+='\\';o+=char(c);}else if(c>=32)o+=char(c);}return o;}
std::uint64_t HashBytes(std::uint64_t h,const void*p,size_t n){for(size_t i=0;i<n;++i){h^=reinterpret_cast<const unsigned char*>(p)[i];h*=1099511628211ULL;}return h;}
std::string Hex(std::uint64_t h){char value[17]{};std::snprintf(value,sizeof(value),"%016llx",static_cast<unsigned long long>(h));return value;}
std::string GraveId(uintptr_t,const std::string&owner,const FVector3d&location){std::uint64_t h=1469598103934665603ULL;h=HashBytes(h,owner.data(),owner.size());h=HashBytes(h,&location,sizeof(location));return Hex(h);}
std::string ItemId(const std::string&grave,int slot,const std::string&,int quantity){std::uint64_t h=1469598103934665603ULL;h=HashBytes(h,grave.data(),grave.size());h=HashBytes(h,&slot,sizeof(slot));h=HashBytes(h,&quantity,sizeof(quantity));return Hex(h);}
std::string GraveCellName(const FVector3d&p);

struct SavedItem{int slot=0,quantity=0,maximum=1;std::string persistence,guid,name,icon;};
struct SavedGrave{int slotCount=0;std::vector<SavedItem> items;};
pthread_mutex_t savedSnapshotMutex=PTHREAD_MUTEX_INITIALIZER;std::unordered_map<std::string,SavedGrave> savedGraveSnapshots;
struct RemovedGrave{std::chrono::steady_clock::time_point at;FVector3d actorLocation{};bool hasActorLocation=false;};
pthread_mutex_t removedGraveMutex=PTHREAD_MUTEX_INITIALIZER;std::unordered_map<std::string,RemovedGrave> removedGraveIds;
pthread_mutex_t liveSeenMutex=PTHREAD_MUTEX_INITIALIZER;std::unordered_map<std::string,std::chrono::steady_clock::time_point> liveGraveSeenAt;
void MarkGraveRemoved(const std::string&id,const FVector3d*actorLocation=nullptr){if(id.empty())return;{Lock lock(removedGraveMutex);RemovedGrave value{std::chrono::steady_clock::now(),{},false};if(actorLocation){value.actorLocation=*actorLocation;value.hasActorLocation=true;}removedGraveIds[id]=value;}{Lock lock(savedSnapshotMutex);savedGraveSnapshots.erase(id);}LogMessage("Invalidated removed gravestone id="+id);}
void ClearGraveRemoved(const std::string&id){Lock lock(removedGraveMutex);removedGraveIds.erase(id);}
bool IsGraveRemoved(const std::string&id){Lock lock(removedGraveMutex);auto at=removedGraveIds.find(id);if(at==removedGraveIds.end())return false;if(std::chrono::steady_clock::now()-at->second.at>std::chrono::minutes(10)){removedGraveIds.erase(at);return false;}return true;}
bool IsRemovedActorLocation(const FVector3d&location){Lock lock(removedGraveMutex);auto now=std::chrono::steady_clock::now();for(auto it=removedGraveIds.begin();it!=removedGraveIds.end();){if(now-it->second.at>std::chrono::minutes(10)){it=removedGraveIds.erase(it);continue;}if(it->second.hasActorLocation&&std::abs(it->second.actorLocation.X-location.X)<100&&std::abs(it->second.actorLocation.Y-location.Y)<100&&std::abs(it->second.actorLocation.Z-location.Z)<300)return true;++it;}return false;}
void NoteLiveGrave(const std::string&id){Lock lock(liveSeenMutex);liveGraveSeenAt[id]=std::chrono::steady_clock::now();}
bool WasRecentlyLive(const std::string&id){Lock lock(liveSeenMutex);auto at=liveGraveSeenAt.find(id);return at!=liveGraveSeenAt.end()&&std::chrono::steady_clock::now()-at->second<std::chrono::seconds(15);}
bool PlayerNearGrave(const FVector3d&location){if(!g_Engine)return false;for(const auto&player:g_Engine->GetAllPlayers()){if(!player.spawned||!player.hasLocation)continue;double dx=player.x-location.X,dy=player.y-location.Y;if(dx*dx+dy*dy<5000.0*5000.0)return true;}return false;}
struct SavedItemMeta{std::string name,icon;int maximum=1;};
std::unordered_map<std::string,SavedItemMeta> savedItemMeta;
pthread_once_t savedItemMetaOnce=PTHREAD_ONCE_INIT;
pthread_mutex_t savedItemMetaMutex=PTHREAD_MUTEX_INITIALIZER;
void LoadSavedItemMeta(){
    std::ifstream in("/home/container/zanik/web/grave-items.tsv");std::string line;
    while(std::getline(in,line)){size_t a=line.find('\t'),b=a==std::string::npos?a:line.find('\t',a+1),c=b==std::string::npos?b:line.find('\t',b+1);if(a==std::string::npos||b==std::string::npos||c==std::string::npos)continue;SavedItemMeta m;m.name=line.substr(a+1,b-a-1);try{m.maximum=std::max(1,std::stoi(line.substr(b+1,c-b-1)));}catch(...){m.maximum=1;}m.icon=line.substr(c+1);savedItemMeta.emplace(line.substr(0,a),std::move(m));}
}
bool ReadBinary(const std::string&path,std::vector<unsigned char>&bytes){std::ifstream in(path,std::ios::binary|std::ios::ate);if(!in)return false;auto size=in.tellg();if(size<=0||size>64*1024*1024)return false;bytes.resize(size_t(size));in.seekg(0);return bool(in.read(reinterpret_cast<char*>(bytes.data()),size));}
uint32_t U32(const std::vector<unsigned char>&b,size_t p){uint32_t v=0;if(p+4<=b.size())std::memcpy(&v,b.data()+p,4);return v;}
bool FindFileRecursive(const std::string&dir,const std::string&name,int depth,std::string&result){if(depth<0)return false;DIR*d=opendir(dir.c_str());if(!d)return false;bool found=false;while(auto*e=readdir(d)){if(e->d_name[0]=='.')continue;std::string path=dir+"/"+e->d_name;if(name==e->d_name){result=path;found=true;break;}bool directory=e->d_type==DT_DIR;if(e->d_type==DT_UNKNOWN){struct stat info{};directory=stat(path.c_str(),&info)==0&&S_ISDIR(info.st_mode);}if(directory&&FindFileRecursive(path,name,depth-1,result)){found=true;break;}}closedir(d);return found;}
std::string JsonString(const std::string&s,const std::string&key,size_t begin,size_t end){std::string needle="\""+key+"\"";size_t p=s.find(needle,begin);if(p==std::string::npos||p>=end||(p=s.find(':',p+needle.size()))==std::string::npos||p>=end||(p=s.find('"',p+1))==std::string::npos||p>=end)return {};std::string out;for(++p;p<end&&p<s.size();++p){char c=s[p];if(c=='"')break;if(c=='\\'&&p+1<end)c=s[++p];out+=c;}return out;}
int JsonInt(const std::string&s,const std::string&key,size_t begin,size_t end,int fallback){std::string needle="\""+key+"\"";size_t p=s.find(needle,begin);if(p==std::string::npos||p>=end||(p=s.find(':',p+needle.size()))==std::string::npos||p>=end)return fallback;while(++p<end&&std::isspace(static_cast<unsigned char>(s[p]))){};char*tail=nullptr;long v=std::strtol(s.c_str()+p,&tail,10);return tail==s.c_str()+p?fallback:int(v);}
bool ParseSavedInventory(const std::string&json,SavedGrave&grave){
    // MaxSlotIndex in the persisted sparse inventory is the highest serialized
    // slot, not the inventory capacity. Every gravestone inventory in this
    // build has 32 native slots arranged as four rows of eight.
    constexpr int GraveSlotCount=32;
    int maxIndex=JsonInt(json,"MaxSlotIndex",0,json.size(),-1);if(maxIndex<0||maxIndex>=GraveSlotCount)return false;grave.slotCount=GraveSlotCount;
    size_t p=0;while((p=json.find('"',p))!=std::string::npos){size_t q=json.find('"',p+1);if(q==std::string::npos)break;std::string key=json.substr(p+1,q-p-1);if(key.empty()||!std::all_of(key.begin(),key.end(),[](unsigned char c){return std::isdigit(c);})||q+1>=json.size()){p=q+1;continue;}size_t open=json.find('{',q),close=open;if(open==std::string::npos)break;int level=0;for(;close<json.size();++close){if(json[close]=='{')++level;else if(json[close]=='}'&&--level==0)break;}if(close>=json.size())return false;SavedItem item;item.slot=std::stoi(key);item.persistence=JsonString(json,"ItemData",open,close);item.guid=JsonString(json,"GUID",open,close);item.quantity=JsonInt(json,"Count",open,close,1);if(item.slot>=0&&item.slot<grave.slotCount&&!item.persistence.empty()&&item.quantity>0){pthread_once(&savedItemMetaOnce,&LoadSavedItemMeta);{Lock lock(savedItemMetaMutex);auto meta=savedItemMeta.find(item.persistence);if(meta!=savedItemMeta.end()){item.name=meta->second.name;item.maximum=meta->second.maximum;item.icon=meta->second.icon;}else item.name=item.persistence;}grave.items.push_back(std::move(item));}p=close+1;}return true;
}
bool ReadSavedGrave(const FVector3d&location,SavedGrave&grave){
    const std::string filename=GraveCellName(location)+".lvl";std::string path;{Lock lock(savedPathMutex);auto known=savedCellPaths.find(filename);if(known!=savedCellPaths.end())path=known->second;}if(path.empty()){for(const std::string&candidate:{"/home/container/RSDragonwilds/Saved/SpudCache/"+filename,"/home/container/RSDragonwilds/Saved/SaveGames/SpudCache/"+filename})if(access(candidate.c_str(),R_OK)==0){path=candidate;break;}if(path.empty())for(const char*root:{"/home/container/RSDragonwilds/Saved/SaveGames","/home/container/RSDragonwilds/Saved","/home/container"})if(FindFileRecursive(root,filename,7,path))break;if(path.empty()){LogMessage("SPUD gravestone file missing name="+filename);return false;}Lock lock(savedPathMutex);savedCellPaths[filename]=path;}std::vector<unsigned char>b;if(!ReadBinary(path,b)||b.size()<16||std::memcmp(b.data(),"LEVL",4)){LogMessage("SPUD gravestone file unreadable path="+path);return false;}
    const size_t levelEnd=std::min(b.size(),size_t(8)+size_t(U32(b,4)));size_t level=8;if(level+4>levelEnd)return false;int32_t stringLength=0;std::memcpy(&stringLength,b.data()+level,4);level+=4;if(stringLength>0)level+=size_t(stringLength);else if(stringLength<0)level+=size_t(-int64_t(stringLength))*2;if(level+8>levelEnd)return false;level+=8;
    size_t satsBegin=0,satsEnd=0;for(size_t p=level;p+8<=levelEnd;){uint32_t n=U32(b,p+4);size_t end=p+8+size_t(n);if(end>levelEnd)return false;if(!std::memcmp(b.data()+p,"SATS",4)){satsBegin=p+8;satsEnd=end;break;}p=end;}if(!satsBegin)return false;
    bool found=false;double bestDistance=0;SavedGrave best;int spawnCount=0,inventoryCount=0;
    for(size_t p=satsBegin;p+8<=satsEnd;){uint32_t n=U32(b,p+4);size_t payload=p+8,end=payload+size_t(n);if(end>satsEnd)return false;if(std::memcmp(b.data()+p,"SPWN",4)){p=end;continue;}++spawnCount;if(payload+4+64>end){p=end;continue;}size_t content=payload+4;FVector3d candidate{};std::memcpy(&candidate,b.data()+content+40,sizeof(candidate));if(IsRemovedActorLocation(candidate)){p=end;continue;}const unsigned char marker[]={'{','\n','\t','"','V','e','r','s','i','o','n','"'};auto first=std::search(b.begin()+content,b.begin()+end,std::begin(marker),std::end(marker));if(first==b.begin()+end){p=end;continue;}auto zero=std::find(first,b.begin()+end,0);if(zero==b.begin()+end){p=end;continue;}std::string json(reinterpret_cast<const char*>(&*first),size_t(zero-first));SavedGrave parsed;if(!ParseSavedInventory(json,parsed)){p=end;continue;}++inventoryCount;double dx=candidate.X-location.X,dy=candidate.Y-location.Y,dz=(candidate.Z-location.Z)*0.1,distance=dx*dx+dy*dy+dz*dz;if(!found||distance<bestDistance){found=true;bestDistance=distance;best=std::move(parsed);}p=end;}
    // The gameplay registry stores its map-marker transform rather than the
    // spawned gravestone transform (currently about 27 m apart). Keep matching
    // bounded to that verified displacement; never search the whole cell.
    if(found&&bestDistance<=3000.0*3000.0){grave=std::move(best);return true;}LogMessage("SPUD gravestone record missing path="+path+" spawns="+std::to_string(spawnCount)+" inventories="+std::to_string(inventoryCount)+" nearest2="+std::to_string(bestDistance));return false;
}

bool ReadSlots(uintptr_t inventory,uintptr_t&data,int32_t&num){data=Mem::ReadPtr(inventory+0x168);num=Mem::ReadI32(inventory+0x170);int32_t max=Mem::ReadI32(inventory+0x174);return num>=0&&num<=512&&max>=num&&(!num||(data&&Mem::Readable(reinterpret_cast<void*>(data),size_t(num)*8)));}
int MaxStack(uintptr_t data){uintptr_t table=data?Mem::ReadPtr(data):0,fn=table?Mem::ReadPtr(table+0x2D8):0;if(!fn||!Mem::InImage(reinterpret_cast<void*>(fn)))return 1;int value=reinterpret_cast<GetMaxStackSizeFn>(fn)(reinterpret_cast<void*>(data));return value>=1&&value<=1000000?value:1;}
std::string OwnerGuid(uintptr_t actor){
    // Exact build 25630937: AGravestone::GetOwningCharacterGuid() is the
    // verified `lea 0x410(%rdi), %rax; ret` accessor.
    return g_Engine->ReadGuid(actor+0x410);
}
uintptr_t GraveInventory(uintptr_t actor){
    if(!getGraveInventory)return 0;
    uintptr_t inventory=reinterpret_cast<uintptr_t>(getGraveInventory(reinterpret_cast<void*>(actor)));
    return inventory&&g_Engine->IsObjectLive(inventory)&&g_Engine->IsA(inventory,"InventoryComponent")?inventory:0;
}
bool IsGrave(uintptr_t actor){if(!actor||!g_Engine->IsObjectLive(actor)||!g_Engine->IsA(actor,"Gravestone")||g_Engine->GetObjectName(actor).rfind("Default__",0)==0)return false;bool destroyed=false;if(g_Engine->ReadBoolProperty(actor,"bActorIsBeingDestroyed",destroyed)&&destroyed)return false;return true;}
uintptr_t FindWorldPartitionSubsystem();
FUEArray RegistryGraves();
bool ResolveGrave(const std::string&owner,const std::string&id,Grave&out);
struct GraveRestoreContext{FVector3d location{};};
void* GetGravePredicateCallable(void*storage){return storage?*reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(storage)+8):nullptr;}
bool InvokeGravePredicate(void*opaque,const SpudBox*box){if(!opaque||!box||!box->valid)return false;const auto&p=reinterpret_cast<GraveRestoreContext*>(opaque)->location;return p.X>=box->minX-25.0&&p.X<=box->maxX+25.0&&p.Y>=box->minY-25.0&&p.Y<=box->maxY+25.0&&p.Z>=box->minZ-250.0&&p.Z<=box->maxZ+250.0;}
void* gravePredicateManagerVtable[]={nullptr,reinterpret_cast<void*>(&GetGravePredicateCallable)};
struct NativeGravePredicate{void*invoke=nullptr;std::uint64_t reserved08=0;void**manager=nullptr;void*context=nullptr;std::uint64_t reserved20=0;void*heapStorage=nullptr;};
static_assert(sizeof(NativeGravePredicate)==0x30,"TFunction layout changed");
uintptr_t FindSpudState(){if(!g_Engine)return 0;for(int32_t i=0;i<g_Engine->GetObjectCount();++i){uintptr_t object=g_Engine->GetObjectByIndex(i);if(!object||!g_Engine->IsObjectLive(object)||!g_Engine->IsA(object,"SpudSubsystem")||g_Engine->GetObjectName(object).rfind("Default__",0)==0)continue;uintptr_t state=Mem::ReadPtr(object+0x348);if(state&&g_Engine->IsObjectLive(state))return state;}return 0;}
uintptr_t FindActiveWorld(){uintptr_t subsystem=FindWorldPartitionSubsystem();if(!subsystem)return 0;uintptr_t world=Mem::ReadPtr(subsystem+Offsets::UObject_Outer);return world&&g_Engine->IsObjectLive(world)?world:0;}
std::string GraveCellName(const FVector3d&p){const int x=int(std::floor(p.X/12800.0)),y=int(std::floor(p.Y/12800.0));return "RSACell12800_X"+std::to_string(x)+"_Y"+std::to_string(y);}
bool RestoreSavedGrave(const std::string&owner,const std::string&id,const FVector3d&location){
    if(!restoreRuntimeSpawnedActorsFromLevel||!g_Engine)return false;uintptr_t state=FindSpudState(),world=FindActiveWorld();if(!state||!world)return false;std::string cell=GraveCellName(location);std::u16string cell16;cell16.reserve(cell.size()+1);for(unsigned char c:cell)cell16.push_back(char16_t(c));cell16.push_back(0);FString levelName{cell16.data(),int32_t(cell16.size()),int32_t(cell16.size())};GraveRestoreContext context{location};NativeGravePredicate predicate{};predicate.invoke=reinterpret_cast<void*>(&InvokeGravePredicate);predicate.manager=gravePredicateManagerVtable;predicate.context=&context;restoreRuntimeSpawnedActorsFromLevel(reinterpret_cast<void*>(state),reinterpret_cast<void*>(world),&predicate,&levelName);Grave resolved;bool ok=ResolveGrave(owner,id,resolved);LogMessage("Native saved gravestone restore id="+id+" cell="+cell+" result="+(ok?std::string("loaded"):std::string("not-found")));return ok;
}
void ReconcileSavedGraves(){static auto next=std::chrono::steady_clock::time_point{};auto now=std::chrono::steady_clock::now();if(now<next||!g_Engine||!g_Engine->GetAllPlayers().empty())return;next=now+std::chrono::seconds(2);FUEArray records=RegistryGraves();for(int32_t i=0;i<records.Num;++i){uintptr_t record=reinterpret_cast<uintptr_t>(records.Data)+uintptr_t(i)*GraveRecordSize;std::string owner=g_Engine->ReadGuid(record+0x18);FVector3d location{};std::memcpy(&location,reinterpret_cast<void*>(record),sizeof(location));if(owner.empty()||!std::isfinite(location.X)||!std::isfinite(location.Y)||!std::isfinite(location.Z))continue;std::string id=GraveId(0,owner,location);Grave grave;if(ResolveGrave(owner,id,grave)){graveRestoreAttempts.erase(id);continue;}auto attempt=graveRestoreAttempts.find(id);if(attempt!=graveRestoreAttempts.end()&&now-attempt->second<std::chrono::seconds(30))continue;graveRestoreAttempts[id]=now;RestoreSavedGrave(owner,id,location);break;}}
void CaptureSpudStateIndex(){
    if(spudIndexCaptured.load()||!g_Engine)return;uintptr_t subsystem=0;for(int32_t i=0;i<g_Engine->GetObjectCount();++i){uintptr_t object=g_Engine->GetObjectByIndex(i);if(object&&g_Engine->IsObjectLive(object)&&g_Engine->IsA(object,"SpudSubsystem")&&g_Engine->GetObjectName(object).rfind("Default__",0)!=0){subsystem=object;break;}}if(!subsystem)return;uintptr_t state=Mem::ReadPtr(subsystem+0x348);if(!state||!g_Engine->IsObjectLive(state)||!Mem::Readable(reinterpret_cast<void*>(state),0x500))return;uintptr_t mapData=Mem::ReadPtr(state+0x28);if(!mapData||!Mem::Readable(reinterpret_cast<void*>(mapData),0x2000))return;bool expected=false;if(!spudIndexCaptured.compare_exchange_strong(expected,true))return;
    struct Header{char magic[16];std::uint32_t version,stateSize,mapSize,reserved;std::uint64_t subsystem,state,mapData;unsigned char stateData[0x500];unsigned char mapBytes[0x2000];}capture{};std::memcpy(capture.magic,"ZANIKSPUDINDEX1",15);capture.version=1;capture.stateSize=sizeof(capture.stateData);capture.mapSize=sizeof(capture.mapBytes);capture.subsystem=subsystem;capture.state=state;capture.mapData=mapData;std::memcpy(capture.stateData,reinterpret_cast<void*>(state),sizeof(capture.stateData));std::memcpy(capture.mapBytes,reinterpret_cast<void*>(mapData),sizeof(capture.mapBytes));int fd=open("/home/container/zanik-data/spud-state-index-capture.bin",O_WRONLY|O_CREAT|O_TRUNC,0600);if(fd>=0){const unsigned char*bytes=reinterpret_cast<const unsigned char*>(&capture);size_t left=sizeof(capture);while(left){ssize_t wrote=write(fd,bytes+(sizeof(capture)-left),left);if(wrote<=0)break;left-=size_t(wrote);}fsync(fd);close(fd);LogMessage("Captured USpudState level index layout");}
}
FUEArray RegistryGraves(){FUEArray best{};if(!g_Engine)return best;auto read=[&](uintptr_t registry,FUEArray&array){if(!registry||!g_Engine->IsObjectLive(registry)||!g_Engine->IsA(registry,"GameplayObjectRegistry")||!Mem::Readable(reinterpret_cast<void*>(registry+RegistryGravesOffset),sizeof(FUEArray)))return false;std::memcpy(&array,reinterpret_cast<void*>(registry+RegistryGravesOffset),sizeof(array));return array.Num>=0&&array.Num<=1024&&array.Max>=array.Num&&(!array.Num||(array.Data&&Mem::Readable(array.Data,size_t(array.Num)*GraveRecordSize)));};if(read(gameplayObjectRegistry,best)&&best.Num>0)return best;gameplayObjectRegistry=0;for(int32_t i=0;i<g_Engine->GetObjectCount();++i){uintptr_t registry=g_Engine->GetObjectByIndex(i);FUEArray array{};if(!read(registry,array))continue;if(array.Num>best.Num){best=array;gameplayObjectRegistry=registry;}}return best;}
bool ResolveRegistryGrave(const std::string&owner,const std::string&id,FVector3d&location){FUEArray graves=RegistryGraves();for(int32_t i=0;i<graves.Num;++i){uintptr_t record=reinterpret_cast<uintptr_t>(graves.Data)+uintptr_t(i)*GraveRecordSize;std::string guid=g_Engine->ReadGuid(record+0x18);if(guid!=owner)continue;FVector3d candidate{};std::memcpy(&candidate,reinterpret_cast<void*>(record),sizeof(candidate));if(std::isfinite(candidate.X)&&std::isfinite(candidate.Y)&&std::isfinite(candidate.Z)&&GraveId(0,guid,candidate)==id){location=candidate;return true;}}return false;}
void LearnSavedItemMetadata(const std::string&owner,const std::string&id,const Grave&grave){
    if(!g_Engine)return;FVector3d registryLocation{};if(!ResolveRegistryGrave(owner,id,registryLocation))return;SavedGrave saved;if(!ReadSavedGrave(registryLocation,saved))return;uintptr_t slots=0;int32_t count=0;if(!ReadSlots(grave.inventory,slots,count))return;
    pthread_once(&savedItemMetaOnce,&LoadSavedItemMeta);using DisplayFn=const FString*(*)(const void*);auto display=reinterpret_cast<DisplayFn>(g_Engine->GetModuleBase()+0x04AA1A80);std::ofstream append;
    for(const auto&persisted:saved.items){if(persisted.slot<0||persisted.slot>=count||persisted.persistence.empty())continue;{Lock lock(savedItemMetaMutex);if(savedItemMeta.count(persisted.persistence))continue;}uintptr_t item=Mem::ReadPtr(slots+uintptr_t(persisted.slot)*8),data=item?Mem::ReadPtr(item+0xB0):0;if(!item||!data||!g_Engine->IsA(item,"Item")||!g_Engine->IsA(data,"ItemData"))continue;std::string key=g_Engine->GetObjectName(data),name;const FString*text=display(reinterpret_cast<const void*>(data+0x68));if(text&&Mem::Readable(text,sizeof(FString)))name=g_Engine->ReadFString(reinterpret_cast<uintptr_t>(text));if(key.empty()||name.empty())continue;name.erase(std::remove_if(name.begin(),name.end(),[](char c){return c=='\t'||c=='\r'||c=='\n';}),name.end());SavedItemMeta metadata{name,key+".png",MaxStack(data)};{Lock lock(savedItemMetaMutex);if(!savedItemMeta.emplace(persisted.persistence,metadata).second)continue;}if(!append.is_open())append.open("/home/container/zanik/web/grave-items.tsv",std::ios::app);if(append)append<<persisted.persistence<<'\t'<<metadata.name<<'\t'<<metadata.maximum<<'\t'<<metadata.icon<<'\n';LogMessage("Learned gravestone item metadata persistence="+persisted.persistence+" item="+key+" name="+name);
    }
    if(append)append.flush();
}
void HookRestoreRuntimeSpawnedActors(void*self,const void*world,const FUEArray*bounds,const void*predicate){
    static std::atomic<int> calls{0};if(!originalRestoreRuntimeSpawnedActors){return;}FUEArray records{};if(gameplayObjectRegistry&&g_Engine&&g_Engine->IsObjectLive(gameplayObjectRegistry)&&Mem::Readable(reinterpret_cast<void*>(gameplayObjectRegistry+RegistryGravesOffset),sizeof(records)))std::memcpy(&records,reinterpret_cast<void*>(gameplayObjectRegistry+RegistryGravesOffset),sizeof(records));int call=calls.fetch_add(1);if(call<6)LogMessage("SPUD runtime restore hook bounds="+std::to_string(bounds?bounds->Num:-1)+" graves="+std::to_string(records.Num));
    if(records.Num<=0||records.Num>1024||!records.Data||!Mem::Readable(records.Data,size_t(records.Num)*GraveRecordSize)){originalRestoreRuntimeSpawnedActors(self,world,bounds,predicate);return;}
    std::vector<SpudBox> expanded;if(bounds&&bounds->Num>0&&bounds->Num<4096&&bounds->Data&&Mem::Readable(bounds->Data,size_t(bounds->Num)*sizeof(SpudBox))){const auto*first=reinterpret_cast<const SpudBox*>(bounds->Data);expanded.assign(first,first+bounds->Num);}expanded.reserve(expanded.size()+records.Num);
    for(int32_t i=0;i<records.Num;++i){uintptr_t record=reinterpret_cast<uintptr_t>(records.Data)+uintptr_t(i)*GraveRecordSize;FVector3d p{};std::memcpy(&p,reinterpret_cast<void*>(record),sizeof(p));if(!std::isfinite(p.X)||!std::isfinite(p.Y)||!std::isfinite(p.Z))continue;expanded.push_back({p.X-200.0,p.Y-200.0,p.Z-200.0,p.X+200.0,p.Y+200.0,p.Z+200.0,1,{}});}
    FUEArray combined{expanded.data(),int32_t(expanded.size()),int32_t(expanded.size())};originalRestoreRuntimeSpawnedActors(self,world,&combined,predicate);
}
bool InstallRestoreRuntimeSpawnedActorsHook(uintptr_t moduleBase,uintptr_t moduleEnd){
    if(originalRestoreRuntimeSpawnedActors)return true;constexpr size_t bytes=17;const unsigned char sig[]={0x55,0x41,0x57,0x41,0x56,0x41,0x55,0x41,0x54,0x53,0x48,0x81,0xec,0x88,0x00,0x00,0x00};if(moduleBase+RestoreRuntimeSpawnedActorsOffset+bytes>moduleEnd||std::memcmp(reinterpret_cast<void*>(moduleBase+RestoreRuntimeSpawnedActorsOffset),sig,sizeof(sig)))return false;auto*target=reinterpret_cast<unsigned char*>(moduleBase+RestoreRuntimeSpawnedActorsOffset);auto*trampoline=static_cast<unsigned char*>(mmap(nullptr,64,PROT_READ|PROT_WRITE|PROT_EXEC,MAP_PRIVATE|MAP_ANONYMOUS,-1,0));if(trampoline==MAP_FAILED)return false;std::memcpy(trampoline,target,bytes);WriteAbsoluteJump(trampoline+bytes,reinterpret_cast<uintptr_t>(target+bytes));originalRestoreRuntimeSpawnedActors=reinterpret_cast<RestoreRuntimeSpawnedActorsFn>(trampoline);long ps=sysconf(_SC_PAGESIZE);uintptr_t page=reinterpret_cast<uintptr_t>(target)&~(uintptr_t(ps)-1);if(mprotect(reinterpret_cast<void*>(page),size_t(ps),PROT_READ|PROT_WRITE|PROT_EXEC)!=0)return false;unsigned char patch[bytes];std::memset(patch,0x90,sizeof(patch));WriteAbsoluteJump(patch,reinterpret_cast<uintptr_t>(&HookRestoreRuntimeSpawnedActors));std::memcpy(target,patch,sizeof(patch));__builtin___clear_cache(reinterpret_cast<char*>(target),reinterpret_cast<char*>(target+bytes));mprotect(reinterpret_cast<void*>(page),size_t(ps),PROT_READ|PROT_EXEC);return true;
}
uintptr_t FindWorldPartitionSubsystem(){
    if(worldPartitionSubsystem&&g_Engine->IsObjectLive(worldPartitionSubsystem)&&g_Engine->IsA(worldPartitionSubsystem,"WorldPartitionSubsystem")){uintptr_t outer=Mem::ReadPtr(worldPartitionSubsystem+Offsets::UObject_Outer);std::string outerName=outer?g_Engine->GetObjectName(outer):std::string();if(outerName.find("L_World")!=std::string::npos)return worldPartitionSubsystem;}
    uintptr_t fallback=0,active=0;
    for(int32_t i=0;i<g_Engine->GetObjectCount();++i){uintptr_t object=g_Engine->GetObjectByIndex(i);if(!object||!g_Engine->IsObjectLive(object)||!g_Engine->IsA(object,"WorldPartitionSubsystem")||g_Engine->GetObjectName(object).rfind("Default__",0)==0)continue;fallback=object;uintptr_t outer=Mem::ReadPtr(object+Offsets::UObject_Outer);std::string outerName=outer?g_Engine->GetObjectName(outer):std::string();if(outerName=="L_World"||outerName.find("L_World")!=std::string::npos){active=object;break;}}
    return active?active:fallback;
}
void ReconcileGraveStreamingProviders(){
    static auto next=std::chrono::steady_clock::time_point{};auto now=std::chrono::steady_clock::now();if(now<next)return;next=now+std::chrono::seconds(2);if(!g_Engine||!registerStreamingProvider||!unregisterStreamingProvider)return;RegistryGraves();uintptr_t previous=worldPartitionSubsystem,subsystem=FindWorldPartitionSubsystem();if(!subsystem)return;if(previous&&previous!=subsystem){for(auto&entry:graveProviders)unregisterStreamingProvider(reinterpret_cast<void*>(previous),entry.second.get());graveProviders.clear();LogMessage("Migrating gravestone streaming sources to active world");}if(worldPartitionSubsystem!=subsystem){worldPartitionSubsystem=subsystem;uintptr_t outer=Mem::ReadPtr(subsystem+Offsets::UObject_Outer);LogMessage("Selected gravestone WorldPartitionSubsystem outer="+(outer?g_Engine->GetObjectName(outer):std::string("none")));}
    if(!haveStreamingSourceTemplate&&playerGetStreamingSources){for(const auto&player:g_Engine->GetAllPlayers()){if(!player.controllerPtr||!g_Engine->IsObjectLive(player.controllerPtr))continue;FUEArray sources{};if(!playerGetStreamingSources(reinterpret_cast<void*>(player.controllerPtr),&sources)||sources.Num<=0||sources.Num>16||!sources.Data||!Mem::Readable(sources.Data,0x160))continue;auto*source=reinterpret_cast<unsigned char*>(sources.Data);std::memcpy(streamingSourceTemplate,source,sizeof(streamingSourceTemplate));haveStreamingSourceTemplate=true;int priority=0,shapeCount=0;std::memcpy(&priority,source+0x5c,4);std::memcpy(&shapeCount,source+0x70,4);std::ostringstream message;message<<"Captured native player streaming source controller="<<g_Engine->GetObjectName(player.controllerPtr)<<" sources="<<sources.Num<<" target="<<int(source[0x38])<<" flag3a="<<int(source[0x3a])<<" priority="<<priority<<" block="<<int(source[0x60])<<" shapes="<<shapeCount;LogMessage(message.str());break;}}
    // An incomplete source (especially one without the game's streaming
    // shape) can block a connecting player's own World Partition startup.
    // Never expose grave providers until a complete native source has been
    // captured from an authoritative live player controller.
    if(!haveStreamingSourceTemplate)return;
    std::unordered_set<std::string> present;FUEArray records=RegistryGraves();
    for(int32_t i=0;i<records.Num;++i){uintptr_t record=reinterpret_cast<uintptr_t>(records.Data)+uintptr_t(i)*GraveRecordSize;std::string owner=g_Engine->ReadGuid(record+0x18);FVector3d location{};std::memcpy(&location,reinterpret_cast<void*>(record),sizeof(location));if(owner.empty()||!std::isfinite(location.X)||!std::isfinite(location.Y)||!std::isfinite(location.Z))continue;std::string id=GraveId(0,owner,location);present.insert(id);auto found=graveProviders.find(id);if(found!=graveProviders.end()){found->second->location=location;continue;}auto provider=std::make_unique<GraveStreamingProvider>();provider->vtable=graveStreamingVtable;provider->location=location;provider->id=id;std::memcpy(&provider->sourceName,reinterpret_cast<void*>(subsystem+Offsets::UObject_Name),sizeof(provider->sourceName));provider->sourceName=(provider->sourceName&0xffffffffULL)|(std::uint64_t(i+1)<<32);registerStreamingProvider(reinterpret_cast<void*>(subsystem),provider.get());graveProviders.emplace(id,std::move(provider));LogMessage("Registered gravestone streaming source id="+id);}
    for(auto it=graveProviders.begin();it!=graveProviders.end();){if(present.count(it->first)){++it;continue;}unregisterStreamingProvider(reinterpret_cast<void*>(subsystem),it->second.get());LogMessage("Unregistered gravestone streaming source id="+it->first);it=graveProviders.erase(it);}
    for(auto&entry:graveProviders)if(!entry.second->completionLogged&&entry.second->calls>2&&isStreamingCompleted){entry.second->completionLogged=true;bool done=isStreamingCompleted(reinterpret_cast<void*>(subsystem),entry.second.get());LogMessage("Gravestone streaming completion id="+entry.first+" complete="+(done?std::string("true"):std::string("false"))+" calls="+std::to_string(entry.second->calls));}
}
void LogUnresolvedGrave(uintptr_t actor,const char* reason){const auto identity=g_Engine->GetObjectIdentity(actor);{Lock lock(probeMutex);if(!identity||!probedGraves.insert(identity).second)return;}std::ostringstream message;message<<"Gravestone probe reason="<<reason<<" class="<<g_Engine->GetObjectClassName(actor)<<" name="<<g_Engine->GetObjectName(actor)<<" properties=";for(const auto&field:g_Engine->GetPropertyOffsets(actor))message<<field.first<<"@0x"<<std::hex<<field.second<<std::dec<<",";message<<" children=";for(int32_t i=0;i<g_Engine->GetObjectCount();++i){uintptr_t object=g_Engine->GetObjectByIndex(i);if(object&&Mem::ReadPtr(object+Offsets::UObject_Outer)==actor)message<<g_Engine->GetObjectClassName(object)<<":"<<g_Engine->GetObjectName(object)<<",";}LogMessage(message.str());}
bool ResolveGrave(const std::string&owner,const std::string&id,Grave&out){
    if(!g_Engine)return false;const int32_t count=g_Engine->GetObjectCount();std::unordered_set<uintptr_t> seen;
    for(int32_t i=0;i<count;++i){uintptr_t actor=g_Engine->GetObjectByIndex(i);if(!IsGrave(actor)||!seen.insert(actor).second)continue;std::string guid=OwnerGuid(actor);if(guid!=owner)continue;uintptr_t root=Mem::ReadPtr(actor+Offsets::Actor_RootComponent);FVector3d location{};if(!root||!Mem::Readable(reinterpret_cast<void*>(root+Offsets::SceneComp_RelativeLocation),sizeof(location)))continue;std::memcpy(&location,reinterpret_cast<void*>(root+Offsets::SceneComp_RelativeLocation),sizeof(location));if(!std::isfinite(location.X)||!std::isfinite(location.Y)||!std::isfinite(location.Z))continue;std::string candidate=GraveId(actor,guid,location);if(candidate!=id)continue;uintptr_t inventory=GraveInventory(actor);if(!inventory)return false;out={actor,inventory,candidate,guid,location};return true;}
    return false;
}
PlayerInfo FindPlayer(const std::string&guid,bool&found){found=false;PlayerInfo result;if(g_Engine)for(const auto&p:g_Engine->GetAllPlayers())if(p.characterGuid==guid){result=p;found=true;break;}return result;}
uintptr_t PlayerInventory(const PlayerInfo&player){if(!g_Engine||!player.controllerPtr)return 0;for(int32_t i=0;i<g_Engine->GetObjectCount();++i){uintptr_t inv=g_Engine->GetObjectByIndex(i);if(!inv||!g_Engine->IsObjectLive(inv)||!g_Engine->IsA(inv,"InventoryComponent")||g_Engine->IsA(inv,"LinkedInventoryComponent")||g_Engine->IsA(inv,"PersonalInventoryComponent")||g_Engine->IsA(inv,"LoadoutComponent"))continue;uintptr_t outer=Mem::ReadPtr(inv+Offsets::UObject_Outer);if(outer==player.controllerPtr||outer==player.pawnPtr)return inv;}return 0;}
bool FindItem(const Grave&grave,const std::string&id,int&slot,uintptr_t&item,uintptr_t&data,int&quantity,std::string&key){uintptr_t slots=0;int32_t count=0;if(!ReadSlots(grave.inventory,slots,count))return false;for(int32_t i=0;i<count;++i){uintptr_t candidate=Mem::ReadPtr(slots+uintptr_t(i)*8);if(!candidate||!g_Engine->IsA(candidate,"Item"))continue;uintptr_t candidateData=Mem::ReadPtr(candidate+0xB0);int q=Mem::ReadI32(candidate+0x58);if(!candidateData||q<=0)continue;std::string candidateKey=g_Engine->GetObjectName(candidateData);if(ItemId(grave.id,i,candidateKey,q)==id){slot=i;item=candidate;data=candidateData;quantity=q;key=candidateKey;return true;}}return false;}
bool FindDestinationSlot(uintptr_t inventory,uintptr_t itemData,int quantity,int&slot){uintptr_t slots=0;int32_t count=0;if(!ReadSlots(inventory,slots,count))return false;const int maximum=MaxStack(itemData);for(int32_t i=0;i<count;++i){uintptr_t item=Mem::ReadPtr(slots+uintptr_t(i)*8);if(item&&Mem::ReadPtr(item+0xB0)==itemData&&Mem::ReadI32(item+0x58)+quantity<=maximum){slot=i;return true;}}for(int32_t i=0;i<count;++i)if(!Mem::ReadPtr(slots+uintptr_t(i)*8)){slot=i;return true;}return false;}
void Finish(const std::shared_ptr<Request>&r,bool ok,const std::string&message){Lock lock(r->mutex);r->success=ok;r->result=message;r->complete=true;pthread_cond_broadcast(&r->changed);}
void RecordTime(const std::chrono::steady_clock::time_point&start){auto value=static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-start).count()),old=maximumMicros.load();while(old<value&&!maximumMicros.compare_exchange_weak(old,value)){}}
bool Execute(const std::shared_ptr<Request>&r){
    if(r->cancelled.load())return true;auto start=std::chrono::steady_clock::now();Grave grave;if(!ResolveGrave(r->ownerGuid,r->graveId,grave)){if(persistenceCompatibility.load()!=1){++rejected;Finish(r,false,"Remote gravestone persistence compatibility unavailable");return true;}FVector3d location{};if(!ResolveRegistryGrave(r->ownerGuid,r->graveId,location)||std::chrono::steady_clock::now()-r->queuedAt>std::chrono::seconds(15)){++stale;Finish(r,false,"Gravestone is no longer available");return true;}auto attempt=graveRestoreAttempts.find(r->graveId);auto now=std::chrono::steady_clock::now();if(attempt==graveRestoreAttempts.end()||now-attempt->second>std::chrono::seconds(10)){graveRestoreAttempts[r->graveId]=now;RestoreSavedGrave(r->ownerGuid,r->graveId,location);}return false;}
    LearnSavedItemMetadata(r->ownerGuid,r->graveId,grave);
    bool adminFound=false,ownerFound=false;PlayerInfo admin=FindPlayer(r->adminGuid,adminFound),owner=FindPlayer(r->ownerGuid,ownerFound);const bool toOwner=r->operation.find("_owner")!=std::string::npos;PlayerInfo destination=toOwner?owner:admin;bool destinationFound=toOwner?ownerFound:adminFound;void* controller=destinationFound?reinterpret_cast<void*>(destination.controllerPtr):nullptr;bool ok=false;
    if(r->operation=="delete_grave"){removeGrave(reinterpret_cast<void*>(grave.actor));ok=true;}
    else if(r->operation=="claim_all_owner"||r->operation=="claim_all_self"){
        if(!destinationFound||!destination.spawned||destination.isDead){Finish(r,false,toOwner?"Gravestone owner is offline or unavailable":"Linked administrator character is offline or unavailable");return true;}uintptr_t target=PlayerInventory(destination);if(!target){Finish(r,false,"Destination inventory is unavailable");return true;}
        uintptr_t slots=0;int32_t count=0;if(!ReadSlots(grave.inventory,slots,count)){Finish(r,false,"Gravestone inventory is unavailable");return true;}std::vector<unsigned char> values;for(int32_t i=0;i<count;++i){uintptr_t item=Mem::ReadPtr(slots+uintptr_t(i)*8);if(!item)continue;uintptr_t data=Mem::ReadPtr(item+0xB0);int quantity=Mem::ReadI32(item+0x58);if(!data||quantity<=0)continue;size_t at=values.size();values.resize(at+16);std::memcpy(values.data()+at,&data,8);std::memcpy(values.data()+at+8,&quantity,4);}FUEArray batch{values.data(),int32_t(values.size()/16),int32_t(values.size()/16)};if(batch.Num&&!canAddItemsByData(reinterpret_cast<void*>(target),&batch)){Finish(r,false,"Destination inventory does not have enough space for the complete gravestone");return true;}moveAllItems(reinterpret_cast<void*>(grave.inventory),reinterpret_cast<void*>(target));ok=true;
    }else{
        int slot=-1,current=0;uintptr_t item=0,itemData=0;std::string key;if(!FindItem(grave,r->itemId,slot,item,itemData,current,key)){++stale;Finish(r,false,"The gravestone item changed before the request was applied");return true;}if(r->quantity<1||r->quantity>current){Finish(r,false,"Requested quantity is no longer available");return true;}
        if(r->operation=="delete_item")ok=removeFromSlot(reinterpret_cast<void*>(grave.inventory),slot,r->quantity,adminFound?reinterpret_cast<void*>(admin.controllerPtr):nullptr);
        else {if(!destinationFound||!destination.spawned||destination.isDead){Finish(r,false,toOwner?"Gravestone owner is offline or unavailable":"Linked administrator character is offline or unavailable");return true;}uintptr_t target=PlayerInventory(destination);if(!target){Finish(r,false,"Destination inventory is unavailable");return true;}if(!canAddItemByData(reinterpret_cast<void*>(target),reinterpret_cast<void*>(itemData),r->quantity)){Finish(r,false,"Destination inventory does not have enough space");return true;}int targetSlot=-1;if(!FindDestinationSlot(target,itemData,r->quantity,targetSlot)){Finish(r,false,"Destination inventory does not have enough space");return true;}ok=moveItem(reinterpret_cast<void*>(grave.inventory),slot,reinterpret_cast<void*>(target),targetSlot,controller,r->quantity);}
    }
    if(ok){
        bool empty=r->operation=="delete_grave";uintptr_t remainingSlots=0;int32_t remainingCount=0;
        if(!empty&&ReadSlots(grave.inventory,remainingSlots,remainingCount)){empty=true;for(int32_t i=0;i<remainingCount;++i)if(Mem::ReadPtr(remainingSlots+uintptr_t(i)*8)){empty=false;break;}}
        if(empty){
            // The browser submits the registry-derived grave ID. A remotely restored
            // actor can have a slightly different transform and therefore a second
            // actor-derived ID. Invalidate both identities immediately, while also
            // retaining the actor location so its stale SPUD record cannot be
            // reassigned to a future grave.
            MarkGraveRemoved(r->graveId,&grave.location);
            if(grave.id!=r->graveId)MarkGraveRemoved(grave.id,&grave.location);
        }
        ++accepted;Finish(r,true,r->operation=="delete_grave"?"Gravestone deleted":"Gravestone updated");
    }else{++rejected;Finish(r,false,"The game rejected the gravestone operation");}RecordTime(start);return true;
}
}

bool InitializeAdminGravestones(uintptr_t moduleBase,uintptr_t moduleEnd){
    if(!ExpectedSha(FileSha256("/proc/self/exe"))){compatibility=-1;return false;}for(uintptr_t offset:{MoveItemOffset,RemoveFromSlotOffset,CanAddItemByDataOffset,CanAddItemsByDataOffset,MoveAllItemsOffset,RemoveGraveOffset,GetGraveInventoryOffset,GetGraveOwnerOffset,GetGraveRegistryOffset,RegisterStreamingProviderOffset,UnregisterStreamingProviderOffset,GetStreamingSourcesOffset,IsStreamingCompletedOffset,RestoreRuntimeSpawnedActorsOffset,RespawnActorOffset,RestoreRuntimeSpawnedActorsFromLevelOffset,GetStreamingSourceOffset,PlayerGetStreamingSourcesOffset,StreamingSourceCopyOffset,StreamingSourceDestructorOffset})if(moduleBase+offset>=moduleEnd){compatibility=-2;return false;}
    const unsigned char moveSig[]={0x55,0x53,0x50,0x4c,0x89,0xc0};const unsigned char removeSlotSig[]={0x55,0x41,0x57,0x41,0x56,0x53};
    const unsigned char inventorySig[]={0x53,0x48,0x83,0xec,0x10,0x48,0x89,0xfb};const unsigned char ownerSig[]={0x48,0x8d,0x87,0x10,0x04,0x00,0x00,0xc3};
    const unsigned char registrySig[]={0x55,0x41,0x56,0x53,0x48,0x83,0xec,0x10,0x48,0x89,0xf8,0x80,0xbe,0x30,0x03,0x00,0x00,0x00};
    const unsigned char registerSig[]={0x48,0x83,0xec,0x18,0x48,0x89,0x74,0x24,0x10,0xc6,0x44,0x24,0x0f,0x00};
    const unsigned char unregisterSig[]={0x50,0x48,0x81,0xc7,0xf8,0x00,0x00,0x00};
    if(std::memcmp(reinterpret_cast<void*>(moduleBase+MoveItemOffset),moveSig,sizeof(moveSig))){compatibility=-31;return false;}
    if(std::memcmp(reinterpret_cast<void*>(moduleBase+RemoveFromSlotOffset),removeSlotSig,sizeof(removeSlotSig))){compatibility=-32;return false;}
    if(std::memcmp(reinterpret_cast<void*>(moduleBase+GetGraveInventoryOffset),inventorySig,sizeof(inventorySig))){compatibility=-33;return false;}
    if(std::memcmp(reinterpret_cast<void*>(moduleBase+GetGraveOwnerOffset),ownerSig,sizeof(ownerSig))){compatibility=-34;return false;}
    if(std::memcmp(reinterpret_cast<void*>(moduleBase+GetGraveRegistryOffset),registrySig,sizeof(registrySig))){compatibility=-37;return false;}
    if(std::memcmp(reinterpret_cast<void*>(moduleBase+RegisterStreamingProviderOffset),registerSig,sizeof(registerSig))){compatibility=-35;return false;}
    if(std::memcmp(reinterpret_cast<void*>(moduleBase+UnregisterStreamingProviderOffset),unregisterSig,sizeof(unregisterSig))){compatibility=-36;return false;}
    moveItem=reinterpret_cast<MoveItemFn>(moduleBase+MoveItemOffset);removeFromSlot=reinterpret_cast<RemoveFromSlotFn>(moduleBase+RemoveFromSlotOffset);canAddItemByData=reinterpret_cast<CanAddItemByDataFn>(moduleBase+CanAddItemByDataOffset);canAddItemsByData=reinterpret_cast<CanAddItemsByDataFn>(moduleBase+CanAddItemsByDataOffset);moveAllItems=reinterpret_cast<MoveAllItemsFn>(moduleBase+MoveAllItemsOffset);removeGrave=reinterpret_cast<RemoveGraveFn>(moduleBase+RemoveGraveOffset);getGraveInventory=reinterpret_cast<GetGraveInventoryFn>(moduleBase+GetGraveInventoryOffset);registerStreamingProvider=reinterpret_cast<RegisterStreamingProviderFn>(moduleBase+RegisterStreamingProviderOffset);unregisterStreamingProvider=reinterpret_cast<UnregisterStreamingProviderFn>(moduleBase+UnregisterStreamingProviderOffset);getStreamingSources=reinterpret_cast<GetStreamingSourcesFn>(moduleBase+GetStreamingSourcesOffset);playerGetStreamingSources=reinterpret_cast<GetStreamingSourcesFn>(moduleBase+PlayerGetStreamingSourcesOffset);copyStreamingSource=reinterpret_cast<StreamingSourceCopyFn>(moduleBase+StreamingSourceCopyOffset);destroyStreamingSource=reinterpret_cast<StreamingSourceDestructorFn>(moduleBase+StreamingSourceDestructorOffset);isStreamingCompleted=reinterpret_cast<IsStreamingCompletedFn>(moduleBase+IsStreamingCompletedOffset);getStreamingSource=reinterpret_cast<GetStreamingSourceFn>(moduleBase+GetStreamingSourceOffset);restoreRuntimeSpawnedActorsFromLevel=reinterpret_cast<RestoreRuntimeSpawnedActorsFromLevelFn>(moduleBase+RestoreRuntimeSpawnedActorsFromLevelOffset);inventoryCompatibility=1;registryCompatibility=1;streamingCompatibility=1;respawnCaptureCompatibility=InstallRespawnActorCaptureHook(moduleBase,moduleEnd)?1:-1;if(respawnCaptureCompatibility.load()!=1)LogMessage("Passive gravestone respawn capture hook unavailable; mutations remain gated normally");else LogMessage("Passive gravestone respawn capture hook armed (forced streaming disabled)");persistenceCompatibility=respawnCaptureCompatibility.load();compatibility=1;return true;
}
void ProcessAdminGravestoneRequests(){std::shared_ptr<Request> request;{Lock lock(queueMutex);while(!requests.empty()&&requests.front()->cancelled.load())requests.pop_front();if(requests.empty())return;request=requests.front();}CaptureSpudStateIndex();if(!request->cancelled.load()&&!Execute(request))return;{Lock lock(queueMutex);if(!requests.empty()&&requests.front()==request)requests.pop_front();}}
bool SubmitAdminGravestoneMutation(const std::string&operation,const std::string&adminGuid,const std::string&adminId,const std::string&ownerGuid,const std::string&ownerId,const std::string&graveId,const std::string&itemId,int quantity,const std::string&session,std::string&result){
    static const std::unordered_set<std::string> allowed={"claim_all_owner","claim_all_self","claim_item_owner","claim_item_self","delete_item","delete_grave"};if(compatibility.load()!=1){result="Gravestone mutation compatibility unavailable";return false;}if(!allowed.count(operation)||graveId.size()!=16||((operation.find("_item")!=std::string::npos||operation=="delete_item")&&(itemId.size()!=16||quantity<1||quantity>1000000))){result="Invalid gravestone operation";return false;}
    auto r=std::make_shared<Request>();r->operation=operation;r->adminGuid=adminGuid;r->adminId=adminId;r->ownerGuid=ownerGuid;r->ownerId=ownerId;r->graveId=graveId;r->itemId=itemId;r->quantity=quantity;r->session=session;{Lock lock(queueMutex);if(requests.size()>=QueueCapacity||std::any_of(requests.begin(),requests.end(),[&](const auto&q){return q->graveId==graveId;})){result="Another gravestone operation is already pending";return false;}requests.push_back(r);++queued;}
    timespec deadline{};clock_gettime(CLOCK_REALTIME,&deadline);deadline.tv_sec+=18;pthread_mutex_lock(&r->mutex);while(!r->complete){int wait=pthread_cond_timedwait(&r->changed,&r->mutex,&deadline);if(wait&&!r->complete){r->cancelled=true;pthread_mutex_unlock(&r->mutex);result="Dragonwilds did not finish loading the gravestone in time";return false;}}result=r->result;bool ok=r->success;pthread_mutex_unlock(&r->mutex);return ok;
}
bool ResolveAdminGravestoneActor(const std::string&owner,const std::string&id,uintptr_t&actor,FVector3d&location){Grave grave;if(!ResolveGrave(owner,id,grave))return false;actor=grave.actor;location=grave.location;return true;}
bool ResolveAdminGravestoneLocation(const std::string&owner,const std::string&id,uintptr_t&actor,FVector3d&location){if(ResolveAdminGravestoneActor(owner,id,actor,location))return true;actor=0;return ResolveRegistryGrave(owner,id,location);}
std::string GetPlayerGravestonesJson(const std::string&characterGuid,const std::string&ownerName){
    if(!g_Engine)return {};using DisplayFn=const FString*(*)(const void*);auto display=reinterpret_cast<DisplayFn>(g_Engine->GetModuleBase()+0x04AA1A80);std::ostringstream out;out<<"{\"compatibility\":"<<(compatibility.load()==1?"true":"false")<<",\"mapVersion\":\""<<Escape(g_Engine->GetMapAssetVersion())<<"\",\"graves\":[";bool firstGrave=true;int graveCount=0,itemCount=0,unresolvedCount=0;std::unordered_set<uintptr_t> seen;std::unordered_set<std::string> seenIds;
    for(int32_t i=0;i<g_Engine->GetObjectCount();++i){uintptr_t actor=g_Engine->GetObjectByIndex(i);if(!IsGrave(actor)||!seen.insert(actor).second)continue;std::string owner=OwnerGuid(actor);if(owner.empty()){++unresolvedCount;LogUnresolvedGrave(actor,"owner");continue;}if(owner!=characterGuid)continue;uintptr_t inventory=GraveInventory(actor),root=Mem::ReadPtr(actor+Offsets::Actor_RootComponent),slots=0;int32_t slotCount=0;FVector3d location{};if(!inventory||!root||!Mem::Readable(reinterpret_cast<void*>(root+Offsets::SceneComp_RelativeLocation),sizeof(location))||!ReadSlots(inventory,slots,slotCount)){++unresolvedCount;LogUnresolvedGrave(actor,"inventory-or-location");continue;}std::memcpy(&location,reinterpret_cast<void*>(root+Offsets::SceneComp_RelativeLocation),sizeof(location));std::string id=GraveId(actor,owner,location);if(IsGraveRemoved(id)||IsRemovedActorLocation(location)){seenIds.insert(id);continue;}double mapX=0,mapY=0;bool mapValid=g_Engine->ProjectWorldToMap(location,mapX,mapY)&&std::isfinite(mapX)&&std::isfinite(mapY)&&mapX>=0&&mapX<=1&&mapY>=0&&mapY<=1;int filled=0,totalQuantity=0;for(int32_t s=0;s<slotCount;++s){uintptr_t item=Mem::ReadPtr(slots+uintptr_t(s)*8);if(item){++filled;totalQuantity+=std::max(0,Mem::ReadI32(item+0x58));}}if(!filled){MarkGraveRemoved(id,&location);seenIds.insert(id);continue;}NoteLiveGrave(id);seenIds.insert(id);if(!firstGrave)out<<',';firstGrave=false;++graveCount;out<<"{\"id\":\""<<id<<"\",\"ownerName\":\""<<Escape(ownerName)<<"\",\"filledSlots\":"<<filled<<",\"totalSlots\":"<<slotCount<<",\"totalQuantity\":"<<totalQuantity<<",\"gridColumns\":8,\"loaded\":true,\"mutationAvailable\":"<<(compatibility.load()==1?"true":"false")<<",\"mapAvailable\":"<<(mapValid?"true":"false")<<",\"teleportAvailable\":"<<(mapValid?"true":"false");if(mapValid)out<<",\"mapX\":"<<mapX<<",\"mapY\":"<<mapY;out<<",\"slots\":[";
        SavedGrave liveSnapshot;liveSnapshot.slotCount=slotCount;
        for(int32_t s=0;s<slotCount;++s){if(s)out<<',';uintptr_t item=Mem::ReadPtr(slots+uintptr_t(s)*8);if(!item||!g_Engine->IsA(item,"Item")){out<<"null";continue;}uintptr_t data=Mem::ReadPtr(item+0xB0);int quantity=Mem::ReadI32(item+0x58);if(!data||quantity<=0){out<<"null";continue;}std::string key=g_Engine->GetObjectName(data),name;const FString* text=display(reinterpret_cast<const void*>(data+0x68));if(text&&Mem::Readable(text,sizeof(FString)))name=g_Engine->ReadFString(reinterpret_cast<uintptr_t>(text));if(name.empty())name=key;std::string iconName;for(unsigned char c:key)if(std::isalnum(c)||c=='_'||c=='-')iconName+=char(c);iconName+=".png";std::string icon="/assets/items/"+iconName;std::string itemId=ItemId(id,s,key,quantity);int maximum=MaxStack(data);SavedItem snapshot;snapshot.slot=s;snapshot.quantity=quantity;snapshot.maximum=maximum;snapshot.name=name;snapshot.icon=iconName;liveSnapshot.items.push_back(std::move(snapshot));out<<"{\"id\":\""<<itemId<<"\",\"revision\":\""<<itemId<<"\",\"slot\":"<<s<<",\"name\":\""<<Escape(name)<<"\",\"quantity\":"<<quantity<<",\"maxStack\":"<<maximum<<",\"icon\":\""<<Escape(icon)<<"\"}";++itemCount;}out<<"]}";{Lock lock(savedSnapshotMutex);savedGraveSnapshots[id]=std::move(liveSnapshot);}}
    // GetGravestones() for this fingerprint copies 0x68-byte records from
    // AGameplayObjectRegistry +0x2E0. The game's map presenter consumes the
    // FVector at +0x00 and owner FDomCharacterGuid at +0x18. This registry is
    // authoritative even while the gravestone actor's World Partition cell is
    // unloaded, so retain a read-only map card until its live inventory loads.
    FUEArray best=RegistryGraves();
    for(int32_t i=0;i<best.Num;++i){uintptr_t record=reinterpret_cast<uintptr_t>(best.Data)+uintptr_t(i)*GraveRecordSize;std::string owner=g_Engine->ReadGuid(record+0x18);if(owner!=characterGuid)continue;FVector3d location{};std::memcpy(&location,reinterpret_cast<void*>(record),sizeof(location));if(!std::isfinite(location.X)||!std::isfinite(location.Y)||!std::isfinite(location.Z))continue;std::string id=GraveId(0,owner,location);if(IsGraveRemoved(id)||!seenIds.insert(id).second)continue;if(WasRecentlyLive(id)&&PlayerNearGrave(location)){MarkGraveRemoved(id);continue;}double mapX=0,mapY=0;bool mapValid=g_Engine->ProjectWorldToMap(location,mapX,mapY)&&std::isfinite(mapX)&&std::isfinite(mapY)&&mapX>=0&&mapX<=1&&mapY>=0&&mapY<=1;SavedGrave saved;bool savedValid=ReadSavedGrave(location,saved);bool fromSnapshot=false;if(!savedValid){Lock lock(savedSnapshotMutex);auto cached=savedGraveSnapshots.find(id);if(cached!=savedGraveSnapshots.end()){saved=cached->second;savedValid=true;fromSnapshot=true;}}else{Lock lock(savedSnapshotMutex);savedGraveSnapshots[id]=saved;}{static std::unordered_set<std::string>logged;Lock lock(probeMutex);std::string state=savedValid?(fromSnapshot?"snapshot":"spud"):"missing";if(logged.insert(id+state).second)LogMessage("SPUD gravestone read id="+id+" source="+state+" slots="+std::to_string(saved.slotCount)+" items="+std::to_string(saved.items.size()));}int filled=int(saved.items.size()),totalQuantity=0;for(const auto&item:saved.items)totalQuantity+=item.quantity;if(!firstGrave)out<<',';firstGrave=false;++graveCount;out<<"{\"id\":\""<<id<<"\",\"ownerName\":\""<<Escape(ownerName)<<"\",\"filledSlots\":"<<filled<<",\"totalSlots\":"<<(savedValid?saved.slotCount:0)<<",\"totalQuantity\":"<<totalQuantity<<",\"gridColumns\":8,\"loaded\":false,\"mutationAvailable\":"<<(savedValid&&compatibility.load()==1?"true":"false")<<",\"mapAvailable\":"<<(mapValid?"true":"false")<<",\"teleportAvailable\":"<<(mapValid?"true":"false");if(mapValid)out<<",\"mapX\":"<<mapX<<",\"mapY\":"<<mapY;out<<",\"slots\":[";if(savedValid){std::unordered_map<int,const SavedItem*>bySlot;for(const auto&item:saved.items)bySlot[item.slot]=&item;for(int s=0;s<saved.slotCount;++s){if(s)out<<',';auto at=bySlot.find(s);if(at==bySlot.end()){out<<"null";continue;}const auto&item=*at->second;std::string itemId=ItemId(id,s,item.guid.empty()?item.persistence:item.guid,item.quantity);std::string icon=item.icon.empty()?std::string():"/assets/items/"+item.icon;out<<"{\"id\":\""<<itemId<<"\",\"revision\":\""<<itemId<<"\",\"slot\":"<<s<<",\"name\":\""<<Escape(item.name)<<"\",\"quantity\":"<<item.quantity<<",\"maxStack\":"<<item.maximum<<",\"icon\":\""<<Escape(icon)<<"\"}";++itemCount;}}out<<"]}";}
    discovered=graveCount;unresolved=unresolvedCount;out<<"],\"graveCount\":"<<graveCount<<",\"itemCount\":"<<itemCount<<'}';return out.str();
}
AdminGravestoneDiagnostics GetAdminGravestoneDiagnostics(){AdminGravestoneDiagnostics d;d.compatibility=compatibility.load();d.inventoryCompatibility=inventoryCompatibility.load();d.registryCompatibility=registryCompatibility.load();d.streamingCompatibility=streamingCompatibility.load();d.respawnCaptureCompatibility=respawnCaptureCompatibility.load();d.persistenceCompatibility=persistenceCompatibility.load();d.discovered=discovered.load();d.unresolved=unresolved.load();d.queuedRequests=queued.load();d.acceptedMutations=accepted.load();d.rejectedRequests=rejected.load();d.staleRequests=stale.load();d.maximumExecutionMicroseconds=maximumMicros.load();return d;}
}
