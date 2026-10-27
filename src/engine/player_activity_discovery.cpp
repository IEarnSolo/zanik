#include "dom_engine.h"
#include "../utils/memory.h"

#include <algorithm>
#include <atomic>
#include <cctype>
#include <chrono>
#include <cstring>
#include <unordered_map>

namespace DomEngine { namespace {
std::atomic<int> regionCompatibility{0},materialCompatibility{0};
std::atomic<std::uint64_t> snapshots{0},dropped{0};
std::unordered_map<std::uint8_t,std::string> regionNames;
std::unordered_map<std::string,ActivityMaterialInfo> materialNames;
bool regionCatalogScanned=false;
std::chrono::steady_clock::time_point lastRegionScan{};

struct SparseSetHeader {
    void* data;
    int32_t num,max;
    std::uint32_t* flags;
    int32_t flagNum,flagMax;
    int32_t firstFree,numFree;
};

uintptr_t ProgressFor(const PlayerInfo&p){
    if(!g_Engine||!p.controllerPtr)return 0;
    int32_t offset=g_Engine->FindPropertyOffset(p.controllerPtr,"BP_Components_Progress");
    if(offset<0)offset=g_Engine->FindPropertyOffset(p.controllerPtr,"ProgressComponent");
    if(offset<0||offset>0x3000)return 0;
    uintptr_t value=Mem::ReadPtr(p.controllerPtr+uintptr_t(offset));
    return value&&g_Engine->IsObjectLive(value)&&g_Engine->IsA(value,"ProgressComponent")?value:0;
}

bool ReadMaterials(uintptr_t progress,std::vector<ActivityMaterialInfo>&out){
    int32_t offset=g_Engine->FindPropertyOffset(progress,"ItemsPickedUp");if(offset<0||offset>0x2000)return false;
    SparseSetHeader a{},b{};if(!Mem::Readable(reinterpret_cast<void*>(progress+uintptr_t(offset)),sizeof(a)))return false;
    std::memcpy(&a,reinterpret_cast<void*>(progress+uintptr_t(offset)),sizeof(a));
    if(a.num<0||a.max<0||a.num>a.max||a.max>8192||a.flagNum<0||a.flagNum<a.max||a.flagMax<a.flagNum||a.flagMax>16384||a.numFree<0||a.numFree>a.num)return false;
    const size_t flagBytes=size_t((a.flagNum+31)/32)*4;if(a.max&&(!a.data||!a.flags||!Mem::Readable(a.data,size_t(a.max)*16)||!Mem::Readable(a.flags,flagBytes)))return false;
    std::vector<std::uint32_t> flags((a.flagNum+31)/32);if(flagBytes)std::memcpy(flags.data(),a.flags,flagBytes);std::vector<std::pair<int32_t,uintptr_t>> occupiedSlots;
    int occupied=0;for(int32_t i=0;i<a.max;++i){if(!(flags[i/32]&(1u<<(i%32))))continue;++occupied;uintptr_t item=Mem::ReadPtr(reinterpret_cast<uintptr_t>(a.data)+uintptr_t(i)*16);if(!item||!g_Engine->IsObjectLive(item)||!g_Engine->IsA(item,"ItemData"))return false;occupiedSlots.push_back({i,item});bool notify=false;if(!g_Engine->ReadBoolProperty(item,"bShowNotificationOnFirstPickup",notify))return false;if(!notify)continue;std::string key=g_Engine->GetObjectName(item);if(key.empty()||key.size()>256)return false;auto known=materialNames.find(key);if(known!=materialNames.end()){out.push_back(known->second);continue;}std::string name=g_Engine->ReadObjectText(item,"Name");if(name.size()>512)return false;if(name.empty())name=key;std::string icon;for(unsigned char c:key)if(std::isalnum(c)||c=='_'||c=='-')icon+=char(c);if(!icon.empty())icon="/assets/items/"+icon+".png";ActivityMaterialInfo info{key,name,icon};materialNames.emplace(key,info);out.push_back(std::move(info));}
    if(occupied!=a.num-a.numFree)return false;
    std::memcpy(&b,reinterpret_cast<void*>(progress+uintptr_t(offset)),sizeof(b));
    if(std::memcmp(&a,&b,sizeof(a))||(flagBytes&&std::memcmp(flags.data(),a.flags,flagBytes)))return false;
    for(const auto&slot:occupiedSlots)
        if(Mem::ReadPtr(reinterpret_cast<uintptr_t>(a.data)+uintptr_t(slot.first)*16)!=slot.second)return false;
    std::sort(out.begin(),out.end(),[](const auto&x,const auto&y){return x.key<y.key;});out.erase(std::unique(out.begin(),out.end(),[](const auto&x,const auto&y){return x.key==y.key;}),out.end());return true;
}

bool ReadRegions(const PlayerInfo&p,std::uint32_t&bitmap,std::vector<ActivityRegionInfo>&out){
    int32_t componentOffset=g_Engine->FindPropertyOffset(p.controllerPtr,"RevealedMapFogComponent");if(componentOffset<0||componentOffset>0x3000)return false;uintptr_t component=Mem::ReadPtr(p.controllerPtr+uintptr_t(componentOffset));if(!component||!g_Engine->IsObjectLive(component)||!g_Engine->IsA(component,"RevealedMapFogComponent"))return false;
    int32_t bitmapOffset=g_Engine->FindPropertyOffset(component,"RevealedMapRegionsBitmap");if(bitmapOffset<0||bitmapOffset>0x1000||!Mem::Read(component+uintptr_t(bitmapOffset),bitmap))return false;
    bool scan=!regionCatalogScanned;for(unsigned i=0;i<32;++i)if((bitmap&(1u<<i))&&!regionNames.count(static_cast<std::uint8_t>(i)))scan=true;
    if(scan){auto now=std::chrono::steady_clock::now();if(regionCatalogScanned&&now-lastRegionScan<std::chrono::seconds(5))return false;lastRegionScan=now;regionCatalogScanned=true;for(int32_t i=0;i<g_Engine->GetObjectCount();++i){uintptr_t region=g_Engine->GetObjectByIndex(i);if(!region||!g_Engine->IsObjectLive(region)||!g_Engine->IsA(region,"MapRegion"))continue;int32_t idOffset=g_Engine->FindPropertyOffset(region,"RegionID");if(idOffset<0||idOffset>0x1000)continue;std::uint8_t id=Mem::ReadU8(region+uintptr_t(idOffset));if(id>=32)continue;std::string name=g_Engine->ReadObjectText(region,"RegionName");if(!name.empty()&&name.size()<=512)regionNames[id]=std::move(name);}}
    for(unsigned i=0;i<32;++i)if(bitmap&(1u<<i)){auto found=regionNames.find(static_cast<std::uint8_t>(i));if(found==regionNames.end())return false;out.push_back({static_cast<std::uint8_t>(i),found->second});}
    std::uint32_t confirmed=0;if(!Mem::Read(component+uintptr_t(bitmapOffset),confirmed)||confirmed!=bitmap)return false;return true;
}
}

std::vector<PlayerActivityDiscoverySnapshot> ReadPlayerActivityDiscoveries(const std::vector<PlayerInfo>&players){std::vector<PlayerActivityDiscoverySnapshot> out;if(!g_Engine)return out;auto now=std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();for(const auto&p:players){if(p.characterGuid.empty()||!p.controllerPtr)continue;PlayerActivityDiscoverySnapshot s;s.characterGuid=p.characterGuid;s.capturedAt=now;s.regionsComplete=ReadRegions(p,s.regionBitmap,s.regions);regionCompatibility=s.regionsComplete?1:-1;uintptr_t progress=ProgressFor(p);s.materialsComplete=progress&&ReadMaterials(progress,s.materials);materialCompatibility=s.materialsComplete?1:-1;if(!s.regionsComplete||!s.materialsComplete)++dropped;out.push_back(std::move(s));++snapshots;}return out;}
PlayerActivityDiscoveryDiagnostics GetPlayerActivityDiscoveryDiagnostics(){return {regionCompatibility.load(),materialCompatibility.load(),snapshots.load(),dropped.load()};}
}
