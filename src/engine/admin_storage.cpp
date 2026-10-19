#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "dom_engine.h"
#include "../utils/logger.h"
#include "../utils/memory.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <atomic>
#include <chrono>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <fstream>
#include <link.h>
#include <memory>
#include <mutex>
#include <pthread.h>
#include <regex>
#include <shared_mutex>
#include <sstream>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unordered_map>
#include <unordered_set>
#include <unistd.h>

namespace DomEngine {
namespace {
constexpr const char* CurrentSha="b5017cf67bc6127dbec443e00fed1f818e82afd5d6d17f912d959a1dd87d5687";
// The current build's native storage entry points are statically understood and
// request preparation is now off-thread. Keep mutation compatibility closed in
// ordinary releases until the bounded dispatcher passes live performance testing.
bool ExpectedSha(const std::string& hash){const char*candidate=std::getenv("ZANIK_STORAGE_PERFORMANCE_CANDIDATE");struct stat marker{};const bool candidateEnabled=(candidate&&std::string(candidate)=="1")||stat("/home/container/zanik-data/storage-performance-candidate",&marker)==0;return hash=="445f8b65047d208adb04a3cb03bc7706f069306d814172ae29caec16c15bec7e"||hash=="ef39023c04e1bde785907e13b9b64d72600455f1633bbfe231f639d31d98cfac"||(candidateEnabled&&hash==CurrentSha);}
bool ExpectedDispatcherSha(const std::string& hash){return ExpectedSha(hash)||hash=="49512c0ee933c3ee4dda5fb5c2f0a25e66d7f26e34795c45b91b5c1d12b61b0c"||hash=="b5017cf67bc6127dbec443e00fed1f818e82afd5d6d17f912d959a1dd87d5687";}
constexpr std::size_t QueueCapacity=32;
constexpr uintptr_t AddItemByDataOffset=0x0A6118E0;
constexpr uintptr_t CanAddItemByDataOffset=0x0A612180;
constexpr uintptr_t MoveItemOffset=0x0A6139B0;
constexpr uintptr_t RemoveFromSlotOffset=0x0A615490;
constexpr uintptr_t CanAddItemsByDataOffset=0x0A612E40;
constexpr uintptr_t AddItemsByDataOffset=0x0A61DCF0;
constexpr uintptr_t BuildingScaledRequirementsOffset=0x0A3B7FC0;
constexpr uintptr_t FMemoryFreeOffset=0x04A60480;
constexpr uintptr_t EngineTickOffset=0x07A96BA0;
constexpr uintptr_t RenameableGetCustomNameOffset=0x0A0BBC60;
constexpr uintptr_t RenameableGetSetterIdOffset=0x0A0BBED0;
constexpr uintptr_t RenameableSetCustomNameOffset=0x0A0BC2B0;
constexpr uintptr_t FTextFromStringOffset=0x04AA6AE0;
constexpr uintptr_t UniqueNetIdDestructorOffset=0x079A74B0;
constexpr uintptr_t FTextDestructorOffset=0x049F69F0;
constexpr size_t EngineTickHookBytes=16;
struct StorageOffsets {
    uintptr_t addItemByData=AddItemByDataOffset,canAddItemByData=CanAddItemByDataOffset,moveItem=MoveItemOffset,removeFromSlot=RemoveFromSlotOffset;
    uintptr_t canAddItemsByData=CanAddItemsByDataOffset,addItemsByData=AddItemsByDataOffset,buildingRequirements=BuildingScaledRequirementsOffset,memoryFree=FMemoryFreeOffset,engineTick=EngineTickOffset;
    uintptr_t renameGet=RenameableGetCustomNameOffset,renameSetter=RenameableGetSetterIdOffset,renameSet=RenameableSetCustomNameOffset,textFromString=FTextFromStringOffset,uniqueIdDestructor=UniqueNetIdDestructorOffset,textDestructor=FTextDestructorOffset;
    uintptr_t buildingEndPlay=0x0A37C2D0;
};
StorageOffsets OffsetsFor(const std::string& hash){
    StorageOffsets value;
    if(hash==CurrentSha)value={0x0A653420,0x0A653CC0,0x0A6554F0,0x0A656FD0,0x0A654980,0x0A65F7E0,0x0A3DAA00,0x04A89480,0x07ABFE00,0x0A0EF130,0x0A0EF3A0,0x0A0EF780,0x04ACFAE0,0x079D0710,0x04A1F9F0,0x0A3B5970};
    return value;
}
uintptr_t DisplayTextOffset(){static const uintptr_t value=FileSha256("/proc/self/exe")==CurrentSha?0x04ACAA80:0x04AA1A80;return value;}

using AddItemByDataFn=bool(*)(void*,void*,int,float,const void*);
using CanAddItemByDataFn=bool(*)(void*,void*,int);
using MoveItemFn=bool(*)(void*,int,void*,int,void*,int);
using RemoveFromSlotFn=bool(*)(void*,int,int,void*);
using CanAddItemsByDataFn=bool(*)(void*,const FUEArray*);
using AddItemsByDataFn=void(*)(void*,const FUEArray*,const FUEArray*,float,const void*);
using BuildingScaledRequirementsFn=FUEArray*(*)(FUEArray*,void*,void*);
using FMemoryFreeFn=void(*)(void*);
using GetMaxStackSizeFn=int(*)(void*);
using EngineTickFn=void(*)(void*,float,int,void*);
using ExecuteGetCustomNameFn=void*(*)(void*,void*);
using ExecuteGetSetterIdFn=void*(*)(void*,void*);
using ExecuteSetCustomNameFn=void(*)(void*,const void*,const void*);
using FTextFromStringFn=void*(*)(void*,const char*);
using UniqueNetIdDestructorFn=void(*)(void*);
using FTextDestructorFn=void(*)(void*);
AddItemByDataFn addItemByData=nullptr;
CanAddItemByDataFn canAddItemByData=nullptr;
MoveItemFn moveItem=nullptr;
RemoveFromSlotFn removeFromSlot=nullptr;
CanAddItemsByDataFn canAddItemsByData=nullptr;
AddItemsByDataFn addItemsByData=nullptr;
BuildingScaledRequirementsFn buildingScaledRequirements=nullptr;
FMemoryFreeFn memoryFree=nullptr;
EngineTickFn originalEngineTick=nullptr;
ExecuteGetCustomNameFn executeGetCustomName=nullptr;
ExecuteGetSetterIdFn executeGetSetterId=nullptr;
ExecuteSetCustomNameFn executeSetCustomName=nullptr;
FTextFromStringFn fTextFromString=nullptr;
UniqueNetIdDestructorFn destroyUniqueNetId=nullptr;
FTextDestructorFn destroyFText=nullptr;

enum class JobState : std::uint8_t { Queued,Running,RollingBack,Completed,Failed };
enum class ActionKind : std::uint8_t { Add,Remove,Move,RenameConstruct,RenameSetterIdentity,RenameSet,RenameDestroyIdentity,RenameDestroyText };
enum class OperationFamily : std::uint8_t { SingleItem,SlotMovement,BatchAddition,BatchMovement,Deletion,Rename,Rollback,Count };
struct NativeAction {
    ActionKind kind=ActionKind::Add;
    OperationFamily family=OperationFamily::SingleItem;
    uintptr_t source=0,destination=0,item=0,itemData=0,actor=0,controller=0;
    std::uint64_t sourceIdentity=0,destinationIdentity=0,itemIdentity=0,itemDataIdentity=0,actorIdentity=0,controllerIdentity=0;
    int sourceSlot=-1,destinationSlot=-1,quantity=0,expectedQuantity=0;
};
struct Request {
    std::string id,operation,session,conflictKey,successMessage;
    std::vector<NativeAction> actions,rollback;
    std::atomic<JobState> state{JobState::Queued};
    std::atomic<std::size_t> actionIndex{0};
    std::atomic<std::size_t> rollbackIndex{0};
    std::atomic<int> resultCode{0};
    std::atomic<std::uint64_t> updatedUnix{0};
    std::atomic<bool> partialDelete{false};
    std::atomic<bool> finishAsFailure{false};
    bool renameTextConstructed=false;
    bool renameSetterConstructed=false;
    uintptr_t expectedWorld=0;
    std::uint64_t expectedWorldIdentity=0;
    alignas(16) unsigned char renameText[16]{};
    alignas(16) unsigned char renameSetter[48]{};
    std::array<char,97> renameValue{};
};
std::array<Request*,QueueCapacity> requestRing{};
std::atomic<std::size_t> requestHead{0},requestTail{0};
Request* activeRequest=nullptr;
std::mutex jobsMutex;
std::unordered_map<std::string,std::shared_ptr<Request>> jobs;
std::atomic<int> compatibility{0};
std::atomic<int> dispatcherCompatibility{0},batchCompatibility{0},renameCompatibility{0},removalTrackingCompatibility{0},catalogCompatibility{0},unlockCatalogCompatibility{0},planCatalogCompatibility{0};
std::atomic<int> buildingTaxonomyCompatibility{0},buildingRequirementsCompatibility{0},buildingIconsCompatibility{0},craftingRecipeCompatibility{0};
std::atomic<std::uint64_t> resolvedBuildingPieces{0},unavailableBuildingPieces{0};
std::atomic<std::uint64_t> queued{0},accepted{0},rejected{0},stale{0},maximumMicros{0};
std::atomic<std::uint64_t> tickCalls{0},maximumTickMicros{0},slowTickCalls{0};
std::atomic<std::uint64_t> maximumRecipeTickMicros{0},maximumCustomNameTickMicros{0};
std::atomic<std::uint64_t> maximumStorageTickMicros{0},maximumGravestoneTickMicros{0};
std::atomic<unsigned> pendingCount{0};
std::atomic<std::uint64_t> snapshotGenerations{0},snapshotHits{0},snapshotMisses{0},snapshotInvalidations{0},maximumSnapshotMicros{0};
std::atomic<std::uint64_t> queueDrops{0},runningJobs{0},completedJobs{0},failedJobs{0},staleIdentities{0},worldChanges{0},rollbackAttempts{0},rollbackFailures{0};
std::array<std::atomic<std::uint64_t>,size_t(OperationFamily::Count)> familyMaximum{};
std::array<std::atomic<bool>,size_t(OperationFamily::Count)> familyCircuitBroken{};
std::atomic<std::uint64_t> storageGeneration{1};
std::atomic<bool> earlyHookInstalled{false};
constexpr size_t BuildingEndPlayHookBytes=16;
constexpr uintptr_t BuildingEndPlayOffset=0x0A37C2D0;
using BuildingEndPlayFn=void(*)(void*,int);
BuildingEndPlayFn originalBuildingEndPlay=nullptr;
pthread_mutex_t removedBuildingsMutex=PTHREAD_MUTEX_INITIALIZER;
std::unordered_set<std::uint64_t> removedBuildingIdentities;
std::deque<std::uint64_t> removedBuildingOrder;
std::atomic<std::uint64_t> buildingRemovalEvents{0};
struct CachedName {std::string value;std::chrono::steady_clock::time_point refreshed;};
pthread_mutex_t customNameMutex=PTHREAD_MUTEX_INITIALIZER;
std::unordered_map<std::uint64_t,CachedName> customNames;
std::unordered_map<std::uint64_t,uintptr_t> pendingCustomNames;

std::string Escape(const std::string&s){std::string o;for(unsigned char c:s){if(c=='"'||c=='\\'){o+='\\';o+=char(c);}else if(c>=32)o+=char(c);}return o;}
const std::unordered_map<std::string,std::string>& BuildingActorNames(){
    static const auto names=[](){std::unordered_map<std::string,std::string> values;const char* root=std::getenv("DW_WEB_ROOT");std::ifstream input(std::string(root&&*root?root:"/home/container/zanik/web")+"/buildings/metadata.tsv");std::string line;while(std::getline(input,line)){if(line.empty()||line[0]=='#')continue;auto a=line.find('\t'),b=a==std::string::npos?std::string::npos:line.find('\t',a+1),c=b==std::string::npos?std::string::npos:line.find('\t',b+1);if(a==std::string::npos||b==std::string::npos||c==std::string::npos)continue;std::string name=line.substr(a+1,b-a-1),actor=line.substr(c+1);if(!actor.empty()&&actor.back()=='\r')actor.pop_back();if(!name.empty()&&!actor.empty())values[std::move(actor)]=std::move(name);}return values;}();return names;
}
std::string CleanType(std::string value){auto exact=BuildingActorNames().find(value);if(exact!=BuildingActorNames().end())return exact->second;const std::string p="BP_BaseBuilding_";if(value.rfind(p,0)==0)value.erase(0,p.size());if(value.size()>2&&value.compare(value.size()-2,2,"_C")==0)value.resize(value.size()-2);std::string readable;for(size_t i=0;i<value.size();++i){char c=value[i];if(c=='_'){if(!readable.empty()&&readable.back()!=' ')readable+=' ';continue;}if(i&&std::isupper(static_cast<unsigned char>(c))&&std::islower(static_cast<unsigned char>(value[i-1]))&&readable.back()!=' ')readable+=' ';readable+=c;}return readable.empty()?"Storage":readable;}
std::string HexHash(std::uint64_t hash){char id[17]{};std::snprintf(id,sizeof(id),"%016llx",static_cast<unsigned long long>(hash));return id;}
std::uint64_t HashBytes(std::uint64_t hash,const void*p,size_t n){for(size_t i=0;i<n;++i){hash^=reinterpret_cast<const unsigned char*>(p)[i];hash*=1099511628211ULL;}return hash;}
std::string ChestId(uintptr_t actor,const std::string&type,const std::string&guid){
    std::uint64_t h=1469598103934665603ULL;h=HashBytes(h,type.data(),type.size());h=HashBytes(h,guid.data(),guid.size());
    uintptr_t root=Mem::ReadPtr(actor+Offsets::Actor_RootComponent);FVector3d location{};
    if(root&&Mem::Readable(reinterpret_cast<void*>(root+Offsets::SceneComp_RelativeLocation),sizeof(location)))std::memcpy(&location,reinterpret_cast<void*>(root+Offsets::SceneComp_RelativeLocation),sizeof(location));
    h=HashBytes(h,&location,sizeof(location));return HexHash(h);
}
std::string ItemId(const std::string&chest,int slot,const std::string&key,int quantity){
    std::uint64_t h=1469598103934665603ULL;h=HashBytes(h,chest.data(),chest.size());h=HashBytes(h,&slot,sizeof(slot));h=HashBytes(h,key.data(),key.size());h=HashBytes(h,&quantity,sizeof(quantity));return HexHash(h);
}
std::string CatalogId(const std::string&key){std::uint64_t h=1469598103934665603ULL;h=HashBytes(h,key.data(),key.size());return HexHash(h);}
struct CatalogMeta {std::string category,aliases;int categoryOrder=999,tier=0,recipeOrder=0;};
CatalogMeta CatalogMetadata(const std::string&key,const std::string&name);
struct RecipeIngredient {uintptr_t data=0;int quantity=0;};
struct RecipeRecord {uintptr_t object=0;std::string id,key,name,type,category,station,icon,unavailableReason;int tier=0,order=0;bool available=true;std::vector<RecipeIngredient> ingredients;};
struct BuildingMeta {std::string name,icon,actor,assetPath,description;};
struct BuildingSection {int order=0;bool favourites=false;std::string key,label;std::vector<std::string> pieces;};
struct BuildingTab {int order=0;std::string key,label;std::vector<BuildingSection> sections;};
struct BuildingCatalog {bool valid=false,iconsValid=false;int expectedPieces=0,expectedReferences=0,schema=0;std::string fingerprint;std::vector<BuildingTab> tabs;std::unordered_map<std::string,BuildingMeta> pieces;};
struct PlanCatalog {bool valid=false;int expected=0;std::string fingerprint;std::unordered_set<std::string> keys;};
std::vector<std::string> SplitTabs(const std::string&line){std::vector<std::string> out;size_t start=0;for(;;){auto at=line.find('\t',start);out.push_back(line.substr(start,at==std::string::npos?std::string::npos:at-start));if(at==std::string::npos)return out;start=at+1;}}
const BuildingCatalog& AuthoritativeBuildingCatalog(){
    static const auto catalog=[](){BuildingCatalog value;const char* root=std::getenv("DW_WEB_ROOT");std::string base=std::string(root&&*root?root:"/home/container/zanik/web")+"/buildings/";std::ifstream input(base+"catalog.tsv");std::string line;int declaredTabs=0,declaredSections=0;
        while(std::getline(input,line)){if(!line.empty()&&line.back()=='\r')line.pop_back();auto f=SplitTabs(line);if(f.empty())continue;if(f[0]=="# zanik-building-catalog"&&f.size()>=7){value.schema=std::atoi(f[1].c_str());declaredTabs=std::atoi(f[2].c_str());declaredSections=std::atoi(f[3].c_str());value.expectedReferences=std::atoi(f[4].c_str());value.expectedPieces=std::atoi(f[5].c_str());value.fingerprint=f[6];}
            else if(f[0]=="T"&&f.size()>=4){BuildingTab tab;tab.order=std::atoi(f[1].c_str());tab.key=f[2];tab.label=f[3];value.tabs.push_back(std::move(tab));}
            else if(f[0]=="S"&&f.size()>=6){int tab=std::atoi(f[1].c_str());if(tab<0||size_t(tab)>=value.tabs.size())continue;BuildingSection section;section.order=std::atoi(f[2].c_str());section.favourites=f[3]=="1";section.key=f[4];section.label=f[5];value.tabs[tab].sections.push_back(std::move(section));}
            else if(f[0]=="M"&&f.size()>=5){int tab=std::atoi(f[1].c_str()),section=std::atoi(f[2].c_str());if(tab>=0&&section>=0&&size_t(tab)<value.tabs.size()&&size_t(section)<value.tabs[tab].sections.size())value.tabs[tab].sections[section].pieces.push_back(f[4]);}
            else if(f[0]=="P"&&f.size()>=7)value.pieces.emplace(f[1],BuildingMeta{f[2],f[3],f[4],f[5],f[6]});}
        size_t sections=0,references=0;bool icons=true;for(const auto&tab:value.tabs)for(const auto&section:tab.sections){++sections;references+=section.pieces.size();}for(const auto&p:value.pieces){struct stat st{};if(stat((base+p.second.icon).c_str(),&st)!=0||st.st_size<1){icons=false;break;}}
        const bool fingerprintValid=value.fingerprint.size()==64&&std::all_of(value.fingerprint.begin(),value.fingerprint.end(),[](unsigned char c){return std::isxdigit(c)!=0;});
        value.iconsValid=icons;value.valid=value.schema==1&&fingerprintValid&&declaredTabs==8&&declaredSections==63&&value.expectedReferences==844&&value.expectedPieces==827&&value.tabs.size()==size_t(declaredTabs)&&sections==size_t(declaredSections)&&references==size_t(value.expectedReferences)&&value.pieces.size()==size_t(value.expectedPieces);return value;}();return catalog;
}
const std::unordered_map<std::string,BuildingMeta>& BuildingMetadata(){
    return AuthoritativeBuildingCatalog().pieces;
}
const PlanCatalog& PlanItemCatalog(){
    static const auto catalog=[](){PlanCatalog value;const char* root=std::getenv("DW_WEB_ROOT");std::string base=std::string(root&&*root?root:"/home/container/zanik/web");std::ifstream input(base+"/plan-items.tsv");std::string line;
        while(std::getline(input,line)){if(!line.empty()&&line.back()=='\r')line.pop_back();auto f=SplitTabs(line);if(f.empty())continue;if(f[0]=="# zanik-plan-items"&&f.size()==4){if(f[1]!="1")return value;value.expected=std::atoi(f[2].c_str());value.fingerprint=f[3];continue;}if(f.size()!=4||f[0].rfind("DA_Consumable_Plan",0)!=0||f[1].rfind("PLAN: ",0)!=0||f[2].size()!=22||f[3]!=f[0]+".png")return PlanCatalog{};struct stat st{};if(stat((base+"/items/"+f[3]).c_str(),&st)!=0||st.st_size<1)return PlanCatalog{};value.keys.insert(f[0]);}
        const bool fingerprintValid=value.fingerprint.size()==64&&std::all_of(value.fingerprint.begin(),value.fingerprint.end(),[](unsigned char c){return std::isxdigit(c)!=0;});value.valid=value.expected==123&&value.keys.size()==size_t(value.expected)&&fingerprintValid;return value;}();return catalog;
}
std::string CleanAssetName(std::string value){for(const char*prefix:{"RECIPE_","BUILDPIECE_","DA_"})if(value.rfind(prefix,0)==0){value.erase(0,std::strlen(prefix));break;}std::replace(value.begin(),value.end(),'_',' ');return value;}
bool RejectedRecipeKey(const std::string&key){std::string lower=key;std::transform(lower.begin(),lower.end(),lower.begin(),[](unsigned char c){return char(std::tolower(c));});return lower.find("test")!=std::string::npos||lower.find("debug")!=std::string::npos||lower.find("deprecated")!=std::string::npos||lower.find("_old")!=std::string::npos||lower.find("refund")!=std::string::npos||lower.find("journal")!=std::string::npos||lower.find("vendor")!=std::string::npos||lower.find("proxy")!=std::string::npos||lower.find("deriveddata")!=std::string::npos;}
std::string ItemDisplayName(uintptr_t data){using Fn=const FString*(*)(const void*);auto fn=reinterpret_cast<Fn>(g_Engine->GetModuleBase()+DisplayTextOffset());const FString* text=fn(reinterpret_cast<const void*>(data+0x68));if(text&&Mem::Readable(text,sizeof(FString))){auto value=g_Engine->ReadFString(reinterpret_cast<uintptr_t>(text));if(!value.empty())return value;}return CleanAssetName(g_Engine->GetObjectName(data));}
bool ReadRecipeIngredients(uintptr_t object,std::vector<RecipeIngredient>&out){uintptr_t data=Mem::ReadPtr(object+0x60);int count=Mem::ReadI32(object+0x68);if(count<=0||count>128||!data||!Mem::Readable(reinterpret_cast<void*>(data),size_t(count)*16))return false;for(int i=0;i<count;++i){uintptr_t item=Mem::ReadPtr(data+uintptr_t(i)*16);int quantity=Mem::ReadI32(data+uintptr_t(i)*16+8);if(!item||!g_Engine->IsA(item,"ItemData")||quantity<=0||quantity>1000000)return false;out.push_back({item,quantity});}return !out.empty();}
bool ReadBuildingIngredients(uintptr_t object,std::vector<RecipeIngredient>&out){FUEArray scaled{};buildingScaledRequirements(&scaled,reinterpret_cast<void*>(object),reinterpret_cast<void*>(g_Engine->GetWorld()));if(scaled.Num<=0||scaled.Num>128||!scaled.Data||!Mem::Readable(scaled.Data,size_t(scaled.Num)*16)){if(scaled.Data)memoryFree(scaled.Data);return false;}auto base=reinterpret_cast<uintptr_t>(scaled.Data);for(int i=0;i<scaled.Num;++i){int quantity=Mem::ReadI32(base+uintptr_t(i)*16);uintptr_t item=Mem::ReadPtr(base+uintptr_t(i)*16+8);if(!item||!g_Engine->IsA(item,"ItemData")||quantity<=0||quantity>1000000){memoryFree(scaled.Data);return false;}out.push_back({item,quantity});}memoryFree(scaled.Data);return !out.empty();}
std::vector<RecipeRecord> RecipeRecords(){std::vector<RecipeRecord> result;if(!g_Engine||!buildingScaledRequirements||!memoryFree)return result;std::unordered_set<std::string> resolved;for(int32_t i=0;i<g_Engine->GetObjectCount();++i){uintptr_t object=g_Engine->GetObjectByIndex(i);if(!object)continue;std::string key=g_Engine->GetObjectName(object);bool crafting=g_Engine->IsA(object,"RecipeData"),building=g_Engine->IsA(object,"BuildingPieceData");if((!crafting&&!building)||key.rfind("Default__",0)==0||RejectedRecipeKey(key))continue;auto buildingMeta=BuildingMetadata().find(key);if(building&&buildingMeta==BuildingMetadata().end())continue;RecipeRecord r;r.object=object;r.key=key;r.id=CatalogId(std::string(crafting?"recipe:":"building:")+key);r.type=crafting?"crafting":"building";if(crafting){if(!ReadRecipeIngredients(object,r.ingredients))continue;uintptr_t outputs=Mem::ReadPtr(object+0x90);int count=Mem::ReadI32(object+0x98);if(count<=0||count>64||!outputs||!Mem::Readable(reinterpret_cast<void*>(outputs),size_t(count)*16))continue;uintptr_t output=Mem::ReadPtr(outputs);if(!output||!g_Engine->IsA(output,"ItemData"))continue;r.name=ItemDisplayName(output);r.icon="/assets/items/"+g_Engine->GetObjectName(output)+".png";auto meta=CatalogMetadata(g_Engine->GetObjectName(output),r.name);r.category=meta.category;r.tier=meta.tier;r.order=meta.recipeOrder;r.station=key.find("Process")!=std::string::npos?"Processing station":"Crafting";}else{if(!ReadBuildingIngredients(object,r.ingredients))continue;r.name=buildingMeta->second.name;r.icon="/assets/buildings/"+buildingMeta->second.icon;r.category="Building";std::smatch match;if(std::regex_search(key,match,std::regex("(?:^|_)T([0-9]+)(?:_|$)")))r.tier=std::stoi(match[1].str());r.station="Building";resolved.insert(key);}result.push_back(std::move(r));}for(const auto&entry:BuildingMetadata())if(!resolved.count(entry.first)){RecipeRecord r;r.key=entry.first;r.id=CatalogId("building:"+entry.first);r.name=entry.second.name;r.icon="/assets/buildings/"+entry.second.icon;r.type="building";r.category="Building";r.station="Building";r.available=false;r.unavailableReason="Recipe materials unavailable for this game build";result.push_back(std::move(r));}resolvedBuildingPieces=resolved.size();unavailableBuildingPieces=AuthoritativeBuildingCatalog().expectedPieces>int(resolved.size())?AuthoritativeBuildingCatalog().expectedPieces-resolved.size():0;std::sort(result.begin(),result.end(),[](const auto&a,const auto&b){if(a.type!=b.type)return a.type<b.type;if(a.category!=b.category)return a.category<b.category;if(a.tier!=b.tier)return a.tier<b.tier;return a.name<b.name;});return result;}
bool ResolveRecipe(const std::string&id,RecipeRecord&out){for(auto&r:RecipeRecords())if(r.id==id&&r.available){out=std::move(r);return true;}return false;}
CatalogMeta CatalogMetadata(const std::string&key,const std::string&name){
    CatalogMeta m;std::string lower=key;std::transform(lower.begin(),lower.end(),lower.begin(),[](unsigned char c){return char(std::tolower(c));});
    auto set=[&](const char* category,int order){m.category=category;m.categoryOrder=order;};
    if(lower.rfind("da_consumable_vestige_",0)==0)set("Vestiges",105);
    else if(lower.rfind("da_consumabe_mount_",0)==0)set("Mounts",106);
    else if(lower.find("_consumable_potion_")!=std::string::npos||lower.find("curediseasepotion")!=std::string::npos)set("Potions",50);
    else if(lower.find("_consumable_tea_")!=std::string::npos||lower.find("_consumable_water_")!=std::string::npos||lower.find("_drink_")!=std::string::npos||lower.find("_ale_")!=std::string::npos||lower.find("_beer_")!=std::string::npos||lower.find("_wine_")!=std::string::npos)set("Drinks",40);
    else if(lower.find("item_consumable_")==0)set("Food",30);
    else if(lower.find("item_armour_")==0||lower.find("item_cape_")==0||lower.find("item_shield_")==0||lower.find("item_jewellery_")==0||lower.find("item_trinket_")==0)set("Armour",20);
    else if(lower.find("item_sword_")==0||lower.find("item_dagger_")==0||lower.find("item_mace_")==0||lower.find("item_club_")==0||lower.find("item_greataxe_")==0||lower.find("item_greatsword_")==0||lower.find("item_scimitar_")==0||lower.find("item_staff_")==0||lower.find("item_wand_")==0||lower.find("item_shortbow_")==0||lower.find("item_longbow_")==0||lower.find("item_crossbow_")==0||lower.find("item_scarabstaff_")==0)set("Weapons",10);
    else if(lower.find("item_pickaxe_")==0||lower.find("item_logging_")==0||lower.find("item_spade_")==0||lower.find("item_hammer_")==0||lower.find("item_fishingrod_")==0||lower.find("item_fishingnet_")==0||lower.find("item_wateringcan_")==0||lower.find("item_secateurs_")==0||lower.find("item_torch_")==0)set("Tools",25);
    else if(lower.find("item_ammo_")==0||lower.find("item_fishbait_")==0)set("Ammunition",60);
    else if(lower.find("item_rune_")==0)set("Runes",70);
    else if(lower.rfind("da_consumable_plan",0)==0)set("Plans",85);
    else if(lower.find("item_farming_seed_")==0||lower.find("_seed_")!=std::string::npos)set("Seeds",80);
    else if(lower.find("item_resources_")==0||lower.find("item_resource_")==0||lower.find("item_herb_")==0)set("Resources",90);
    else if(lower.find("item_farming_")==0)set("Farming",100);
    else if(lower.find("item_quest_")==0||lower.find("item_activity_")==0||lower.find("item_journal_")==0||lower.find("item_artifact_")==0)set("Quest items",110);
    else if(lower.find("item_fuel_")==0||lower.find("item_bucket_")==0)set("Materials",95);
    else set("Miscellaneous",999);
    std::smatch match;if(std::regex_search(key,match,std::regex("(?:^|_)T([0-9]+)(?:_|$)")))m.tier=std::stoi(match[1].str());
    static const std::pair<const char*,int> progression[]={{"Wood",1},{"Stone",1},{"Bone",2},{"Bronze",3},{"Iron",4},{"Steel",5},{"Mithril",6},{"Adamant",7},{"Rune",8}};
    if(!m.tier)for(const auto& value:progression){std::string token=value.first;std::transform(token.begin(),token.end(),token.begin(),[](unsigned char c){return char(std::tolower(c));});if(lower.find("_"+token+"_")!=std::string::npos||(lower.size()>token.size()&&lower.compare(lower.size()-token.size(),token.size(),token)==0)){m.tier=value.second;break;}}
    std::string alias=key.rfind("ITEM_",0)==0?key.substr(5):key;std::replace(alias.begin(),alias.end(),'_',' ');m.aliases=alias+" "+name;m.recipeOrder=m.tier?m.tier*100:100000;return m;
}
struct Chest {uintptr_t actor=0,inventory=0;std::string id,type,guid;};
std::vector<Chest> FindChests(const std::string&guid){
    std::vector<Chest> result;if(!g_Engine)return result;std::unordered_map<uintptr_t,uintptr_t> owners;std::unordered_set<uintptr_t> linked;
    const int32_t count=g_Engine->GetObjectCount();
    for(int32_t i=0;i<count;++i){uintptr_t o=g_Engine->GetObjectByIndex(i);if(!o||g_Engine->GetObjectName(o).rfind("Default__",0)==0)continue;uintptr_t actor=Mem::ReadPtr(o+Offsets::UObject_Outer);if(!actor)continue;if(g_Engine->IsA(o,"OwnershipComponent"))owners[actor]=o;else if(g_Engine->IsA(o,"LinkedInventoryComponent"))linked.insert(actor);}
    for(int32_t i=0;i<count;++i){uintptr_t inv=g_Engine->GetObjectByIndex(i);if(!inv||!g_Engine->IsObjectLive(inv)||!g_Engine->IsA(inv,"InventoryComponent")||g_Engine->IsA(inv,"LinkedInventoryComponent"))continue;uintptr_t actor=Mem::ReadPtr(inv+Offsets::UObject_Outer);auto it=owners.find(actor);if(it==owners.end()||!linked.count(actor)||!g_Engine->IsObjectLive(actor)||WasAdminStorageBuildingRemoved(actor)||g_Engine->ReadGuid(it->second+0x158)!=guid)continue;bool beingDestroyed=false;if(g_Engine->ReadBoolProperty(actor,"bActorIsBeingDestroyed",beingDestroyed)&&beingDestroyed)continue;std::string type=CleanType(g_Engine->GetObjectClassName(actor));result.push_back({actor,inv,ChestId(actor,type,guid),type,guid});}
    return result;
}
bool FindChest(const std::string&guid,const std::string&id,Chest&out){
    if(!g_Engine)return false;std::unordered_map<uintptr_t,uintptr_t> owners;
    const int32_t count=g_Engine->GetObjectCount();
    for(int32_t i=0;i<count;++i){uintptr_t o=g_Engine->GetObjectByIndex(i);if(!o||g_Engine->GetObjectName(o).rfind("Default__",0)==0)continue;uintptr_t actor=Mem::ReadPtr(o+Offsets::UObject_Outer);if(actor&&g_Engine->IsA(o,"OwnershipComponent"))owners[actor]=o;}
    for(int32_t i=0;i<count;++i){uintptr_t inv=g_Engine->GetObjectByIndex(i);if(!inv||!g_Engine->IsObjectLive(inv)||!g_Engine->IsA(inv,"InventoryComponent")||g_Engine->IsA(inv,"LinkedInventoryComponent"))continue;uintptr_t actor=Mem::ReadPtr(inv+Offsets::UObject_Outer);auto it=owners.find(actor);if(it==owners.end()||!g_Engine->IsObjectLive(actor)||WasAdminStorageBuildingRemoved(actor)||g_Engine->ReadGuid(it->second+0x158)!=guid)continue;bool beingDestroyed=false;if(g_Engine->ReadBoolProperty(actor,"bActorIsBeingDestroyed",beingDestroyed)&&beingDestroyed)continue;std::string type=CleanType(g_Engine->GetObjectClassName(actor));std::string candidate=ChestId(actor,type,guid);if(candidate==id){out={actor,inv,candidate,type,guid};return true;}}
    return false;
}

bool ResolveStorageLocationInternal(const std::string& characterGuid,const std::string& chestId,FVector3d& location){
    Chest chest;if(!FindChest(characterGuid,chestId,chest))return false;
    const uintptr_t root=Mem::ReadPtr(chest.actor+Offsets::Actor_RootComponent);
    if(!root||!Mem::Readable(reinterpret_cast<void*>(root+Offsets::SceneComp_RelativeLocation),sizeof(location)))return false;
    std::memcpy(&location,reinterpret_cast<void*>(root+Offsets::SceneComp_RelativeLocation),sizeof(location));
    return std::isfinite(location.X)&&std::isfinite(location.Y)&&std::isfinite(location.Z);
}
bool ReadSlots(uintptr_t inventory,uintptr_t&data,int32_t&num){data=Mem::ReadPtr(inventory+0x168);num=Mem::ReadI32(inventory+0x170);int32_t max=Mem::ReadI32(inventory+0x174);return num>=0&&num<=512&&max>=num&&(!num||(data&&Mem::Readable(reinterpret_cast<void*>(data),size_t(num)*8)));}
int MaxStackSize(uintptr_t itemData){if(!itemData||!Mem::Readable(reinterpret_cast<void*>(itemData),sizeof(uintptr_t)))return 1;uintptr_t table=Mem::ReadPtr(itemData),fn=table?Mem::ReadPtr(table+0x2D8):0;if(!fn||!Mem::InImage(reinterpret_cast<void*>(fn)))return 1;int value=reinterpret_cast<GetMaxStackSizeFn>(fn)(reinterpret_cast<void*>(itemData));return value>=1&&value<=1000000?value:1;}
struct SnapshotItem {
    std::string id,key,name,icon;
    int slot=-1,quantity=0,maxStack=1;
    uintptr_t object=0,data=0;
    std::uint64_t objectIdentity=0,dataIdentity=0;
};
struct SnapshotChest {
    std::string id,type,customName;
    uintptr_t actor=0,inventory=0;
    std::uint64_t actorIdentity=0,inventoryIdentity=0;
    FVector3d location{};
    bool linked=false,privateStorage=false,mapValid=false;
    double mapX=0,mapY=0;
    int totalSlots=0;
    std::vector<SnapshotItem> items;
};
struct StorageSnapshot {
    std::uint64_t generation=0,removalGeneration=0;
    uintptr_t world=0,adminController=0,adminPawn=0,adminInventory=0;
    std::uint64_t worldIdentity=0,adminControllerIdentity=0,adminPawnIdentity=0,adminInventoryIdentity=0;
    bool adminFound=false,adminSpawned=false,adminDead=false;
    std::string targetGuid,adminGuid;
    std::vector<SnapshotChest> chests;
    std::chrono::steady_clock::time_point createdAt;
};
std::shared_mutex snapshotMutex;
std::unordered_map<std::string,std::shared_ptr<const StorageSnapshot>> snapshotCache;

std::string ReadCustomNameDirect(uintptr_t actor){
    if(!g_Engine||!actor)return {};const int32_t offset=g_Engine->FindPropertyOffset(actor,"CustomName");
    if(offset<0||offset>0x4000||!Mem::Readable(reinterpret_cast<void*>(actor+uintptr_t(offset)),16))return {};
    using DisplayFn=const FString*(*)(const void*);auto display=reinterpret_cast<DisplayFn>(g_Engine->GetModuleBase()+DisplayTextOffset());
    const FString* shown=display(reinterpret_cast<const void*>(actor+uintptr_t(offset)));
    return shown&&Mem::Readable(shown,sizeof(FString))?g_Engine->ReadFString(reinterpret_cast<uintptr_t>(shown)):std::string{};
}
std::shared_ptr<const StorageSnapshot> BuildStorageSnapshot(const std::string&targetGuid,const std::string&adminGuid,bool allowCache){
    const auto started=std::chrono::steady_clock::now();const std::string cacheKey=targetGuid+'\n'+adminGuid;
    if(allowCache){std::shared_lock<std::shared_mutex> lock(snapshotMutex);auto found=snapshotCache.find(cacheKey);if(found!=snapshotCache.end()&&found->second&&std::chrono::steady_clock::now()-found->second->createdAt<std::chrono::seconds(2)&&found->second->generation==storageGeneration.load()&&found->second->world==g_Engine->GetWorld()&&found->second->worldIdentity==g_Engine->GetObjectIdentity(found->second->world)&&found->second->removalGeneration==buildingRemovalEvents.load()){++snapshotHits;return found->second;}}
    ++snapshotMisses;auto snapshot=std::make_shared<StorageSnapshot>();snapshot->createdAt=std::chrono::steady_clock::now();snapshot->generation=storageGeneration.load();snapshot->removalGeneration=buildingRemovalEvents.load();snapshot->targetGuid=targetGuid;snapshot->adminGuid=adminGuid;snapshot->world=g_Engine?g_Engine->GetWorld():0;snapshot->worldIdentity=g_Engine&&snapshot->world?g_Engine->GetObjectIdentity(snapshot->world):0;
    if(!g_Engine||!snapshot->world||!snapshot->worldIdentity)return {};
    std::unordered_map<uintptr_t,uintptr_t> owners,linked,inventories;const int32_t count=g_Engine->GetObjectCount();
    for(int32_t i=0;i<count;++i){uintptr_t object=g_Engine->GetObjectByIndex(i);if(!object)continue;const std::string name=g_Engine->GetObjectName(object);if(name.rfind("Default__",0)==0||name.find("GEN_VARIABLE")!=std::string::npos)continue;uintptr_t actor=Mem::ReadPtr(object+Offsets::UObject_Outer);if(!actor)continue;if(g_Engine->IsA(object,"OwnershipComponent"))owners[actor]=object;else if(g_Engine->IsA(object,"LinkedInventoryComponent"))linked[actor]=object;else if(g_Engine->IsA(object,"InventoryComponent"))inventories[actor]=object;}
    PlayerInfo admin{};for(const auto&player:g_Engine->GetAllPlayers())if(player.characterGuid==adminGuid){admin=player;snapshot->adminFound=true;snapshot->adminSpawned=player.spawned;snapshot->adminDead=player.isDead;snapshot->adminController=player.controllerPtr;snapshot->adminPawn=player.pawnPtr;snapshot->adminControllerIdentity=g_Engine->GetObjectIdentity(player.controllerPtr);snapshot->adminPawnIdentity=g_Engine->GetObjectIdentity(player.pawnPtr);break;}
    for(const auto&entry:inventories){uintptr_t actor=entry.first,inventory=entry.second;auto own=owners.find(actor),link=linked.find(actor);
        if(snapshot->adminFound&&!snapshot->adminInventory&&(actor==snapshot->adminController||actor==snapshot->adminPawn)){const std::string name=g_Engine->GetObjectName(inventory);if(!g_Engine->IsA(inventory,"LinkedInventoryComponent")&&!g_Engine->IsA(inventory,"PersonalInventoryComponent")&&!g_Engine->IsA(inventory,"LoadoutComponent")&&name.find("Inventory")!=std::string::npos){snapshot->adminInventory=inventory;snapshot->adminInventoryIdentity=g_Engine->GetObjectIdentity(inventory);}}
        if(own==owners.end())continue;
        if(link==linked.end()||g_Engine->ReadGuid(own->second+0x158)!=targetGuid||!g_Engine->IsObjectLive(actor)||!g_Engine->IsObjectLive(inventory))continue;bool destroying=false;if(g_Engine->ReadBoolProperty(actor,"bActorIsBeingDestroyed",destroying)&&destroying)continue;
        SnapshotChest chest;chest.actor=actor;chest.inventory=inventory;chest.actorIdentity=g_Engine->GetObjectIdentity(actor);chest.inventoryIdentity=g_Engine->GetObjectIdentity(inventory);if(!chest.actorIdentity||!chest.inventoryIdentity)continue;chest.type=CleanType(g_Engine->GetObjectClassName(actor));chest.id=ChestId(actor,chest.type,targetGuid);chest.customName=ReadCustomNameDirect(actor);chest.linked=Mem::ReadU8(link->second+0xF8)!=0;chest.privateStorage=Mem::ReadU8(own->second+0x169)!=0;
        uintptr_t root=Mem::ReadPtr(actor+Offsets::Actor_RootComponent);if(root&&Mem::Readable(reinterpret_cast<void*>(root+Offsets::SceneComp_RelativeLocation),sizeof(chest.location)))std::memcpy(&chest.location,reinterpret_cast<void*>(root+Offsets::SceneComp_RelativeLocation),sizeof(chest.location));chest.mapValid=g_Engine->ProjectWorldToMap(chest.location,chest.mapX,chest.mapY)&&std::isfinite(chest.mapX)&&std::isfinite(chest.mapY)&&chest.mapX>=0&&chest.mapX<=1&&chest.mapY>=0&&chest.mapY<=1;
        uintptr_t slots=0;int32_t num=0;if(!ReadSlots(inventory,slots,num))continue;chest.totalSlots=num;for(int32_t slot=0;slot<num;++slot){uintptr_t item=Mem::ReadPtr(slots+uintptr_t(slot)*8);if(!item||!g_Engine->IsA(item,"Item"))continue;uintptr_t data=Mem::ReadPtr(item+0xB0);int quantity=Mem::ReadI32(item+0x58);if(!data||quantity<=0||quantity>1000000)continue;SnapshotItem value;value.slot=slot;value.quantity=quantity;value.object=item;value.data=data;value.objectIdentity=g_Engine->GetObjectIdentity(item);value.dataIdentity=g_Engine->GetObjectIdentity(data);value.key=g_Engine->GetObjectName(data);value.id=ItemId(chest.id,slot,value.key,quantity);value.name=ItemDisplayName(data);value.icon="/assets/items/"+value.key+".png";value.maxStack=MaxStackSize(data);chest.items.push_back(std::move(value));}snapshot->chests.push_back(std::move(chest));}
    std::sort(snapshot->chests.begin(),snapshot->chests.end(),[](const auto&a,const auto&b){return a.id<b.id;});{std::unique_lock<std::shared_mutex> lock(snapshotMutex);snapshotCache[cacheKey]=snapshot;}++snapshotGenerations;const auto micros=std::uint64_t(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-started).count());if(micros>maximumSnapshotMicros.load())maximumSnapshotMicros.store(micros);return snapshot;
}
const SnapshotChest* SnapshotFindChest(const StorageSnapshot&s,const std::string&id){for(const auto&chest:s.chests)if(chest.id==id)return &chest;return nullptr;}
const SnapshotItem* SnapshotFindItem(const SnapshotChest&chest,const std::string&id){for(const auto&item:chest.items)if(item.id==id)return &item;return nullptr;}
bool IsUnlockCatalogKey(const std::string&key){return key.rfind("DA_Consumable_Vestige_",0)==0||key.rfind("DA_Consumabe_Mount_",0)==0;}
bool AllowedCatalogKey(const std::string&key){return key.rfind("ITEM_",0)==0||(unlockCatalogCompatibility.load()==1&&IsUnlockCatalogKey(key))||(planCatalogCompatibility.load()==1&&PlanItemCatalog().keys.count(key));}
bool UnlockCatalogAssetsValid(){
    static const char* required[]={"DA_Consumable_Vestige_Weapon_Club_Swingslash.png","DA_Consumable_Vestige_Weapon_Longbow_HunterStag.png","DA_Consumable_Vestige_Weapon_Staff_GrievingMoon.png","DA_Consumable_Vestige_Weapon_Staff_ToktzMejTal.png","DA_Consumable_Vestige_Weapon_Whip_Abysmal.png","DA_Consumabe_Mount_TerrorBird_02_04.png","DA_Consumabe_Mount_TerrorBird_02_02.png"};
    const char* root=std::getenv("DW_WEB_ROOT");const std::string itemRoot=std::string(root&&*root?root:"/home/container/zanik/web")+"/items/";
    for(const char*name:required){struct stat st{};std::string path=itemRoot+name;if(stat(path.c_str(),&st)!=0||st.st_size<1)return false;}return true;
}
uintptr_t FindCatalogData(const std::string&id,std::string*keyOut=nullptr){
    if(!g_Engine)return 0;const int32_t count=g_Engine->GetObjectCount();
    for(int32_t i=0;i<count;++i){uintptr_t data=g_Engine->GetObjectByIndex(i);if(!data||!g_Engine->IsA(data,"ItemData"))continue;std::string key=g_Engine->GetObjectName(data);if(!AllowedCatalogKey(key)||CatalogId(key)!=id)continue;if(keyOut)*keyOut=key;return data;}return 0;
}
uintptr_t FindPlayerInventory(const PlayerInfo&player){
    if(!g_Engine||!player.controllerPtr)return 0;const int32_t count=g_Engine->GetObjectCount();
    for(int32_t i=0;i<count;++i){uintptr_t inv=g_Engine->GetObjectByIndex(i);if(!inv||!g_Engine->IsA(inv,"InventoryComponent")||g_Engine->IsA(inv,"LinkedInventoryComponent")||g_Engine->IsA(inv,"PersonalInventoryComponent")||g_Engine->IsA(inv,"LoadoutComponent"))continue;uintptr_t outer=Mem::ReadPtr(inv+Offsets::UObject_Outer);if(outer!=player.controllerPtr&&outer!=player.pawnPtr)continue;std::string name=g_Engine->GetObjectName(inv);if(name.find("Inventory")!=std::string::npos)return inv;}return 0;
}
bool FindItem(const Chest&chest,const std::string&id,int&slot,uintptr_t&item,uintptr_t&itemData,int&quantity,std::string&key){
    uintptr_t data=0;int32_t num=0;if(!ReadSlots(chest.inventory,data,num))return false;
    for(int32_t i=0;i<num;++i){uintptr_t candidate=Mem::ReadPtr(data+uintptr_t(i)*8);if(!candidate||!g_Engine->IsA(candidate,"Item"))continue;uintptr_t candidateData=Mem::ReadPtr(candidate+0xB0);int q=Mem::ReadI32(candidate+0x58);if(!candidateData||q<=0)continue;std::string candidateKey=g_Engine->GetObjectName(candidateData);if(ItemId(chest.id,i,candidateKey,q)==id){slot=i;item=candidate;itemData=candidateData;quantity=q;key=candidateKey;return true;}}
    return false;
}
struct ResolvedItem {int slot=-1,quantity=0;uintptr_t item=0,data=0;std::string key,id;};
bool ResolveItems(const Chest&chest,const std::vector<std::string>&ids,std::vector<ResolvedItem>&out){
    if(ids.empty()||ids.size()>128)return false;std::unordered_set<std::string> unique;
    for(const auto&id:ids){if(id.size()!=16||!unique.insert(id).second)return false;ResolvedItem value;value.id=id;
        if(!FindItem(chest,id,value.slot,value.item,value.data,value.quantity,value.key))return false;out.push_back(std::move(value));}
    return true;
}
bool BatchFits(const Chest&chest,const std::vector<ResolvedItem>&items){
    std::vector<unsigned char> containers(items.size()*16);
    for(size_t i=0;i<items.size();++i){void* data=reinterpret_cast<void*>(items[i].data);std::memcpy(containers.data()+i*16,&data,8);std::memcpy(containers.data()+i*16+8,&items[i].quantity,4);}
    FUEArray values{containers.data(),int32_t(items.size()),int32_t(items.size())};return canAddItemsByData(reinterpret_cast<void*>(chest.inventory),&values);
}
PlayerInfo FindPlayer(const std::string&guid,bool&found){found=false;PlayerInfo result;if(!g_Engine)return result;for(const auto&p:g_Engine->GetAllPlayers())if(p.characterGuid==guid){result=p;found=true;break;}return result;}

struct Lock{pthread_mutex_t* value;explicit Lock(pthread_mutex_t&v):value(&v){pthread_mutex_lock(value);}~Lock(){pthread_mutex_unlock(value);}};
bool IdentityMatches(uintptr_t object,std::uint64_t identity){return object&&identity&&g_Engine&&g_Engine->GetObjectIdentity(object)==identity;}
std::uint64_t UnixNow(){return std::uint64_t(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count());}
void FinishJob(Request&r,bool success,int code){r.resultCode.store(code);r.updatedUnix.store(UnixNow());r.state.store(success?JobState::Completed:JobState::Failed,std::memory_order_release);pendingCount.fetch_sub(1);if(success){++accepted;++completedJobs;}else{++rejected;++failedJobs;}snapshotInvalidations.fetch_add(1);storageGeneration.fetch_add(1);}
bool RunNativeAction(Request&r,const NativeAction&a){
    const bool cleanup=a.kind==ActionKind::RenameDestroyIdentity||a.kind==ActionKind::RenameDestroyText;
    if(cleanup){
        if(a.kind==ActionKind::RenameDestroyIdentity&&r.renameSetterConstructed){destroyUniqueNetId(r.renameSetter);r.renameSetterConstructed=false;}
        if(a.kind==ActionKind::RenameDestroyText&&r.renameTextConstructed){destroyFText(r.renameText);r.renameTextConstructed=false;}
        return true;
    }
    if(!g_Engine||g_Engine->GetWorld()!=r.expectedWorld||g_Engine->GetObjectIdentity(r.expectedWorld)!=r.expectedWorldIdentity){++worldChanges;r.resultCode=-3;return false;}
    if(familyCircuitBroken[size_t(a.family)].load()){r.resultCode=-4;return false;}
    if((a.source&&!IdentityMatches(a.source,a.sourceIdentity))||(a.destination&&!IdentityMatches(a.destination,a.destinationIdentity))||(a.item&&!IdentityMatches(a.item,a.itemIdentity))||(a.itemData&&!IdentityMatches(a.itemData,a.itemDataIdentity))||(a.actor&&!IdentityMatches(a.actor,a.actorIdentity))||(a.controller&&!IdentityMatches(a.controller,a.controllerIdentity))){++stale;++staleIdentities;r.resultCode=-1;return false;}
    if(a.item&&a.sourceSlot>=0){uintptr_t slots=0;int32_t count=0;if(!ReadSlots(a.source,slots,count)||a.sourceSlot>=count||Mem::ReadPtr(slots+uintptr_t(a.sourceSlot)*8)!=a.item||Mem::ReadPtr(a.item+0xB0)!=a.itemData||(a.expectedQuantity>0&&Mem::ReadI32(a.item+0x58)!=a.expectedQuantity)){++stale;++staleIdentities;r.resultCode=-1;return false;}}
    bool ok=true;alignas(8) unsigned char emptyTags[32]{};switch(a.kind){
        case ActionKind::Add:ok=addItemByData(reinterpret_cast<void*>(a.destination),reinterpret_cast<void*>(a.itemData),a.quantity,1.0f,emptyTags);break;
        case ActionKind::Remove:ok=removeFromSlot(reinterpret_cast<void*>(a.source),a.sourceSlot,a.quantity,reinterpret_cast<void*>(a.controller));break;
        case ActionKind::Move:ok=moveItem(reinterpret_cast<void*>(a.source),a.sourceSlot,reinterpret_cast<void*>(a.destination),a.destinationSlot,reinterpret_cast<void*>(a.controller),a.quantity);break;
        case ActionKind::RenameConstruct:fTextFromString(r.renameText,r.renameValue.data());r.renameTextConstructed=true;break;
        case ActionKind::RenameSetterIdentity:executeGetSetterId(r.renameSetter,reinterpret_cast<void*>(a.actor));r.renameSetterConstructed=true;break;
        case ActionKind::RenameSet:executeSetCustomName(reinterpret_cast<void*>(a.actor),r.renameText,r.renameSetter);break;
        case ActionKind::RenameDestroyIdentity:break;
        case ActionKind::RenameDestroyText:break;
    }return ok;
}
void ProcessOneStorageAction(){
    if(!activeRequest){const auto tail=requestTail.load(std::memory_order_relaxed),head=requestHead.load(std::memory_order_acquire);if(tail==head)return;activeRequest=requestRing[tail%QueueCapacity];requestTail.store(tail+1,std::memory_order_release);if(!activeRequest)return;activeRequest->state.store(JobState::Running);activeRequest->updatedUnix.store(UnixNow());++runningJobs;}
    Request&r=*activeRequest;const bool rolling=r.state.load()==JobState::RollingBack;if((rolling&&r.rollbackIndex.load()==0)||(!rolling&&r.actionIndex.load()>=r.actions.size())){if(rolling){FinishJob(r,false,r.resultCode.load()==-5?-5:-2);}else if(r.finishAsFailure.load())FinishJob(r,false,r.resultCode.load()?r.resultCode.load():-2);else FinishJob(r,true,r.resultCode.load()==2?2:1);runningJobs.fetch_sub(1);activeRequest=nullptr;return;}
    const NativeAction action=rolling?r.rollback[r.rollbackIndex.load()-1]:r.actions[r.actionIndex.load()];const auto started=std::chrono::steady_clock::now();const bool ok=RunNativeAction(r,action);const auto micros=std::uint64_t(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-started).count());auto& maximum=familyMaximum[size_t(action.family)];if(micros>maximum.load())maximum.store(micros);if(micros>maximumMicros.load())maximumMicros.store(micros);
    if(micros>=1000){familyCircuitBroken[size_t(action.family)]=true;if(r.resultCode.load()==0)r.resultCode=2;}
    if(ok){if(rolling)r.rollbackIndex.fetch_sub(1);else{r.actionIndex.fetch_add(1);if(action.kind==ActionKind::Move){NativeAction reverse=action;std::swap(reverse.source,reverse.destination);std::swap(reverse.sourceIdentity,reverse.destinationIdentity);std::swap(reverse.sourceSlot,reverse.destinationSlot);uintptr_t slots=0;int32_t count=0;if(ReadSlots(reverse.source,slots,count)&&reverse.sourceSlot>=0&&reverse.sourceSlot<count){reverse.item=Mem::ReadPtr(slots+uintptr_t(reverse.sourceSlot)*8);reverse.itemIdentity=g_Engine->GetObjectIdentity(reverse.item);reverse.itemData=reverse.item?Mem::ReadPtr(reverse.item+0xB0):0;reverse.itemDataIdentity=g_Engine->GetObjectIdentity(reverse.itemData);reverse.expectedQuantity=reverse.item?Mem::ReadI32(reverse.item+0x58):0;}reverse.family=OperationFamily::Rollback;r.rollback.push_back(reverse);}}return;}
    if(!rolling&&(r.renameTextConstructed||r.renameSetterConstructed)){r.finishAsFailure=true;r.actionIndex=r.renameSetterConstructed?3:4;return;}
    if(!rolling&&!r.rollback.empty()){++rollbackAttempts;r.rollbackIndex=r.rollback.size();r.state=JobState::RollingBack;return;}
    if(rolling)++rollbackFailures;if(action.kind==ActionKind::Remove&&r.actionIndex.load()>0){r.partialDelete=true;r.resultCode=-5;}FinishJob(r,false,r.resultCode.load()?r.resultCode.load():-2);runningJobs.fetch_sub(1);activeRequest=nullptr;
}

void WriteJump(unsigned char* at,uintptr_t target){at[0]=0x48;at[1]=0xB8;std::memcpy(at+2,&target,8);at[10]=0xFF;at[11]=0xE0;}
void HookBuildingEndPlay(void* actor,int reason){
    if(reason==0){++buildingRemovalEvents;++snapshotInvalidations;storageGeneration.fetch_add(1);}
    originalBuildingEndPlay(actor,reason);
}
bool InstallBuildingEndPlayHook(uintptr_t moduleBase,uintptr_t moduleEnd,uintptr_t offset){
    if(originalBuildingEndPlay)return true;if(moduleBase+offset+BuildingEndPlayHookBytes>moduleEnd)return false;auto* target=reinterpret_cast<unsigned char*>(moduleBase+offset);const unsigned char expected[BuildingEndPlayHookBytes]={0x55,0x41,0x57,0x41,0x56,0x41,0x55,0x41,0x54,0x53,0x50,0x89,0xf5,0x49,0x89,0xfe};if(std::memcmp(target,expected,sizeof(expected)))return false;
    const unsigned char signature[]={0x55,0x41,0x57,0x41,0x56,0x41,0x55,0x41,0x54,0x53,0x50,0x89,0xF5,0x49,0x89,0xFE,0xE8,0x9B,0x05,0x72,0xFD,0x49,0x8D,0x9E,0x80,0x03,0x00,0x00,0x83,0xFD,0x03,0x0F,0x85,0x8B,0x00,0x00,0x00};if(!Mem::Readable(target,sizeof(signature))||std::memcmp(target,signature,sizeof(signature)))return false;
    auto* trampoline=static_cast<unsigned char*>(mmap(nullptr,64,PROT_READ|PROT_WRITE|PROT_EXEC,MAP_PRIVATE|MAP_ANONYMOUS,-1,0));if(trampoline==MAP_FAILED)return false;std::memcpy(trampoline,target,BuildingEndPlayHookBytes);WriteJump(trampoline+BuildingEndPlayHookBytes,reinterpret_cast<uintptr_t>(target+BuildingEndPlayHookBytes));originalBuildingEndPlay=reinterpret_cast<BuildingEndPlayFn>(trampoline);
    long pageSize=sysconf(_SC_PAGESIZE);uintptr_t page=reinterpret_cast<uintptr_t>(target)&~(uintptr_t(pageSize)-1);if(mprotect(reinterpret_cast<void*>(page),size_t(pageSize),PROT_READ|PROT_WRITE|PROT_EXEC)!=0){originalBuildingEndPlay=nullptr;munmap(trampoline,64);return false;}unsigned char patch[BuildingEndPlayHookBytes];std::memset(patch,0x90,sizeof(patch));WriteJump(patch,reinterpret_cast<uintptr_t>(&HookBuildingEndPlay));std::memcpy(target,patch,sizeof(patch));__builtin___clear_cache(reinterpret_cast<char*>(target),reinterpret_cast<char*>(target+sizeof(patch)));mprotect(reinterpret_cast<void*>(page),size_t(pageSize),PROT_READ|PROT_EXEC);return true;
}
void HookEngineTick(void* actor,float delta,int levelTick,void* tickFunction){
    originalEngineTick(actor,delta,levelTick,tickFunction);
    const auto tickStarted=std::chrono::steady_clock::now();++tickCalls;
    auto measure=[](const std::chrono::steady_clock::time_point&started,std::atomic<std::uint64_t>&maximum){
        const auto elapsed=static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now()-started).count());if(elapsed>maximum.load())maximum.store(elapsed);return elapsed;
    };
    auto sectionStarted=std::chrono::steady_clock::now();
    ProcessPlayerRecipeTracking();
    ProcessPlayerQuestTracking();
    measure(sectionStarted,maximumRecipeTickMicros);sectionStarted=std::chrono::steady_clock::now();
    measure(sectionStarted,maximumCustomNameTickMicros);sectionStarted=std::chrono::steady_clock::now();
    if(pendingCount.load(std::memory_order_acquire)&&std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-tickStarted).count()<250)ProcessOneStorageAction();
    measure(sectionStarted,maximumStorageTickMicros);sectionStarted=std::chrono::steady_clock::now();
    ProcessAdminGravestoneRequests();
    measure(sectionStarted,maximumGravestoneTickMicros);
    const auto elapsed=measure(tickStarted,maximumTickMicros);if(elapsed>=1000)++slowTickCalls;
}
}

bool WasAdminStorageBuildingRemoved(uintptr_t actor){
    if(!g_Engine)return false;const auto identity=g_Engine->GetObjectIdentity(actor);if(!identity)return false;Lock lock(removedBuildingsMutex);return removedBuildingIdentities.count(identity)!=0;
}

bool ResolveAdminStorageLocation(const std::string& characterGuid,const std::string& chestId,FVector3d& location){
    return ResolveStorageLocationInternal(characterGuid,chestId,location);
}
bool ResolveAdminStorageActor(const std::string& characterGuid,const std::string& chestId,uintptr_t& actor,FVector3d& location){
    Chest chest;if(!FindChest(characterGuid,chestId,chest))return false;actor=chest.actor;
    const uintptr_t root=Mem::ReadPtr(chest.actor+Offsets::Actor_RootComponent);
    if(!root||!Mem::Readable(reinterpret_cast<void*>(root+Offsets::SceneComp_RelativeLocation),sizeof(location)))return false;
    std::memcpy(&location,reinterpret_cast<void*>(root+Offsets::SceneComp_RelativeLocation),sizeof(location));
    return std::isfinite(location.X)&&std::isfinite(location.Y)&&std::isfinite(location.Z);
}
bool ReadAdminStorageLocations(const std::string& characterGuid,std::vector<FVector3d>& locations){
    locations.clear();if(!g_Engine||characterGuid.empty())return false;
    std::unordered_map<uintptr_t,uintptr_t> owners;const int32_t count=g_Engine->GetObjectCount();
    for(int32_t i=0;i<count;++i){
        const uintptr_t object=g_Engine->GetObjectByIndex(i);if(!object||g_Engine->GetObjectName(object).rfind("Default__",0)==0)continue;
        const uintptr_t actor=Mem::ReadPtr(object+Offsets::UObject_Outer);
        if(actor&&g_Engine->IsA(object,"OwnershipComponent")&&g_Engine->ReadGuid(object+0x158)==characterGuid)owners[actor]=object;
    }
    std::unordered_set<uintptr_t> seen;
    for(int32_t i=0;i<count;++i){
        const uintptr_t inventory=g_Engine->GetObjectByIndex(i);if(!inventory||!g_Engine->IsObjectLive(inventory)||
            !g_Engine->IsA(inventory,"InventoryComponent")||g_Engine->IsA(inventory,"LinkedInventoryComponent"))continue;
        const uintptr_t actor=Mem::ReadPtr(inventory+Offsets::UObject_Outer);
        if(!actor||!owners.count(actor)||!seen.insert(actor).second||!g_Engine->IsObjectLive(actor)||WasAdminStorageBuildingRemoved(actor))continue;
        const uintptr_t root=Mem::ReadPtr(actor+Offsets::Actor_RootComponent);FVector3d location{};
        if(!root||!Mem::Readable(reinterpret_cast<void*>(root+Offsets::SceneComp_RelativeLocation),sizeof(location)))continue;
        std::memcpy(&location,reinterpret_cast<void*>(root+Offsets::SceneComp_RelativeLocation),sizeof(location));
        if(std::isfinite(location.X)&&std::isfinite(location.Y)&&std::isfinite(location.Z))locations.push_back(location);
    }
    return true;
}
std::string GetAdminStorageCustomName(uintptr_t actor){return ReadCustomNameDirect(actor);}

int ExecutableBounds(struct dl_phdr_info* info,size_t,void* data){
    if(info->dlpi_name&&info->dlpi_name[0])return 0;
    auto* bounds=reinterpret_cast<uintptr_t*>(data);bool found=false;uintptr_t lowest=0,highest=0;
    for(int i=0;i<info->dlpi_phnum;++i){const auto& p=info->dlpi_phdr[i];if(p.p_type!=PT_LOAD)continue;uintptr_t begin=uintptr_t(info->dlpi_addr)+p.p_vaddr,end=begin+p.p_memsz;if(!found||begin<lowest)lowest=begin;if(!found||end>highest)highest=end;found=true;}
    bounds[0]=lowest;bounds[1]=highest;bounds[2]=found?1:0;
    return found?1:0;
}

bool InitializeAdminStorage(uintptr_t moduleBase,uintptr_t moduleEnd){
    const std::string hash=FileSha256("/proc/self/exe");if(!ExpectedSha(hash)){compatibility=-1;return false;}const auto offsets=OffsetsFor(hash);
    if(!earlyHookInstalled.load()||!originalEngineTick){compatibility=-6;return false;}
    for(uintptr_t o:{offsets.addItemByData,offsets.canAddItemByData,offsets.moveItem,offsets.removeFromSlot,offsets.canAddItemsByData,offsets.addItemsByData,offsets.buildingRequirements,offsets.memoryFree,offsets.engineTick,offsets.renameGet,offsets.renameSetter,offsets.renameSet,offsets.textFromString,offsets.uniqueIdDestructor,offsets.textDestructor,offsets.buildingEndPlay})if(moduleBase+o<moduleBase||moduleBase+o+EngineTickHookBytes>moduleEnd){compatibility=-2;return false;}
    const unsigned char addSig[]={0x55,0x41,0x57,0x41,0x56,0x41,0x54,0x53,0x48,0x83,0xEC,0x30};
    const unsigned char moveSig[]={0x55,0x53,0x50,0x4C,0x89,0xC0,0x48,0x89,0xFB};
    const unsigned char removeSig[]={0x55,0x41,0x57,0x41,0x56,0x53,0x48,0x83,0xEC,0x28};
    if(std::memcmp(reinterpret_cast<void*>(moduleBase+offsets.addItemByData),addSig,sizeof(addSig))||std::memcmp(reinterpret_cast<void*>(moduleBase+offsets.moveItem),moveSig,sizeof(moveSig))||std::memcmp(reinterpret_cast<void*>(moduleBase+offsets.removeFromSlot),removeSig,sizeof(removeSig))){compatibility=-3;return false;}
    const unsigned char batchSig[]={0x55,0x41,0x57,0x41,0x56,0x41,0x55,0x41,0x54};const unsigned char buildSig[]={0x41,0x57,0x41,0x56,0x41,0x54,0x53};batchCompatibility=(!std::memcmp(reinterpret_cast<void*>(moduleBase+offsets.addItemsByData),batchSig,sizeof(batchSig))&&!std::memcmp(reinterpret_cast<void*>(moduleBase+offsets.buildingRequirements),buildSig,sizeof(buildSig)))?1:-1;
    planCatalogCompatibility=PlanItemCatalog().valid?1:-1;
    if(planCatalogCompatibility.load()!=1)LogMessage("Plan item catalog assets unavailable; plan items remain visible in storage but cannot be added through catalog mode");
    addItemByData=reinterpret_cast<AddItemByDataFn>(moduleBase+offsets.addItemByData);canAddItemByData=reinterpret_cast<CanAddItemByDataFn>(moduleBase+offsets.canAddItemByData);moveItem=reinterpret_cast<MoveItemFn>(moduleBase+offsets.moveItem);removeFromSlot=reinterpret_cast<RemoveFromSlotFn>(moduleBase+offsets.removeFromSlot);canAddItemsByData=reinterpret_cast<CanAddItemsByDataFn>(moduleBase+offsets.canAddItemsByData);addItemsByData=reinterpret_cast<AddItemsByDataFn>(moduleBase+offsets.addItemsByData);buildingScaledRequirements=reinterpret_cast<BuildingScaledRequirementsFn>(moduleBase+offsets.buildingRequirements);memoryFree=reinterpret_cast<FMemoryFreeFn>(moduleBase+offsets.memoryFree);executeGetCustomName=reinterpret_cast<ExecuteGetCustomNameFn>(moduleBase+offsets.renameGet);executeGetSetterId=reinterpret_cast<ExecuteGetSetterIdFn>(moduleBase+offsets.renameSetter);executeSetCustomName=reinterpret_cast<ExecuteSetCustomNameFn>(moduleBase+offsets.renameSet);fTextFromString=reinterpret_cast<FTextFromStringFn>(moduleBase+offsets.textFromString);destroyUniqueNetId=reinterpret_cast<UniqueNetIdDestructorFn>(moduleBase+offsets.uniqueIdDestructor);destroyFText=reinterpret_cast<FTextDestructorFn>(moduleBase+offsets.textDestructor);
    // The current build's setter returned successfully during live validation, but the
    // authoritative CustomName property did not reflect the requested value. Keep only
    // this independently breakable capability closed until setter/readback is re-proven.
    renameCompatibility=hash=="b5017cf67bc6127dbec443e00fed1f818e82afd5d6d17f912d959a1dd87d5687"?-1:1;
    if(renameCompatibility.load()!=1)LogMessage("Storage rename setter/readback validation failed for this game build; rename remains disabled");
    catalogCompatibility=1;unlockCatalogCompatibility=UnlockCatalogAssetsValid()?1:-1;const auto& buildCatalog=AuthoritativeBuildingCatalog();buildingTaxonomyCompatibility=buildCatalog.valid?1:-1;buildingIconsCompatibility=buildCatalog.iconsValid?1:-1;buildingRequirementsCompatibility=batchCompatibility.load()==1?1:-1;craftingRecipeCompatibility=batchCompatibility.load()==1?1:-1;if(unlockCatalogCompatibility.load()!=1)LogMessage("Vestige and mount unlock catalog assets unavailable; ordinary item catalog remains enabled");if(buildingTaxonomyCompatibility.load()!=1)LogMessage("Authoritative building catalogue is invalid; crafting recipes remain enabled");if(buildingIconsCompatibility.load()!=1)LogMessage("One or more authoritative building catalogue icons are unavailable");removalTrackingCompatibility=InstallBuildingEndPlayHook(moduleBase,moduleEnd,offsets.buildingEndPlay)?1:-1;if(removalTrackingCompatibility.load()!=1)LogMessage("Immediate storage removal tracking unavailable; storage editing remains enabled with ordinary actor reconciliation");dispatcherCompatibility=1;compatibility=1;return true;
}
bool InitializeAdminStorageHookEarly(){
    if(earlyHookInstalled.load())return true;
    std::string hash=FileSha256("/proc/self/exe");if(!ExpectedDispatcherSha(hash)){LogMessage("Admin storage dispatcher fingerprint mismatch: "+hash);return false;}
    const bool currentDispatcher=hash=="49512c0ee933c3ee4dda5fb5c2f0a25e66d7f26e34795c45b91b5c1d12b61b0c"||hash=="b5017cf67bc6127dbec443e00fed1f818e82afd5d6d17f912d959a1dd87d5687";
    const uintptr_t dispatcherOffset=currentDispatcher?0x07ABFE00:EngineTickOffset;
    uintptr_t bounds[3]={0,0,0};dl_iterate_phdr(&ExecutableBounds,bounds);if(!bounds[2]||bounds[0]+dispatcherOffset+EngineTickHookBytes>bounds[1]){LogMessage("Admin storage dispatcher executable bounds unavailable");return false;}
    auto* target=reinterpret_cast<unsigned char*>(bounds[0]+dispatcherOffset);
    const unsigned char expected[EngineTickHookBytes]={0x53,0x48,0x83,0xEC,0x10,0x8B,0x47,0x08,0xA8,0x10,0x0F,0x85,0x92,0x00,0x00,0x00};
    if(std::memcmp(target,expected,sizeof(expected))){char message[160]{};std::snprintf(message,sizeof(message),"Admin storage dispatcher signature mismatch base=0x%llx target=0x%llx bytes=%02x%02x%02x%02x",static_cast<unsigned long long>(bounds[0]),static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(target)),target[0],target[1],target[2],target[3]);LogMessage(message);return false;}
    auto* trampoline=static_cast<unsigned char*>(mmap(nullptr,64,PROT_READ|PROT_WRITE|PROT_EXEC,MAP_PRIVATE|MAP_ANONYMOUS,-1,0));if(trampoline==MAP_FAILED)return false;
    std::memcpy(trampoline,target,10);
    // Relocate the original JNE: false continues at +16; true returns through
    // the original function's shared epilogue at +0xA2.
    trampoline[10]=0x0F;trampoline[11]=0x84;std::int32_t skip=12;std::memcpy(trampoline+12,&skip,4);
    WriteJump(trampoline+16,reinterpret_cast<uintptr_t>(target+0xA2));
    WriteJump(trampoline+28,reinterpret_cast<uintptr_t>(target+EngineTickHookBytes));
    originalEngineTick=reinterpret_cast<EngineTickFn>(trampoline);
    long pageSize=sysconf(_SC_PAGESIZE);uintptr_t page=reinterpret_cast<uintptr_t>(target)&~(uintptr_t(pageSize)-1);if(mprotect(reinterpret_cast<void*>(page),size_t(pageSize),PROT_READ|PROT_WRITE|PROT_EXEC)!=0){originalEngineTick=nullptr;munmap(trampoline,64);return false;}
    unsigned char patch[EngineTickHookBytes];std::memset(patch,0x90,sizeof(patch));WriteJump(patch,reinterpret_cast<uintptr_t>(&HookEngineTick));std::memcpy(target,patch,sizeof(patch));__builtin___clear_cache(reinterpret_cast<char*>(target),reinterpret_cast<char*>(target+sizeof(patch)));mprotect(reinterpret_cast<void*>(page),size_t(pageSize),PROT_READ|PROT_EXEC);earlyHookInstalled=true;dispatcherCompatibility=1;return true;
}
void ProcessAdminStorageRequests(){ProcessOneStorageAction();}
std::string NewRequestId(){static std::atomic<std::uint64_t> sequence{1};std::uint64_t a=std::uint64_t(std::chrono::steady_clock::now().time_since_epoch().count()),b=sequence.fetch_add(1);return HexHash(a)+HexHash(b);}
NativeAction BaseAction(const StorageSnapshot&s,const SnapshotChest&chest,OperationFamily family){NativeAction a;a.family=family;a.actor=chest.actor;a.actorIdentity=chest.actorIdentity;a.controller=s.adminController;a.controllerIdentity=s.adminControllerIdentity;return a;}
bool AddMovePlan(Request&r,const StorageSnapshot&s,const SnapshotChest&destination,const std::vector<std::pair<const SnapshotChest*,const SnapshotItem*>>&items,std::string&error){
    struct VirtualSlot{uintptr_t data=0;int quantity=0,maxStack=1;};std::vector<VirtualSlot> slots(size_t(destination.totalSlots));for(const auto&item:destination.items)if(item.slot>=0&&item.slot<destination.totalSlots)slots[size_t(item.slot)]={item.data,item.quantity,item.maxStack};
    for(const auto&entry:items){const auto&source=*entry.first;const auto*item=entry.second;int remaining=item->quantity;for(int phase=0;phase<2&&remaining>0;++phase)for(int slot=0;slot<destination.totalSlots&&remaining>0;++slot){auto&target=slots[size_t(slot)];if((phase==0&&(target.data!=item->data||target.quantity>=target.maxStack))||(phase==1&&target.data))continue;const int amount=std::min(remaining,phase==0?target.maxStack-target.quantity:item->maxStack);if(amount<=0)continue;NativeAction action=BaseAction(s,source,OperationFamily::BatchMovement);action.kind=ActionKind::Move;action.source=source.inventory;action.sourceIdentity=source.inventoryIdentity;action.destination=destination.inventory;action.destinationIdentity=destination.inventoryIdentity;action.item=item->object;action.itemIdentity=item->objectIdentity;action.itemData=item->data;action.itemDataIdentity=item->dataIdentity;action.sourceSlot=item->slot;action.destinationSlot=slot;action.quantity=amount;action.expectedQuantity=remaining;r.actions.push_back(action);if(!target.data){target.data=item->data;target.maxStack=item->maxStack;}target.quantity+=amount;remaining-=amount;}if(remaining){error="The destination storage does not have enough space for the complete selection";return false;}}
    return true;
}
bool BuildMutationPlan(const std::string&operation,const std::string&adminGuid,const std::string&targetGuid,const std::string&chestId,const std::string&itemId,const std::string&catalogId,int quantity,const std::string&destinationChestId,int destinationSlot,const std::vector<std::string>&itemIds,const std::string&customName,Request&r,std::string&error){
    auto snapshot=BuildStorageSnapshot(targetGuid,adminGuid,true);if(!snapshot){error="Storage snapshot preparation failed";return false;}r.expectedWorld=snapshot->world;r.expectedWorldIdentity=snapshot->worldIdentity;const SnapshotChest* chest=SnapshotFindChest(*snapshot,chestId);if(!chest){error="Storage container is no longer available";return false;}const SnapshotChest* destination=destinationChestId.empty()?nullptr:SnapshotFindChest(*snapshot,destinationChestId);auto selected=[&](){std::vector<const SnapshotItem*> out;std::unordered_set<std::string> unique;for(const auto&id:itemIds){if(!unique.insert(id).second)return std::vector<const SnapshotItem*>{};auto* value=SnapshotFindItem(*chest,id);if(!value)return std::vector<const SnapshotItem*>{};out.push_back(value);}return out;};
    if(operation=="rename"){if(renameCompatibility.load()!=1){error="Storage naming is unavailable for this game build";return false;}if(familyCircuitBroken[size_t(OperationFamily::Rename)]){error="Storage naming was disabled after a slow native call";return false;}std::memcpy(r.renameValue.data(),customName.data(),customName.size());NativeAction a=BaseAction(*snapshot,*chest,OperationFamily::Rename);for(ActionKind kind:{ActionKind::RenameConstruct,ActionKind::RenameSetterIdentity,ActionKind::RenameSet,ActionKind::RenameDestroyIdentity,ActionKind::RenameDestroyText}){a.kind=kind;r.actions.push_back(a);}r.successMessage="Storage renamed";return true;}
    if(operation=="add"){std::string key;uintptr_t data=FindCatalogData(catalogId,&key);if(!data){error="Selected item is no longer available";return false;}if(!canAddItemByData(reinterpret_cast<void*>(chest->inventory),reinterpret_cast<void*>(data),quantity)){error="The storage does not have enough space";return false;}NativeAction a=BaseAction(*snapshot,*chest,OperationFamily::SingleItem);a.kind=ActionKind::Add;a.destination=chest->inventory;a.destinationIdentity=chest->inventoryIdentity;a.itemData=data;a.itemDataIdentity=g_Engine->GetObjectIdentity(data);a.quantity=quantity;r.actions.push_back(a);}
    else if(operation=="add_recipe"){RecipeRecord recipe;if(!ResolveRecipe(catalogId,recipe)){error="Selected recipe is no longer available";return false;}std::vector<unsigned char> capacity(recipe.ingredients.size()*16);for(size_t i=0;i<recipe.ingredients.size();++i){long long amount=1LL*recipe.ingredients[i].quantity*quantity;if(amount<1||amount>1000000){error="Recipe quantity is too large";return false;}void* data=reinterpret_cast<void*>(recipe.ingredients[i].data);int q=int(amount);std::memcpy(capacity.data()+i*16,&data,8);std::memcpy(capacity.data()+i*16+8,&q,4);}FUEArray values{capacity.data(),int32_t(recipe.ingredients.size()),int32_t(recipe.ingredients.size())};if(!canAddItemsByData(reinterpret_cast<void*>(chest->inventory),&values)){error="The storage does not have enough space for the complete recipe";return false;}for(const auto&ingredient:recipe.ingredients){NativeAction a=BaseAction(*snapshot,*chest,OperationFamily::BatchAddition);a.kind=ActionKind::Add;a.destination=chest->inventory;a.destinationIdentity=chest->inventoryIdentity;a.itemData=ingredient.data;a.itemDataIdentity=g_Engine->GetObjectIdentity(ingredient.data);a.quantity=ingredient.quantity*quantity;r.actions.push_back(a);}}
    else if(operation=="slot_move"){auto* item=SnapshotFindItem(*chest,itemId);if(!item||!destination||destinationSlot<0||destinationSlot>=destination->totalSlots){error="The item or destination changed before the request was queued";return false;}if(chest->id==destination->id&&item->slot==destinationSlot){r.successMessage="Storage unchanged";return true;}NativeAction a=BaseAction(*snapshot,*chest,OperationFamily::SlotMovement);a.kind=ActionKind::Move;a.source=chest->inventory;a.sourceIdentity=chest->inventoryIdentity;a.destination=destination->inventory;a.destinationIdentity=destination->inventoryIdentity;a.item=item->object;a.itemIdentity=item->objectIdentity;a.itemData=item->data;a.itemDataIdentity=item->dataIdentity;a.sourceSlot=item->slot;a.destinationSlot=destinationSlot;a.quantity=item->quantity;a.expectedQuantity=item->quantity;r.actions.push_back(a);}
    else if(operation=="move_batch"||operation=="organize_batch"){if(!destination){error="Destination storage is no longer available";return false;}std::vector<std::pair<const SnapshotChest*,const SnapshotItem*>> items;if(operation=="move_batch"){auto values=selected();for(auto*item:values)items.push_back({chest,item});}else{auto seeds=selected();if(seeds.size()!=itemIds.size()){error="One or more selected items changed";return false;}std::unordered_set<uintptr_t> data;for(auto*item:seeds)data.insert(item->data);for(const auto&source:snapshot->chests)if(source.id!=destination->id)for(const auto&item:source.items)if(data.count(item.data))items.push_back({&source,&item});}if(items.empty()&&operation=="organize_batch"){r.successMessage="All matching items are already organized here";return true;}if(items.size()!=itemIds.size()&&operation=="move_batch"){error="One or more selected items changed";return false;}if(!AddMovePlan(r,*snapshot,*destination,items,error))return false;r.successMessage=operation=="organize_batch"?"Items organized":"Storage updated";}
    else if(operation=="copy_batch"){if(!destination){error="Destination storage is no longer available";return false;}auto items=selected();if(items.size()!=itemIds.size()){error="One or more selected items changed";return false;}std::vector<unsigned char> capacity(items.size()*16);for(size_t i=0;i<items.size();++i){void* data=reinterpret_cast<void*>(items[i]->data);std::memcpy(capacity.data()+i*16,&data,8);std::memcpy(capacity.data()+i*16+8,&items[i]->quantity,4);}FUEArray values{capacity.data(),int32_t(items.size()),int32_t(items.size())};if(!canAddItemsByData(reinterpret_cast<void*>(destination->inventory),&values)){error="The destination storage does not have enough space";return false;}for(auto*item:items){NativeAction a=BaseAction(*snapshot,*destination,OperationFamily::BatchAddition);a.kind=ActionKind::Add;a.destination=destination->inventory;a.destinationIdentity=destination->inventoryIdentity;a.itemData=item->data;a.itemDataIdentity=item->dataIdentity;a.quantity=item->quantity;r.actions.push_back(a);}}
    else if(operation=="delete_batch"){auto items=selected();if(items.size()!=itemIds.size()){error="One or more selected items changed";return false;}std::sort(items.begin(),items.end(),[](auto*a,auto*b){return a->slot>b->slot;});for(auto*item:items){NativeAction a=BaseAction(*snapshot,*chest,OperationFamily::Deletion);a.kind=ActionKind::Remove;a.source=chest->inventory;a.sourceIdentity=chest->inventoryIdentity;a.item=item->object;a.itemIdentity=item->objectIdentity;a.itemData=item->data;a.itemDataIdentity=item->dataIdentity;a.sourceSlot=item->slot;a.quantity=item->quantity;a.expectedQuantity=item->quantity;r.actions.push_back(a);}}
    else {auto* item=SnapshotFindItem(*chest,itemId);if(!item){error="The item changed before the request was queued";return false;}if(quantity<1||(operation!="add_existing"&&quantity>item->quantity)){error="Requested quantity is no longer available";return false;}if(operation=="delete"){NativeAction a=BaseAction(*snapshot,*chest,OperationFamily::Deletion);a.kind=ActionKind::Remove;a.source=chest->inventory;a.sourceIdentity=chest->inventoryIdentity;a.item=item->object;a.itemIdentity=item->objectIdentity;a.itemData=item->data;a.itemDataIdentity=item->dataIdentity;a.sourceSlot=item->slot;a.quantity=quantity;a.expectedQuantity=item->quantity;r.actions.push_back(a);}else if(operation=="add_existing"){if(!canAddItemByData(reinterpret_cast<void*>(chest->inventory),reinterpret_cast<void*>(item->data),quantity)){error="The storage does not have enough space";return false;}NativeAction a=BaseAction(*snapshot,*chest,OperationFamily::SingleItem);a.kind=ActionKind::Add;a.destination=chest->inventory;a.destinationIdentity=chest->inventoryIdentity;a.itemData=item->data;a.itemDataIdentity=item->dataIdentity;a.quantity=quantity;r.actions.push_back(a);}else if(operation=="take"){if(!snapshot->adminFound||!snapshot->adminSpawned||snapshot->adminDead||!snapshot->adminInventory){error="Linked administrator character is offline or unavailable";return false;}if(!canAddItemByData(reinterpret_cast<void*>(snapshot->adminInventory),reinterpret_cast<void*>(item->data),quantity)){error="Administrator inventory does not have enough space";return false;}uintptr_t slots=0;int32_t count=0;if(!ReadSlots(snapshot->adminInventory,slots,count)){error="Administrator inventory slots are unavailable";return false;}int target=-1;for(int32_t i=0;i<count;++i){uintptr_t existing=Mem::ReadPtr(slots+uintptr_t(i)*8);if(existing&&Mem::ReadPtr(existing+0xB0)==item->data&&Mem::ReadI32(existing+0x58)+quantity<=item->maxStack){target=i;break;}}if(target<0)for(int32_t i=0;i<count;++i)if(!Mem::ReadPtr(slots+uintptr_t(i)*8)){target=i;break;}if(target<0){error="Administrator inventory does not have enough space";return false;}NativeAction a=BaseAction(*snapshot,*chest,OperationFamily::SingleItem);a.kind=ActionKind::Move;a.source=chest->inventory;a.sourceIdentity=chest->inventoryIdentity;a.destination=snapshot->adminInventory;a.destinationIdentity=snapshot->adminInventoryIdentity;a.item=item->object;a.itemIdentity=item->objectIdentity;a.itemData=item->data;a.itemDataIdentity=item->dataIdentity;a.sourceSlot=item->slot;a.destinationSlot=target;a.quantity=quantity;a.expectedQuantity=item->quantity;r.actions.push_back(a);}else{error="Invalid storage operation";return false;}}
    for(const auto&action:r.actions){if(familyCircuitBroken[size_t(action.family)].load()||(action.kind==ActionKind::Move&&familyCircuitBroken[size_t(OperationFamily::Rollback)].load())){error="This storage operation is disabled after exceeding the game-thread limit";return false;}}
    if(r.successMessage.empty())r.successMessage="Storage updated";r.rollback.reserve(r.actions.size());return true;
}
bool SubmitAdminStorageMutation(const std::string&operation,const std::string&adminGuid,const std::string&adminId,const std::string&targetGuid,const std::string&targetId,const std::string&chestId,const std::string&itemId,const std::string&catalogId,int quantity,const std::string&session,std::string&requestId,std::string&result,const std::string&destinationChestId,int destinationSlot,const std::vector<std::string>&itemIds,const std::string&customName){
    (void)adminId;(void)targetId;if(compatibility.load()!=1){result="Storage editing compatibility unavailable";return false;}auto r=std::make_shared<Request>();r->id=NewRequestId();r->operation=operation;r->session=session;r->conflictKey=targetGuid;r->updatedUnix=UnixNow();if(!BuildMutationPlan(operation,adminGuid,targetGuid,chestId,itemId,catalogId,quantity,destinationChestId,destinationSlot,itemIds,customName,*r,result))return false;
    {std::lock_guard<std::mutex> lock(jobsMutex);const auto now=UnixNow();for(auto it=jobs.begin();it!=jobs.end();){auto state=it->second->state.load();if((state==JobState::Completed||state==JobState::Failed)&&now>it->second->updatedUnix.load()+300)it=jobs.erase(it);else{if((state==JobState::Queued||state==JobState::Running||state==JobState::RollingBack)&&it->second->conflictKey==r->conflictKey){result="Another storage operation is already pending";return false;}++it;}}const auto head=requestHead.load(std::memory_order_relaxed),tail=requestTail.load(std::memory_order_acquire);if(head-tail>=QueueCapacity){++queueDrops;result="Storage mutation queue is full";return false;}jobs[r->id]=r;requestRing[head%QueueCapacity]=r.get();requestHead.store(head+1,std::memory_order_release);pendingCount.fetch_add(1);++queued;}
    requestId=r->id;result="Storage operation queued";return true;
}
std::string GetAdminStorageMutationStatus(const std::string&id,const std::string&session){
    std::shared_ptr<Request> r;
    {
        std::lock_guard<std::mutex> lock(jobsMutex);
        auto found=jobs.find(id);
        if(found==jobs.end()||found->second->session!=session)return {};
        const auto state=found->second->state.load();
        if((state==JobState::Completed||state==JobState::Failed)&&UnixNow()>found->second->updatedUnix.load()+300){jobs.erase(found);return {};}
        r=found->second;
    }
    const auto state=r->state.load();const char*name=state==JobState::Queued?"queued":state==JobState::Running?"running":state==JobState::RollingBack?"rolling-back":state==JobState::Completed?"completed":"failed";std::string message;if(state==JobState::Completed)message=r->resultCode.load()==2?r->successMessage+"; this operation was disabled for later requests after exceeding the game-thread limit":r->successMessage;else if(state==JobState::Failed){switch(r->resultCode.load()){case -1:message="A storage object changed before the operation completed";break;case -3:message="The game world changed before the operation completed";break;case -4:message="This storage operation is disabled after exceeding the game-thread limit";break;case -5:message="A delete partially completed; refresh storage immediately";break;default:message="The game rejected the storage operation";}}
    const auto progress=state==JobState::RollingBack?r->rollback.size()-r->rollbackIndex.load():r->actionIndex.load();const auto total=state==JobState::RollingBack?r->rollback.size():r->actions.size();
    std::ostringstream out;out<<"{\"id\":\""<<Escape(id)<<"\",\"status\":\""<<name<<"\",\"operation\":\""<<Escape(r->operation)<<"\",\"progress\":"<<progress<<",\"total\":"<<total<<",\"message\":"<<(message.empty()?"null":"\""+Escape(message)+"\"")<<'}';return out.str();
}
std::string GetAdminPlayerStorageJson(const std::string&characterGuid,const std::string&administratorCharacterGuid){
    auto snapshot=BuildStorageSnapshot(characterGuid,administratorCharacterGuid,true);if(!snapshot)return {};std::ostringstream out;out<<"{\"compatibility\":true,\"mapVersion\":\""<<Escape(g_Engine->GetMapAssetVersion())<<"\",\"snapshotGeneration\":"<<snapshot->generation<<",\"chests\":[";bool firstChest=true;std::size_t itemCount=0;
    for(const auto&chest:snapshot->chests){if(!firstChest)out<<',';firstChest=false;out<<"{\"id\":\""<<chest.id<<"\",\"type\":\""<<Escape(chest.type)<<"\",\"customName\":"<<(chest.customName.empty()?"null":"\""+Escape(chest.customName)+"\"")<<",\"filledSlots\":"<<chest.items.size()<<",\"totalSlots\":"<<chest.totalSlots<<",\"gridColumns\":8,\"linked\":"<<(chest.linked?"true":"false")<<",\"private\":"<<(chest.privateStorage?"true":"false")<<",\"mapAvailable\":"<<(chest.mapValid?"true":"false")<<",\"teleportAvailable\":"<<(chest.mapValid?"true":"false");if(chest.mapValid)out<<",\"mapX\":"<<chest.mapX<<",\"mapY\":"<<chest.mapY;out<<",\"items\":[";bool firstItem=true;for(const auto&item:chest.items){if(!firstItem)out<<',';firstItem=false;++itemCount;out<<"{\"id\":\""<<item.id<<"\",\"slot\":"<<item.slot<<",\"revision\":\""<<item.id<<"\",\"name\":\""<<Escape(item.name)<<"\",\"quantity\":"<<item.quantity<<",\"maxStack\":"<<item.maxStack<<",\"icon\":\""<<Escape(item.icon)<<"\"}";}out<<"],\"slots\":[";for(int slot=0;slot<chest.totalSlots;++slot){if(slot)out<<',';const SnapshotItem* item=nullptr;for(const auto&candidate:chest.items)if(candidate.slot==slot){item=&candidate;break;}if(!item)out<<"null";else out<<"{\"id\":\""<<item->id<<"\",\"slot\":"<<item->slot<<",\"revision\":\""<<item->id<<"\",\"name\":\""<<Escape(item->name)<<"\",\"quantity\":"<<item->quantity<<",\"maxStack\":"<<item->maxStack<<",\"icon\":\""<<Escape(item->icon)<<"\"}";}out<<"]}";}out<<"],\"chestCount\":"<<snapshot->chests.size()<<",\"itemCount\":"<<itemCount<<'}';return out.str();
}
void InvalidateAdminStorageSnapshots(){++snapshotInvalidations;storageGeneration.fetch_add(1);}
AdminStorageDiagnostics GetAdminStorageDiagnostics(){AdminStorageDiagnostics d;d.compatibility=compatibility.load();d.dispatcherCompatibility=dispatcherCompatibility.load();d.mutationCompatibility=compatibility.load();d.batchCompatibility=batchCompatibility.load();d.organizationCompatibility=compatibility.load();d.renameCompatibility=renameCompatibility.load();d.removalTrackingCompatibility=removalTrackingCompatibility.load();d.catalogCompatibility=catalogCompatibility.load();d.unlockCatalogCompatibility=unlockCatalogCompatibility.load();d.planCatalogCompatibility=planCatalogCompatibility.load();d.planCatalogItems=PlanItemCatalog().keys.size();d.buildingTaxonomyCompatibility=buildingTaxonomyCompatibility.load();d.buildingRequirementsCompatibility=buildingRequirementsCompatibility.load();d.buildingIconsCompatibility=buildingIconsCompatibility.load();d.craftingRecipeCompatibility=craftingRecipeCompatibility.load();const auto& catalog=AuthoritativeBuildingCatalog();d.expectedBuildingPieces=catalog.expectedPieces;d.resolvedBuildingPieces=resolvedBuildingPieces.load();d.unavailableBuildingPieces=unavailableBuildingPieces.load();d.buildingTabs=catalog.tabs.size();for(const auto&tab:catalog.tabs){d.buildingSections+=tab.sections.size();for(const auto&section:tab.sections)d.buildingReferences+=section.pieces.size();}d.queuedRequests=queued.load();d.acceptedMutations=accepted.load();d.rejectedRequests=rejected.load();d.staleRequests=stale.load();d.maximumExecutionMicroseconds=maximumMicros.load();d.tickCalls=tickCalls.load();d.maximumTickMicroseconds=maximumTickMicros.load();d.slowTickCalls=slowTickCalls.load();d.maximumRecipeTickMicroseconds=maximumRecipeTickMicros.load();d.maximumCustomNameTickMicroseconds=maximumCustomNameTickMicros.load();d.maximumStorageTickMicroseconds=maximumStorageTickMicros.load();d.maximumGravestoneTickMicroseconds=maximumGravestoneTickMicros.load();d.snapshotCompatibility=g_Engine?1:-1;d.singleItemCompatibility=familyCircuitBroken[size_t(OperationFamily::SingleItem)]?-1:compatibility.load();d.slotMovementCompatibility=familyCircuitBroken[size_t(OperationFamily::SlotMovement)]?-1:compatibility.load();d.batchAdditionCompatibility=familyCircuitBroken[size_t(OperationFamily::BatchAddition)]?-1:batchCompatibility.load();d.batchMovementCompatibility=familyCircuitBroken[size_t(OperationFamily::BatchMovement)]?-1:compatibility.load();d.deletionCompatibility=familyCircuitBroken[size_t(OperationFamily::Deletion)]?-1:compatibility.load();d.renameCompatibility=familyCircuitBroken[size_t(OperationFamily::Rename)]?-1:renameCompatibility.load();d.rollbackCompatibility=familyCircuitBroken[size_t(OperationFamily::Rollback)]?-1:compatibility.load();d.snapshotGenerations=snapshotGenerations.load();d.snapshotCacheHits=snapshotHits.load();d.snapshotCacheMisses=snapshotMisses.load();d.snapshotInvalidations=snapshotInvalidations.load();d.maximumSnapshotPreparationMicroseconds=maximumSnapshotMicros.load();d.queueDepth=pendingCount.load();d.queueDrops=queueDrops.load();d.runningJobs=runningJobs.load();d.completedJobs=completedJobs.load();d.failedJobs=failedJobs.load();d.staleIdentities=staleIdentities.load();d.worldChanges=worldChanges.load();d.rollbackAttempts=rollbackAttempts.load();d.rollbackFailures=rollbackFailures.load();d.singleItemMaximumMicroseconds=familyMaximum[size_t(OperationFamily::SingleItem)].load();d.slotMovementMaximumMicroseconds=familyMaximum[size_t(OperationFamily::SlotMovement)].load();d.batchAdditionMaximumMicroseconds=familyMaximum[size_t(OperationFamily::BatchAddition)].load();d.batchMovementMaximumMicroseconds=familyMaximum[size_t(OperationFamily::BatchMovement)].load();d.deletionMaximumMicroseconds=familyMaximum[size_t(OperationFamily::Deletion)].load();d.renameMaximumMicroseconds=familyMaximum[size_t(OperationFamily::Rename)].load();d.rollbackMaximumMicroseconds=familyMaximum[size_t(OperationFamily::Rollback)].load();for(size_t i=0;i<size_t(OperationFamily::Count);++i)if(familyCircuitBroken[i])d.circuitBreakers|=1u<<i;return d;}
std::string GetAdminStorageCatalogJson(){
    if(compatibility.load()!=1||!g_Engine)return {};using GetDisplayStringFn=const FString*(*)(const void*);auto display=reinterpret_cast<GetDisplayStringFn>(g_Engine->GetModuleBase()+DisplayTextOffset());
    const char* root=std::getenv("DW_WEB_ROOT");const std::string itemRoot=std::string(root&&*root?root:"/home/container/zanik/web")+"/items/";std::ostringstream out;out<<"{\"items\":[";bool first=true;int count=0;const int32_t objects=g_Engine->GetObjectCount();
    for(int32_t i=0;i<objects;++i){uintptr_t data=g_Engine->GetObjectByIndex(i);if(!data||!g_Engine->IsA(data,"ItemData"))continue;std::string key=g_Engine->GetObjectName(data);if(!AllowedCatalogKey(key))continue;std::string lower=key;std::transform(lower.begin(),lower.end(),lower.begin(),[](unsigned char c){return char(std::tolower(c));});if(lower.find("test")!=std::string::npos||lower.find("debug")!=std::string::npos||lower.find("developer")!=std::string::npos||lower.find("deprecated")!=std::string::npos||lower.find("_old")!=std::string::npos)continue;std::string icon=itemRoot+key+".png";struct stat st{};if(stat(icon.c_str(),&st)!=0||st.st_size<1)continue;std::string name;const FString* text=display(reinterpret_cast<const void*>(data+0x68));if(text&&Mem::Readable(text,sizeof(FString)))name=g_Engine->ReadFString(reinterpret_cast<uintptr_t>(text));if(name.empty())continue;int maxStack=MaxStackSize(data);auto meta=CatalogMetadata(key,name);if(!first)out<<',';first=false;++count;out<<"{\"id\":\""<<CatalogId(key)<<"\",\"name\":\""<<Escape(name)<<"\",\"icon\":\"/assets/items/"<<Escape(key)<<".png\",\"maxStack\":"<<maxStack<<",\"stackable\":"<<(maxStack>1?"true":"false")<<",\"category\":\""<<Escape(meta.category)<<"\",\"categoryOrder\":"<<meta.categoryOrder<<",\"tier\":"<<(meta.tier?std::to_string(meta.tier):"null")<<",\"basePowerLevel\":"<<(meta.tier?std::to_string(meta.tier):"null")<<",\"recipeOrder\":"<<meta.recipeOrder<<",\"aliases\":[\""<<Escape(meta.aliases)<<"\"]}";}
    out<<"],\"count\":"<<count<<'}';return out.str();
}
std::string GetAdminStorageRecipesJson(){
    if(compatibility.load()!=1||batchCompatibility.load()!=1||!g_Engine)return {};
    auto records=RecipeRecords();std::unordered_set<std::string> present;for(const auto&r:records)if(r.type=="building")present.insert(r.key);
    std::ostringstream out;out<<"{\"recipes\":[";bool first=true;
    for(const auto&r:records){if(!first)out<<',';first=false;out<<"{\"id\":\""<<r.id<<"\",\"key\":\""<<Escape(r.key)<<"\",\"name\":\""<<Escape(r.name)<<"\",\"type\":\""<<r.type<<"\",\"category\":\""<<Escape(r.category)<<"\",\"station\":\""<<Escape(r.station)<<"\",\"available\":"<<(r.available?"true":"false")<<",\"unavailableReason\":"<<(r.unavailableReason.empty()?"null":"\""+Escape(r.unavailableReason)+"\"")<<",\"tier\":"<<(r.tier?std::to_string(r.tier):"null")<<",\"order\":"<<r.order<<",\"icon\":\""<<Escape(r.icon)<<"\",\"aliases\":[\""<<Escape(CleanAssetName(r.key))<<"\"],\"ingredients\":[";bool fi=true;for(const auto&i:r.ingredients){if(!fi)out<<',';fi=false;std::string key=g_Engine->GetObjectName(i.data);out<<"{\"id\":\""<<CatalogId(key)<<"\",\"name\":\""<<Escape(ItemDisplayName(i.data))<<"\",\"icon\":\"/assets/items/"<<Escape(key)<<".png\",\"quantity\":"<<i.quantity<<'}';}out<<"]}";}
    const auto&catalog=AuthoritativeBuildingCatalog();out<<"],\"buildingTabs\":[";bool ft=true;
    for(const auto&tab:catalog.tabs){if(!ft)out<<',';ft=false;out<<"{\"key\":\""<<Escape(tab.key)<<"\",\"label\":\""<<Escape(tab.label)<<"\",\"order\":"<<tab.order<<",\"sections\":[";bool fs=true;for(const auto&section:tab.sections){if(!fs)out<<',';fs=false;out<<"{\"key\":\""<<Escape(section.key)<<"\",\"label\":\""<<Escape(section.label)<<"\",\"order\":"<<section.order<<",\"favourites\":"<<(section.favourites?"true":"false")<<",\"recipeIds\":[";bool fr=true;for(const auto&key:section.pieces){if(!present.count(key))continue;if(!fr)out<<',';fr=false;out<<'\"'<<CatalogId("building:"+key)<<'\"';}out<<"]}";}out<<"]}";}
    out<<"],\"count\":"<<records.size()<<",\"buildingCounts\":{\"expected\":"<<catalog.expectedPieces<<",\"resolved\":"<<resolvedBuildingPieces.load()<<",\"unavailable\":"<<unavailableBuildingPieces.load()<<",\"references\":"<<catalog.expectedReferences<<"}}";return out.str();
}
}

