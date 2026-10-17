#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "dom_engine.h"
#include "../utils/logger.h"
#include "../utils/memory.h"

#include <algorithm>
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
#include <pthread.h>
#include <regex>
#include <sstream>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unordered_map>
#include <unordered_set>
#include <unistd.h>

namespace DomEngine {
namespace {
bool ExpectedSha(const std::string& hash){return hash=="445f8b65047d208adb04a3cb03bc7706f069306d814172ae29caec16c15bec7e"||hash=="ef39023c04e1bde785907e13b9b64d72600455f1633bbfe231f639d31d98cfac";}
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

struct Request {
    std::string operation,adminGuid,adminId,targetGuid,targetId,chestId,itemId,catalogId,session,destinationChestId,customName;
    std::vector<std::string> itemIds;
    int quantity=0,destinationSlot=-1;
    bool complete=false,success=false;
    std::atomic<bool> cancelled{false};
    std::string result;
    pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t changed=PTHREAD_COND_INITIALIZER;
    ~Request(){pthread_cond_destroy(&changed);pthread_mutex_destroy(&mutex);}
};
pthread_mutex_t queueMutex=PTHREAD_MUTEX_INITIALIZER;
std::deque<std::shared_ptr<Request>> requests;
std::atomic<int> compatibility{0};
std::atomic<int> dispatcherCompatibility{0},batchCompatibility{0},renameCompatibility{0},removalTrackingCompatibility{0},catalogCompatibility{0},unlockCatalogCompatibility{0},planCatalogCompatibility{0};
std::atomic<int> buildingTaxonomyCompatibility{0},buildingRequirementsCompatibility{0},buildingIconsCompatibility{0},craftingRecipeCompatibility{0};
std::atomic<std::uint64_t> resolvedBuildingPieces{0},unavailableBuildingPieces{0};
std::atomic<std::uint64_t> queued{0},accepted{0},rejected{0},stale{0},maximumMicros{0};
std::atomic<std::uint64_t> tickCalls{0},maximumTickMicros{0},slowTickCalls{0};
std::atomic<std::uint64_t> maximumRecipeTickMicros{0},maximumCustomNameTickMicros{0};
std::atomic<std::uint64_t> maximumStorageTickMicros{0},maximumGravestoneTickMicros{0};
std::atomic<unsigned> pendingCount{0};
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
std::string ItemDisplayName(uintptr_t data){using Fn=const FString*(*)(const void*);auto fn=reinterpret_cast<Fn>(g_Engine->GetModuleBase()+0x04AA1A80);const FString* text=fn(reinterpret_cast<const void*>(data+0x68));if(text&&Mem::Readable(text,sizeof(FString))){auto value=g_Engine->ReadFString(reinterpret_cast<uintptr_t>(text));if(!value.empty())return value;}return CleanAssetName(g_Engine->GetObjectName(data));}
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
bool IsUnlockCatalogKey(const std::string&key){return key.rfind("DA_Consumable_Vestige_",0)==0||key.rfind("DA_Consumabe_Mount_",0)==0;}
bool AllowedCatalogKey(const std::string&key){return key.rfind("ITEM_",0)==0||(unlockCatalogCompatibility.load()==1&&IsUnlockCatalogKey(key))||(planCatalogCompatibility.load()==1&&PlanItemCatalog().keys.count(key));}
bool UnlockCatalogAssetsValid(){
    static const char* required[]={"DA_Consumable_Vestige_Weapon_Club_Swingslash.png","DA_Consumable_Vestige_Weapon_Longbow_HunterStag.png","DA_Consumable_Vestige_Weapon_Staff_GrievingMoon.png","DA_Consumable_Vestige_Weapon_Staff_ToktzMejTal.png","DA_Consumable_Vestige_Weapon_Whip_Abysmal.png","DA_Consumabe_Mount_TerrorBird_02_04.png","DA_Consumabe_Mount_TerrorBird_02_02.png"};
    for(const char*name:required){struct stat st{};std::string path="/home/container/zanik/web/items/"+std::string(name);if(stat(path.c_str(),&st)!=0||st.st_size<1)return false;}return true;
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
void Finish(const std::shared_ptr<Request>&r,bool ok,const std::string&message){Lock lock(r->mutex);r->success=ok;r->result=message;r->complete=true;pthread_cond_broadcast(&r->changed);}
void Execute(const std::shared_ptr<Request>&r){
    if(r->cancelled.load())return;
    auto start=std::chrono::steady_clock::now();Chest chest;
    if(!FindChest(r->targetGuid,r->chestId,chest)){++stale;Finish(r,false,"Storage container is no longer available");return;}
    bool adminFound=false;PlayerInfo admin=FindPlayer(r->adminGuid,adminFound);void* controller=adminFound?reinterpret_cast<void*>(admin.controllerPtr):nullptr;bool ok=false;
    if(r->operation=="rename"){
        alignas(16) unsigned char text[16]{},setterId[48]{};
        fTextFromString(text,r->customName.c_str());executeGetSetterId(setterId,reinterpret_cast<void*>(chest.actor));executeSetCustomName(reinterpret_cast<void*>(chest.actor),text,setterId);destroyUniqueNetId(setterId);destroyFText(text);
        const auto identity=g_Engine->GetObjectIdentity(chest.actor);if(identity){Lock lock(customNameMutex);customNames[identity]={r->customName,std::chrono::steady_clock::now()};pendingCustomNames.erase(identity);}ok=true;
    }else if(r->operation=="slot_move"){
        int slot=-1,current=0;uintptr_t item=0,itemData=0;std::string key;uintptr_t slotData=0;int32_t slotCount=0;
        if(!FindItem(chest,r->itemId,slot,item,itemData,current,key)){++stale;Finish(r,false,"The item changed before the request was applied");return;}
        Chest destination;if(!FindChest(r->targetGuid,r->destinationChestId,destination)){++stale;Finish(r,false,"Destination storage is no longer available");return;}
        if(!ReadSlots(destination.inventory,slotData,slotCount)||r->destinationSlot<0||r->destinationSlot>=slotCount){Finish(r,false,"Destination slot is no longer available");return;}
        if(chest.id==destination.id&&slot==r->destinationSlot){Finish(r,true,"Storage unchanged");return;}
        ok=moveItem(reinterpret_cast<void*>(chest.inventory),slot,reinterpret_cast<void*>(destination.inventory),r->destinationSlot,controller,current);
    }else if(r->operation=="delete_batch"){
        std::vector<ResolvedItem> items;if(!ResolveItems(chest,r->itemIds,items)){++stale;Finish(r,false,"One or more selected items changed before the request was applied");return;}
        // Resolve the complete selection before the first mutation. Requests execute on
        // the game thread, so the validated slots cannot change between these removals.
        ok=true;std::sort(items.begin(),items.end(),[](const auto&a,const auto&b){return a.slot>b.slot;});
        for(const auto&value:items)if(!removeFromSlot(reinterpret_cast<void*>(chest.inventory),value.slot,value.quantity,controller)){ok=false;break;}
    }else if(r->operation=="organize_batch"){
        Chest destination;if(!FindChest(r->targetGuid,r->destinationChestId,destination)){++stale;Finish(r,false,"Destination storage is no longer available");return;}
        std::vector<ResolvedItem> selected;if(!ResolveItems(chest,r->itemIds,selected)){++stale;Finish(r,false,"One or more selected items changed before the request was applied");return;}
        std::unordered_set<uintptr_t> selectedData;for(const auto&item:selected)selectedData.insert(item.data);
        struct LocatedItem{Chest chest;ResolvedItem item;};std::vector<LocatedItem> located;std::vector<ResolvedItem> capacityItems;
        for(const auto&source:FindChests(r->targetGuid)){if(source.id==destination.id)continue;uintptr_t slots=0;int32_t count=0;if(!ReadSlots(source.inventory,slots,count))continue;for(int32_t slot=0;slot<count;++slot){uintptr_t item=Mem::ReadPtr(slots+uintptr_t(slot)*8);if(!item||!g_Engine->IsA(item,"Item"))continue;uintptr_t data=Mem::ReadPtr(item+0xB0);int quantity=Mem::ReadI32(item+0x58);if(!data||quantity<=0||!selectedData.count(data))continue;ResolvedItem value{slot,quantity,item,data,g_Engine->GetObjectName(data),{}};located.push_back({source,value});capacityItems.push_back(value);if(located.size()>4096){Finish(r,false,"Too many matching stacks to organize safely in one operation");return;}}}
        if(located.empty()){Finish(r,true,"All matching items are already organized here");return;}
        if(!BatchFits(destination,capacityItems)){Finish(r,false,"The selected storage does not have enough space for all matching items");return;}
        struct AppliedMove{uintptr_t sourceInventory;int sourceSlot,targetSlot,quantity;};std::vector<AppliedMove> applied;ok=true;
        for(auto&locatedItem:located){auto&source=locatedItem.chest;auto&value=locatedItem.item;uintptr_t sourceSlots=0;int32_t sourceCount=0;if(!ReadSlots(source.inventory,sourceSlots,sourceCount)||value.slot<0||value.slot>=sourceCount){ok=false;break;}uintptr_t liveItem=Mem::ReadPtr(sourceSlots+uintptr_t(value.slot)*8),liveData=liveItem?Mem::ReadPtr(liveItem+0xB0):0;int liveQuantity=liveItem?Mem::ReadI32(liveItem+0x58):0;if(liveItem!=value.item||liveData!=value.data||liveQuantity!=value.quantity){ok=false;break;}
            uintptr_t destinationData=0;int32_t destinationSlots=0;if(!ReadSlots(destination.inventory,destinationData,destinationSlots)){ok=false;break;}std::vector<int> candidates;const int maxStack=MaxStackSize(liveData);
            for(int32_t i=0;i<destinationSlots;++i){uintptr_t existing=Mem::ReadPtr(destinationData+uintptr_t(i)*8);if(!existing)continue;uintptr_t existingData=Mem::ReadPtr(existing+0xB0);int existingCount=Mem::ReadI32(existing+0x58);if(existingData==liveData&&existingCount>=0&&existingCount<maxStack)candidates.push_back(i);}for(int32_t i=0;i<destinationSlots;++i)if(!Mem::ReadPtr(destinationData+uintptr_t(i)*8))candidates.push_back(i);
            int remaining=liveQuantity;for(int targetSlot:candidates){if(remaining<=0)break;uintptr_t existing=Mem::ReadPtr(destinationData+uintptr_t(targetSlot)*8);int amount=remaining;if(existing){int existingCount=Mem::ReadI32(existing+0x58);amount=std::min(remaining,std::max(0,maxStack-existingCount));}else amount=std::min(remaining,maxStack);if(amount>0&&moveItem(reinterpret_cast<void*>(source.inventory),value.slot,reinterpret_cast<void*>(destination.inventory),targetSlot,controller,amount)){remaining-=amount;applied.push_back({source.inventory,value.slot,targetSlot,amount});}}
            if(remaining){ok=false;break;}
        }
        if(!ok){bool rolledBack=true;for(auto it=applied.rbegin();it!=applied.rend();++it)if(!moveItem(reinterpret_cast<void*>(destination.inventory),it->targetSlot,reinterpret_cast<void*>(it->sourceInventory),it->sourceSlot,controller,it->quantity))rolledBack=false;if(!rolledBack){++rejected;Finish(r,false,"Organizing failed and the game could not fully restore the original slots; refresh storage immediately");return;}}
    }else if(r->operation=="move_batch"||r->operation=="copy_batch"){
        Chest destination;if(!FindChest(r->targetGuid,r->destinationChestId,destination)){++stale;Finish(r,false,"Destination storage is no longer available");return;}
        std::vector<ResolvedItem> items;if(!ResolveItems(chest,r->itemIds,items)){++stale;Finish(r,false,"One or more selected items changed before the request was applied");return;}
        if(r->operation=="move_batch"&&chest.id==destination.id){Finish(r,false,"Choose another storage for Move");return;}
        if(!BatchFits(destination,items)){Finish(r,false,"The destination storage does not have enough space for the complete selection");return;}
        if(r->operation=="copy_batch"){
            std::vector<void*> data;std::vector<int> quantities;for(const auto&item:items){data.push_back(reinterpret_cast<void*>(item.data));quantities.push_back(item.quantity);}
            FUEArray dataArray{data.data(),int32_t(data.size()),int32_t(data.size())},quantityArray{quantities.data(),int32_t(quantities.size()),int32_t(quantities.size())};alignas(8) unsigned char emptyTags[32]{};
            addItemsByData(reinterpret_cast<void*>(destination.inventory),&dataArray,&quantityArray,1.0f,emptyTags);ok=true;
        }else{
            struct AppliedMove{int sourceSlot,targetSlot,quantity;};std::vector<AppliedMove> applied;ok=true;for(auto&value:items){int liveSlot=-1,liveQuantity=0;uintptr_t liveItem=0,liveData=0;std::string liveKey;
                if(!FindItem(chest,value.id,liveSlot,liveItem,liveData,liveQuantity,liveKey)){ok=false;break;}
                uintptr_t destinationData=0;int32_t destinationSlots=0;if(!ReadSlots(destination.inventory,destinationData,destinationSlots)){ok=false;break;}
                std::vector<int> candidates;const int maxStack=MaxStackSize(liveData);
                for(int32_t i=0;i<destinationSlots;++i){uintptr_t existing=Mem::ReadPtr(destinationData+uintptr_t(i)*8);if(!existing)continue;uintptr_t existingData=Mem::ReadPtr(existing+0xB0);int existingCount=Mem::ReadI32(existing+0x58);if(existingData==liveData&&existingCount>=0&&existingCount<maxStack)candidates.push_back(i);}
                for(int32_t i=0;i<destinationSlots;++i)if(!Mem::ReadPtr(destinationData+uintptr_t(i)*8))candidates.push_back(i);
                int remaining=liveQuantity;for(int targetSlot:candidates){if(remaining<=0)break;uintptr_t existing=Mem::ReadPtr(destinationData+uintptr_t(targetSlot)*8);int amount=remaining;if(existing){int existingCount=Mem::ReadI32(existing+0x58);amount=std::min(remaining,std::max(0,maxStack-existingCount));}else amount=std::min(remaining,maxStack);if(amount>0&&moveItem(reinterpret_cast<void*>(chest.inventory),liveSlot,reinterpret_cast<void*>(destination.inventory),targetSlot,controller,amount)){remaining-=amount;applied.push_back({liveSlot,targetSlot,amount});}}
                if(remaining) {ok=false;break;}
            }
            if(!ok){bool rolledBack=true;for(auto it=applied.rbegin();it!=applied.rend();++it)if(!moveItem(reinterpret_cast<void*>(destination.inventory),it->targetSlot,reinterpret_cast<void*>(chest.inventory),it->sourceSlot,controller,it->quantity))rolledBack=false;if(!rolledBack){++rejected;Finish(r,false,"The batch failed and the game could not fully restore its original slots; refresh storage immediately");return;}}
        }
    }else if(r->operation=="add_recipe"){
        RecipeRecord recipe;if(!ResolveRecipe(r->catalogId,recipe)){++stale;Finish(r,false,"Selected recipe is no longer available");return;}std::vector<void*> items;std::vector<int> quantities;long long total=0;for(const auto&ingredient:recipe.ingredients){long long amount=1LL*ingredient.quantity*r->quantity;if(amount<1||amount>1000000||total+amount>4000000){Finish(r,false,"Recipe quantity is too large");return;}items.push_back(reinterpret_cast<void*>(ingredient.data));quantities.push_back(int(amount));total+=amount;}FUEArray itemArray{items.data(),int32_t(items.size()),int32_t(items.size())},quantityArray{quantities.data(),int32_t(quantities.size()),int32_t(quantities.size())};std::vector<unsigned char> containers(items.size()*16);for(size_t i=0;i<items.size();++i){std::memcpy(containers.data()+i*16,&items[i],8);std::memcpy(containers.data()+i*16+8,&quantities[i],4);}FUEArray capacityArray{containers.data(),int32_t(items.size()),int32_t(items.size())};if(!canAddItemsByData(reinterpret_cast<void*>(chest.inventory),&capacityArray)){Finish(r,false,"The storage does not have enough space for the complete recipe");return;}alignas(8) unsigned char emptyTags[32]{};addItemsByData(reinterpret_cast<void*>(chest.inventory),&itemArray,&quantityArray,1.0f,emptyTags);ok=true;
    }else if(r->operation=="add"){
        std::string key;uintptr_t itemData=FindCatalogData(r->catalogId,&key);if(!itemData){++stale;Finish(r,false,"Selected item is no longer available");return;}
        if(!canAddItemByData(reinterpret_cast<void*>(chest.inventory),reinterpret_cast<void*>(itemData),r->quantity)){Finish(r,false,"The storage does not have enough space");return;}
        alignas(8) unsigned char emptyTags[32]{};ok=addItemByData(reinterpret_cast<void*>(chest.inventory),reinterpret_cast<void*>(itemData),r->quantity,1.0f,emptyTags);
    }else{
        int slot=-1,current=0;uintptr_t item=0,itemData=0;std::string key;
        if(!FindItem(chest,r->itemId,slot,item,itemData,current,key)){++stale;Finish(r,false,"The item changed before the request was applied");return;}
        if(r->quantity<1||(r->operation!="add_existing"&&r->quantity>current)){Finish(r,false,"Requested quantity is no longer available");return;}
        if(r->operation=="delete")ok=removeFromSlot(reinterpret_cast<void*>(chest.inventory),slot,r->quantity,controller);
        else if(r->operation=="add_existing"){
            if(!canAddItemByData(reinterpret_cast<void*>(chest.inventory),reinterpret_cast<void*>(itemData),r->quantity)){Finish(r,false,"The storage does not have enough space");return;}
            alignas(8) unsigned char emptyTags[32]{};ok=addItemByData(reinterpret_cast<void*>(chest.inventory),reinterpret_cast<void*>(itemData),r->quantity,1.0f,emptyTags);
        }
        else if(r->operation=="take"){
            if(!adminFound||!admin.spawned||admin.isDead){Finish(r,false,"Linked administrator character is offline or unavailable");return;}
            uintptr_t target=FindPlayerInventory(admin);if(!target){Finish(r,false,"Administrator inventory is unavailable");return;}
            if(!canAddItemByData(reinterpret_cast<void*>(target),reinterpret_cast<void*>(itemData),r->quantity)){Finish(r,false,"Administrator inventory does not have enough space");return;}
            uintptr_t targetData=0;int32_t targetSlots=0;if(!ReadSlots(target,targetData,targetSlots)){Finish(r,false,"Administrator inventory slots are unavailable");return;}
            const int maxStack=MaxStackSize(itemData);std::vector<int> candidates;
            for(int32_t i=0;i<targetSlots;++i){uintptr_t existing=Mem::ReadPtr(targetData+uintptr_t(i)*8);if(!existing)candidates.push_back(i);}
            for(int32_t i=0;i<targetSlots;++i){uintptr_t existing=Mem::ReadPtr(targetData+uintptr_t(i)*8);if(!existing)continue;uintptr_t existingData=Mem::ReadPtr(existing+0xB0);int existingCount=Mem::ReadI32(existing+0x58);if(existingData==itemData&&existingCount>=0&&existingCount+r->quantity<=maxStack)candidates.insert(candidates.begin(),i);}
            for(int targetSlot:candidates)if(moveItem(reinterpret_cast<void*>(chest.inventory),slot,reinterpret_cast<void*>(target),targetSlot,reinterpret_cast<void*>(admin.controllerPtr),r->quantity)){ok=true;break;}
        }
    }
    if(ok){++accepted;Finish(r,true,r->operation=="organize_batch"?"Items organized":"Storage updated");}else{++rejected;Finish(r,false,"The game rejected the inventory operation");}
    auto micros=static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-start).count());auto previous=maximumMicros.load();while(previous<micros&&!maximumMicros.compare_exchange_weak(previous,micros)){}
}

void WriteJump(unsigned char* at,uintptr_t target){at[0]=0x48;at[1]=0xB8;std::memcpy(at+2,&target,8);at[10]=0xFF;at[11]=0xE0;}
void HookBuildingEndPlay(void* actor,int reason){
    if(reason==0&&g_Engine){const auto identity=g_Engine->GetObjectIdentity(reinterpret_cast<uintptr_t>(actor));if(identity){Lock lock(removedBuildingsMutex);if(removedBuildingIdentities.insert(identity).second){removedBuildingOrder.push_back(identity);++buildingRemovalEvents;if(removedBuildingOrder.size()>65536){removedBuildingIdentities.erase(removedBuildingOrder.front());removedBuildingOrder.pop_front();}}}}
    originalBuildingEndPlay(actor,reason);
}
bool InstallBuildingEndPlayHook(uintptr_t moduleBase,uintptr_t moduleEnd){
    if(originalBuildingEndPlay)return true;if(moduleBase+BuildingEndPlayOffset+BuildingEndPlayHookBytes>moduleEnd)return false;auto* target=reinterpret_cast<unsigned char*>(moduleBase+BuildingEndPlayOffset);const unsigned char expected[BuildingEndPlayHookBytes]={0x55,0x41,0x57,0x41,0x56,0x41,0x55,0x41,0x54,0x53,0x50,0x89,0xf5,0x49,0x89,0xfe};if(std::memcmp(target,expected,sizeof(expected)))return false;
    const unsigned char signature[]={0x55,0x41,0x57,0x41,0x56,0x41,0x55,0x41,0x54,0x53,0x50,0x89,0xF5,0x49,0x89,0xFE,0xE8,0x9B,0x05,0x72,0xFD,0x49,0x8D,0x9E,0x80,0x03,0x00,0x00,0x83,0xFD,0x03,0x0F,0x85,0x8B,0x00,0x00,0x00};if(!Mem::Readable(target,sizeof(signature))||std::memcmp(target,signature,sizeof(signature)))return false;
    auto* trampoline=static_cast<unsigned char*>(mmap(nullptr,64,PROT_READ|PROT_WRITE|PROT_EXEC,MAP_PRIVATE|MAP_ANONYMOUS,-1,0));if(trampoline==MAP_FAILED)return false;std::memcpy(trampoline,target,BuildingEndPlayHookBytes);WriteJump(trampoline+BuildingEndPlayHookBytes,reinterpret_cast<uintptr_t>(target+BuildingEndPlayHookBytes));originalBuildingEndPlay=reinterpret_cast<BuildingEndPlayFn>(trampoline);
    long pageSize=sysconf(_SC_PAGESIZE);uintptr_t page=reinterpret_cast<uintptr_t>(target)&~(uintptr_t(pageSize)-1);if(mprotect(reinterpret_cast<void*>(page),size_t(pageSize),PROT_READ|PROT_WRITE|PROT_EXEC)!=0){originalBuildingEndPlay=nullptr;munmap(trampoline,64);return false;}unsigned char patch[BuildingEndPlayHookBytes];std::memset(patch,0x90,sizeof(patch));WriteJump(patch,reinterpret_cast<uintptr_t>(&HookBuildingEndPlay));std::memcpy(target,patch,sizeof(patch));__builtin___clear_cache(reinterpret_cast<char*>(target),reinterpret_cast<char*>(target+sizeof(patch)));mprotect(reinterpret_cast<void*>(page),size_t(pageSize),PROT_READ|PROT_EXEC);return true;
}
void HookEngineTick(void* actor,float delta,int levelTick,void* tickFunction){
    originalEngineTick(actor,delta,levelTick,tickFunction);
    const auto tickStarted=std::chrono::steady_clock::now();++tickCalls;
    auto measure=[](const std::chrono::steady_clock::time_point&started,std::atomic<std::uint64_t>&maximum){
        const auto elapsed=static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now()-started).count());auto prior=maximum.load();
        while(elapsed>prior&&!maximum.compare_exchange_weak(prior,elapsed)){}return elapsed;
    };
    auto sectionStarted=std::chrono::steady_clock::now();
    ProcessPlayerRecipeTracking();
    ProcessPlayerQuestTracking();
    measure(sectionStarted,maximumRecipeTickMicros);sectionStarted=std::chrono::steady_clock::now();
    if(executeGetCustomName&&destroyFText&&g_Engine){std::vector<std::pair<std::uint64_t,uintptr_t>> work;{Lock lock(customNameMutex);for(auto it=pendingCustomNames.begin();it!=pendingCustomNames.end()&&work.size()<4;){work.push_back(*it);it=pendingCustomNames.erase(it);}}
        using GetDisplayStringFn=const FString*(*)(const void*);auto display=reinterpret_cast<GetDisplayStringFn>(g_Engine->GetModuleBase()+0x04AA1A80);
        for(const auto&entry:work){std::string value;if(g_Engine->IsObjectLive(entry.second)){alignas(8) unsigned char text[16]{};executeGetCustomName(text,reinterpret_cast<void*>(entry.second));const FString* shown=display(text);if(shown&&Mem::Readable(shown,sizeof(FString)))value=g_Engine->ReadFString(reinterpret_cast<uintptr_t>(shown));destroyFText(text);}Lock lock(customNameMutex);customNames[entry.first]={std::move(value),std::chrono::steady_clock::now()};}}
    measure(sectionStarted,maximumCustomNameTickMicros);sectionStarted=std::chrono::steady_clock::now();
    if(pendingCount.load(std::memory_order_acquire))ProcessAdminStorageRequests();
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
std::string GetAdminStorageCustomName(uintptr_t actor){if(!g_Engine||!actor)return {};const auto identity=g_Engine->GetObjectIdentity(actor);if(!identity)return {};const auto now=std::chrono::steady_clock::now();Lock lock(customNameMutex);auto found=customNames.find(identity);if(found==customNames.end()||now-found->second.refreshed>=std::chrono::seconds(4))pendingCustomNames[identity]=actor;return found==customNames.end()?std::string{}:found->second.value;}

int ExecutableBounds(struct dl_phdr_info* info,size_t,void* data){
    if(info->dlpi_name&&info->dlpi_name[0])return 0;
    auto* bounds=reinterpret_cast<uintptr_t*>(data);bool found=false;uintptr_t lowest=0,highest=0;
    for(int i=0;i<info->dlpi_phnum;++i){const auto& p=info->dlpi_phdr[i];if(p.p_type!=PT_LOAD)continue;uintptr_t begin=uintptr_t(info->dlpi_addr)+p.p_vaddr,end=begin+p.p_memsz;if(!found||begin<lowest)lowest=begin;if(!found||end>highest)highest=end;found=true;}
    bounds[0]=lowest;bounds[1]=highest;bounds[2]=found?1:0;
    return found?1:0;
}

bool InitializeAdminStorage(uintptr_t moduleBase,uintptr_t moduleEnd){
    if(!ExpectedSha(FileSha256("/proc/self/exe"))){compatibility=-1;return false;}
    if(!earlyHookInstalled.load()||!originalEngineTick){compatibility=-6;return false;}
    for(uintptr_t o:{AddItemByDataOffset,CanAddItemByDataOffset,MoveItemOffset,RemoveFromSlotOffset,CanAddItemsByDataOffset,AddItemsByDataOffset,BuildingScaledRequirementsOffset,FMemoryFreeOffset,EngineTickOffset,RenameableGetCustomNameOffset,RenameableGetSetterIdOffset,RenameableSetCustomNameOffset,FTextFromStringOffset,UniqueNetIdDestructorOffset,FTextDestructorOffset})if(moduleBase+o<moduleBase||moduleBase+o+EngineTickHookBytes>moduleEnd){compatibility=-2;return false;}
    const unsigned char addSig[]={0x55,0x41,0x57,0x41,0x56,0x41,0x54,0x53,0x48,0x83,0xEC,0x30};
    const unsigned char moveSig[]={0x55,0x53,0x50,0x4C,0x89,0xC0,0x48,0x89,0xFB};
    const unsigned char removeSig[]={0x55,0x41,0x57,0x41,0x56,0x53,0x48,0x83,0xEC,0x28};
    if(std::memcmp(reinterpret_cast<void*>(moduleBase+AddItemByDataOffset),addSig,sizeof(addSig))||std::memcmp(reinterpret_cast<void*>(moduleBase+MoveItemOffset),moveSig,sizeof(moveSig))||std::memcmp(reinterpret_cast<void*>(moduleBase+RemoveFromSlotOffset),removeSig,sizeof(removeSig))){compatibility=-3;return false;}
    const unsigned char batchSig[]={0x55,0x41,0x57,0x41,0x56,0x41,0x55,0x41,0x54};const unsigned char buildSig[]={0x41,0x57,0x41,0x56,0x41,0x54,0x53};batchCompatibility=(!std::memcmp(reinterpret_cast<void*>(moduleBase+AddItemsByDataOffset),batchSig,sizeof(batchSig))&&!std::memcmp(reinterpret_cast<void*>(moduleBase+BuildingScaledRequirementsOffset),buildSig,sizeof(buildSig)))?1:-1;
    planCatalogCompatibility=PlanItemCatalog().valid?1:-1;
    if(planCatalogCompatibility.load()!=1)LogMessage("Plan item catalog assets unavailable; plan items remain visible in storage but cannot be added through catalog mode");
    addItemByData=reinterpret_cast<AddItemByDataFn>(moduleBase+AddItemByDataOffset);canAddItemByData=reinterpret_cast<CanAddItemByDataFn>(moduleBase+CanAddItemByDataOffset);moveItem=reinterpret_cast<MoveItemFn>(moduleBase+MoveItemOffset);removeFromSlot=reinterpret_cast<RemoveFromSlotFn>(moduleBase+RemoveFromSlotOffset);canAddItemsByData=reinterpret_cast<CanAddItemsByDataFn>(moduleBase+CanAddItemsByDataOffset);addItemsByData=reinterpret_cast<AddItemsByDataFn>(moduleBase+AddItemsByDataOffset);buildingScaledRequirements=reinterpret_cast<BuildingScaledRequirementsFn>(moduleBase+BuildingScaledRequirementsOffset);memoryFree=reinterpret_cast<FMemoryFreeFn>(moduleBase+FMemoryFreeOffset);executeGetCustomName=reinterpret_cast<ExecuteGetCustomNameFn>(moduleBase+RenameableGetCustomNameOffset);executeGetSetterId=reinterpret_cast<ExecuteGetSetterIdFn>(moduleBase+RenameableGetSetterIdOffset);executeSetCustomName=reinterpret_cast<ExecuteSetCustomNameFn>(moduleBase+RenameableSetCustomNameOffset);fTextFromString=reinterpret_cast<FTextFromStringFn>(moduleBase+FTextFromStringOffset);destroyUniqueNetId=reinterpret_cast<UniqueNetIdDestructorFn>(moduleBase+UniqueNetIdDestructorOffset);destroyFText=reinterpret_cast<FTextDestructorFn>(moduleBase+FTextDestructorOffset);renameCompatibility=1;catalogCompatibility=1;unlockCatalogCompatibility=UnlockCatalogAssetsValid()?1:-1;const auto& buildCatalog=AuthoritativeBuildingCatalog();buildingTaxonomyCompatibility=buildCatalog.valid?1:-1;buildingIconsCompatibility=buildCatalog.iconsValid?1:-1;buildingRequirementsCompatibility=batchCompatibility.load()==1?1:-1;craftingRecipeCompatibility=batchCompatibility.load()==1?1:-1;if(unlockCatalogCompatibility.load()!=1)LogMessage("Vestige and mount unlock catalog assets unavailable; ordinary item catalog remains enabled");if(buildingTaxonomyCompatibility.load()!=1)LogMessage("Authoritative building catalogue is invalid; crafting recipes remain enabled");if(buildingIconsCompatibility.load()!=1)LogMessage("One or more authoritative building catalogue icons are unavailable");removalTrackingCompatibility=InstallBuildingEndPlayHook(moduleBase,moduleEnd)?1:-1;if(removalTrackingCompatibility.load()!=1)LogMessage("Immediate storage removal tracking unavailable; storage editing remains enabled with ordinary actor reconciliation");dispatcherCompatibility=1;compatibility=1;return true;
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
void ProcessAdminStorageRequests(){std::shared_ptr<Request> request;{Lock lock(queueMutex);while(!requests.empty()&&requests.front()->cancelled.load()){requests.pop_front();pendingCount.fetch_sub(1);}if(requests.empty())return;request=requests.front();requests.pop_front();pendingCount.fetch_sub(1);}if(!request->cancelled.load())Execute(request);}
bool SubmitAdminStorageMutation(const std::string&operation,const std::string&adminGuid,const std::string&adminId,const std::string&targetGuid,const std::string&targetId,const std::string&chestId,const std::string&itemId,const std::string&catalogId,int quantity,const std::string&session,std::string&result,const std::string&destinationChestId,int destinationSlot,const std::vector<std::string>&itemIds,const std::string&customName){
    if(compatibility.load()!=1){result="Storage editing compatibility unavailable";return false;}const bool slotOperation=operation=="slot_move",batchOperation=operation=="move_batch"||operation=="copy_batch"||operation=="organize_batch",deleteBatch=operation=="delete_batch";
    if(operation=="rename"&&renameCompatibility.load()!=1){result="Storage naming compatibility unavailable";return false;}
    if((operation=="add_recipe"||operation=="copy_batch")&&batchCompatibility.load()!=1){result="Batch storage compatibility unavailable";return false;}
    if((operation!="add"&&operation!="add_recipe"&&operation!="add_existing"&&operation!="delete"&&operation!="take"&&operation!="rename"&&!slotOperation&&!batchOperation&&!deleteBatch)||(operation!="rename"&&!slotOperation&&!batchOperation&&!deleteBatch&&(quantity<1||quantity>(operation=="add_recipe"?1000:1000000)))||(operation=="rename"&&customName.size()>96)||(slotOperation&&destinationSlot<0)||(batchOperation&&(destinationChestId.empty()||itemIds.empty()))||(deleteBatch&&itemIds.empty())){result="Invalid storage operation";return false;}
    auto r=std::make_shared<Request>();r->operation=operation;r->adminGuid=adminGuid;r->adminId=adminId;r->targetGuid=targetGuid;r->targetId=targetId;r->chestId=chestId;r->itemId=itemId;r->catalogId=catalogId;r->quantity=quantity;r->session=session;r->destinationChestId=destinationChestId;r->destinationSlot=destinationSlot;r->itemIds=itemIds;r->customName=customName;
    {Lock lock(queueMutex);if(requests.size()>=QueueCapacity||std::any_of(requests.begin(),requests.end(),[&](const auto&q){return q->chestId==chestId&&q->session==session;})){result="Another storage operation is already pending";return false;}requests.push_back(r);pendingCount.fetch_add(1,std::memory_order_release);++queued;}
    timespec deadline{};clock_gettime(CLOCK_REALTIME,&deadline);deadline.tv_sec+=3;pthread_mutex_lock(&r->mutex);while(!r->complete){int wait=pthread_cond_timedwait(&r->changed,&r->mutex,&deadline);if(wait!=0&&!r->complete){r->cancelled=true;pthread_mutex_unlock(&r->mutex);{Lock lock(queueMutex);auto old=requests.size();requests.erase(std::remove(requests.begin(),requests.end(),r),requests.end());if(requests.size()!=old)pendingCount.fetch_sub(1);}result="Game thread did not process the request in time";return false;}}result=r->result;bool success=r->success;pthread_mutex_unlock(&r->mutex);return success;
}
AdminStorageDiagnostics GetAdminStorageDiagnostics(){AdminStorageDiagnostics d;d.compatibility=compatibility.load();d.dispatcherCompatibility=dispatcherCompatibility.load();d.mutationCompatibility=compatibility.load();d.batchCompatibility=batchCompatibility.load();d.organizationCompatibility=compatibility.load();d.renameCompatibility=renameCompatibility.load();d.removalTrackingCompatibility=removalTrackingCompatibility.load();d.catalogCompatibility=catalogCompatibility.load();d.unlockCatalogCompatibility=unlockCatalogCompatibility.load();d.planCatalogCompatibility=planCatalogCompatibility.load();d.planCatalogItems=PlanItemCatalog().keys.size();d.buildingTaxonomyCompatibility=buildingTaxonomyCompatibility.load();d.buildingRequirementsCompatibility=buildingRequirementsCompatibility.load();d.buildingIconsCompatibility=buildingIconsCompatibility.load();d.craftingRecipeCompatibility=craftingRecipeCompatibility.load();const auto& catalog=AuthoritativeBuildingCatalog();d.expectedBuildingPieces=catalog.expectedPieces;d.resolvedBuildingPieces=resolvedBuildingPieces.load();d.unavailableBuildingPieces=unavailableBuildingPieces.load();d.buildingTabs=catalog.tabs.size();for(const auto&tab:catalog.tabs){d.buildingSections+=tab.sections.size();for(const auto&section:tab.sections)d.buildingReferences+=section.pieces.size();}d.queuedRequests=queued.load();d.acceptedMutations=accepted.load();d.rejectedRequests=rejected.load();d.staleRequests=stale.load();d.maximumExecutionMicroseconds=maximumMicros.load();d.tickCalls=tickCalls.load();d.maximumTickMicroseconds=maximumTickMicros.load();d.slowTickCalls=slowTickCalls.load();d.maximumRecipeTickMicroseconds=maximumRecipeTickMicros.load();d.maximumCustomNameTickMicroseconds=maximumCustomNameTickMicros.load();d.maximumStorageTickMicroseconds=maximumStorageTickMicros.load();d.maximumGravestoneTickMicroseconds=maximumGravestoneTickMicros.load();return d;}
std::string GetAdminStorageCatalogJson(){
    if(compatibility.load()!=1||!g_Engine)return {};using GetDisplayStringFn=const FString*(*)(const void*);auto display=reinterpret_cast<GetDisplayStringFn>(g_Engine->GetModuleBase()+0x04AA1A80);
    std::ostringstream out;out<<"{\"items\":[";bool first=true;int count=0;const int32_t objects=g_Engine->GetObjectCount();
    for(int32_t i=0;i<objects;++i){uintptr_t data=g_Engine->GetObjectByIndex(i);if(!data||!g_Engine->IsA(data,"ItemData"))continue;std::string key=g_Engine->GetObjectName(data);if(!AllowedCatalogKey(key))continue;std::string lower=key;std::transform(lower.begin(),lower.end(),lower.begin(),[](unsigned char c){return char(std::tolower(c));});if(lower.find("test")!=std::string::npos||lower.find("debug")!=std::string::npos||lower.find("developer")!=std::string::npos||lower.find("deprecated")!=std::string::npos||lower.find("_old")!=std::string::npos)continue;std::string icon="/home/container/zanik/web/items/"+key+".png";struct stat st{};if(stat(icon.c_str(),&st)!=0||st.st_size<1)continue;std::string name;const FString* text=display(reinterpret_cast<const void*>(data+0x68));if(text&&Mem::Readable(text,sizeof(FString)))name=g_Engine->ReadFString(reinterpret_cast<uintptr_t>(text));if(name.empty())continue;int maxStack=MaxStackSize(data);auto meta=CatalogMetadata(key,name);if(!first)out<<',';first=false;++count;out<<"{\"id\":\""<<CatalogId(key)<<"\",\"name\":\""<<Escape(name)<<"\",\"icon\":\"/assets/items/"<<Escape(key)<<".png\",\"maxStack\":"<<maxStack<<",\"stackable\":"<<(maxStack>1?"true":"false")<<",\"category\":\""<<Escape(meta.category)<<"\",\"categoryOrder\":"<<meta.categoryOrder<<",\"tier\":"<<(meta.tier?std::to_string(meta.tier):"null")<<",\"basePowerLevel\":"<<(meta.tier?std::to_string(meta.tier):"null")<<",\"recipeOrder\":"<<meta.recipeOrder<<",\"aliases\":[\""<<Escape(meta.aliases)<<"\"]}";}
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

