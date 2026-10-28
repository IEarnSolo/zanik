#include "dom_engine.h"
#include "../utils/memory.h"
#include "../utils/logger.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cctype>
#include <cstring>
#include <memory>
#include <pthread.h>
#include <sstream>
#include <unordered_map>

namespace DomEngine { namespace {
enum class EntryKind { Material, Perk, Spell };
struct Entry { EntryKind kind;uintptr_t object=0;std::string key,name,icon; };
struct Target {std::string guid;uintptr_t controller=0,pawn=0,state=0,progress=0,perks=0;bool regionsComplete=false;std::uint32_t regionBitmap=0;std::vector<ActivityRegionInfo> regions;};
struct Scan {Target target;size_t cursor=0;bool materialsComplete=true,unlocksComplete=true;std::vector<ActivityMaterialInfo> materials;std::vector<ActivityUnlockInfo> unlocks;};
using ProcessEventFn=void(*)(void*,void*,void*);
std::atomic<int> regionCompatibility{0},materialCompatibility{0},unlockCompatibility{0};
std::atomic<std::uint64_t> snapshots{0},dropped{0};
std::atomic<std::uint64_t> materialCatalogEntries{0},perkCatalogEntries{0},spellCatalogEntries{0};
std::unordered_map<std::uint8_t,std::string> regionNames;
bool regionCatalogScanned=false;
std::chrono::steady_clock::time_point lastRegionScan{};
std::vector<Entry> catalog;std::vector<Target> targets;
std::unordered_map<std::string,PlayerActivityDiscoverySnapshot> completedSnapshots;
std::unordered_map<std::string,uintptr_t> progressCache,perkCache;
pthread_mutex_t targetMutex=PTHREAD_MUTEX_INITIALIZER,snapshotMutex=PTHREAD_MUTEX_INITIALIZER;
struct Lock{pthread_mutex_t*p;explicit Lock(pthread_mutex_t&v):p(&v){pthread_mutex_lock(p);}~Lock(){pthread_mutex_unlock(p);}};
std::unique_ptr<Scan> active;ProcessEventFn processEvent=nullptr;
uintptr_t hasPickedUpItem=0,isPerkUnlocked=0,isSpellUnlocked=0;
std::string materialFingerprint,unlockFingerprint;

std::string Clean(std::string value){for(char&c:value)if(c=='_')c=' ';for(const char*prefix:{"DA ","BP ","PERK ","SPELL ","ITEM "})if(value.rfind(prefix,0)==0)value.erase(0,std::char_traits<char>::length(prefix));return value;}
std::string Fingerprint(const std::vector<std::string>&values){std::uint64_t hash=1469598103934665603ull;for(const auto&value:values){for(unsigned char c:value){hash^=c;hash*=1099511628211ull;}hash^=0xff;hash*=1099511628211ull;}std::ostringstream out;out<<std::hex<<hash;return out.str();}
std::string AssetIcon(const std::string&group,const std::string&key){std::string safe;for(unsigned char c:key)if(std::isalnum(c)||c=='_'||c=='-')safe+=char(c);return safe.empty()?std::string{}:"/assets/"+group+"/"+safe+".png";}
uintptr_t ComponentFor(const Target&t,const char*type,const std::initializer_list<const char*>names,std::unordered_map<std::string,uintptr_t>&cache){auto known=cache.find(t.guid);if(known!=cache.end()&&g_Engine->IsObjectLive(known->second)&&g_Engine->IsA(known->second,type))return known->second;for(const char*name:names){int32_t offset=g_Engine->FindPropertyOffset(t.controller,name);if(offset<0||offset>0x3000)continue;uintptr_t value=Mem::ReadPtr(t.controller+uintptr_t(offset));if(value&&g_Engine->IsObjectLive(value)&&g_Engine->IsA(value,type)){cache[t.guid]=value;return value;}}for(int32_t i=0;i<g_Engine->GetObjectCount();++i){uintptr_t value=g_Engine->GetObjectByIndex(i);if(!value||!g_Engine->IsObjectLive(value)||!g_Engine->IsA(value,type))continue;uintptr_t outer=value;for(int depth=0;depth<7&&outer;++depth){outer=Mem::ReadPtr(outer+Offsets::UObject_Outer);if(outer&&(outer==t.controller||outer==t.pawn||outer==t.state)){cache[t.guid]=value;return value;}}}return 0;}
uintptr_t ProgressFor(const Target&t){return ComponentFor(t,"ProgressComponent",{"BP_Components_Progress","ProgressComponent"},progressCache);}
uintptr_t PerksFor(const Target&t){return ComponentFor(t,"SkillPerkComponent",{"BP_Components_SkillPerk","BP_Components_SkillPerks","SkillPerkComponent"},perkCache);}
bool ReadRegions(const Target&p,std::uint32_t&bitmap,std::vector<ActivityRegionInfo>&out){int32_t componentOffset=g_Engine->FindPropertyOffset(p.controller,"RevealedMapFogComponent");if(componentOffset<0||componentOffset>0x3000)return false;uintptr_t component=Mem::ReadPtr(p.controller+uintptr_t(componentOffset));if(!component||!g_Engine->IsObjectLive(component)||!g_Engine->IsA(component,"RevealedMapFogComponent"))return false;int32_t bitmapOffset=g_Engine->FindPropertyOffset(component,"RevealedMapRegionsBitmap");if(bitmapOffset<0||bitmapOffset>0x1000||!Mem::Read(component+uintptr_t(bitmapOffset),bitmap))return false;bool scan=!regionCatalogScanned;for(unsigned i=0;i<32;++i)if((bitmap&(1u<<i))&&!regionNames.count(static_cast<std::uint8_t>(i)))scan=true;if(scan){auto now=std::chrono::steady_clock::now();if(regionCatalogScanned&&now-lastRegionScan<std::chrono::seconds(5))return false;lastRegionScan=now;regionCatalogScanned=true;for(int32_t i=0;i<g_Engine->GetObjectCount();++i){uintptr_t region=g_Engine->GetObjectByIndex(i);if(!region||!g_Engine->IsObjectLive(region)||!g_Engine->IsA(region,"MapRegion"))continue;int32_t idOffset=g_Engine->FindPropertyOffset(region,"RegionID");if(idOffset<0||idOffset>0x1000)continue;std::uint8_t id=Mem::ReadU8(region+uintptr_t(idOffset));if(id>=32)continue;std::string name=g_Engine->ReadObjectText(region,"RegionName");if(!name.empty()&&name.size()<=512)regionNames[id]=std::move(name);}}for(unsigned i=0;i<32;++i)if(bitmap&(1u<<i)){auto found=regionNames.find(static_cast<std::uint8_t>(i));if(found==regionNames.end())return false;out.push_back({static_cast<std::uint8_t>(i),found->second});}std::uint32_t confirmed=0;return Mem::Read(component+uintptr_t(bitmapOffset),confirmed)&&confirmed==bitmap;}
bool Query(uintptr_t component,uintptr_t function,uintptr_t object){alignas(8)struct{void*data;std::uint8_t result;std::uint8_t padding[7];}params{};params.data=reinterpret_cast<void*>(object);processEvent(reinterpret_cast<void*>(component),reinterpret_cast<void*>(function),&params);return params.result!=0;}
bool ValidateQuery(uintptr_t function){int input=-1,result=-1;for(const auto&field:g_Engine->GetStructPropertyOffsets(function)){if(field.first=="ReturnValue")result=field.second;else input=field.second;}return (input==0&&result==8)||(input<0&&result<0);}
bool Online(const Target&t){Lock lock(targetMutex);return std::any_of(targets.begin(),targets.end(),[&](const auto&v){return v.guid==t.guid&&v.controller==t.controller;});}
}

int PlayerActivitySourceCompatibility(bool queryFound,bool queryValid,bool catalogReady){return !queryFound?-2:!queryValid?-3:!catalogReady?-4:1;}
bool PlayerActivityDiscoveryReady(int materialStatus,int unlockStatus){return materialStatus==1||unlockStatus==1;}

bool InitializePlayerActivityDiscovery(uintptr_t moduleBase,uintptr_t moduleEnd){
    uintptr_t a=0,b=0,processOffset=0,c=0,d=0;
    if(!GetAdminTeleportOffsets(a,b,processOffset,c,d)||moduleBase+processOffset>=moduleEnd){
        materialCompatibility=-10;unlockCompatibility=-10;return false;
    }
    processEvent=reinterpret_cast<ProcessEventFn>(moduleBase+processOffset);
    hasPickedUpItem=g_Engine->FindFunction("ProgressComponent","HasPickedUpItem");
    isSpellUnlocked=g_Engine->FindFunction("ProgressComponent","IsSpellUnlocked");
    isPerkUnlocked=g_Engine->FindFunction("SkillPerkComponent","IsPerkUnlocked");
    const bool materialQueryFound=hasPickedUpItem!=0;
    const bool materialQueryValid=materialQueryFound&&ValidateQuery(hasPickedUpItem);
    const bool unlockQueriesFound=isSpellUnlocked&&isPerkUnlocked;
    const bool unlockQueriesValid=unlockQueriesFound&&ValidateQuery(isSpellUnlocked)&&ValidateQuery(isPerkUnlocked);

    catalog.clear();std::vector<std::string> materialKeys,unlockKeys;
    for(int32_t i=0;i<g_Engine->GetObjectCount();++i){
        uintptr_t object=g_Engine->GetObjectByIndex(i);if(!object||!g_Engine->IsObjectLive(object))continue;
        std::string key=g_Engine->GetObjectName(object);if(key.empty()||key.size()>256||key.rfind("Default__",0)==0)continue;
        if(g_Engine->IsA(object,"ItemData")){
            bool notify=false;if(!g_Engine->ReadBoolProperty(object,"bShowNotificationOnFirstPickup",notify)||!notify)continue;
            std::string name=g_Engine->ReadObjectText(object,"Name");if(name.empty())name=Clean(key);
            catalog.push_back({EntryKind::Material,object,key,name,AssetIcon("items",key)});materialKeys.push_back(key);
        }else if(g_Engine->IsA(object,"SkillPerkData")){
            std::string name=g_Engine->ReadObjectText(object,"PerkName");if(name.empty())name=Clean(key);
            catalog.push_back({EntryKind::Perk,object,key,name,{}});unlockKeys.push_back("perk:"+key);
        }else if(g_Engine->IsA(object,"UtilitySpellData")){
            std::string name=g_Engine->ReadObjectText(object,"SpellDisplayName");if(name.empty())name=Clean(key);
            catalog.push_back({EntryKind::Spell,object,key,name,{}});unlockKeys.push_back("spell:"+key);
        }
    }
    std::sort(catalog.begin(),catalog.end(),[](const auto&x,const auto&y){return int(x.kind)<int(y.kind)||(x.kind==y.kind&&x.key<y.key);});
    std::sort(materialKeys.begin(),materialKeys.end());std::sort(unlockKeys.begin(),unlockKeys.end());
    materialFingerprint=Fingerprint(materialKeys);unlockFingerprint=Fingerprint(unlockKeys);
    const auto perks=std::count_if(catalog.begin(),catalog.end(),[](const auto&e){return e.kind==EntryKind::Perk;});
    const auto spells=std::count_if(catalog.begin(),catalog.end(),[](const auto&e){return e.kind==EntryKind::Spell;});
    materialCatalogEntries=materialKeys.size();perkCatalogEntries=perks;spellCatalogEntries=spells;
    materialCompatibility=PlayerActivitySourceCompatibility(materialQueryFound,materialQueryValid,!materialKeys.empty());
    unlockCompatibility=PlayerActivitySourceCompatibility(unlockQueriesFound,unlockQueriesValid,perks>0&&spells>0);
    LogMessage("Player activity discovery catalog resolved "+std::to_string(materialKeys.size())+" first-pickup materials, "+std::to_string(perks)+" perks, and "+std::to_string(spells)+" spells; material compatibility "+std::to_string(materialCompatibility.load())+", unlock compatibility "+std::to_string(unlockCompatibility.load()));
    return PlayerActivityDiscoveryReady(materialCompatibility.load(),unlockCompatibility.load());
}
void UpdatePlayerActivityDiscoveryTargets(const std::vector<PlayerInfo>&players){std::vector<Target> fresh;for(const auto&p:players){if(p.characterGuid.empty()||!p.controllerPtr)continue;Target t;t.guid=p.characterGuid;t.controller=p.controllerPtr;t.pawn=p.pawnPtr;t.state=p.playerStatePtr;t.progress=ProgressFor(t);t.perks=PerksFor(t);t.regionsComplete=ReadRegions(t,t.regionBitmap,t.regions);regionCompatibility=t.regionsComplete?1:-1;if(!t.regionsComplete)++dropped;fresh.push_back(std::move(t));}Lock lock(targetMutex);targets=std::move(fresh);}
void ProcessPlayerActivityDiscoveryTracking(){if(!g_Engine||!processEvent||catalog.empty())return;if(active&&!Online(active->target))active.reset();if(!active){std::vector<Target> current;{Lock lock(targetMutex);current=targets;}if(current.empty())return;static size_t targetCursor=0;if(targetCursor>=current.size())targetCursor=0;active=std::make_unique<Scan>();active->target=current[targetCursor++];active->materialsComplete=bool(active->target.progress)&&materialCompatibility.load()==1;active->unlocksComplete=bool(active->target.progress)&&bool(active->target.perks)&&unlockCompatibility.load()==1;active->materials.reserve(256);active->unlocks.reserve(128);}const auto started=std::chrono::steady_clock::now();int queries=0;while(active&&active->cursor<catalog.size()&&queries<8){const auto&e=catalog[active->cursor++];++queries;bool unlocked=false;if(e.kind==EntryKind::Material){if(active->materialsComplete){if(!g_Engine->IsObjectLive(active->target.progress)||!g_Engine->IsA(active->target.progress,"ProgressComponent")||!g_Engine->IsObjectLive(e.object))active->materialsComplete=false;else unlocked=Query(active->target.progress,hasPickedUpItem,e.object);if(unlocked)active->materials.push_back({e.key,e.name,e.icon});}}else if(active->unlocksComplete){uintptr_t component=e.kind==EntryKind::Perk?active->target.perks:active->target.progress;uintptr_t function=e.kind==EntryKind::Perk?isPerkUnlocked:isSpellUnlocked;const char*componentType=e.kind==EntryKind::Perk?"SkillPerkComponent":"ProgressComponent";if(!g_Engine->IsObjectLive(component)||!g_Engine->IsA(component,componentType)||!g_Engine->IsObjectLive(e.object))active->unlocksComplete=false;else unlocked=Query(component,function,e.object);if(unlocked)active->unlocks.push_back({e.kind==EntryKind::Perk?"perk":"spell",e.key,e.name,e.icon});}if(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-started).count()>=250)break;}if(active&&active->cursor==catalog.size()){PlayerActivityDiscoverySnapshot snapshot;snapshot.characterGuid=active->target.guid;snapshot.regionsComplete=active->target.regionsComplete;snapshot.regionBitmap=active->target.regionBitmap;snapshot.regions=active->target.regions;snapshot.materialsComplete=active->materialsComplete;snapshot.materials=std::move(active->materials);snapshot.materialFingerprint=materialFingerprint;snapshot.unlocksComplete=active->unlocksComplete;snapshot.unlocks=std::move(active->unlocks);snapshot.unlockFingerprint=unlockFingerprint;snapshot.capturedAt=std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();if(!snapshot.materialsComplete||!snapshot.unlocksComplete)++dropped;{Lock lock(snapshotMutex);completedSnapshots[snapshot.characterGuid]=std::move(snapshot);}++snapshots;active.reset();}}
std::vector<PlayerActivityDiscoverySnapshot> ReadPlayerActivityDiscoveries(){Lock lock(snapshotMutex);std::vector<PlayerActivityDiscoverySnapshot> out;out.reserve(completedSnapshots.size());for(const auto&pair:completedSnapshots)out.push_back(pair.second);return out;}
PlayerActivityDiscoveryDiagnostics GetPlayerActivityDiscoveryDiagnostics(){return {regionCompatibility.load(),materialCompatibility.load(),unlockCompatibility.load(),materialCatalogEntries.load(),perkCatalogEntries.load(),spellCatalogEntries.load(),snapshots.load(),dropped.load()};}
}
