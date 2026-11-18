#include "dom_engine.h"
#include "../status.h"
#include "../utils/logger.h"
#include "../utils/memory.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cctype>
#include <cstring>
#include <deque>
#include <fstream>
#include <iomanip>
#include <memory>
#include <pthread.h>
#include <set>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

namespace DomEngine { namespace {
enum class Kind { Material, Perk, Spell, Recipe, Building, Quest, Region };
struct QuestObjective { std::string key,text;std::uint32_t comparisonIndex=0;bool resolved=false; };
struct Entry { Kind kind;std::string key,name,icon,related;int target=0;uintptr_t object=0;std::uint64_t identity=0;std::string objectKey;std::vector<QuestObjective> objectives; };
struct Target { std::string guid;uintptr_t controller=0,pawn=0,state=0,progress=0,perks=0,regions=0,skills=0,quests=0;std::uint64_t controllerIdentity=0;std::vector<SkillInfo> skillSnapshot; };
enum class RequestState { Queued, Running, Completed, Failed };
struct Request {
    std::string id,session,guid,family,phase,key,related,operation,message;
    int target=0,attempts=0,initialLevel=-1,lastLevel=-1,stage=0;
    std::chrono::steady_clock::time_point notBefore{},deadline{};
    bool terminalAcknowledged=false;
    std::atomic<RequestState> state{RequestState::Queued};
    std::atomic<std::uint64_t> updated{0};
    pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;
};
using ProcessEventFn=void(*)(void*,void*,void*);
std::vector<Entry> entries;std::vector<Target> targets;std::deque<std::shared_ptr<Request>> queue;
std::unordered_map<std::string,std::shared_ptr<Request>> requests;
pthread_mutex_t targetMutex=PTHREAD_MUTEX_INITIALIZER,requestMutex=PTHREAD_MUTEX_INITIALIZER,entryMutex=PTHREAD_MUTEX_INITIALIZER;
struct Lock{pthread_mutex_t*p;explicit Lock(pthread_mutex_t&v):p(&v){pthread_mutex_lock(p);}~Lock(){pthread_mutex_unlock(p);}};
ProcessEventFn processEvent=nullptr;
uintptr_t enableCheats=0,resetProgress=0,resetQuests=0,giveQuest=0,completeQuest=0,addItem=0,hurtMe=0,revive=0;
uintptr_t unlockRecipes=0,unlockBuildings=0,setQuestObjective=0,revealMapFog=0,onRepRegions=0,getRevealedMapFogComponent=0,savePlayerState=0;
uintptr_t flushNetDormancy=0,forceNetUpdate=0;
uintptr_t markPropertyDirty=0,netPushModelHelpers=0;
uintptr_t nativeSkillReset=0,nativeSkillXp=0,nativeRegionQuery=0;
std::size_t revealControllerOffset=0,revealRegionOffset=8,onRepPreviousOffset=0;
std::uint32_t regionBitmapNameIndex=0;
std::atomic<int> compatibility{0},cheatCompatibility{0},persistenceCompatibility{0},regionCompatibility{0};
std::atomic<int> regionReplicationCompatibility{0};
std::atomic<int> regionPropertyDirtyCompatibility{0};
std::atomic<std::uint64_t> queued{0},completed{0},failed{0},stale{0},maximumMicros{0};
std::atomic<std::uint64_t> regionReplicationWakeCalls{0},regionReplicationWakeFailures{0};
std::atomic<std::uint64_t> regionPropertyDirtyCalls{0},regionPropertyDirtyFailures{0};
std::atomic<std::uint64_t> resolvedEntries{0};
std::atomic<std::uint64_t> regionEntries{0},resolvedRegionEntries{0};
std::atomic<std::uint64_t> counter{0};
std::chrono::steady_clock::time_point nextCatalogRefresh{};

std::uint64_t Now(){return std::uint64_t(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count());}
void Maximum(std::uint64_t value){auto old=maximumMicros.load();while(value>old&&!maximumMicros.compare_exchange_weak(old,value)){} }
std::vector<std::string> Split(const std::string&v,char sep){std::vector<std::string> out;size_t at=0;for(;;){auto end=v.find(sep,at);out.push_back(v.substr(at,end==std::string::npos?end:end-at));if(end==std::string::npos)return out;at=end+1;}}
std::string Escape(const std::string&v){std::ostringstream o;for(unsigned char c:v){if(c=='\\'||c=='\"')o<<'\\'<<char(c);else if(c=='\n')o<<"\\n";else if(c=='\r')o<<"\\r";else if(c=='\t')o<<"\\t";else if(c<0x20)o<<"\\u00"<<std::hex<<std::setw(2)<<std::setfill('0')<<int(c)<<std::dec;else o<<char(c);}return o.str();}
std::string RequestId(){const auto n=++counter;const auto t=std::chrono::steady_clock::now().time_since_epoch().count();std::ostringstream o;o<<std::hex<<std::setw(16)<<std::setfill('0')<<std::hash<std::string>{}(std::to_string(t)+":"+std::to_string(n))<<std::setw(16)<<std::setfill('0')<<std::hash<std::string>{}(std::to_string(n)+":"+std::to_string(t));return o.str();}
bool Safe(const std::string&v,size_t max=256){return !v.empty()&&v.size()<=max&&v.find_first_not_of("0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_-.:/")==std::string::npos;}
std::u16string Wide(const std::string&v){std::u16string out;out.reserve(v.size()+1);for(unsigned char c:v)out.push_back(char16_t(c));out.push_back(0);return out;}
struct StringParam { std::u16string storage;FString value{};explicit StringParam(const std::string&s):storage(Wide(s)){value={storage.data(),int32_t(storage.size()),int32_t(storage.size())};} };

uintptr_t Component(const Target&t,const char*type,const std::initializer_list<const char*>properties){for(const char*name:properties){const int32_t offset=g_Engine->FindPropertyOffset(t.controller,name);if(offset>=0&&offset<=0x3000){const auto value=Mem::ReadPtr(t.controller+uintptr_t(offset));if(value&&g_Engine->IsObjectLive(value)&&g_Engine->IsA(value,type))return value;}}for(int32_t i=0;i<g_Engine->GetObjectCount();++i){auto value=g_Engine->GetObjectByIndex(i);if(!value||!g_Engine->IsObjectLive(value)||!g_Engine->IsA(value,type))continue;auto outer=value;for(int depth=0;depth<7&&outer;++depth){outer=Mem::ReadPtr(outer+Offsets::UObject_Outer);if(outer&&(outer==t.controller||outer==t.pawn||outer==t.state))return value;}}return 0;}
uintptr_t RegionComponent(const Target&t){if(getRevealedMapFogComponent){alignas(8)struct{void*returnValue;}params{};if(InvokeSharedProcessEvent(reinterpret_cast<void*>(t.controller),reinterpret_cast<void*>(getRevealedMapFogComponent),&params)||(processEvent?(processEvent(reinterpret_cast<void*>(t.controller),reinterpret_cast<void*>(getRevealedMapFogComponent),&params),true):false)){const auto value=reinterpret_cast<uintptr_t>(params.returnValue);if(value&&g_Engine->IsObjectLive(value)&&g_Engine->IsA(value,"RevealedMapFogComponent"))return value;}}return Component(t,"RevealedMapFogComponent",{"BP_RevealedMapFogComponent","RevealedMapFogComponent","RevealedMapFogComponentRef"});}
Target ResolveTarget(const PlayerInfo&p){Target t{};t.guid=p.characterGuid;t.controller=p.controllerPtr;t.pawn=p.pawnPtr;t.state=p.playerStatePtr;t.controllerIdentity=g_Engine->GetObjectIdentity(t.controller);t.progress=Component(t,"ProgressComponent",{"BP_Components_Progress","ProgressComponent"});t.perks=Component(t,"SkillPerkComponent",{"BP_Components_SkillPerk","BP_Components_SkillPerks","SkillPerkComponent"});t.regions=RegionComponent(t);t.quests=Component(t,"QuestProgressComponent",{"BP_Components_QuestProgress","QuestProgressComponent"});t.skills=Mem::ReadPtr(t.controller+Offsets::DomPlayerController_SkillComponent);if(!t.skills||!g_Engine->IsObjectLive(t.skills)||!g_Engine->IsA(t.skills,"SkillComponent")){t.skills=0;const int32_t skillOffset=g_Engine->FindPropertyOffset(t.controller,"SkillComponent");if(skillOffset>=0&&skillOffset<=0x3000)t.skills=Mem::ReadPtr(t.controller+uintptr_t(skillOffset));}if(!t.skills){const int32_t fallback=g_Engine->FindPropertyOffset(t.controller,"BP_Components_Skill");if(fallback>=0&&fallback<=0x3000)t.skills=Mem::ReadPtr(t.controller+uintptr_t(fallback));}t.skillSnapshot=p.skills;return t;}
bool LiveTarget(const std::string&guid,Target&out){Lock lock(targetMutex);auto found=std::find_if(targets.begin(),targets.end(),[&](const auto&t){return t.guid==guid;});if(found==targets.end())return false;out=*found;return out.controller&&g_Engine->IsObjectLive(out.controller)&&g_Engine->GetObjectIdentity(out.controller)==out.controllerIdentity;}
bool SnapshotTarget(const std::string&guid,Target&out){Lock lock(targetMutex);auto found=std::find_if(targets.begin(),targets.end(),[&](const auto&t){return t.guid==guid;});if(found==targets.end())return false;out=*found;return out.controller!=0;}
bool FindEntry(const std::string&family,const std::string&key,Entry&result){const auto kind=family=="material"?Kind::Material:family=="perk"?Kind::Perk:family=="spell"?Kind::Spell:family=="recipe"?Kind::Recipe:family=="building"?Kind::Building:family=="quest"?Kind::Quest:Kind::Region;Lock lock(entryMutex);auto found=std::find_if(entries.begin(),entries.end(),[&](const auto&e){return (e.kind==kind||(family=="recipe"&&e.kind==Kind::Building))&&e.key==key;});if(found==entries.end())return false;result=*found;return true;}
bool Layout(uintptr_t function,const std::vector<std::pair<std::string,int>>&expected){
    if(!function)return false;
    const auto fields=g_Engine->GetStructPropertyOffsets(function);
    if(fields.empty())return true;
    std::set<int32_t> offsets;
    for(const auto&field:fields)if(field.first!="ReturnValue"&&field.second>=0&&field.second<=256)offsets.insert(field.second);
    if(offsets.empty())return true;
    if(offsets.size()!=expected.size())return false;
    return std::all_of(expected.begin(),expected.end(),[&](const auto&want){return offsets.count(want.second)!=0;});
}
bool Call(uintptr_t object,uintptr_t function,void*params=nullptr){return object&&function&&g_Engine->IsObjectLive(object)&&(InvokeSharedProcessEvent(reinterpret_cast<void*>(object),reinterpret_cast<void*>(function),params)||(processEvent?(processEvent(reinterpret_cast<void*>(object),reinterpret_cast<void*>(function),params),true):false));}
bool WakeRegionReplication(const Target&t){
    ++regionReplicationWakeCalls;
    if(!t.controller||!flushNetDormancy||!forceNetUpdate||!g_Engine->IsObjectLive(t.controller)||
       g_Engine->GetObjectIdentity(t.controller)!=t.controllerIdentity||
       !Call(t.controller,flushNetDormancy)||!Call(t.controller,forceNetUpdate)){
        ++regionReplicationWakeFailures;
        return false;
    }
    return true;
}
bool MarkRegionBitmapDirty(const Target&t){
    ++regionPropertyDirtyCalls;
    if(!t.regions||!markPropertyDirty||!netPushModelHelpers||!regionBitmapNameIndex||
       !g_Engine->IsObjectLive(t.regions)||!g_Engine->IsA(t.regions,"RevealedMapFogComponent")||
       !g_Engine->IsObjectLive(netPushModelHelpers)){
        ++regionPropertyDirtyFailures;
        return false;
    }
    alignas(8) struct{void*object;std::uint32_t comparisonIndex;std::uint32_t number;}params{
        reinterpret_cast<void*>(t.regions),regionBitmapNameIndex,0};
    if(!Call(netPushModelHelpers,markPropertyDirty,&params)){
        ++regionPropertyDirtyFailures;
        return false;
    }
    return true;
}
bool EntryLive(const Entry&e,const char*type){return e.object&&e.identity&&g_Engine->IsObjectLive(e.object)&&g_Engine->GetObjectIdentity(e.object)==e.identity&&g_Engine->IsA(e.object,type);}
uintptr_t OwnedCheatManager(const Target&t){for(int32_t i=0;i<g_Engine->GetObjectCount();++i){auto candidate=g_Engine->GetObjectByIndex(i);if(!candidate||!g_Engine->IsObjectLive(candidate)||!g_Engine->IsA(candidate,"DominionCheatManager"))continue;auto outer=candidate;for(int depth=0;depth<6&&outer;++depth){outer=Mem::ReadPtr(outer+Offsets::UObject_Outer);if(outer==t.controller)return candidate;if(outer==candidate)break;}}return 0;}
bool ClassIsA(uintptr_t type,const char*name){for(int depth=0;type&&depth<32;++depth){if(g_Engine->GetObjectName(type)==name)return true;const auto next=Mem::ReadPtr(type+Offsets::UStruct_SuperStruct);if(next==type)break;type=next;}return false;}
uintptr_t DominionCheatClass(){for(int32_t i=0;i<g_Engine->GetObjectCount();++i){const auto candidate=g_Engine->GetObjectByIndex(i);if(!candidate||!g_Engine->IsObjectLive(candidate)||g_Engine->GetObjectClassName(candidate)!="Class"||g_Engine->GetObjectName(candidate)!="DominionCheatManager")continue;auto type=candidate;for(int depth=0;type&&depth<16;++depth){if(g_Engine->GetObjectName(type)=="CheatManager")return candidate;const auto next=Mem::ReadPtr(type+Offsets::UStruct_SuperStruct);if(next==type)break;type=next;}}return 0;}
bool CheatPropertyOffsets(const Target&t,uintptr_t desiredClass,int32_t&managerOffset,int32_t&classOffset){
    managerOffset=g_Engine->FindPropertyOffset(t.controller,"CheatManager");
    classOffset=g_Engine->FindPropertyOffset(t.controller,"CheatClass");
    if(managerOffset>=0&&managerOffset<=0x1000&&classOffset>=0&&classOffset<=0x1000)return true;
    // Linux shipping keeps these two native APlayerController members but can
    // strip their FProperty records, including neighboring base-class fields.
    // The analyzer proves the constructor writes both exact slots. At runtime,
    // require an actual PlayerController plus a live configured CheatManager
    // UClass in the second slot before trusting the adjacent manager slot.
    if(!g_Engine->IsA(t.controller,"PlayerController")||
       !Mem::Readable(reinterpret_cast<void*>(t.controller+0x428),24)||
       !desiredClass||!g_Engine->IsObjectLive(desiredClass)||!ClassIsA(desiredClass,"CheatManager"))return false;
    const auto manager=Mem::ReadPtr(t.controller+0x428),configuredClass=Mem::ReadPtr(t.controller+0x430);
    if((manager&&(!g_Engine->IsObjectLive(manager)||!g_Engine->IsA(manager,"CheatManager")))||
       !configuredClass||!g_Engine->IsObjectLive(configuredClass)||!ClassIsA(configuredClass,"CheatManager"))return false;
    managerOffset=0x428;classOffset=0x430;return true;
}
uintptr_t ConstructCheatManager(const Target&t,uintptr_t desiredClass,int32_t managerOffset){
    // The Linux shipping build compiles APlayerController::AddCheats down to a
    // no-op. Reproduce Unreal's normal AddCheats construction sequence with
    // the exact analyzed engine constructor APIs, then retain the manager in
    // the controller's native strong-reference slot before initialization.
    constexpr uintptr_t ParamsConstructorOffset=0x04EB6B10;
    constexpr uintptr_t ConstructObjectOffset=0x04EB37A0;
    constexpr uintptr_t InitCheatManagerOffset=0x07CCF370;
    const auto module=g_Engine->GetModuleBase();
    const auto paramsConstructor=module+ParamsConstructorOffset;
    const auto constructObject=module+ConstructObjectOffset;
    const auto initCheatManager=module+InitCheatManagerOffset;
    if(!Mem::InImage(reinterpret_cast<void*>(paramsConstructor))||
       !Mem::InImage(reinterpret_cast<void*>(constructObject))||
       !Mem::InImage(reinterpret_cast<void*>(initCheatManager)))return 0;
    alignas(16) unsigned char params[0x80]{};
    using ParamsConstructorFn=void(*)(void*,const void*);
    using ConstructObjectFn=void*(*)(const void*);
    using InitCheatManagerFn=void(*)(void*);
    reinterpret_cast<ParamsConstructorFn>(paramsConstructor)(params,reinterpret_cast<const void*>(desiredClass));
    std::memcpy(params+8,&t.controller,sizeof(t.controller));
    const auto created=reinterpret_cast<uintptr_t>(reinterpret_cast<ConstructObjectFn>(constructObject)(params));
    if(!created||!g_Engine->IsObjectLive(created)||!g_Engine->IsA(created,"DominionCheatManager")||
       Mem::ReadPtr(created+Offsets::UObject_Outer)!=t.controller)return 0;
    const auto property=t.controller+uintptr_t(managerOffset);
    if(!Mem::SafeWrite(property,created)||Mem::ReadPtr(property)!=created)return 0;
    reinterpret_cast<InitCheatManagerFn>(initCheatManager)(reinterpret_cast<void*>(created));
    if(g_Engine->IsObjectLive(created)&&g_Engine->IsA(created,"DominionCheatManager")&&Mem::ReadPtr(property)==created)return created;
    if(Mem::ReadPtr(property)==created)Mem::SafeWrite(property,uintptr_t(0));
    return 0;
}
uintptr_t CheatManager(Target&t){
    const auto desiredClass=DominionCheatClass();
    int32_t managerOffset=-1,classOffset=-1;
    const bool offsetsValid=CheatPropertyOffsets(t,desiredClass,managerOffset,classOffset);
    auto readProperty=[&](){if(managerOffset<0||managerOffset>0x1000)return uintptr_t(0);const auto value=Mem::ReadPtr(t.controller+uintptr_t(managerOffset));return value&&g_Engine->IsObjectLive(value)&&g_Engine->IsA(value,"DominionCheatManager")?value:uintptr_t(0);};
    if(auto cheat=readProperty())return cheat;
    if(auto cheat=OwnedCheatManager(t))return cheat;
    if(!offsetsValid){LogMessage("Admin activity cheat manager creation failed: validated controller cheat layout unavailable");return 0;}
    const auto existing=Mem::ReadPtr(t.controller+uintptr_t(managerOffset));
    if(existing){LogMessage("Admin activity cheat manager creation refused: another live manager is already assigned");return 0;}
    Call(t.controller,enableCheats);
    if(auto cheat=readProperty())return cheat;
    if(auto cheat=OwnedCheatManager(t))return cheat;
    const auto configuredClass=Mem::ReadPtr(t.controller+uintptr_t(classOffset));
    const auto constructionClass=configuredClass&&g_Engine->IsObjectLive(configuredClass)&&ClassIsA(configuredClass,"DominionCheatManager")?configuredClass:desiredClass;
    if(auto cheat=ConstructCheatManager(t,constructionClass,managerOffset))return cheat;
    LogMessage("Admin activity cheat manager creation failed: engine object construction rejected");return 0;
}
bool CallString(uintptr_t object,uintptr_t function,const std::string&value){StringParam input(value);return Call(object,function,&input.value);}
bool CallAddItem(uintptr_t cheat,const std::string&key){alignas(8) unsigned char params[64]{};StringParam input(key);const int32_t count=1,durability=100;std::memcpy(params,&input.value,sizeof(input.value));std::memcpy(params+16,&count,sizeof(count));std::memcpy(params+20,&durability,sizeof(durability));return Call(cheat,addItem,params);}
int SkillLevel(const Target&t,const std::string&key){if(!t.skills||!g_Engine->IsObjectLive(t.skills)||!g_Engine->IsA(t.skills,"SkillComponent"))return -1;FUEArray list{};if(!Mem::Read(t.skills+Offsets::Skill_Skills,list)||list.Num<0||list.Num>128||list.Max<list.Num||!list.Data||!Mem::Readable(list.Data,size_t(list.Num)*Offsets::FSkill_Size))return -1;for(int32_t i=0;i<list.Num;++i){const auto at=reinterpret_cast<uintptr_t>(list.Data)+uintptr_t(i)*Offsets::FSkill_Size;const int xp=Mem::ReadI32(at+Offsets::FSkill_CurrentXP);const auto data=Mem::ReadPtr(at+Offsets::FSkill_SkillData);if(data&&g_Engine->IsObjectLive(data)&&g_Engine->GetObjectName(data)==key)return Status::SkillLevel(xp);}return -1;}
bool SkillRecord(const Target&t,const std::string&key,int&xp,uintptr_t&data){if(!t.skills||!g_Engine->IsObjectLive(t.skills)||!g_Engine->IsA(t.skills,"SkillComponent"))return false;FUEArray list{};if(!Mem::Read(t.skills+Offsets::Skill_Skills,list)||list.Num<0||list.Num>128||list.Max<list.Num||!list.Data||!Mem::Readable(list.Data,size_t(list.Num)*Offsets::FSkill_Size))return false;for(int32_t i=0;i<list.Num;++i){const auto at=reinterpret_cast<uintptr_t>(list.Data)+uintptr_t(i)*Offsets::FSkill_Size;const auto candidate=Mem::ReadPtr(at+Offsets::FSkill_SkillData);if(candidate&&g_Engine->IsObjectLive(candidate)&&g_Engine->GetObjectName(candidate)==key){xp=Mem::ReadI32(at+Offsets::FSkill_CurrentXP);data=candidate;return true;}}return false;}
bool QueueSkillXp(Target&t,const std::string&key,int target,std::string&error){int current=0;uintptr_t data=0;if(!nativeSkillXp||!SkillRecord(t,key,current,data)){error="The authoritative skill XP path is unavailable";return false;}const int wanted=Status::SkillXpForLevel(target);if(wanted<0||wanted<current){error="The requested skill XP target is invalid";return false;}if(wanted==current)return true;uintptr_t wrapped=data;using QueueFn=void(*)(void*,const uintptr_t*,int);reinterpret_cast<QueueFn>(nativeSkillXp)(reinterpret_cast<void*>(t.skills),&wrapped,wanted-current);return true;}
bool ResetSkillNative(Target&t,const std::string&key,std::string&error){
    if(!nativeSkillReset){error="The reviewed native skill reset is unavailable";return false;}
    if(!t.skills||!g_Engine->IsObjectLive(t.skills)||!g_Engine->IsA(t.skills,"SkillComponent")){error="The authoritative skill component is unavailable";return false;}
    const auto owner=Mem::ReadPtr(t.skills+0xa8);if(!owner||!g_Engine->IsObjectLive(owner)){error="The authoritative skill-component owner is unavailable";return false;}
    const auto role=Mem::ReadU8(owner+0x170);if(role!=3){error="The selected skill component is not authoritative";return false;}
    FUEArray list{};
    if(!Mem::Read(t.skills+Offsets::Skill_Skills,list)||list.Num<0||list.Num>128||list.Max<list.Num||!list.Data||!Mem::Readable(list.Data,size_t(list.Num)*Offsets::FSkill_Size)){error="The authoritative skill array layout is unavailable";return false;}
    for(int32_t i=0;i<list.Num;++i){const auto at=reinterpret_cast<uintptr_t>(list.Data)+uintptr_t(i)*Offsets::FSkill_Size;const auto data=Mem::ReadPtr(at+Offsets::FSkill_SkillData);if(!data||!g_Engine->IsObjectLive(data)||g_Engine->GetObjectName(data)!=key)continue;using ResetFn=void(*)(void*,const void*);reinterpret_cast<ResetFn>(nativeSkillReset)(reinterpret_cast<void*>(t.skills),reinterpret_cast<const void*>(data));if(SkillLevel(t,key)==1)return true;error="The game did not change the authoritative skill level";return false;}
    error="The selected skill record is unavailable";return false;
}
bool Save(Target&t){if(!savePlayerState)return false;uintptr_t subsystem=0;for(int32_t i=0;i<g_Engine->GetObjectCount();++i){auto candidate=g_Engine->GetObjectByIndex(i);if(!candidate||!g_Engine->IsObjectLive(candidate)||!g_Engine->IsA(candidate,"PersistenceSubsystem")||g_Engine->GetObjectName(candidate).rfind("Default__",0)==0)continue;auto outer=candidate;bool ownedByGameInstance=false;for(int depth=0;depth<8&&outer;++depth){outer=Mem::ReadPtr(outer+Offsets::UObject_Outer);if(outer&&g_Engine->IsObjectLive(outer)&&g_Engine->IsA(outer,"GameInstance")){ownedByGameInstance=true;break;}}if(ownedByGameInstance){subsystem=candidate;break;}}if(!subsystem)return false;struct{void*controller;}params{reinterpret_cast<void*>(t.controller)};return Call(subsystem,savePlayerState,&params);}
void Finish(Request&r,bool ok,const std::string&message){r.state.store(ok?RequestState::Completed:RequestState::Failed);r.message=message;r.updated.store(Now());if(ok)++completed;else{++failed;LogMessage("Admin activity test request failed: "+message);}}
bool DirectUnlock(Target&t,const Entry&e){uintptr_t function=e.kind==Kind::Building?unlockBuildings:unlockRecipes,component=t.progress;const char*type=e.kind==Kind::Building?"BuildingPieceData":"RecipeData";if(!component||!g_Engine->IsObjectLive(component)||!g_Engine->IsA(component,"ProgressComponent")||!function||!EntryLive(e,type))return false;void*item=reinterpret_cast<void*>(e.object);FUEArray array{&item,1,1};return Call(component,function,&array);}
bool ResetRegion(Target&t,const Entry&e,std::string&error){
    if(!t.regions||!g_Engine->IsObjectLive(t.regions)||!g_Engine->IsA(t.regions,"RevealedMapFogComponent")){error="The authoritative revealed-region component is unavailable";return false;}
    if(!nativeRegionQuery){error="The authoritative revealed-region bitmap layout is unavailable";return false;}
    if(e.target<0||e.target>=32){error="The selected region identifier is invalid";return false;}
    const int32_t reflected=g_Engine->FindPropertyOffset(t.regions,"RevealedMapRegionsBitmap");
    if(reflected>=0&&uintptr_t(reflected)!=Offsets::RevealedMapFog_RevealedMapRegionsBitmap){error="The reflected revealed-region bitmap conflicts with the reviewed native layout";return false;}
    const auto address=t.regions+Offsets::RevealedMapFog_RevealedMapRegionsBitmap;
    std::uint32_t prior=0;if(!Mem::SafeRead(address,prior)){error="The authoritative revealed-region bitmap could not be read";return false;}
    using QueryFn=bool(*)(const void*,unsigned int);const auto query=reinterpret_cast<QueryFn>(nativeRegionQuery);
    const auto mask=std::uint32_t(1)<<unsigned(e.target);
    if(query(reinterpret_cast<const void*>(t.regions),unsigned(e.target))!=bool(prior&mask)){error="The authoritative revealed-region bitmap failed behavioral validation";return false;}
    const auto next=prior&~mask;if(next==prior){if(MarkRegionBitmapDirty(t)&&WakeRegionReplication(t))return true;error="The revealed-region client refresh could not be queued";return false;}
    if(!Mem::SafeWrite(address,next)){error="The game rejected the revealed-region bitmap reset";return false;}
    auto replicate=[&](std::uint32_t previous){alignas(8) unsigned char params[16]{};std::memcpy(params+onRepPreviousOffset,&previous,sizeof(previous));return Call(t.regions,onRepRegions,params);};
    if(!replicate(prior)){Mem::SafeWrite(address,prior);error="The game rejected the revealed-region replication update";return false;}
    std::uint32_t readback=0;if(!Mem::SafeRead(address,readback)||readback!=next||query(reinterpret_cast<const void*>(t.regions),unsigned(e.target))){Mem::SafeWrite(address,prior);replicate(next);error="The game did not retain the revealed-region bitmap reset";return false;}
    if(!MarkRegionBitmapDirty(t)||!WakeRegionReplication(t)){Mem::SafeWrite(address,prior);replicate(next);MarkRegionBitmapDirty(t);WakeRegionReplication(t);error="The revealed-region client refresh could not be queued";return false;}
    return true;
}
uintptr_t LiveDefaultObject(const char*className){
    const auto cached=g_Engine->FindObject(className,true);
    if(cached&&g_Engine->IsObjectLive(cached))return cached;
    for(int32_t i=0;i<g_Engine->GetObjectCount();++i){
        const auto candidate=g_Engine->GetObjectByIndex(i);
        if(candidate&&g_Engine->IsObjectLive(candidate)&&g_Engine->GetObjectClassName(candidate)==className&&
           g_Engine->GetObjectName(candidate).rfind("Default__",0)==0)return candidate;
    }
    return 0;
}
uintptr_t LiveRegionObject(Entry&e){
    if(EntryLive(e,"MapRegion"))return e.object;
    uintptr_t found=0;
    for(int32_t i=0;i<g_Engine->GetObjectCount();++i){
        const auto candidate=g_Engine->GetObjectByIndex(i);
        if(!candidate||!g_Engine->IsObjectLive(candidate)||!g_Engine->IsA(candidate,"MapRegion")||
           g_Engine->GetMapRegionObjectName(candidate)!=e.name)continue;
        found=candidate;break;
    }
    if(!found)return 0;
    e.object=found;e.identity=g_Engine->GetObjectIdentity(found);
    Lock lock(entryMutex);
    auto cached=std::find_if(entries.begin(),entries.end(),[&](const auto&item){return item.kind==Kind::Region&&item.key==e.key;});
    if(cached!=entries.end()){cached->object=e.object;cached->identity=e.identity;}
    return e.object;
}
bool RevealRegion(Target&t,Entry&e,std::string&error){
    if(!revealMapFog){error="The authoritative region reveal function is unavailable";return false;}
    const auto region=LiveRegionObject(e);
    if(!region){error="The selected live region object is unavailable";return false;}
    const auto library=LiveDefaultObject("DominionRuntimeBlueprintLibrary");
    if(!library){error="The authoritative region reveal library is unavailable";return false;}
    alignas(8) unsigned char params[64]{};void*controller=reinterpret_cast<void*>(t.controller),*regionParam=reinterpret_cast<void*>(region);
    std::memcpy(params+revealControllerOffset,&controller,sizeof(controller));std::memcpy(params+revealRegionOffset,&regionParam,sizeof(regionParam));
    if(!Call(library,revealMapFog,params)){error="The game rejected the region reveal trigger";return false;}
    return true;
}
bool SetObjective(Target&t,const Entry&e,const QuestObjective&objective){if(!t.quests||!g_Engine->IsObjectLive(t.quests)||!g_Engine->IsA(t.quests,"QuestProgressComponent")||!setQuestObjective||!EntryLive(e,"QuestData")||!objective.resolved)return false;alignas(8) unsigned char params[64]{};void*quest=reinterpret_cast<void*>(e.object);std::memcpy(params,&quest,sizeof(quest));std::memcpy(params+8,&objective.comparisonIndex,sizeof(objective.comparisonIndex));return Call(t.quests,setQuestObjective,params);}
bool QuestSnapshotAt(const std::string&guid,const std::string&quest,const std::string&objective){for(const auto&snapshot:ReadPlayerQuestSnapshots()){if(snapshot.characterGuid!=guid||!snapshot.complete)continue;auto found=std::find_if(snapshot.quests.begin(),snapshot.quests.end(),[&](const auto&item){return item.id==quest;});return found!=snapshot.quests.end()&&found->status=="in_progress"&&found->objective==objective;}return false;}
void Process(std::shared_ptr<Request> request){auto& r=*request;Lock requestLock(r.mutex);Target target;if(!LiveTarget(r.guid,target)){Finish(r,false,"Target character went offline");++stale;return;}Entry entry{};const bool hasEntry=FindEntry(r.family,r.key,entry);if(r.family!="skill"&&r.family!="death"&&!hasEntry){Finish(r,false,"Catalog entry is no longer available");return;}const bool skillOperation=r.family=="skill"||r.family=="perk"||r.family=="spell";const bool nativeSkillOperation=skillOperation&&(r.phase=="prepare"||r.phase=="trigger");const auto cheat=nativeSkillOperation?uintptr_t(0):CheatManager(target);if(!nativeSkillOperation&&!cheat){Finish(r,false,"The target cheat manager is unavailable");return;}r.state.store(RequestState::Running);r.updated.store(Now());
    if(r.phase=="prepare"){
        bool ok=false;
        std::string preparationError;if(skillOperation)ok=ResetSkillNative(target,r.related,preparationError);
        else if(r.family=="recipe"||r.family=="material")ok=Call(cheat,resetProgress);
        else if(r.family=="quest")ok=Call(cheat,resetQuests);
        else if(r.family=="region")ok=ResetRegion(target,entry,preparationError);
        else if(r.family=="death")ok=Call(cheat,revive);
        const bool saved=ok&&Save(target);Finish(r,ok&&saved,ok?(saved?"Preparation completed; waiting for authoritative activity scans":"Game reset completed but the character save was not accepted"):(preparationError.empty()?"The game rejected the preparation":preparationError));return;
    }
    if(r.family=="skill"||r.family=="perk"||r.family=="spell"){
        const auto now=std::chrono::steady_clock::now();const int level=SkillLevel(target,r.related);if(level<0){Finish(r,false,"Skill state is unavailable");return;}if(r.initialLevel<0)r.initialLevel=level;r.lastLevel=level;if(level>=r.target&&(r.family=="skill"||r.attempts>0)){const bool saved=Save(target);Finish(r,saved,saved?"Target level reached through the game XP path":"Target level reached but the character save was not accepted");return;}if(r.attempts==0){std::string xpError;if(!QueueSkillXp(target,r.related,r.target,xpError)){Finish(r,false,xpError);return;}r.attempts=1;r.deadline=now+std::chrono::seconds(15);r.notBefore=now+std::chrono::milliseconds(100);{Lock lock(requestMutex);queue.push_back(request);}return;}if(now>=r.deadline){Finish(r,false,"The native XP trigger did not reach the requested level before the safety timeout");return;}if(now<r.notBefore){Lock lock(requestMutex);queue.push_back(request);return;}r.notBefore=now+std::chrono::milliseconds(100);{Lock lock(requestMutex);queue.push_back(request);}return;
    }
    bool ok=false;std::string triggerError;
    if(r.family=="quest"&&r.operation=="progress"){
        if(entry.objectives.size()<2){Finish(r,false,"This quest does not expose two safe objective states");return;}
        const auto now=std::chrono::steady_clock::now();
        if(r.stage==0){if(!CallString(cheat,giveQuest,entry.related)){Finish(r,false,"The game rejected the quest start");return;}r.stage=1;r.deadline=now+std::chrono::seconds(30);r.notBefore=now+std::chrono::milliseconds(500);{Lock lock(requestMutex);queue.push_back(request);}return;}
        if(now<r.notBefore){Lock lock(requestMutex);queue.push_back(request);return;}
        if(now>=r.deadline){Finish(r,false,"The quest start was not confirmed before the objective transition timeout");return;}
        if(!QuestSnapshotAt(r.guid,r.key,entry.objectives.front().text)){r.notBefore=now+std::chrono::milliseconds(500);Lock lock(requestMutex);queue.push_back(request);return;}
        ok=SetObjective(target,entry,entry.objectives[1]);
    }
    else if(r.family=="quest")ok=CallString(cheat,r.operation=="complete"?completeQuest:giveQuest,entry.related);
    else if(r.family=="material")ok=CallAddItem(cheat,entry.key);
    else if(r.family=="recipe")ok=DirectUnlock(target,entry);
    else if(r.family=="region")ok=RevealRegion(target,entry,triggerError);
    else if(r.family=="death"){struct{int32_t amount;}params{1000000};ok=Call(cheat,hurtMe,&params);Finish(r,ok,ok?"The game death path accepted the trigger; waiting for the authoritative activity scan":"The game rejected the death trigger");return;}
    const bool saved=ok&&Save(target);Finish(r,ok&&saved,ok?(saved?"Game trigger completed and saved":"Game trigger completed but the character save was not accepted"):(triggerError.empty()?"The game rejected the trigger":triggerError));
}
bool LoadCatalog(const std::string&materials,const std::string&unlocks,const std::string&recipes,const std::string&quests){
    entries.clear();std::string line;
    {std::ifstream in(materials);while(std::getline(in,line)){if(line.empty()||line[0]=='#')continue;auto f=Split(line,'\t');if(f.size()==3){Entry e{};e.kind=Kind::Material;e.key=f[0];e.name=f[1];e.icon=f[2];e.objectKey=f[0];entries.push_back(std::move(e));}}}
    {std::ifstream in(unlocks);while(std::getline(in,line)){if(line.empty()||line[0]=='#')continue;auto f=Split(line,'\t');if(f.size()!=8)continue;int level=0;try{level=std::stoi(f[7]);}catch(...){continue;}Entry e{};e.kind=f[1]=="spell"?Kind::Spell:Kind::Perk;e.key=f[2];e.name=f[3];e.icon=f[4];e.related=f[6];e.target=level;e.objectKey=f[0];entries.push_back(std::move(e));}}
    {std::ifstream in(recipes);while(std::getline(in,line)){if(line.empty()||line[0]=='#')continue;auto f=Split(line,'\t');if(f.size()==3){Entry e{};e.kind=f[1]=="building"?Kind::Building:Kind::Recipe;e.key=f[0];e.name=f[2];e.related=f[2];e.objectKey=f[2];entries.push_back(std::move(e));}}}
    {std::ifstream in(quests);while(std::getline(in,line)){if(line.empty()||line[0]=='#')continue;auto f=Split(line,'\t');if(f.size()==3){Entry e{};e.kind=Kind::Quest;e.key=f[0];e.name=f[1];e.related=f[2];e.objectKey=f[2];entries.push_back(std::move(e));}else if(f.size()==4&&f[0]=="O"){auto found=std::find_if(entries.begin(),entries.end(),[&](const auto&e){return e.kind==Kind::Quest&&e.key==f[1];});if(found!=entries.end()&&!f[2].empty()&&!f[3].empty())found->objectives.push_back({f[2],f[3],0});}}}
    static const char*names[]={"Temple Woods","Bramblemead Valley","Whispering Swamp","Fractured Plains","Stormtouched Highlands","Bloodblight Swamp","Bleakfields Valley","Hope's Fall","Lake of Lost Souls","Coalridge Pass","Silverthorn Keep","Dragons' Run","Witchwillow Range","Emberwood","Forgotten Temple","The Approach","The Library","The Courtyard","The Menagerie","The Garrison","The Nexus","The Pastures","The Cathedral","The Bastion","The Grand Hall","Alcarrid Oasis","Dunes of Uzzer","Manafem Plains","The Burning Spire","Rasthin's Rest","Scorned Wilderness"};
    for(int i=1;i<=31;++i){Entry e{};e.kind=Kind::Region;e.key=std::to_string(i);e.name=names[i-1];e.icon="/assets/region-discovery-icon.png";e.target=i;entries.push_back(std::move(e));}
    return !entries.empty();
}
bool ResolveEntries(){
    Lock entryLock(entryMutex);
    std::unordered_map<std::string,uintptr_t> objects;
    for(int32_t i=0;i<g_Engine->GetObjectCount();++i){auto object=g_Engine->GetObjectByIndex(i);if(!object||!g_Engine->IsObjectLive(object))continue;if(g_Engine->IsA(object,"ItemData")||g_Engine->IsA(object,"SkillPerkData")||g_Engine->IsA(object,"RecipeData")||g_Engine->IsA(object,"BuildingPieceData")||g_Engine->IsA(object,"QuestData"))objects.emplace(g_Engine->GetObjectName(object),object);else if(g_Engine->IsA(object,"MapRegion")){const auto name=g_Engine->GetMapRegionObjectName(object);if(!name.empty())objects.emplace("region-name:"+name,object);}}
    std::size_t resolved=0,regions=0,resolvedRegions=0;
    for(auto&e:entries){const std::string lookup=e.kind==Kind::Region?"region-name:"+e.name:e.objectKey;auto found=objects.find(lookup);e.object=found==objects.end()?0:found->second;e.identity=e.object?g_Engine->GetObjectIdentity(e.object):0;if(e.kind==Kind::Region){++regions;if(e.object)++resolvedRegions;}if(e.object)++resolved;}
    std::unordered_map<std::string,std::uint32_t> wanted;wanted.emplace("RevealedMapRegionsBitmap",0);
    for(const auto&e:entries)for(const auto&o:e.objectives)wanted.emplace(o.key,0);
    if(!wanted.empty()){
        const auto pool=g_Engine->GetGNames();const int32_t currentBlock=Mem::ReadI32(pool+Offsets::NamePool_CurrentBlock),currentCursor=Mem::ReadI32(pool+Offsets::NamePool_CurrentByteCursor);
        if(currentBlock<0||currentBlock>=Offsets::NamePool_MaxBlocks||currentCursor<0)return false;
        for(int32_t blockIndex=0;blockIndex<=currentBlock;++blockIndex){const auto block=Mem::ReadPtr(pool+Offsets::NamePool_Blocks+uintptr_t(blockIndex)*sizeof(uintptr_t));if(!block)continue;const std::size_t limit=blockIndex==currentBlock?std::size_t(currentCursor):std::size_t(1u<<Offsets::NameBlockOffsetBits)*Offsets::NameEntryStride;for(std::size_t offset=0;offset+2<=limit;){std::uint16_t header=0;if(!Mem::Read(block+offset,header))break;const std::size_t length=header>>6;if(!length||length>1024){offset+=Offsets::NameEntryStride;continue;}const bool wide=(header&1)!=0;const std::size_t bytes=2+length*(wide?2:1),aligned=(bytes+Offsets::NameEntryStride-1)&~std::size_t(Offsets::NameEntryStride-1);const std::uint32_t index=(std::uint32_t(blockIndex)<<Offsets::NameBlockOffsetBits)|std::uint32_t(offset/Offsets::NameEntryStride);auto found=wanted.find(g_Engine->NameToString(index,0));if(found!=wanted.end())found->second=index;offset+=std::max<std::size_t>(aligned,Offsets::NameEntryStride);}}
        auto regionName=wanted.find("RevealedMapRegionsBitmap");regionBitmapNameIndex=regionName==wanted.end()?0:regionName->second;
        for(auto&e:entries)for(auto&o:e.objectives){auto found=wanted.find(o.key);if(found!=wanted.end()&&(found->second||o.key=="None")){o.comparisonIndex=found->second;o.resolved=true;}}
    }
    resolvedEntries=resolved;regionEntries=regions;resolvedRegionEntries=resolvedRegions;
    return std::all_of(entries.begin(),entries.end(),[](const auto&e){return e.kind==Kind::Region||e.object!=0;});
}
} // namespace

bool InitializeAdminActivityTesting(const std::string&materialCatalog,const std::string&unlockCatalog,
                                    const std::string&recipeCatalog,const std::string&questCatalog,
                                    uintptr_t moduleBase,uintptr_t moduleEnd){
    uintptr_t a=0,b=0,processOffset=0,c=0,d=0;
    if(!GetAdminTeleportOffsets(a,b,processOffset,c,d)||moduleBase+processOffset>=moduleEnd){compatibility=-1;return false;}
    processEvent=reinterpret_cast<ProcessEventFn>(moduleBase+processOffset);
    const unsigned char skillResetSignature[]={0x41,0x56,0x53,0x50,0x48,0x8b,0x87,0xa8,0x00,0x00,0x00,0x80,0xb8,0x70,0x01,0x00,0x00,0x03,0x75,0x67,0x48,0x63,0x87,0x90,0x01,0x00,0x00,0x48,0x85,0xc0,0x74,0x5b,0x48,0x8b,0x9f,0x88,0x01,0x00,0x00,0x48,0xc1,0xe0,0x04,0x0f,0x1f,0x44,0x00,0x00};
    for(const uintptr_t offset:{uintptr_t(0x0A72E2D0),uintptr_t(0x0A72E230)}){const auto candidate=moduleBase+offset;if(candidate<moduleBase||candidate+sizeof(skillResetSignature)>moduleEnd||!Mem::Readable(reinterpret_cast<void*>(candidate),sizeof(skillResetSignature))||std::memcmp(reinterpret_cast<void*>(candidate),skillResetSignature,sizeof(skillResetSignature)))continue;if(nativeSkillReset){nativeSkillReset=0;break;}nativeSkillReset=candidate;}
    const unsigned char skillXpSignature[]={0x55,0x41,0x57,0x41,0x56,0x41,0x54,0x53,0x48,0x83,0xec,0x50,0x89,0xd5,0x49,0x89,0xf7,0x48,0x89,0xfb,0x4c,0x8d,0xb7,0xb8,0x01,0x00,0x00,0x4c,0x8b,0x26};
    for(const uintptr_t offset:{uintptr_t(0x0A72EF80),uintptr_t(0x0A72EEE0)}){const auto candidate=moduleBase+offset;if(candidate<moduleBase||candidate+sizeof(skillXpSignature)>moduleEnd||!Mem::Readable(reinterpret_cast<void*>(candidate),sizeof(skillXpSignature))||std::memcmp(reinterpret_cast<void*>(candidate),skillXpSignature,sizeof(skillXpSignature)))continue;if(nativeSkillXp){nativeSkillXp=0;break;}nativeSkillXp=candidate;}
    const unsigned char regionQuerySignature[]={0x8b,0x87,0xe8,0x00,0x00,0x00,0x0f,0xa3,0xf0,0x0f,0x92,0xc0,0xc3};
    for(const uintptr_t offset:{uintptr_t(0x0A532AF0),uintptr_t(0x0A532A50)}){const auto candidate=moduleBase+offset;if(candidate<moduleBase||candidate+sizeof(regionQuerySignature)>moduleEnd||!Mem::Readable(reinterpret_cast<void*>(candidate),sizeof(regionQuerySignature))||std::memcmp(reinterpret_cast<void*>(candidate),regionQuerySignature,sizeof(regionQuerySignature)))continue;if(nativeRegionQuery){nativeRegionQuery=0;break;}nativeRegionQuery=candidate;}
    if(!LoadCatalog(materialCatalog,unlockCatalog,recipeCatalog,questCatalog)){compatibility=-2;return false;}
    enableCheats=g_Engine->FindFunction("PlayerController","EnableCheats");
    resetProgress=g_Engine->FindFunction("DominionCheatManager","domResetProgress");
    resetQuests=g_Engine->FindFunction("DominionCheatManager","domResetQuests");
    giveQuest=g_Engine->FindFunction("DominionCheatManager","domGiveQuest");
    completeQuest=g_Engine->FindFunction("DominionCheatManager","domCompleteQuest");
    addItem=g_Engine->FindFunction("DominionCheatManager","domAddItem");
    hurtMe=g_Engine->FindFunction("DominionCheatManager","domHurtMe");
    revive=g_Engine->FindFunction("DominionCheatManager","domRevive");
    unlockRecipes=g_Engine->FindFunction("ProgressComponent","UnlockRecipes");
    unlockBuildings=g_Engine->FindFunction("ProgressComponent","UnlockBuildings");
    setQuestObjective=g_Engine->FindFunction("QuestProgressComponent","SetQuestObjective");
    revealMapFog=g_Engine->FindFunction("DominionRuntimeBlueprintLibrary","RevealMapFog");
    onRepRegions=g_Engine->FindFunction("RevealedMapFogComponent","OnRep_RevealedMapRegionsBitmap");
    getRevealedMapFogComponent=g_Engine->FindFunction("DominionPlayerController","GetRevealedMapFogComponent");
    flushNetDormancy=g_Engine->FindFunction("Actor","FlushNetDormancy");
    forceNetUpdate=g_Engine->FindFunction("Actor","ForceNetUpdate");
    markPropertyDirty=g_Engine->FindFunction("NetPushModelHelpers","MarkPropertyDirty");
    netPushModelHelpers=g_Engine->FindObject("NetPushModelHelpers",true);
    savePlayerState=g_Engine->FindFunction("PersistenceSubsystem","SavePlayerState");
    const bool giveQuestLayout=Layout(giveQuest,{{"QuestName",0}});
    const bool completeQuestLayout=Layout(completeQuest,{{"QuestName",0}});
    const bool addItemLayout=Layout(addItem,{{"ItemName",0},{"Count",16},{"DurabilityPercentage",20}});
    const bool hurtLayout=Layout(hurtMe,{{"Amount",0}});
    const bool saveLayout=Layout(savePlayerState,{{"DomPC",0}});
    const bool revealLayout=Layout(revealMapFog,{{"PlayerController",0},{"Region",8}});
    const bool onRepLayout=Layout(onRepRegions,{{"PreviousBitmap",0}});
    const bool flushLayout=Layout(flushNetDormancy,{});
    const bool forceLayout=Layout(forceNetUpdate,{});
    const bool markDirtyLayout=Layout(markPropertyDirty,{{"Object",0},{"PropertyName",8}});
    const bool recipesLayout=Layout(unlockRecipes,{{"RecipeDatas",0}});
    const bool buildingsLayout=Layout(unlockBuildings,{{"BuildingDataRefs",0}});
    const bool objectiveLayout=Layout(setQuestObjective,{{"QuestData",0},{"ObjectiveName",8}});
    const bool cheats=nativeSkillReset&&nativeSkillXp&&enableCheats&&resetProgress&&resetQuests&&giveQuest&&
        completeQuest&&addItem&&hurtMe&&revive&&giveQuestLayout&&
        completeQuestLayout&&addItemLayout&&hurtLayout;
    cheatCompatibility=cheats?1:-1;
    persistenceCompatibility=savePlayerState&&saveLayout?1:-1;
    regionReplicationCompatibility=flushNetDormancy&&forceNetUpdate&&flushLayout&&forceLayout?1:-1;
    regionCompatibility=nativeRegionQuery&&revealMapFog&&onRepRegions&&getRevealedMapFogComponent&&revealLayout&&onRepLayout&&regionReplicationCompatibility==1?1:-1;
    const bool directUnlocks=unlockRecipes&&unlockBuildings&&setQuestObjective&&recipesLayout&&buildingsLayout&&objectiveLayout;
    const bool catalogResolved=ResolveEntries();
    regionPropertyDirtyCompatibility=markPropertyDirty&&netPushModelHelpers&&markDirtyLayout&&regionBitmapNameIndex?1:-1;
    regionReplicationCompatibility=regionReplicationCompatibility==1&&regionPropertyDirtyCompatibility==1?1:-1;
    regionCompatibility=regionCompatibility==1&&regionReplicationCompatibility==1?1:-1;
    if(regionCompatibility==1&&(regionEntries.load()==0||resolvedRegionEntries.load()!=regionEntries.load()))regionCompatibility=-1;
    compatibility=cheatCompatibility==1&&persistenceCompatibility==1&&regionCompatibility==1&&directUnlocks&&catalogResolved?1:-3;
    if(compatibility==1)LogMessage("Admin activity test laboratory compatibility verified");
    else {LogMessage("Admin activity test laboratory unavailable (functions="+
        std::to_string(bool(enableCheats))+std::to_string(bool(resetProgress))+std::to_string(bool(resetQuests))+
        std::to_string(bool(giveQuest))+
        std::to_string(bool(completeQuest))+std::to_string(bool(addItem))+std::to_string(bool(hurtMe))+
        std::to_string(bool(revive))+std::to_string(bool(savePlayerState))+std::to_string(bool(revealMapFog))+
        std::to_string(bool(onRepRegions))+std::to_string(bool(getRevealedMapFogComponent))+std::to_string(bool(flushNetDormancy))+std::to_string(bool(forceNetUpdate))+std::to_string(bool(markPropertyDirty))+std::to_string(bool(netPushModelHelpers))+std::to_string(bool(unlockRecipes))+std::to_string(bool(unlockBuildings))+
        std::to_string(bool(setQuestObjective))+", layouts="+std::to_string(giveQuestLayout)+std::to_string(completeQuestLayout)+
        std::to_string(addItemLayout)+std::to_string(hurtLayout)+std::to_string(saveLayout)+
        std::to_string(revealLayout)+std::to_string(onRepLayout)+std::to_string(flushLayout)+std::to_string(forceLayout)+std::to_string(markDirtyLayout)+std::to_string(recipesLayout)+
        std::to_string(buildingsLayout)+std::to_string(objectiveLayout)+", catalog="+
        std::to_string(resolvedEntries.load())+"/"+std::to_string(entries.size())+")");}
    return compatibility==1;
}
void UpdateAdminActivityTestPlayers(const std::vector<PlayerInfo>&players){std::vector<Target> fresh;for(const auto&p:players)if(!p.characterGuid.empty()&&p.controllerPtr)fresh.push_back(ResolveTarget(p));Lock lock(targetMutex);targets=std::move(fresh);const auto now=std::chrono::steady_clock::now();if(resolvedEntries.load()<entries.size()&&now>=nextCatalogRefresh){ResolveEntries();nextCatalogRefresh=now+std::chrono::seconds(10);}}
void ProcessAdminActivityTestRequests(){std::shared_ptr<Request> request;{Lock lock(requestMutex);if(queue.empty())return;request=queue.front();queue.pop_front();}const auto started=std::chrono::steady_clock::now();Process(request);Maximum(std::uint64_t(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-started).count()));}
bool ResolveAdminActivityTestSelection(const std::string&family,const std::string&key,std::string&related,int&target){if(family=="skill"){if(!Safe(key,128)){return false;}related=key;return target>=2&&target<=100;}if(family=="death"){related.clear();target=0;return key=="death";}Entry entry{};if(!FindEntry(family,key,entry)||!entry.object)return false;related=entry.related;target=entry.target;return true;}
bool SubmitAdminActivityTest(const std::string&family,const std::string&phase,const std::string&guid,const std::string&key,const std::string&related,int target,const std::string&operation,const std::string&session,std::string&id,std::string&error){
    if(compatibility.load()!=1){error="Activity testing compatibility unavailable";return false;}
    if(!Safe(guid,128)||!Safe(key,512)||(!related.empty()&&!Safe(related,128))||(phase!="prepare"&&phase!="trigger")||target<0||target>200||session.empty()||session.size()>128){error="Invalid activity test request";return false;}
    static const std::set<std::string> families={"skill","perk","spell","recipe","quest","material","region","death"};if(!families.count(family)){error="Unsupported activity family";return false;}
    Target live;if(!SnapshotTarget(guid,live)){error="Target character must be online";return false;}
    Entry entry{};const bool hasEntry=family!="skill"&&family!="death"&&FindEntry(family,key,entry);
    if(family!="skill"&&family!="death"&&(!hasEntry||!entry.object)){error="The selected catalog entry is unavailable in the current game build";return false;}
    std::string authoritativeRelated=related;int authoritativeTarget=target;
    if(hasEntry&&(family=="perk"||family=="spell")){authoritativeRelated=entry.related;authoritativeTarget=entry.target;}
    if(family=="skill"&&(related!=key||target<1||target>100)){error="Invalid skill target";return false;}
    if((family=="perk"||family=="spell")&&(authoritativeRelated.empty()||authoritativeTarget<1||authoritativeTarget>100)){error="Invalid unlock dependency";return false;}
    if(family=="quest"&&operation!="start"&&operation!="progress"&&operation!="complete"){error="Invalid quest operation";return false;}
    if(family=="quest"&&operation=="progress"&&(entry.objectives.size()<2||!entry.objectives[0].resolved||!entry.objectives[1].resolved)){error="This quest does not expose a safe objective transition";return false;}
    auto request=std::make_shared<Request>();request->id=RequestId();request->session=session;request->guid=guid;request->family=family;request->phase=phase;request->key=key;request->related=authoritativeRelated;request->target=authoritativeTarget;request->operation=operation;request->updated.store(Now());
    {Lock lock(requestMutex);const auto now=Now();for(auto it=requests.begin();it!=requests.end();){const auto&old=*it->second;const auto state=old.state.load();if((state==RequestState::Completed||state==RequestState::Failed)&&now>old.updated.load()+3600)it=requests.erase(it);else ++it;}for(const auto&item:requests){const auto state=item.second->state.load();if(item.second->guid==guid&&(state==RequestState::Queued||state==RequestState::Running)){error="Another activity test is pending for this character";return false;}}if(queue.size()>=32||requests.size()>=256){error="Activity test queue is full";return false;}requests[request->id]=request;queue.push_back(request);}
    ++queued;id=request->id;return true;
}
std::string GetAdminActivityTestStatus(const std::string&id,const std::string&session,std::string&confirmedPreparationGuid,std::string&failedPreparationGuid,std::string&failedTriggerGuid){std::shared_ptr<Request> request;{Lock lock(requestMutex);auto found=requests.find(id);if(found==requests.end()||found->second->session!=session)return {};request=found->second;}Lock requestLock(request->mutex);auto&r=*request;const auto current=r.state.load();if(!r.terminalAcknowledged&&(current==RequestState::Completed||current==RequestState::Failed)){if(r.phase=="prepare")(current==RequestState::Completed?confirmedPreparationGuid:failedPreparationGuid)=r.guid;else if(current==RequestState::Failed)failedTriggerGuid=r.guid;r.terminalAcknowledged=true;}const char*state=current==RequestState::Queued?"queued":current==RequestState::Running?"running":current==RequestState::Completed?"completed":"failed";return "{\"id\":\""+r.id+"\",\"status\":\""+state+"\",\"message\":\""+Escape(r.message)+"\",\"attempts\":"+std::to_string(r.attempts)+",\"level\":"+std::to_string(r.lastLevel)+"}";}
std::string GetAdminActivityTestCatalogJson(const std::string&guid){
    std::set<std::string> unlockedMaterials,unlockedPerks,unlockedSpells;
    for(const auto&s:ReadPlayerActivityDiscoveries())if(s.characterGuid==guid){for(const auto&m:s.materials)unlockedMaterials.insert(m.key);for(const auto&u:s.unlocks)(u.type=="perk"?unlockedPerks:unlockedSpells).insert(u.key);}
    std::set<std::string> unlockedRecipes;for(const auto&s:ReadPlayerRecipeSnapshots())if(s.characterGuid==guid)unlockedRecipes.insert(s.unlockedIds.begin(),s.unlockedIds.end());
    std::unordered_map<std::string,std::string> questStates;for(const auto&s:ReadPlayerQuestSnapshots())if(s.characterGuid==guid)for(const auto&q:s.quests)questStates[q.id]=q.status;
    Target live;const bool online=LiveTarget(guid,live);std::uint32_t regionBitmap=0;bool regionStateKnown=false;
    if(online&&nativeRegionQuery&&live.regions&&g_Engine->IsObjectLive(live.regions)&&g_Engine->IsA(live.regions,"RevealedMapFogComponent")&&Mem::SafeRead(live.regions+Offsets::RevealedMapFog_RevealedMapRegionsBitmap,regionBitmap)){
        using QueryFn=bool(*)(const void*,unsigned int);const auto query=reinterpret_cast<QueryFn>(nativeRegionQuery);regionStateKnown=true;
        for(unsigned bit=1;bit<=31;++bit)if(query(reinterpret_cast<const void*>(live.regions),bit)!=bool(regionBitmap&(std::uint32_t(1)<<bit))){regionStateKnown=false;break;}
    }
    std::ostringstream out;out<<"{\"online\":"<<(online?"true":"false")<<",\"compatibility\":"<<(compatibility.load()==1?"true":"false")<<",\"regionStateKnown\":"<<(regionStateKnown?"true":"false")<<",\"entries\":[";bool first=true;Lock entryLock(entryMutex);
    for(const auto&e:entries){
        if(!first)out<<',';first=false;const char*family=e.kind==Kind::Material?"material":e.kind==Kind::Perk?"perk":e.kind==Kind::Spell?"spell":(e.kind==Kind::Recipe||e.kind==Kind::Building)?"recipe":e.kind==Kind::Quest?"quest":"region";
        const bool unlocked=e.kind==Kind::Material?unlockedMaterials.count(e.key):e.kind==Kind::Perk?unlockedPerks.count(e.key):e.kind==Kind::Spell?unlockedSpells.count(e.key):(e.kind==Kind::Recipe||e.kind==Kind::Building)?unlockedRecipes.count(e.key):e.kind==Kind::Quest?(questStates.count(e.key)&&questStates[e.key]!="not_started"):e.kind==Kind::Region&&regionStateKnown&&bool(regionBitmap&(std::uint32_t(1)<<unsigned(e.target)));
        out<<"{\"family\":\""<<family<<"\",\"key\":\""<<Escape(e.key)<<"\",\"name\":\""<<Escape(e.name)<<"\",\"icon\":"<<(e.icon.empty()?"null":"\""+Escape(e.icon)+"\"")<<",\"related\":\""<<Escape(e.related)<<"\",\"target\":"<<e.target<<",\"unlocked\":"<<(unlocked?"true":"false")<<",\"resolved\":"<<(e.object?"true":"false");
        if(e.kind==Kind::Region)out<<",\"stateKnown\":"<<(regionStateKnown?"true":"false")<<",\"discovered\":"<<(unlocked?"true":"false");
        if(e.kind==Kind::Quest)out<<",\"state\":\""<<Escape(questStates[e.key])<<"\",\"progressAvailable\":"<<(e.objectives.size()>=2&&e.objectives[0].resolved&&e.objectives[1].resolved?"true":"false");out<<'}';
    }
    out<<"],\"skills\":[";first=true;if(online)for(const auto&skill:live.skillSnapshot){if(!first)out<<',';first=false;out<<"{\"key\":\""<<Escape(skill.skill)<<"\",\"name\":\""<<Escape(skill.skill.rfind("SKILL_",0)==0?skill.skill.substr(6):skill.skill)<<"\",\"level\":"<<Status::SkillLevel(skill.currentXP)<<'}';}out<<"]}";return out.str();
}
AdminActivityTestDiagnostics GetAdminActivityTestDiagnostics(){return {compatibility.load(),cheatCompatibility.load(),persistenceCompatibility.load(),regionCompatibility.load(),regionReplicationCompatibility.load(),regionPropertyDirtyCompatibility.load(),entries.size(),resolvedEntries.load(),regionEntries.load(),resolvedRegionEntries.load(),queued.load(),completed.load(),failed.load(),stale.load(),maximumMicros.load(),regionReplicationWakeCalls.load(),regionReplicationWakeFailures.load(),regionPropertyDirtyCalls.load(),regionPropertyDirtyFailures.load()};}
} // namespace DomEngine
