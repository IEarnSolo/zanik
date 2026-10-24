#include "dom_engine.h"
#include "../utils/logger.h"
#include "../utils/memory.h"

#include <cstdio>
#include <cstring>
#include <sstream>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <fstream>

namespace DomEngine {

namespace {

const std::unordered_map<std::string,std::string>& BuildingActorDisplayNames(){
    static const auto names=[](){
        std::unordered_map<std::string,std::string> values;
        const char* root=std::getenv("DW_WEB_ROOT");
        std::ifstream input(std::string(root&&*root?root:"/home/container/zanik/web")+"/buildings/metadata.tsv");
        std::string line;
        while(std::getline(input,line)){
            if(line.empty()||line[0]=='#')continue;
            auto a=line.find('\t'),b=a==std::string::npos?std::string::npos:line.find('\t',a+1),c=b==std::string::npos?std::string::npos:line.find('\t',b+1);
            if(a==std::string::npos||b==std::string::npos||c==std::string::npos)continue;
            std::string name=line.substr(a+1,b-a-1),actor=line.substr(c+1);
            if(!actor.empty()&&actor.back()=='\r')actor.pop_back();
            if(!name.empty()&&!actor.empty())values[std::move(actor)]=std::move(name);
        }
        return values;
    }();
    return names;
}

void AppendUtf8(std::string& out, uint32_t codepoint) {
    if (codepoint < 0x80) {
        out += (char)codepoint;
    } else if (codepoint < 0x800) {
        out += (char)(0xC0 | (codepoint >> 6));
        out += (char)(0x80 | (codepoint & 0x3F));
    } else if (codepoint < 0x10000) {
        out += (char)(0xE0 | (codepoint >> 12));
        out += (char)(0x80 | ((codepoint >> 6) & 0x3F));
        out += (char)(0x80 | (codepoint & 0x3F));
    } else {
        out += (char)(0xF0 | (codepoint >> 18));
        out += (char)(0x80 | ((codepoint >> 12) & 0x3F));
        out += (char)(0x80 | ((codepoint >> 6) & 0x3F));
        out += (char)(0x80 | (codepoint & 0x3F));
    }
}

std::string Utf16ToUtf8(const char16_t* data, int32_t length) {
    std::string out;
    out.reserve((size_t)length);
    for (int32_t i = 0; i < length; i++) {
        uint32_t unit = data[i];
        if (unit >= 0xD800 && unit <= 0xDBFF && i + 1 < length) {
            uint32_t low = data[i + 1];
            if (low >= 0xDC00 && low <= 0xDFFF) {
                AppendUtf8(out, 0x10000 + ((unit - 0xD800) << 10) + (low - 0xDC00));
                i++;
                continue;
            }
        }
        AppendUtf8(out, unit);
    }
    return out;
}

bool IsPrintableAscii(const std::string& value) {
    if (value.empty()) return false;
    for (unsigned char c : value) {
        if (c < 0x20 || c > 0x7E) return false;
    }
    return true;
}

}

int32_t Engine::GetObjectCount() const {
    if (!gObjects) return 0;
    return Mem::ReadI32(gObjects + Offsets::ObjArray_NumElements);
}

uintptr_t Engine::GetObjectByIndex(int32_t index) const {
    if (!gObjects || index < 0) return 0;

    int32_t numElements = Mem::ReadI32(gObjects + Offsets::ObjArray_NumElements);
    int32_t numChunks = Mem::ReadI32(gObjects + Offsets::ObjArray_NumChunks);
    if (index >= numElements) return 0;

    int32_t chunkIndex = index / Offsets::ObjectsPerChunk;
    int32_t inChunk = index % Offsets::ObjectsPerChunk;
    if (chunkIndex >= numChunks) return 0;

    uintptr_t chunks = Mem::ReadPtr(gObjects + Offsets::ObjArray_Objects);
    if (!chunks) return 0;

    uintptr_t chunk = Mem::ReadPtr(chunks + (uintptr_t)chunkIndex * sizeof(uintptr_t));
    if (!chunk) return 0;

    return Mem::ReadPtr(chunk + (uintptr_t)inChunk * Offsets::ObjectItem_Size);
}

bool Engine::IsObjectLive(uintptr_t object) const {
    if(!object||!gObjects||!Mem::Readable(reinterpret_cast<void*>(object+Offsets::UObject_Index),sizeof(int32_t)))return false;
    const int32_t index=Mem::ReadI32(object+Offsets::UObject_Index);
    const int32_t numElements=Mem::ReadI32(gObjects+Offsets::ObjArray_NumElements);
    const int32_t numChunks=Mem::ReadI32(gObjects+Offsets::ObjArray_NumChunks);
    if(index<0||index>=numElements)return false;
    const int32_t chunkIndex=index/Offsets::ObjectsPerChunk,inChunk=index%Offsets::ObjectsPerChunk;
    if(chunkIndex<0||chunkIndex>=numChunks)return false;
    const uintptr_t chunks=Mem::ReadPtr(gObjects+Offsets::ObjArray_Objects);
    const uintptr_t chunk=chunks?Mem::ReadPtr(chunks+uintptr_t(chunkIndex)*sizeof(uintptr_t)):0;
    if(!chunk)return false;
    const uintptr_t item=chunk+uintptr_t(inChunk)*Offsets::ObjectItem_Size;
    if(Mem::ReadPtr(item)!=object)return false;
    const std::uint32_t flags=static_cast<std::uint32_t>(Mem::ReadI32(item+Offsets::ObjectItem_Flags));
    // UE5 EInternalObjectFlags. DestroyActor marks the actor as garbage long
    // before FUObjectArray releases it during the next garbage collection.
    constexpr std::uint32_t Unreachable=1u<<28,PendingKill=1u<<29,Garbage=1u<<31;
    return (flags&(Unreachable|PendingKill|Garbage))==0;
}

std::uint64_t Engine::GetObjectIdentity(uintptr_t object) const {
    if(!object||!gObjects||!Mem::Readable(reinterpret_cast<void*>(object+Offsets::UObject_Index),sizeof(int32_t)))return 0;
    const int32_t index=Mem::ReadI32(object+Offsets::UObject_Index),numElements=Mem::ReadI32(gObjects+Offsets::ObjArray_NumElements),numChunks=Mem::ReadI32(gObjects+Offsets::ObjArray_NumChunks);
    if(index<0||index>=numElements)return 0;const int32_t chunkIndex=index/Offsets::ObjectsPerChunk,inChunk=index%Offsets::ObjectsPerChunk;if(chunkIndex<0||chunkIndex>=numChunks)return 0;
    const uintptr_t chunks=Mem::ReadPtr(gObjects+Offsets::ObjArray_Objects),chunk=chunks?Mem::ReadPtr(chunks+uintptr_t(chunkIndex)*sizeof(uintptr_t)):0;if(!chunk)return 0;
    const uintptr_t item=chunk+uintptr_t(inChunk)*Offsets::ObjectItem_Size;if(Mem::ReadPtr(item)!=object)return 0;
    const std::uint32_t serial=static_cast<std::uint32_t>(Mem::ReadI32(item+Offsets::ObjectItem_Serial));return (std::uint64_t(serial)<<32)|std::uint32_t(index);
}

std::string Engine::NameToString(uint32_t comparisonIndex, uint32_t number) const {
    if (!gNames) return "";

    uint32_t blockIndex = comparisonIndex >> Offsets::NameBlockOffsetBits;
    uint32_t inBlock = comparisonIndex & ((1u << Offsets::NameBlockOffsetBits) - 1u);
    if (blockIndex >= (uint32_t)Offsets::NamePool_MaxBlocks) return "";

    uintptr_t block = Mem::ReadPtr(gNames + Offsets::NamePool_Blocks +
                                   (uintptr_t)blockIndex * sizeof(uintptr_t));
    if (!block) return "";

    uintptr_t entry = block + (uintptr_t)inBlock * Offsets::NameEntryStride;
    uint16_t header = 0;
    if (!Mem::Read(entry, header)) return "";

    bool isWide = (header & 1) != 0;
    int32_t length = (int32_t)(header >> 6);
    if (length <= 0 || length > 1024) return "";

    std::string result;
    if (isWide) {
        size_t bytes = (size_t)length * sizeof(char16_t);
        if (!Mem::Readable((void*)(entry + 2), bytes)) return "";
        result = Utf16ToUtf8((const char16_t*)(entry + 2), length);
    } else {
        if (!Mem::Readable((void*)(entry + 2), (size_t)length)) return "";
        result.assign((const char*)(entry + 2), (size_t)length);
    }

    if (number > 0) result += "_" + std::to_string(number - 1);
    return result;
}

std::string Engine::GetObjectName(uintptr_t object) const {
    if (!object || !Mem::Readable((void*)object, 0x28)) return "";
    uint32_t comparisonIndex = (uint32_t)Mem::ReadI32(object + Offsets::UObject_Name);
    uint32_t number = (uint32_t)Mem::ReadI32(object + Offsets::UObject_Name + 4);
    return NameToString(comparisonIndex, number);
}

std::string Engine::GetObjectClassName(uintptr_t object) const {
    if (!object || !Mem::Readable((void*)object, 0x28)) return "";
    uintptr_t classPtr = Mem::ReadPtr(object + Offsets::UObject_Class);
    return GetObjectName(classPtr);
}

bool Engine::IsA(uintptr_t object, const char* className) const {
    if (!object || !className) return false;

    uintptr_t classPtr = Mem::ReadPtr(object + Offsets::UObject_Class);
    for (int depth = 0; classPtr && depth < 32; depth++) {
        if (GetObjectName(classPtr) == className) return true;
        classPtr = Mem::ReadPtr(classPtr + Offsets::UStruct_SuperStruct);
    }
    return false;
}

uintptr_t Engine::FindObject(const char* className, bool wantDefaultObject) const {
    if (!className) return 0;

    int32_t count = GetObjectCount();
    for (int32_t i = 0; i < count; i++) {
        uintptr_t object = GetObjectByIndex(i);
        if (!object) continue;
        if (GetObjectClassName(object) != className) continue;

        bool isDefault = GetObjectName(object).rfind("Default__", 0) == 0;
        if (isDefault != wantDefaultObject) continue;
        return object;
    }
    return 0;
}

uintptr_t Engine::GetWorld() const {
    if(cachedWorld&&GetObjectClassName(cachedWorld)=="World"&&
       Mem::ReadPtr(cachedWorld+Offsets::World_PersistentLevel)){
        worldSource="cached-object-sweep";return cachedWorld;
    }
    for(uintptr_t offset:{Offsets::GWorld_26161880,Offsets::GWorld_25991736,Offsets::GWorld_25811348,Offsets::GWorld_25630937,Offsets::GWorld_25501739,Offsets::GWorld}){
        uintptr_t world=Mem::ReadPtr(moduleBase+offset);
        if(world&&GetObjectClassName(world)=="World"&&
           Mem::ReadPtr(world+Offsets::World_PersistentLevel)){
            cachedWorld=world;worldSource="validated-gworld";return world;
        }
    }

    int32_t count = GetObjectCount();
    for (int32_t i = 0; i < count; i++) {
        uintptr_t candidate = GetObjectByIndex(i);
        if (!candidate) continue;
        if (GetObjectClassName(candidate) != "World") continue;
        if (GetObjectName(candidate).rfind("Default__", 0) == 0) continue;
        if (!Mem::ReadPtr(candidate + Offsets::World_PersistentLevel)) continue;

        cachedWorld=candidate;
        worldSource = "gobjects-sweep";
        return candidate;
    }

    worldSource = "";
    return 0;
}

uintptr_t Engine::GetGameState() const {
    uintptr_t world = GetWorld();
    if (!world) return 0;
    return Mem::ReadPtr(world + Offsets::World_GameState);
}

std::string Engine::ReadFString(uintptr_t address) const {
    if (!address || !Mem::Readable((void*)address, sizeof(FString))) return "";

    FString value = {};
    memcpy(&value, (void*)address, sizeof(FString));
    if (!value.Data || value.Num <= 1 || value.Num > 8192) return "";
    if (value.Max < value.Num) return "";

    size_t bytes = (size_t)value.Num * sizeof(char16_t);
    if (!Mem::Readable(value.Data, bytes)) return "";

    return Utf16ToUtf8(value.Data, value.Num - 1);
}

std::string Engine::ReadGuid(uintptr_t address) const {
    if (!address || !Mem::Readable((void*)address, 16)) return "";

    uint32_t parts[4] = {};
    memcpy(parts, (void*)address, sizeof(parts));
    if (!parts[0] && !parts[1] && !parts[2] && !parts[3]) return "";

    char buf[33];
    snprintf(buf, sizeof(buf), "%08X%08X%08X%08X", parts[0], parts[1], parts[2], parts[3]);
    return buf;
}

std::string Engine::ReadUniqueNetId(uintptr_t netIdRepl, std::string& outSource) const {
    outSource.clear();
    if (!netIdRepl || !Mem::Readable((void*)netIdRepl, Offsets::NetIdRepl_Size)) return "";

    const uintptr_t candidates[] = {Offsets::NetIdRepl_SharedPtrA, Offsets::NetIdRepl_SharedPtrB};
    for (uintptr_t candidate : candidates) {
        uintptr_t netId = Mem::ReadPtr(netIdRepl + candidate);
        if (!netId || !Mem::Readable((void*)netId, Offsets::NetId_FirstMember + sizeof(FString))) {
            continue;
        }

        if (!Mem::InImage((void*)Mem::ReadPtr(netId))) continue;

        std::string value = ReadFString(netId + Offsets::NetId_FirstMember);
        if (!IsPrintableAscii(value) || value.size() > 128) continue;

        outSource = "net-id-string";
        return value;
    }

    return "";
}

int32_t Engine::FindPropertyOffset(uintptr_t object,const char* propertyName)const{
    if(!object||!propertyName||!Mem::Readable(reinterpret_cast<void*>(object),Offsets::UObject_Class+sizeof(uintptr_t)))return -1;
    uintptr_t type=Mem::ReadPtr(object+Offsets::UObject_Class);
    for(int depth=0;type&&depth<64;++depth){
        if(!Mem::Readable(reinterpret_cast<void*>(type),Offsets::UStruct_ChildProperties+sizeof(uintptr_t)))break;
        uintptr_t field=Mem::ReadPtr(type+Offsets::UStruct_ChildProperties);
        for(int count=0;field&&count<4096;++count){
            if(!Mem::Readable(reinterpret_cast<void*>(field),Offsets::FProperty_OffsetInternal+sizeof(int32_t)))break;
            uint32_t nameIndex=0,number=0;
            Mem::Read(field+Offsets::FField_Name,nameIndex);
            Mem::Read(field+Offsets::FField_Name+4,number);
            if(NameToString(nameIndex,number)==propertyName){
                int32_t offset=-1;Mem::Read(field+Offsets::FProperty_OffsetInternal,offset);return offset;
            }
            uintptr_t next=Mem::ReadPtr(field+Offsets::FField_Next);
            if(next==field)break;
            field=next;
        }
        uintptr_t parent=Mem::ReadPtr(type+Offsets::UStruct_SuperStruct);
        if(parent==type)break;
        type=parent;
    }
    return -1;
}

bool Engine::ReadBoolProperty(uintptr_t object,const char* propertyName,bool& value)const{
    value=false;
    if(!object||!propertyName||!Mem::Readable(reinterpret_cast<void*>(object),Offsets::UObject_Class+sizeof(uintptr_t)))return false;
    uintptr_t type=Mem::ReadPtr(object+Offsets::UObject_Class);
    for(int depth=0;type&&depth<64;++depth){
        if(!Mem::Readable(reinterpret_cast<void*>(type),Offsets::UStruct_ChildProperties+sizeof(uintptr_t)))break;
        uintptr_t field=Mem::ReadPtr(type+Offsets::UStruct_ChildProperties);
        for(int count=0;field&&count<4096;++count){
            // UE5 FBoolProperty stores FieldSize, ByteOffset, ByteMask and
            // FieldMask directly after FProperty.  Resolve it by reflected
            // name so this fails closed if the layout changes.
            constexpr uintptr_t BoolProperty_ByteOffset=0x79;
            constexpr uintptr_t BoolProperty_FieldMask=0x7B;
            if(!Mem::Readable(reinterpret_cast<void*>(field),BoolProperty_FieldMask+1))break;
            uint32_t nameIndex=0,number=0;Mem::Read(field+Offsets::FField_Name,nameIndex);Mem::Read(field+Offsets::FField_Name+4,number);
            if(NameToString(nameIndex,number)==propertyName){
                int32_t offset=-1;std::uint8_t byteOffset=0,fieldMask=0;
                Mem::Read(field+Offsets::FProperty_OffsetInternal,offset);Mem::Read(field+BoolProperty_ByteOffset,byteOffset);Mem::Read(field+BoolProperty_FieldMask,fieldMask);
                // Native (non-bitfield) bools use 0xff; packed bools use one
                // bit.  Both are valid FBoolProperty representations.
                const bool validMask=fieldMask==0xff||(fieldMask&&!(fieldMask&(fieldMask-1)));
                if(offset<0||offset>0x4000||byteOffset>8||!validMask)return false;
                const uintptr_t address=object+uintptr_t(offset)+byteOffset;if(!Mem::Readable(reinterpret_cast<void*>(address),1))return false;
                value=(Mem::ReadU8(address)&fieldMask)!=0;return true;
            }
            uintptr_t next=Mem::ReadPtr(field+Offsets::FField_Next);if(next==field)break;field=next;
        }
        uintptr_t parent=Mem::ReadPtr(type+Offsets::UStruct_SuperStruct);if(parent==type)break;type=parent;
    }
    return false;
}

std::vector<std::pair<std::string,int32_t>> Engine::GetPropertyOffsets(uintptr_t object)const{
    std::vector<std::pair<std::string,int32_t>> result;
    if(!object||!Mem::Readable(reinterpret_cast<void*>(object),Offsets::UObject_Class+sizeof(uintptr_t)))return result;
    uintptr_t type=Mem::ReadPtr(object+Offsets::UObject_Class);
    for(int depth=0;type&&depth<64;++depth){
        if(!Mem::Readable(reinterpret_cast<void*>(type),Offsets::UStruct_ChildProperties+sizeof(uintptr_t)))break;
        uintptr_t field=Mem::ReadPtr(type+Offsets::UStruct_ChildProperties);
        for(int count=0;field&&count<4096;++count){
            if(!Mem::Readable(reinterpret_cast<void*>(field),Offsets::FProperty_OffsetInternal+sizeof(int32_t)))break;
            uint32_t nameIndex=0,number=0;int32_t offset=-1;
            Mem::Read(field+Offsets::FField_Name,nameIndex);Mem::Read(field+Offsets::FField_Name+4,number);
            Mem::Read(field+Offsets::FProperty_OffsetInternal,offset);
            std::string name=NameToString(nameIndex,number);if(!name.empty())result.emplace_back(std::move(name),offset);
            uintptr_t next=Mem::ReadPtr(field+Offsets::FField_Next);if(next==field)break;field=next;
        }
        uintptr_t parent=Mem::ReadPtr(type+Offsets::UStruct_SuperStruct);if(parent==type)break;type=parent;
    }
    return result;
}

std::vector<std::pair<std::string,int32_t>> Engine::GetStructPropertyOffsets(uintptr_t structure)const{
    std::vector<std::pair<std::string,int32_t>> result;
    for(int depth=0;structure&&depth<64;++depth){
        if(!Mem::Readable(reinterpret_cast<void*>(structure),Offsets::UStruct_ChildProperties+sizeof(uintptr_t)))break;
        uintptr_t field=Mem::ReadPtr(structure+Offsets::UStruct_ChildProperties);
        for(int count=0;field&&count<4096;++count){
            if(!Mem::Readable(reinterpret_cast<void*>(field),Offsets::FProperty_OffsetInternal+sizeof(int32_t)))break;
            uint32_t nameIndex=0,number=0;int32_t offset=-1;
            Mem::Read(field+Offsets::FField_Name,nameIndex);Mem::Read(field+Offsets::FField_Name+4,number);
            Mem::Read(field+Offsets::FProperty_OffsetInternal,offset);
            std::string name=NameToString(nameIndex,number);if(!name.empty())result.emplace_back(std::move(name),offset);
            uintptr_t next=Mem::ReadPtr(field+Offsets::FField_Next);if(next==field)break;field=next;
        }
        uintptr_t parent=Mem::ReadPtr(structure+Offsets::UStruct_SuperStruct);if(parent==structure)break;structure=parent;
    }
    return result;
}

uintptr_t Engine::FindFunction(const char* className,const char* functionName)const{
    if(!className||!functionName)return 0;
    const int32_t count=GetObjectCount();
    for(int32_t i=0;i<count;++i){
        uintptr_t object=GetObjectByIndex(i);if(!object||GetObjectClassName(object)!="Function"||
            GetObjectName(object)!=functionName)continue;
        uintptr_t outer=Mem::ReadPtr(object+Offsets::UObject_Outer);
        for(int depth=0;outer&&depth<8;++depth){
            if(GetObjectName(outer)==className)return object;
            uintptr_t next=Mem::ReadPtr(outer+Offsets::UObject_Outer);if(next==outer)break;outer=next;
        }
    }
    return 0;
}

std::string Engine::GetStorageDiagnosticsJson() const {
    auto escape=[](const std::string& s){std::string out;for(unsigned char c:s){if(c=='"'||c=='\\'){out+='\\';out+=char(c);}else if(c>=32)out+=char(c);}return out;};
    auto outerPath=[&](uintptr_t object){std::string path;for(int depth=0;object&&depth<8;++depth){std::string n=GetObjectName(object);if(!n.empty())path+=(path.empty()?"":" <- ")+n;uintptr_t next=Mem::ReadPtr(object+Offsets::UObject_Outer);if(next==object)break;object=next;}return path;};
    std::ostringstream out;out<<"{\"objects\":[";bool first=true;int matched=0;
    const int32_t count=GetObjectCount();
    for(int32_t i=0;i<count&&matched<64;++i){
        uintptr_t object=GetObjectByIndex(i);if(!object)continue;
        const std::string cls=GetObjectClassName(object);
        const bool interesting=IsA(object,"InventoryComponent")||IsA(object,"OwnershipComponent")||
            cls=="GlobalBuildingManager";
        if(!interesting)continue;++matched;if(!first)out<<',';first=false;
        out<<"{\"class\":\""<<escape(cls)<<"\",\"name\":\""<<escape(GetObjectName(object))
           <<"\",\"outer\":\""<<escape(outerPath(object))<<"\",\"properties\":[";
        bool fp=true;std::unordered_set<std::string> seen;
        for(const auto& p:GetPropertyOffsets(object)){
            if(p.first.empty()||!seen.insert(p.first).second)continue;if(!fp)out<<',';fp=false;
            out<<"{\"name\":\""<<escape(p.first)<<"\",\"offset\":"<<p.second;
            if(p.second>=0&&p.second<0x4000&&Mem::Readable(reinterpret_cast<void*>(object+p.second),16)){
                uint64_t q=0;memcpy(&q,reinterpret_cast<void*>(object+p.second),sizeof(q));out<<",\"u64\":\"0x"<<std::hex<<q<<std::dec<<"\"";
                if(p.first.find("Guid")!=std::string::npos){std::string guid=ReadGuid(object+p.second);if(!guid.empty())out<<",\"guid\":\""<<guid<<"\"";}
                if(p.first.find("Json")!=std::string::npos){std::string text=ReadFString(object+p.second);if(!text.empty()&&text.size()<65536)out<<",\"text\":\""<<escape(text)<<"\"";}
            }out<<'}';
        }out<<"]";
        out<<"}";
    }
    out<<"],\"matched\":"<<matched<<",\"objectCount\":"<<count<<'}';return out.str();
}

std::string Engine::GetPlayerStorageJson(const std::string& characterGuid) const {
    static const std::string expectedSha="445f8b65047d208adb04a3cb03bc7706f069306d814172ae29caec16c15bec7e";
    static const std::string updatedSha="ef39023c04e1bde785907e13b9b64d72600455f1633bbfe231f639d31d98cfac";
    static const std::string umbralHotfixSha="49512c0ee933c3ee4dda5fb5c2f0a25e66d7f26e34795c45b91b5c1d12b61b0c";
    static const std::string latestSha="b5017cf67bc6127dbec443e00fed1f818e82afd5d6d17f912d959a1dd87d5687";
    static const std::string currentSha="45cd13cf24658153f4c53777816ffa38ac893a471ebdf5d9c9768f7e4e74c966";
    static const std::string executableSha=FileSha256("/proc/self/exe");
    if(executableSha!=expectedSha&&executableSha!=updatedSha&&executableSha!=umbralHotfixSha&&executableSha!=latestSha&&executableSha!=currentSha)return "";
    auto escape=[](const std::string& s){std::string out;for(unsigned char c:s){if(c=='"'||c=='\\'){out+='\\';out+=char(c);}else if(c>=32)out+=char(c);}return out;};
    auto cleanType=[](std::string value){
        auto exact=BuildingActorDisplayNames().find(value);if(exact!=BuildingActorDisplayNames().end())return exact->second;
        const std::string prefix="BP_BaseBuilding_";if(value.rfind(prefix,0)==0)value.erase(0,prefix.size());
        if(value.size()>2&&value.compare(value.size()-2,2,"_C")==0)value.erase(value.size()-2);
        std::string readable;for(size_t i=0;i<value.size();++i){char c=value[i];if(c=='_'){if(!readable.empty()&&readable.back()!=' ')readable+=' ';continue;}if(i&&std::isupper(static_cast<unsigned char>(c))&&std::islower(static_cast<unsigned char>(value[i-1]))&&readable.back()!=' ')readable+=' ';readable+=c;}return readable.empty()?std::string("Storage"):readable;
    };
    std::unordered_map<uintptr_t,uintptr_t> ownership;
    std::unordered_map<uintptr_t,uintptr_t> inventories;
    std::unordered_map<uintptr_t,uintptr_t> linkedInventories;
    const int32_t count=GetObjectCount();
    for(int32_t i=0;i<count;++i){uintptr_t object=GetObjectByIndex(i);if(!object)continue;
        std::string name=GetObjectName(object);if(name.rfind("Default__",0)==0||name.find("GEN_VARIABLE")!=std::string::npos)continue;
        uintptr_t actor=Mem::ReadPtr(object+Offsets::UObject_Outer);if(!actor)continue;
        if(IsA(object,"OwnershipComponent"))ownership[actor]=object;
        else if(IsA(object,"LinkedInventoryComponent"))linkedInventories[actor]=object;
        else if(IsA(object,"InventoryComponent"))inventories[actor]=object;
    }
    using GetDisplayStringFn=const FString* (*)(const void*);
    using GetSlotCountFn=int (*)(const void*);
    using GetMaxStackSizeFn=int(*)(void*);
    const bool currentFamily=executableSha==umbralHotfixSha||executableSha==latestSha||executableSha==currentSha;
    const uintptr_t displayAddress=moduleBase+(currentFamily?0x04ACAA80:0x04AA1A80);
    const uintptr_t emptyAddress=moduleBase+(executableSha==currentSha?0x0A65B160:(executableSha==latestSha?0x0A65A8B0:(currentFamily?0x0A65A7A0:0x0A618DE0)));
    const uintptr_t filledAddress=moduleBase+(executableSha==currentSha?0x0A65DBE0:(executableSha==latestSha?0x0A65D330:(currentFamily?0x0A65D220:0x0A61B860)));
    const unsigned char displaySignature[]={0x53,0x48,0x89,0xfb,0x48,0x8b,0x3f,0x48,0x85,0xff,0x74,0x18};
    const unsigned char emptySignature[]={0x8b,0x87,0xd8,0x01,0x00,0x00,0x48,0x63,0x8f,0x70,0x01,0x00,0x00,0x29,0xc8};
    const unsigned char filledSignature[]={0x8b,0x87,0xd8,0x01,0x00,0x00,0x48,0x63,0x8f,0x70,0x01,0x00,0x00,0x41,0x89,0xc0};
    if(!Mem::Readable(reinterpret_cast<void*>(displayAddress),sizeof(displaySignature))||
       !Mem::Readable(reinterpret_cast<void*>(emptyAddress),sizeof(emptySignature))||
       !Mem::Readable(reinterpret_cast<void*>(filledAddress),sizeof(filledSignature))||
       std::memcmp(reinterpret_cast<void*>(displayAddress),displaySignature,sizeof(displaySignature))||
       std::memcmp(reinterpret_cast<void*>(emptyAddress),emptySignature,sizeof(emptySignature))||
       std::memcmp(reinterpret_cast<void*>(filledAddress),filledSignature,sizeof(filledSignature)))return "";
    auto displayString=reinterpret_cast<GetDisplayStringFn>(displayAddress);
    auto getEmptySlots=reinterpret_cast<GetSlotCountFn>(emptyAddress);
    auto getFilledSlots=reinterpret_cast<GetSlotCountFn>(filledAddress);
    std::vector<std::pair<uintptr_t,uintptr_t>> orderedInventories(inventories.begin(),inventories.end());
    auto storageOrder=[&](const std::pair<uintptr_t,uintptr_t>& entry){
        const uintptr_t actor=entry.first;std::uint32_t pieceId=0xffffffffu;const int32_t offset=FindPropertyOffset(actor,"BuildingPieceID");
        if(offset>=0&&offset<0x4000&&Mem::Readable(reinterpret_cast<void*>(actor+offset),sizeof(pieceId)))Mem::Read(actor+offset,pieceId);
        auto own=ownership.find(actor);const std::string guid=own==ownership.end()?std::string{}:ReadGuid(own->second+0x158),type=cleanType(GetObjectClassName(actor));
        std::uint64_t hash=1469598103934665603ULL;auto mix=[&](const void*p,size_t n){for(size_t k=0;k<n;++k){hash^=reinterpret_cast<const unsigned char*>(p)[k];hash*=1099511628211ULL;}};mix(type.data(),type.size());mix(guid.data(),guid.size());uintptr_t root=Mem::ReadPtr(actor+Offsets::Actor_RootComponent);FVector3d location{};if(root&&Mem::Readable(reinterpret_cast<void*>(root+Offsets::SceneComp_RelativeLocation),sizeof(location)))std::memcpy(&location,reinterpret_cast<void*>(root+Offsets::SceneComp_RelativeLocation),sizeof(location));mix(&location,sizeof(location));return std::pair<std::uint32_t,std::uint64_t>{pieceId,hash};
    };
    std::sort(orderedInventories.begin(),orderedInventories.end(),[&](const auto&a,const auto&b){return storageOrder(a)<storageOrder(b);});
    std::ostringstream out;out<<"{\"compatibility\":true,\"mapVersion\":\""<<escape(GetMapAssetVersion())<<"\",\"chests\":[";bool firstChest=true;int chestCount=0,itemCount=0;
    for(const auto& inventoryEntry:orderedInventories){
        uintptr_t actor=inventoryEntry.first,inventory=inventoryEntry.second;auto ownIt=ownership.find(actor),linkedIt=linkedInventories.find(actor);
        if(ownIt==ownership.end()||linkedIt==linkedInventories.end())continue;
        if(!IsObjectLive(actor)||!IsObjectLive(inventory)||WasAdminStorageBuildingRemoved(actor))continue;
        bool beingDestroyed=false;if(ReadBoolProperty(actor,"bActorIsBeingDestroyed",beingDestroyed)&&beingDestroyed)continue;
        uintptr_t owner=ownIt->second;std::string guid=ReadGuid(owner+0x158);if(guid!=characterGuid)continue;
        uintptr_t data=Mem::ReadPtr(inventory+0x168);int32_t num=Mem::ReadI32(inventory+0x170),max=Mem::ReadI32(inventory+0x174);
        if(num<0||num>512||max<num||(num&&(!data||!Mem::Readable(reinterpret_cast<void*>(data),size_t(num)*8))))continue;
        std::string type=cleanType(GetObjectClassName(actor));
        std::uint64_t hash=1469598103934665603ULL;auto mix=[&](const void*p,size_t n){for(size_t k=0;k<n;++k){hash^=reinterpret_cast<const unsigned char*>(p)[k];hash*=1099511628211ULL;}};
        mix(type.data(),type.size());mix(guid.data(),guid.size());uintptr_t root=Mem::ReadPtr(actor+Offsets::Actor_RootComponent);FVector3d location{};
        if(root&&Mem::Readable(reinterpret_cast<void*>(root+Offsets::SceneComp_RelativeLocation),sizeof(location)))std::memcpy(&location,reinterpret_cast<void*>(root+Offsets::SceneComp_RelativeLocation),sizeof(location));mix(&location,sizeof(location));
        char id[17]{};std::snprintf(id,sizeof(id),"%016llx",static_cast<unsigned long long>(hash));
        if(!firstChest)out<<',';firstChest=false;++chestCount;
        int filledSlots=getFilledSlots(reinterpret_cast<const void*>(inventory)),emptySlots=getEmptySlots(reinterpret_cast<const void*>(inventory));
        if(filledSlots<0||emptySlots<0||filledSlots>512||emptySlots>512||filledSlots+emptySlots>512){filledSlots=0;for(int32_t slot=0;slot<num;++slot)if(Mem::ReadPtr(data+uintptr_t(slot)*8))++filledSlots;emptySlots=std::max(0,num-filledSlots);}
        double mapX=0,mapY=0;const bool mapValid=ProjectWorldToMap(location,mapX,mapY)&&std::isfinite(mapX)&&std::isfinite(mapY)&&mapX>=0&&mapX<=1&&mapY>=0&&mapY<=1;
        const int totalSlots=filledSlots+emptySlots;const int gridColumns=8;
        const std::string customName=GetAdminStorageCustomName(actor);
        out<<"{\"id\":\""<<id<<"\",\"type\":\""<<escape(type)<<"\",\"customName\":"<<(customName.empty()?"null":"\""+escape(customName)+"\"")<<",\"filledSlots\":"<<filledSlots<<",\"totalSlots\":"<<totalSlots<<",\"gridColumns\":"<<gridColumns<<",\"linked\":"<<(Mem::ReadU8(linkedIt->second+0xF8)?"true":"false")
           <<",\"private\":"<<(Mem::ReadU8(owner+0x169)?"true":"false")<<",\"mapAvailable\":"<<(mapValid?"true":"false")
           <<",\"teleportAvailable\":"<<(mapValid?"true":"false");
        if(mapValid)out<<",\"mapX\":"<<mapX<<",\"mapY\":"<<mapY;
        out<<",\"items\":[";bool firstItem=true;
        for(int32_t slot=0;slot<num;++slot){uintptr_t item=Mem::ReadPtr(data+uintptr_t(slot)*8);if(!item||!IsA(item,"Item")||!Mem::Readable(reinterpret_cast<void*>(item+0xB0),8))continue;
            uintptr_t itemData=Mem::ReadPtr(item+0xB0);if(!itemData)continue;int32_t quantity=Mem::ReadI32(item+0x58);if(quantity<=0||quantity>1000000)continue;
            std::string name;const FString* display=displayString(reinterpret_cast<const void*>(itemData+0x68));if(display&&Mem::Readable(display,sizeof(FString)))name=ReadFString(reinterpret_cast<uintptr_t>(display));
            if(name.empty()){name=GetObjectName(itemData);for(char&c:name)if(c=='_')c=' ';}
            std::string itemKey=GetObjectName(itemData);std::string iconKey;for(unsigned char c:itemKey)if(std::isalnum(c)||c=='_'||c=='-')iconKey+=char(c);
            std::uint64_t itemHash=1469598103934665603ULL;auto itemMix=[&](const void*p,size_t n){for(size_t k=0;k<n;++k){itemHash^=reinterpret_cast<const unsigned char*>(p)[k];itemHash*=1099511628211ULL;}};itemMix(id,16);itemMix(&slot,sizeof(slot));itemMix(itemKey.data(),itemKey.size());itemMix(&quantity,sizeof(quantity));char itemId[17]{};std::snprintf(itemId,sizeof(itemId),"%016llx",static_cast<unsigned long long>(itemHash));
            uintptr_t table=Mem::ReadPtr(itemData),maxFn=table?Mem::ReadPtr(table+0x2D8):0;int maxStack=1;if(maxFn&&Mem::InImage(reinterpret_cast<void*>(maxFn))){int value=reinterpret_cast<GetMaxStackSizeFn>(maxFn)(reinterpret_cast<void*>(itemData));if(value>=1&&value<=1000000)maxStack=value;}
            if(!firstItem)out<<',';firstItem=false;++itemCount;out<<"{\"id\":\""<<itemId<<"\",\"slot\":"<<slot<<",\"revision\":\""<<itemId<<"\",\"name\":\""<<escape(name)<<"\",\"quantity\":"<<quantity<<",\"maxStack\":"<<maxStack
                <<",\"icon\":\"/assets/items/"<<escape(iconKey)<<".png\"}";
        }
        out<<"],\"slots\":[";for(int32_t slot=0;slot<totalSlots;++slot){if(slot)out<<',';uintptr_t item=slot<num?Mem::ReadPtr(data+uintptr_t(slot)*8):0;if(!item||!IsA(item,"Item")){out<<"null";continue;}uintptr_t itemData=Mem::ReadPtr(item+0xB0);int32_t quantity=Mem::ReadI32(item+0x58);if(!itemData||quantity<=0){out<<"null";continue;}std::string key=GetObjectName(itemData),name;const FString* text=displayString(reinterpret_cast<const void*>(itemData+0x68));if(text&&Mem::Readable(text,sizeof(FString)))name=ReadFString(reinterpret_cast<uintptr_t>(text));if(name.empty()){name=key;for(char&c:name)if(c=='_')c=' ';}std::string iconKey;for(unsigned char c:key)if(std::isalnum(c)||c=='_'||c=='-')iconKey+=char(c);std::uint64_t itemHash=1469598103934665603ULL;auto itemMix=[&](const void*p,size_t n){for(size_t k=0;k<n;++k){itemHash^=reinterpret_cast<const unsigned char*>(p)[k];itemHash*=1099511628211ULL;}};itemMix(id,16);itemMix(&slot,sizeof(slot));itemMix(key.data(),key.size());itemMix(&quantity,sizeof(quantity));char itemId[17]{};std::snprintf(itemId,sizeof(itemId),"%016llx",static_cast<unsigned long long>(itemHash));uintptr_t table=Mem::ReadPtr(itemData),maxFn=table?Mem::ReadPtr(table+0x2D8):0;int maxStack=1;if(maxFn&&Mem::InImage(reinterpret_cast<void*>(maxFn))){int value=reinterpret_cast<GetMaxStackSizeFn>(maxFn)(reinterpret_cast<void*>(itemData));if(value>=1&&value<=1000000)maxStack=value;}out<<"{\"id\":\""<<itemId<<"\",\"slot\":"<<slot<<",\"revision\":\""<<itemId<<"\",\"name\":\""<<escape(name)<<"\",\"quantity\":"<<quantity<<",\"maxStack\":"<<maxStack<<",\"icon\":\"/assets/items/"<<escape(iconKey)<<".png\"}";}out<<"]}";
    }
    out<<"],\"chestCount\":"<<chestCount<<",\"itemCount\":"<<itemCount<<'}';return out.str();
}

}

