#include "dom_engine.h"
#include "../utils/logger.h"
#include "../utils/memory.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <deque>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <pthread.h>
#include <sys/mman.h>
#include <unistd.h>

namespace DomEngine {
namespace {
constexpr size_t HookBytes=16;
constexpr size_t QueueCapacity=16;
struct Request{
    std::string characterGuid,publicId,destinationId,sessionPrefix,targetCharacterGuid,targetPublicId,action;
    uintptr_t preparedActor=0;
    std::uint64_t preparedActorIdentity=0;
    FVector3d preparedLocation{};
};
using TickFn=void(*)(void*,float,int,void*);
struct FRotator3d{double Pitch=0,Yaw=0,Roll=0;};
using TeleportFn=bool(*)(void*,const FVector3d*,const FRotator3d*,bool,bool);
using ProcessEventFn=void(*)(void*,void*,void*);
using TryTeleportFn=void(*)(void*,const std::uint32_t*,const std::uint32_t*);
using CanTeleportFn=bool(*)(void*,const std::uint32_t*,const std::uint32_t*);
using BPTriggerDelayedTeleportFn=bool(*)(void*,const void*);
TickFn originalTick=nullptr;TeleportFn actorTeleport=nullptr;ProcessEventFn processEvent=nullptr;
TryTeleportFn nativeTryTeleport=nullptr;CanTeleportFn originalCanTeleport=nullptr;
uintptr_t lineTraceObject=0,lineTraceFunction=0;
uintptr_t teleportSubsystem=0,teleportWithParamsFunction=0,getActorBoundsFunction=0;
BPTriggerDelayedTeleportFn bpTriggerDelayedTeleport=nullptr;
struct LineTraceLayout{int world=-1,start=-1,end=-1,channel=-1,complex=-1,ignore=-1,debug=-1,outHit=-1,ignoreSelf=-1,result=-1;};
LineTraceLayout lineTraceLayout;
struct NearbyObstacle{uintptr_t actor=0;FVector3d origin{},extent{};std::string name,cls;};
pthread_mutex_t requestMutex=PTHREAD_MUTEX_INITIALIZER;std::deque<Request> requests;
struct PendingTeleport{
    std::string characterGuid,publicId,destinationId,sessionPrefix;
    uintptr_t pawn=0,component=0;FVector3d target{};int32_t costOffset=-1;std::uint8_t oldCost=0;
    bool storage=false;std::string targetCharacterGuid,targetPublicId;FVector3d origin{};
    std::chrono::steady_clock::time_point started{};
};
std::vector<PendingTeleport> pendingTeleports;
struct QueueLock{QueueLock(){pthread_mutex_lock(&requestMutex);}~QueueLock(){pthread_mutex_unlock(&requestMutex);}};
std::atomic<int> compatibility{0};
std::atomic<int> groundTraceCompatibility{0},storagePipelineCompatibility{0},nativeCaptureCompatibility{0};
std::atomic<std::uint64_t> queued{0},accepted{0},rejected{0},maximumMicros{0};
std::atomic<std::uint64_t> acceptedStorage{0},staleStorage{0},unsafeStorage{0};
std::atomic<std::uint64_t> tickCalls{0},maximumTickMicros{0},slowTickCalls{0};
std::atomic<std::uint64_t> maximumStorageTeleportMicros{0},slowStorageTeleportCalls{0};
std::atomic<bool> storageTeleportCircuitBroken{false};
// Keep storage teleports fail-closed until the native delayed-teleport ABI has
// been captured from a genuine game teleport and validated.  The experimental
// reflected TeleportWithParams call can crash the dedicated server.
std::atomic<bool> storageTeleportTemplateValid{false};

// Read-only capture point for the parameter object constructed by the game.
// The assembly shim restores every caller-saved GPR and SIMD argument register
// before tail-jumping to the exact original prologue trampoline.
alignas(16) unsigned char capturedTeleportParams[0x50]{};
std::atomic<std::uint64_t> capturedTeleportSequence{0};
std::uint64_t flushedTeleportSequence=0;
uintptr_t capturedTeleportPawn=0;
__attribute__((used,visibility("hidden"))) void* zanikTriggerDelayedTrampoline asm("zanikTriggerDelayedTrampoline")=nullptr;

extern "C" __attribute__((noinline)) void CaptureNativeTeleportParams(void* pawn,const void* params){
    if(!pawn||!params||!Mem::Readable(params,sizeof(capturedTeleportParams)))return;
    const auto* bytes=reinterpret_cast<const unsigned char*>(params);
    // Capture only registry-authored native travel. Storage requests deliberately
    // clear bNoCheck and must never replace the trusted template.
    if(bytes[0x44]!=1)return;
    std::memcpy(capturedTeleportParams,params,sizeof(capturedTeleportParams));
    capturedTeleportPawn=reinterpret_cast<uintptr_t>(pawn);
    capturedTeleportSequence.fetch_add(1,std::memory_order_release);
}

extern "C" __attribute__((naked)) void HookNativeTriggerDelayedTeleport(){
    asm volatile(
        "pushfq\n\t"
        "push %rax\n\t" "push %rcx\n\t" "push %rdx\n\t" "push %rsi\n\t" "push %rdi\n\t"
        "push %r8\n\t" "push %r9\n\t" "push %r10\n\t" "push %r11\n\t"
        "sub $136, %rsp\n\t"
        "movdqu %xmm0, 0(%rsp)\n\t" "movdqu %xmm1, 16(%rsp)\n\t"
        "movdqu %xmm2, 32(%rsp)\n\t" "movdqu %xmm3, 48(%rsp)\n\t"
        "movdqu %xmm4, 64(%rsp)\n\t" "movdqu %xmm5, 80(%rsp)\n\t"
        "movdqu %xmm6, 96(%rsp)\n\t" "movdqu %xmm7, 112(%rsp)\n\t"
        "call CaptureNativeTeleportParams\n\t"
        "movdqu 0(%rsp), %xmm0\n\t" "movdqu 16(%rsp), %xmm1\n\t"
        "movdqu 32(%rsp), %xmm2\n\t" "movdqu 48(%rsp), %xmm3\n\t"
        "movdqu 64(%rsp), %xmm4\n\t" "movdqu 80(%rsp), %xmm5\n\t"
        "movdqu 96(%rsp), %xmm6\n\t" "movdqu 112(%rsp), %xmm7\n\t"
        "add $136, %rsp\n\t"
        "pop %r11\n\t" "pop %r10\n\t" "pop %r9\n\t" "pop %r8\n\t" "pop %rdi\n\t"
        "pop %rsi\n\t" "pop %rdx\n\t" "pop %rcx\n\t" "pop %rax\n\t" "popfq\n\t"
        "jmp *zanikTriggerDelayedTrampoline(%rip)\n\t");
}

void FlushNativeTeleportCapture(){
    const auto sequence=capturedTeleportSequence.load(std::memory_order_acquire);
    if(!sequence||sequence==flushedTeleportSequence)return;
    flushedTeleportSequence=sequence;
    FVector3d location{};FRotator3d rotation{};
    std::memcpy(&location,capturedTeleportParams+0x10,sizeof(location));
    std::memcpy(&rotation,capturedTeleportParams+0x28,sizeof(rotation));
    std::ostringstream hex;hex<<std::hex<<std::setfill('0');
    for(unsigned char byte:capturedTeleportParams)hex<<std::setw(2)<<unsigned(byte);
    std::ostringstream message;
    message<<"Native teleport params captured sequence="<<sequence<<" pawn=0x"<<std::hex<<capturedTeleportPawn<<std::dec
           <<" location="<<location.X<<","<<location.Y<<","<<location.Z
           <<" rotation="<<rotation.Pitch<<","<<rotation.Yaw<<","<<rotation.Roll
           <<" flags="<<unsigned(capturedTeleportParams[0x40])<<","<<unsigned(capturedTeleportParams[0x41])<<","<<unsigned(capturedTeleportParams[0x42])
           <<","<<unsigned(capturedTeleportParams[0x43])<<","<<unsigned(capturedTeleportParams[0x44])<<","<<unsigned(capturedTeleportParams[0x45]);
    LogMessage(message.str());
    std::ofstream report("/home/container/zanik-data/teleport-native-capture.log",std::ios::app);
    if(report)report<<message.str()<<" raw="<<hex.str()<<'\n';
    std::ofstream raw("/home/container/zanik-data/teleport-native-capture.bin",std::ios::binary|std::ios::trunc);
    if(raw)raw.write(reinterpret_cast<const char*>(capturedTeleportParams),sizeof(capturedTeleportParams));
    const bool leadingNull=std::all_of(capturedTeleportParams,capturedTeleportParams+0x10,[](unsigned char value){return value==0;});
    storageTeleportTemplateValid=leadingNull&&capturedTeleportParams[0x40]==1&&capturedTeleportParams[0x44]==1;
}

bool LoadCapturedTeleportTemplate(){
    std::ifstream raw("/home/container/zanik-data/teleport-native-capture.bin",std::ios::binary);
    if(!raw)return false;
    raw.read(reinterpret_cast<char*>(capturedTeleportParams),sizeof(capturedTeleportParams));
    if(raw.gcount()!=static_cast<std::streamsize>(sizeof(capturedTeleportParams))||raw.peek()!=std::char_traits<char>::eof())return false;
    const bool leadingNull=std::all_of(capturedTeleportParams,capturedTeleportParams+0x10,[](unsigned char value){return value==0;});
    if(!leadingNull||capturedTeleportParams[0x40]!=1||capturedTeleportParams[0x44]!=1)return false;
    // Padding observed in the native stack frame is not part of the reflected
    // properties and must never be replayed.
    capturedTeleportParams[0x46]=capturedTeleportParams[0x47]=0;
    capturedTeleportParams[0x4d]=capturedTeleportParams[0x4e]=capturedTeleportParams[0x4f]=0;
    storageTeleportTemplateValid=true;return true;
}

std::vector<std::pair<std::string,int32_t>> ReflectedFields(uintptr_t structure){
    std::vector<std::pair<std::string,int32_t>> result;uintptr_t field=Mem::ReadPtr(structure+0x50);
    for(int count=0;field&&count<256;++count){
        if(!Mem::Readable(reinterpret_cast<void*>(field),0x48))break;uint32_t index=0,number=0;int32_t offset=-1;
        Mem::Read(field+0x20,index);Mem::Read(field+0x24,number);Mem::Read(field+0x44,offset);
        const std::string name=g_Engine->NameToString(index,number);if(!name.empty()&&offset>=0&&offset<0x1000)result.emplace_back(name,offset);
        uintptr_t next=Mem::ReadPtr(field+0x18);if(next==field)break;field=next;
    }
    return result;
}

void LogTeleportReflection(){
    if(!g_Engine)return;
    std::ofstream report("/home/container/zanik-data/teleport-reflection.log",std::ios::trunc);
    auto record=[&](const std::string& message){LogMessage(message);if(report)report<<message<<'\n';};
    const int32_t count=g_Engine->GetObjectCount();
    for(int32_t i=0;i<count;++i){
        const uintptr_t object=g_Engine->GetObjectByIndex(i);if(!object)continue;
        const std::string name=g_Engine->GetObjectName(object),cls=g_Engine->GetObjectClassName(object);
        if(cls=="ScriptStruct"&&(name=="PlayerTeleportParams"||name=="FPlayerTeleportParams")){
            std::string message="Teleport reflection struct="+name;
            for(const auto& property:ReflectedFields(object))message+=" "+property.first+"=0x"+[] (int value){char text[24]{};std::snprintf(text,sizeof(text),"%x",value);return std::string(text);}(property.second);
            record(message);
        }
        if(cls=="TeleportationSubsystem"&&name.rfind("Default__",0)!=0)record("Teleport reflection subsystem object="+name);
    }
    for(const auto& entry:{std::make_pair("TeleportationSubsystem","TeleportWithParams"),
                           std::make_pair("DominionPlayerCharacter","BP_TriggerDelayedTeleport"),
                           std::make_pair("Actor","GetActorBounds")}){
        const uintptr_t function=g_Engine->FindFunction(entry.first,entry.second);std::string message="Teleport reflection function="+std::string(entry.first)+"."+entry.second;
        if(!function){record(message+" missing");continue;}
        for(const auto& property:ReflectedFields(function))message+=" "+property.first+"=0x"+[] (int value){char text[24]{};std::snprintf(text,sizeof(text),"%x",value);return std::string(text);}(property.second);
        record(message);
    }
}

void WriteJump(unsigned char* at,uintptr_t target){at[0]=0x48;at[1]=0xB8;std::memcpy(at+2,&target,8);at[10]=0xFF;at[11]=0xE0;}

struct ScopedBypass{void* component=nullptr;std::uint32_t from[4]{},to[4]{};};
thread_local ScopedBypass* activeBypass=nullptr;
bool SameGuid(const std::uint32_t* a,const std::uint32_t* b){return a&&b&&std::memcmp(a,b,16)==0;}
bool HookCanTeleport(void* component,const std::uint32_t* from,const std::uint32_t* to){
    ScopedBypass* bypass=activeBypass;
    if(bypass&&component==bypass->component&&SameGuid(from,bypass->from)&&SameGuid(to,bypass->to))return true;
    return originalCanTeleport(component,from,to);
}

uintptr_t FindTeleportComponent(uintptr_t pawn,uintptr_t controller){
    if(!g_Engine||!pawn)return 0;
    const int32_t count=g_Engine->GetObjectCount();
    for(int32_t i=0;i<count;++i){
        uintptr_t object=g_Engine->GetObjectByIndex(i);if(!object||
            !g_Engine->IsA(object,"LodestoneTeleportationComponent")||
            g_Engine->GetObjectName(object).rfind("Default__",0)==0)continue;
        if(Mem::Readable(reinterpret_cast<void*>(object+0xA8),sizeof(uintptr_t))){
            uintptr_t owner=Mem::ReadPtr(object+0xA8);if(owner==pawn||owner==controller)return object;
        }
        uintptr_t outer=Mem::ReadPtr(object+Offsets::UObject_Outer);
        for(int depth=0;outer&&depth<6;++depth){if(outer==pawn||outer==controller)return object;uintptr_t next=Mem::ReadPtr(outer+Offsets::UObject_Outer);if(next==outer)break;outer=next;}
    }
    return 0;
}

int32_t VerifiedCostFlagOffset(uintptr_t component){
    if(!component)return -1;
    uintptr_t type=Mem::ReadPtr(component+Offsets::UObject_Class);
    for(int depth=0;type&&depth<32;++depth){
        uintptr_t field=Mem::ReadPtr(type+Offsets::UStruct_ChildProperties);
        for(int count=0;field&&count<2048;++count){
            if(!Mem::Readable(reinterpret_cast<void*>(field),0x48))return -1;
            uint32_t nameIndex=0,number=0;int32_t offset=-1;
            Mem::Read(field+0x20,nameIndex);Mem::Read(field+0x24,number);Mem::Read(field+0x44,offset);
            if(g_Engine->NameToString(nameIndex,number)=="bEnableTeleportationCost")return offset==0x110?offset:-1;
            uintptr_t next=Mem::ReadPtr(field+0x18);if(next==field)break;field=next;
        }
        uintptr_t parent=Mem::ReadPtr(type+Offsets::UStruct_SuperStruct);if(parent==type)break;type=parent;
    }
    return -1;
}

void RestoreCost(PendingTeleport& pending){
    if(pending.costOffset==0x110&&Mem::Readable(reinterpret_cast<void*>(pending.component+pending.costOffset),1))
        std::memcpy(reinterpret_cast<void*>(pending.component+pending.costOffset),&pending.oldCost,1);
}

bool TraceGround(double x,double y,double aroundZ,double& floorZ){
    floorZ=INFINITY;if(!lineTraceObject||!lineTraceFunction||!processEvent)return false;
    const FVector3d traceStart{x,y,aroundZ+220.0},traceEnd{x,y,aroundZ-700.0};
    alignas(16) unsigned char params[1024]{};
    auto put=[&](int offset,const void* value,size_t size){if(offset>=0&&size_t(offset)+size<=sizeof(params))std::memcpy(params+offset,value,size);};
    uintptr_t world=g_Engine->GetWorld();put(lineTraceLayout.world,&world,sizeof(world));put(lineTraceLayout.start,&traceStart,sizeof(traceStart));put(lineTraceLayout.end,&traceEnd,sizeof(traceEnd));
    const std::uint8_t zero=0;put(lineTraceLayout.channel,&zero,1);put(lineTraceLayout.complex,&zero,1);put(lineTraceLayout.debug,&zero,1);put(lineTraceLayout.ignoreSelf,&zero,1);
    processEvent(reinterpret_cast<void*>(lineTraceObject),reinterpret_cast<void*>(lineTraceFunction),params);
    if(lineTraceLayout.result<0||!params[lineTraceLayout.result]||lineTraceLayout.outHit<0)return false;
    for(int offset=lineTraceLayout.outHit;offset+int(sizeof(FVector3d))<=lineTraceLayout.outHit+224;offset+=8){FVector3d hit{};std::memcpy(&hit,params+offset,sizeof(hit));
        if(std::isfinite(hit.X)&&std::isfinite(hit.Y)&&std::isfinite(hit.Z)&&std::abs(hit.X-x)<2.0&&std::abs(hit.Y-y)<2.0&&hit.Z<traceStart.Z-2.0&&hit.Z>traceEnd.Z+2.0)floorZ=std::min(floorZ,hit.Z);}
    return std::isfinite(floorZ);
}

bool FindSafeStorageLanding(uintptr_t pawn,const FVector3d& chest,double yaw,FVector3d& landing){
    constexpr double Pi=3.14159265358979323846;
    constexpr double radii[]={240.0,300.0,380.0,480.0,600.0};
    const double base=yaw*Pi/180.0;
    if(!lineTraceObject||!lineTraceFunction)return false;
    for(double radius:radii)for(int i=0;i<16;++i){
        const double angle=base+Pi+(Pi*2.0*i/16.0);
        const double x=chest.X+std::cos(angle)*radius,y=chest.Y+std::sin(angle)*radius;
        const FVector3d traceStart{x,y,chest.Z+180.0},traceEnd{x,y,chest.Z-650.0};
        alignas(16) unsigned char params[1024]{};
        auto put=[&](int offset,const void* value,size_t size){if(offset>=0&&size_t(offset)+size<=sizeof(params))std::memcpy(params+offset,value,size);};
        uintptr_t world=g_Engine->GetWorld();put(lineTraceLayout.world,&world,sizeof(world));
        put(lineTraceLayout.start,&traceStart,sizeof(traceStart));put(lineTraceLayout.end,&traceEnd,sizeof(traceEnd));
        const std::uint8_t visibility=0,disabled=0;put(lineTraceLayout.channel,&visibility,1);put(lineTraceLayout.complex,&disabled,1);
        put(lineTraceLayout.debug,&disabled,1);put(lineTraceLayout.ignoreSelf,&disabled,1);
        processEvent(reinterpret_cast<void*>(lineTraceObject),reinterpret_cast<void*>(lineTraceFunction),params);
        if(lineTraceLayout.result<0||!params[lineTraceLayout.result]||lineTraceLayout.outHit<0)continue;
        // FHitResult contains Location and ImpactPoint vectors. Locate the
        // reflected output vector defensively instead of binding its private
        // layout: X/Y must match this vertical trace and Z must be between its
        // endpoints (excluding the copied TraceStart/TraceEnd members).
        double floorZ=INFINITY;
        for(int offset=lineTraceLayout.outHit;offset+int(sizeof(FVector3d))<=lineTraceLayout.outHit+224;offset+=8){
            FVector3d hit{};std::memcpy(&hit,params+offset,sizeof(hit));
            if(std::isfinite(hit.X)&&std::isfinite(hit.Y)&&std::isfinite(hit.Z)&&
               std::abs(hit.X-x)<2.0&&std::abs(hit.Y-y)<2.0&&hit.Z<traceStart.Z-2.0&&hit.Z>traceEnd.Z+2.0)
                floorZ=std::min(floorZ,hit.Z);
        }
        if(!std::isfinite(floorZ))continue;
        // Dragonwilds' character capsule centre sits about 96 uu above the
        // walkable surface. TeleportTo performs the authoritative full-capsule
        // encroachment test; the trace supplies the ground height it lacks.
        const FVector3d candidate{x,y,floorZ+100.0};
        const FRotator3d rotation{0.0,angle*180.0/Pi+180.0,0.0};
        if(!actorTeleport(reinterpret_cast<void*>(pawn),&candidate,&rotation,false,false))continue;
        for(const auto& player:g_Engine->GetAllPlayers())if(player.pawnPtr==pawn&&player.hasLocation){
            const double dx=player.x-x,dy=player.y-y;
            if(dx*dx+dy*dy<=14400.0&&player.z>=floorZ+70.0&&player.z<=floorZ+180.0){landing={player.x,player.y,player.z};return true;}
            break;
        }
    }
    return false;
}

bool InitializeGroundTrace(){
    lineTraceFunction=g_Engine->FindFunction("KismetSystemLibrary","LineTraceSingle");
    lineTraceObject=g_Engine->FindObject("KismetSystemLibrary",true);
    if(!lineTraceFunction||!lineTraceObject){
        LogMessage("Ground trace reflection lookup function="+std::to_string(lineTraceFunction)+" object="+std::to_string(lineTraceObject));
        return false;
    }
    for(const auto& property:ReflectedFields(lineTraceFunction)){
        const auto& name=property.first;const int offset=property.second;
        if(name=="WorldContextObject")lineTraceLayout.world=offset;else if(name=="Start")lineTraceLayout.start=offset;
        else if(name=="End")lineTraceLayout.end=offset;else if(name=="TraceChannel")lineTraceLayout.channel=offset;
        else if(name=="bTraceComplex")lineTraceLayout.complex=offset;else if(name=="ActorsToIgnore")lineTraceLayout.ignore=offset;
        else if(name=="DrawDebugType")lineTraceLayout.debug=offset;else if(name=="OutHit")lineTraceLayout.outHit=offset;
        else if(name=="bIgnoreSelf")lineTraceLayout.ignoreSelf=offset;else if(name=="ReturnValue")lineTraceLayout.result=offset;
    }
    const int required[]={lineTraceLayout.world,lineTraceLayout.start,lineTraceLayout.end,lineTraceLayout.channel,lineTraceLayout.complex,
                          lineTraceLayout.ignore,lineTraceLayout.debug,lineTraceLayout.outHit,lineTraceLayout.ignoreSelf,lineTraceLayout.result};
    const bool valid=std::all_of(std::begin(required),std::end(required),[](int value){return value>=0&&value<768;});
    if(!valid){
        std::ostringstream message;message<<"Ground trace layout world="<<lineTraceLayout.world<<" start="<<lineTraceLayout.start
            <<" end="<<lineTraceLayout.end<<" channel="<<lineTraceLayout.channel<<" complex="<<lineTraceLayout.complex
            <<" ignore="<<lineTraceLayout.ignore<<" debug="<<lineTraceLayout.debug<<" hit="<<lineTraceLayout.outHit
            <<" ignoreSelf="<<lineTraceLayout.ignoreSelf<<" result="<<lineTraceLayout.result;LogMessage(message.str());
    }
    return valid;
}

bool InitializeStorageTeleportPipeline(){
    teleportWithParamsFunction=g_Engine->FindFunction("TeleportationSubsystem","TeleportWithParams");
    getActorBoundsFunction=g_Engine->FindFunction("Actor","GetActorBounds");
    for(int32_t i=0;i<g_Engine->GetObjectCount();++i){uintptr_t object=g_Engine->GetObjectByIndex(i);if(object&&g_Engine->GetObjectClassName(object)=="TeleportationSubsystem"&&g_Engine->GetObjectName(object).rfind("Default__",0)!=0){teleportSubsystem=object;break;}}
    if(!teleportSubsystem||!teleportWithParamsFunction||!getActorBoundsFunction)return false;
    auto has=[](const std::vector<std::pair<std::string,int32_t>>& fields,const char* name,int offset){return std::any_of(fields.begin(),fields.end(),[&](const auto& p){return p.first==name&&p.second==offset;});};
    const auto teleportFields=ReflectedFields(teleportWithParamsFunction),boundsFields=ReflectedFields(getActorBoundsFunction);
    const bool layout=has(teleportFields,"PlayerCharacter",0x0)&&has(teleportFields,"TeleportParams",0x8)&&has(teleportFields,"ReturnValue",0x68)&&
                      has(boundsFields,"Origin",0x8)&&has(boundsFields,"BoxExtent",0x20);
    if(!layout)LogMessage("Admin storage single-stage reflected layout differs from the captured fingerprint layout");
    return true;
}

bool InitializeNativeTeleportCapture(uintptr_t moduleBase,uintptr_t moduleEnd,uintptr_t tryTeleportOffset){
    struct Profile{uintptr_t tryTeleport,trigger,wrapper;size_t hookBytes;unsigned char expected[17];};
    static const Profile profiles[]={
        {0xA714910,0xA539560,0xA539A60,16,{0x41,0x57,0x41,0x56,0x41,0x55,0x41,0x54,0x53,0x48,0x81,0xEC,0x10,0x01,0x00,0x00,0x00}},
        // Build 25991736: BP_TriggerDelayedTeleport's wrapper has one first
        // rel32 call to this exact TriggerDelayedTeleport implementation.
        {0xA7628F0,0xA5765A0,0xA576B80,17,{0x55,0x41,0x57,0x41,0x56,0x41,0x55,0x41,0x54,0x53,0x48,0x81,0xEC,0x18,0x01,0x00,0x00}}
    };
    const Profile* profile=nullptr;for(const auto& candidate:profiles)if(candidate.tryTeleport==tryTeleportOffset){profile=&candidate;break;}
    if(!profile)return false;
    const uintptr_t TriggerDelayedTeleportOffset=profile->trigger,WrapperOffset=profile->wrapper;
    const size_t CaptureHookBytes=profile->hookBytes;
    if(moduleBase+TriggerDelayedTeleportOffset+CaptureHookBytes>moduleEnd)return false;
    auto* target=reinterpret_cast<unsigned char*>(moduleBase+TriggerDelayedTeleportOffset);
    if(std::memcmp(target,profile->expected,CaptureHookBytes)!=0||moduleBase+WrapperOffset+0x2b>moduleEnd)return false;
    auto* wrapper=reinterpret_cast<unsigned char*>(moduleBase+WrapperOffset);
    static const unsigned char wrapperPrefix[]={0x53,0x48,0x83,0xEC,0x40,0x0F,0x57,0xC0,0x0F,0x29,0x44,0x24,0x30};
    if(std::memcmp(wrapper,wrapperPrefix,sizeof(wrapperPrefix))!=0||wrapper[0x26]!=0xE8)return false;
    std::int32_t relative=0;std::memcpy(&relative,wrapper+0x27,sizeof(relative));
    if(reinterpret_cast<uintptr_t>(wrapper+0x2b+relative)!=reinterpret_cast<uintptr_t>(target))return false;
    bpTriggerDelayedTeleport=reinterpret_cast<BPTriggerDelayedTeleportFn>(wrapper);
    // A previously captured and structurally validated template is sufficient
    // for replay. Do not install a diagnostic capture hook on every supported
    // server; that keeps ordinary game teleports free of Zanik work.
    if(storageTeleportTemplateValid.load()){
        LogMessage("Validated native delayed teleport wrapper enabled from persisted template");
        return true;
    }
    auto* trampoline=static_cast<unsigned char*>(mmap(nullptr,64,PROT_READ|PROT_WRITE|PROT_EXEC,MAP_PRIVATE|MAP_ANONYMOUS,-1,0));
    if(trampoline==MAP_FAILED)return false;
    std::memcpy(trampoline,target,CaptureHookBytes);
    WriteJump(trampoline+CaptureHookBytes,reinterpret_cast<uintptr_t>(target+CaptureHookBytes));
    zanikTriggerDelayedTrampoline=trampoline;
    const long pageSize=sysconf(_SC_PAGESIZE);const uintptr_t page=reinterpret_cast<uintptr_t>(target)&~(uintptr_t(pageSize)-1);
    if(mprotect(reinterpret_cast<void*>(page),size_t(pageSize),PROT_READ|PROT_WRITE|PROT_EXEC)!=0){
        zanikTriggerDelayedTrampoline=nullptr;munmap(trampoline,64);return false;
    }
    unsigned char patch[CaptureHookBytes];std::memset(patch,0x90,sizeof(patch));
    WriteJump(patch,reinterpret_cast<uintptr_t>(&HookNativeTriggerDelayedTeleport));
    std::memcpy(target,patch,sizeof(patch));
    __builtin___clear_cache(reinterpret_cast<char*>(target),reinterpret_cast<char*>(target+CaptureHookBytes));
    mprotect(reinterpret_cast<void*>(page),size_t(pageSize),PROT_READ|PROT_EXEC);
    LogMessage("Native delayed teleport parameter capture and validated player wrapper enabled");
    return true;
}

void ReconcilePending(void* tickingPawn){
    QueueLock lock;const auto now=std::chrono::steady_clock::now();
    for(auto it=pendingTeleports.begin();it!=pendingTeleports.end();){
        if(it->pawn!=reinterpret_cast<uintptr_t>(tickingPawn)){++it;continue;}
        bool arrived=false;
        for(const auto& player:g_Engine->GetAllPlayers())if(player.pawnPtr==it->pawn&&player.hasLocation){
            const double dx=player.x-it->target.X,dy=player.y-it->target.Y,dz=player.z-it->target.Z;
            arrived=dx*dx+dy*dy+dz*dz<250000.0;break;
        }
        if(arrived&&it->storage){
            FVector3d chest{},landing{};double yaw=0.0;
            for(const auto& player:g_Engine->GetAllPlayers())if(player.pawnPtr==it->pawn){yaw=player.yaw;break;}
            if(ResolveAdminStorageLocation(it->targetCharacterGuid,it->destinationId,chest)&&
               FindSafeStorageLanding(it->pawn,chest,yaw,landing)){
                RestoreCost(*it);++accepted;++acceptedStorage;
                LogMessage("Admin streamed storage teleport completed character="+it->publicId+" owner="+it->targetPublicId+" chest="+it->destinationId+" session="+it->sessionPrefix);
                it=pendingTeleports.erase(it);continue;
            }
            if(now-it->started<=std::chrono::seconds(8)){++it;continue;}
        }
        const bool timedOut=now-it->started>std::chrono::seconds(20);
        if(!arrived&&!timedOut){++it;continue;}
        RestoreCost(*it);
        if(arrived&&!it->storage){++accepted;LogMessage("Admin native lodestone teleport completed character="+it->publicId+" session="+it->sessionPrefix+" destination="+it->destinationId);}
        else if(it->storage){
            ++rejected;++unsafeStorage;
            const FRotator3d rotation{};actorTeleport(reinterpret_cast<void*>(it->pawn),&it->origin,&rotation,false,false);
            LogMessage("Admin streamed storage teleport rejected after preload: no collision-safe arrival chest="+it->destinationId);
        }
        else{++rejected;LogMessage("Admin native lodestone teleport timed out character="+it->publicId+" destination="+it->destinationId);}
        it=pendingTeleports.erase(it);
    }
}

std::vector<NearbyObstacle> ReadNearbyBuildingBounds(uintptr_t targetActor,const FVector3d& center){
    std::vector<NearbyObstacle> result;if(!g_Engine||!getActorBoundsFunction)return result;
    const int32_t count=g_Engine->GetObjectCount();
    for(int32_t i=0;i<count;++i){
        const uintptr_t actor=g_Engine->GetObjectByIndex(i);if(!actor||actor==targetActor||!g_Engine->IsObjectLive(actor)||!g_Engine->IsA(actor,"Actor"))continue;
        const std::string cls=g_Engine->GetObjectClassName(actor),name=g_Engine->GetObjectName(actor);
        std::string key=cls+" "+name;std::transform(key.begin(),key.end(),key.begin(),[](unsigned char c){return char(std::tolower(c));});
        // Player-built pieces are the obstruction class missing from the old
        // storage-only selector. Include furniture and lodestones as well.
        if(key.find("building")==std::string::npos&&key.find("foundation")==std::string::npos&&
           key.find("floor")==std::string::npos&&key.find("wall")==std::string::npos&&
           key.find("roof")==std::string::npos&&key.find("storage")==std::string::npos&&
           key.find("chest")==std::string::npos&&key.find("crate")==std::string::npos&&
           key.find("bed")==std::string::npos&&key.find("lodestone")==std::string::npos&&
           key.find("campfire")==std::string::npos)continue;
        const uintptr_t root=Mem::ReadPtr(actor+Offsets::Actor_RootComponent);FVector3d location{};
        if(!root||!Mem::Readable(reinterpret_cast<void*>(root+Offsets::SceneComp_RelativeLocation),sizeof(location)))continue;
        std::memcpy(&location,reinterpret_cast<void*>(root+Offsets::SceneComp_RelativeLocation),sizeof(location));
        if(!std::isfinite(location.X)||std::hypot(location.X-center.X,location.Y-center.Y)>2200.0)continue;
        alignas(16) unsigned char params[64]{};params[0]=1;
        processEvent(reinterpret_cast<void*>(actor),reinterpret_cast<void*>(getActorBoundsFunction),params);
        NearbyObstacle obstacle;obstacle.actor=actor;obstacle.name=name;obstacle.cls=cls;
        std::memcpy(&obstacle.origin,params+0x8,sizeof(obstacle.origin));std::memcpy(&obstacle.extent,params+0x20,sizeof(obstacle.extent));
        if(!std::isfinite(obstacle.origin.X)||!std::isfinite(obstacle.extent.X)||obstacle.extent.X<1.0||obstacle.extent.Y<1.0||obstacle.extent.Z<1.0||
           obstacle.extent.X>5000.0||obstacle.extent.Y>5000.0||obstacle.extent.Z>5000.0)continue;
        result.push_back(std::move(obstacle));
    }
    return result;
}

bool CapsuleOverlapsObstacle(const FVector3d& candidate,const NearbyObstacle& obstacle){
    constexpr double CapsuleRadius=62.0,CapsuleHalfHeight=100.0,Separation=8.0;
    return std::abs(candidate.X-obstacle.origin.X)<obstacle.extent.X+CapsuleRadius+Separation&&
           std::abs(candidate.Y-obstacle.origin.Y)<obstacle.extent.Y+CapsuleRadius+Separation&&
           std::abs(candidate.Z-obstacle.origin.Z)<obstacle.extent.Z+CapsuleHalfHeight+Separation;
}

FVector3d ChooseStorageArrival(const FVector3d& origin,const FVector3d& extent,const PlayerInfo& player,
                               const std::vector<FVector3d>& storages,const std::vector<NearbyObstacle>& obstacles,
                               double& chosenRadius,double& nearestStorage,int& rejectedGeometry){
    constexpr double Pi=3.14159265358979323846;
    double toward=std::atan2(player.y-origin.Y,player.x-origin.X);
    if(!std::isfinite(toward))toward=player.yaw*Pi/180.0+Pi;
    const double baseRadius=std::hypot(extent.X,extent.Y)+100.0;
    const double additions[]={0.0,160.0,320.0,480.0,640.0,800.0};
    FVector3d overall{origin.X+std::cos(toward)*baseRadius,origin.Y+std::sin(toward)*baseRadius,origin.Z-extent.Z+100.0};
    double overallScore=-1.0;chosenRadius=baseRadius;nearestStorage=0.0;rejectedGeometry=0;
    for(double addition:additions){
        const double radius=baseRadius+addition;FVector3d ringBest{};double ringScore=-1.0;
        for(int i=0;i<16;++i){
            const double angle=toward+(2.0*Pi*i/16.0);
            FVector3d candidate{origin.X+std::cos(angle)*radius,origin.Y+std::sin(angle)*radius,origin.Z-extent.Z+100.0};
            double floorZ=0.0;if(TraceGround(candidate.X,candidate.Y,candidate.Z,floorZ)&&std::abs(floorZ-(origin.Z-extent.Z))<450.0)candidate.Z=floorZ+100.0;
            bool obstructed=false;for(const auto& obstacle:obstacles)if(CapsuleOverlapsObstacle(candidate,obstacle)){obstructed=true;break;}
            if(obstructed){++rejectedGeometry;continue;}
            double score=INFINITY;
            for(const auto& storage:storages){
                const double fromTarget=std::hypot(storage.X-origin.X,storage.Y-origin.Y);
                if(fromTarget<10.0||std::abs(storage.Z-candidate.Z)>350.0)continue;
                score=std::min(score,std::hypot(storage.X-candidate.X,storage.Y-candidate.Y));
            }
            if(!std::isfinite(score))score=1000000.0;
            if(score>ringScore){ringScore=score;ringBest=candidate;}
        }
        if(ringScore>overallScore){overallScore=ringScore;overall=ringBest;chosenRadius=radius;nearestStorage=ringScore;}
        // A 280 uu centre-to-centre gap gives the player capsule room beside
        // ordinary storage bounds without pushing clear arrivals unnecessarily far away.
        if(ringScore>=280.0){chosenRadius=radius;nearestStorage=ringScore;return ringBest;}
    }
    if(overallScore<0.0)return {NAN,NAN,NAN};
    return overall;
}

void RunOne(void* tickingPawn){
    Request request;
    {
        QueueLock lock;
        if(requests.empty())return;
        request=requests.front();
    }
    auto started=std::chrono::steady_clock::now();
    PlayerInfo player;bool found=false;
    for(const auto& candidate:g_Engine->GetAllPlayers()){
        if(candidate.characterGuid==request.characterGuid&&candidate.pawnPtr==reinterpret_cast<uintptr_t>(tickingPawn)){
            player=candidate;found=true;break;
        }
    }
    if(!found)return; // The matching pawn will drain it on its own tick.
    {
        QueueLock lock;
        if(requests.empty()||requests.front().destinationId!=request.destinationId||
           requests.front().characterGuid!=request.characterGuid)return;
        requests.pop_front();
    }
    if(player.isDead||!player.spawned){++rejected;LogMessage("Admin teleport rejected: player unavailable id="+request.publicId);return;}
    if(request.action=="storage"||request.action=="gravestone"){
        FVector3d chest{};uintptr_t chestActor=0;
        if(!bpTriggerDelayedTeleport||!storageTeleportTemplateValid.load()||!getActorBoundsFunction||
           !std::isfinite(request.preparedLocation.X)||!std::isfinite(request.preparedLocation.Y)||!std::isfinite(request.preparedLocation.Z)){
            ++rejected;++staleStorage;LogMessage("Admin storage teleport rejected: stale chest="+request.destinationId);return;
        }
        chest=request.preparedLocation;chestActor=request.preparedActor;
        if(chestActor&&(!g_Engine->IsObjectLive(chestActor)||g_Engine->GetObjectIdentity(chestActor)!=request.preparedActorIdentity)){
            ++rejected;++staleStorage;LogMessage("Admin storage teleport rejected: changed destination="+request.destinationId);return;
        }
        FVector3d origin=chest,extent{55.0,55.0,80.0};if(chestActor){alignas(16) unsigned char bounds[64]{};bounds[0]=1;processEvent(reinterpret_cast<void*>(chestActor),reinterpret_cast<void*>(getActorBoundsFunction),bounds);std::memcpy(&origin,bounds+0x8,sizeof(origin));std::memcpy(&extent,bounds+0x20,sizeof(extent));}
        if(!std::isfinite(origin.X)||!std::isfinite(origin.Y)||!std::isfinite(origin.Z)||!std::isfinite(extent.X)||!std::isfinite(extent.Y)||!std::isfinite(extent.Z)||
           extent.X<1.0||extent.Y<1.0||extent.Z<1.0||extent.X>3000.0||extent.Y>3000.0||extent.Z>3000.0){++rejected;++unsafeStorage;LogMessage("Admin storage teleport rejected: invalid authoritative bounds chest="+request.destinationId);return;}
        constexpr double Pi=3.14159265358979323846;
        double toward=std::atan2(player.y-origin.Y,player.x-origin.X);if(!std::isfinite(toward))toward=player.yaw*Pi/180.0+Pi;
        const double chosenRadius=std::hypot(extent.X,extent.Y)+120.0;
        FVector3d target{origin.X+std::cos(toward)*chosenRadius,origin.Y+std::sin(toward)*chosenRadius,origin.Z-extent.Z+100.0};
        double tracedFloor=0.0;const bool floorLoaded=TraceGround(target.X,target.Y,target.Z,tracedFloor)&&std::abs(tracedFloor-(origin.Z-extent.Z))<450.0;
        if(floorLoaded)target.Z=tracedFloor+100.0;
        const double playerDistance=std::hypot(target.X-player.x,target.Y-player.y);
        // Player distance is the reliable client-streaming indicator. The prior
        // floorLoaded dependency was always false on this build and forced a
        // loading screen even when the destination was already beside the player.
        const bool alreadyLoaded=playerDistance<=5000.0&&std::abs(target.Z-player.z)<=1500.0;
        alignas(16) unsigned char params[0x50]{};std::memcpy(params,capturedTeleportParams,sizeof(params));
        std::memcpy(params+0x10,&target,sizeof(target));
        const FRotator3d targetRotation{0.0,std::atan2(origin.Y-target.Y,origin.X-target.X)*180.0/3.14159265358979323846,0.0};
        std::memcpy(params+0x28,&targetRotation,sizeof(targetRotation));
        params[0x40]=alreadyLoaded?0:1;
        // Storage positions are dynamic rather than registry-authored safe
        // arrivals, so retain the game's destination validation.
        params[0x44]=0;
        const auto nativeStarted=std::chrono::steady_clock::now();
        const bool scheduled=bpTriggerDelayedTeleport(tickingPawn,params);
        const auto nativeMicros=std::uint64_t(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-nativeStarted).count());
        auto nativePrior=maximumStorageTeleportMicros.load();while(nativeMicros>nativePrior&&!maximumStorageTeleportMicros.compare_exchange_weak(nativePrior,nativeMicros)){}
        if(nativeMicros>=1000){++slowStorageTeleportCalls;storageTeleportCircuitBroken=true;}
        if(!scheduled){
            ++rejected;++unsafeStorage;
            LogMessage("Admin native storage teleport rejected by game location validation chest="+request.destinationId);
            return;
        }
        ++accepted;++acceptedStorage;
        LogMessage("Admin native delayed storage teleport scheduled character="+request.publicId+" owner="+request.targetPublicId+" chest="+request.destinationId+
                   " loaded="+(alreadyLoaded?"true":"false")+" distance="+std::to_string(playerDistance)+" ring="+std::to_string(chosenRadius)+
                   " nativeMicros="+std::to_string(nativeMicros)+" target="+std::to_string(target.X)+","+
                   std::to_string(target.Y)+","+std::to_string(target.Z)+" session="+request.sessionPrefix);
        return;
    }
    if(!request.action.empty()){
        PlayerInfo target;bool targetFound=false;
        for(const auto& candidate:g_Engine->GetAllPlayers())if(candidate.characterGuid==request.targetCharacterGuid){target=candidate;targetFound=true;break;}
        if(!targetFound||!target.spawned||target.isDead||!target.hasLocation){++rejected;LogMessage("Admin player teleport rejected: target unavailable id="+request.targetPublicId);return;}
        constexpr double Pi=3.14159265358979323846,Radius=150.0;
        const double base=target.yaw*Pi/180.0;bool moved=false;FVector3d landing{};
        for(int i=0;i<8&&!moved;++i){
            const double angle=base+Pi+(Pi*2.0*i/8.0);
            landing={target.x+std::cos(angle)*Radius,target.y+std::sin(angle)*Radius,target.z+10.0};
            const FRotator3d rotation{0.0,target.yaw,0.0};
            moved=actorTeleport(tickingPawn,&landing,&rotation,false,false);
        }
        if(!moved){++rejected;LogMessage("Admin player teleport rejected: no collision-safe arrival near target="+request.targetPublicId);return;}
        uintptr_t component=FindTeleportComponent(reinterpret_cast<uintptr_t>(tickingPawn),player.controllerPtr);
        uintptr_t function=g_Engine->FindFunction("BaseTeleportationComponent","Client_OnTeleportSuccessful");
        if(component&&function){FVector3d params=landing;processEvent(reinterpret_cast<void*>(component),reinterpret_cast<void*>(function),&params);}
        ++accepted;LogMessage("Admin player teleport accepted action="+request.action+" mover="+request.publicId+" target="+request.targetPublicId+" session="+request.sessionPrefix);
        return;
    }
    std::vector<LodestoneInfo> stones;const LodestoneInfo* destination=nullptr;
    if(ReadLodestones(stones))for(const auto& stone:stones)if(stone.id==request.destinationId){destination=&stone;break;}
    if(!destination||!std::isfinite(destination->worldLocation.X)||!std::isfinite(destination->worldLocation.Y)||
       !std::isfinite(destination->worldLocation.Z)){++rejected;LogMessage("Admin teleport rejected: stale lodestone id="+request.destinationId);return;}
    uintptr_t component=FindTeleportComponent(reinterpret_cast<uintptr_t>(tickingPawn),player.controllerPtr);
    const int32_t costOffset=VerifiedCostFlagOffset(component);
    if(!component||costOffset!=0x110){++rejected;compatibility=-7;LogMessage("Admin teleport disabled: exact lodestone cost flag layout unavailable");return;}
    const LodestoneInfo* source=nullptr;double nearest=INFINITY;
    for(const auto& stone:stones){
        if(stone.id==destination->id||!(stone.nativeGuid[0]|stone.nativeGuid[1]|stone.nativeGuid[2]|stone.nativeGuid[3]))continue;
        const double dx=stone.worldLocation.X-player.x,dy=stone.worldLocation.Y-player.y,dz=stone.worldLocation.Z-player.z;
        const double distance=dx*dx+dy*dy+dz*dz;if(distance<nearest){nearest=distance;source=&stone;}
    }
    if(!source){++rejected;LogMessage("Admin teleport rejected: no distinct authoritative source lodestone");return;}
    std::uint8_t oldCost=0;std::memcpy(&oldCost,reinterpret_cast<void*>(component+costOffset),1);
    const std::uint8_t disabled=0;std::memcpy(reinterpret_cast<void*>(component+costOffset),&disabled,1);
    ScopedBypass bypass; bypass.component=reinterpret_cast<void*>(component);
    std::memcpy(bypass.from,source->nativeGuid,16);std::memcpy(bypass.to,destination->nativeGuid,16);
    activeBypass=&bypass;
    nativeTryTeleport(reinterpret_cast<void*>(component),source->nativeGuid,destination->nativeGuid);
    activeBypass=nullptr;
    {
        QueueLock lock;
        pendingTeleports.push_back({request.characterGuid,request.publicId,request.destinationId,request.sessionPrefix,
                                    reinterpret_cast<uintptr_t>(tickingPawn),component,destination->arrivalLocation,
                                    costOffset,oldCost,false,{},{},{},std::chrono::steady_clock::now()});
    }
    LogMessage("Admin native lodestone teleport started character="+request.publicId+" session="+request.sessionPrefix+" destination="+request.destinationId);
    auto micros=static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-started).count());
    auto previous=maximumMicros.load();while(previous<micros&&!maximumMicros.compare_exchange_weak(previous,micros)){}
}

void HookTick(void* pawn,float delta,int levelTick,void* tickFunction){
    originalTick(pawn,delta,levelTick,tickFunction);
    const auto started=std::chrono::steady_clock::now();++tickCalls;
    FlushNativeTeleportCapture();
    ReconcilePending(pawn);
    bool pending=false;{QueueLock lock;pending=!requests.empty();}
    if(pending)RunOne(pawn);
    const auto elapsed=static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now()-started).count());
    auto prior=maximumTickMicros.load();while(elapsed>prior&&!maximumTickMicros.compare_exchange_weak(prior,elapsed)){}
    if(elapsed>=1000)++slowTickCalls;
}
}

bool InitializeAdminTeleport(uintptr_t moduleBase,uintptr_t moduleEnd){
    if(originalTick)return true;uintptr_t tickOffset=0,teleportOffset=0,processOffset=0,tryOffset=0,canOffset=0;
    if(!GetAdminTeleportOffsets(tickOffset,teleportOffset,processOffset,tryOffset,canOffset)){compatibility=-1;return false;}
    if(moduleBase+tickOffset+HookBytes>moduleEnd||moduleBase+teleportOffset>=moduleEnd||moduleBase+processOffset>=moduleEnd||
       moduleBase+tryOffset>=moduleEnd||moduleBase+canOffset+17>moduleEnd){compatibility=-2;return false;}
    const unsigned char canExpected[]={0x55,0x41,0x57,0x41,0x56,0x41,0x55,0x41,0x54,0x53,0x48,0x83,0xEC,0x48,0x49,0x89,0xD7,0x49,0x89,0xF6,0x48,0x89,0xFB,0xF3,0x0F,0x6F,0x06,0x66,0x0F,0x38,0x17,0xC0};
    const unsigned char tryExpected[]={0x55,0x41,0x57,0x41,0x56,0x41,0x55,0x41,0x54,0x53,0x48,0x81,0xEC,0x08,0x01,0x00,0x00,0x49,0x89,0xD6,0x49,0x89,0xF7,0x48,0x89,0xFB,0x48,0x8B,0x07,0xFF,0x90,0x80,0x05,0x00,0x00};
    auto* canTarget=reinterpret_cast<unsigned char*>(moduleBase+canOffset);
    auto* tryTarget=reinterpret_cast<unsigned char*>(moduleBase+tryOffset);
    if(std::memcmp(canTarget,canExpected,sizeof(canExpected))!=0||std::memcmp(tryTarget,tryExpected,sizeof(tryExpected))!=0){compatibility=-6;return false;}
    auto* target=reinterpret_cast<unsigned char*>(moduleBase+tickOffset);
    const unsigned char expected[HookBytes]={0x55,0x41,0x57,0x41,0x56,0x53,0x48,0x83,0xEC,0x28,0xF3,0x0F,0x11,0x44,0x24,0x24};
    if(std::memcmp(target,expected,sizeof(expected))!=0){compatibility=-3;return false;}
    auto* trampoline=static_cast<unsigned char*>(mmap(nullptr,64,PROT_READ|PROT_WRITE|PROT_EXEC,MAP_PRIVATE|MAP_ANONYMOUS,-1,0));
    if(trampoline==MAP_FAILED){compatibility=-4;return false;}
    std::memcpy(trampoline,target,HookBytes);WriteJump(trampoline+HookBytes,reinterpret_cast<uintptr_t>(target+HookBytes));
    originalTick=reinterpret_cast<TickFn>(trampoline);actorTeleport=reinterpret_cast<TeleportFn>(moduleBase+teleportOffset);
    processEvent=reinterpret_cast<ProcessEventFn>(moduleBase+processOffset);
    groundTraceCompatibility=InitializeGroundTrace()?1:-1;
    if(groundTraceCompatibility.load()!=1)LogMessage("Admin storage teleport ground-trace compatibility unavailable; storage teleports will fail closed");
    storagePipelineCompatibility=InitializeStorageTeleportPipeline()?1:-1;
    if(storagePipelineCompatibility.load()!=1)LogMessage("Admin storage single-stage teleport compatibility unavailable; storage teleports will fail closed");
    long pageSize=sysconf(_SC_PAGESIZE);uintptr_t page=reinterpret_cast<uintptr_t>(target)&~(uintptr_t(pageSize)-1);
    if(mprotect(reinterpret_cast<void*>(page),size_t(pageSize),PROT_READ|PROT_WRITE|PROT_EXEC)!=0){originalTick=nullptr;munmap(trampoline,64);compatibility=-5;return false;}
    unsigned char patch[HookBytes];std::memset(patch,0x90,sizeof(patch));WriteJump(patch,reinterpret_cast<uintptr_t>(&HookTick));
    std::memcpy(target,patch,sizeof(patch));__builtin___clear_cache(reinterpret_cast<char*>(target),reinterpret_cast<char*>(target+HookBytes));
    mprotect(reinterpret_cast<void*>(page),size_t(pageSize),PROT_READ|PROT_EXEC);

    auto* canTrampoline=static_cast<unsigned char*>(mmap(nullptr,64,PROT_READ|PROT_WRITE|PROT_EXEC,MAP_PRIVATE|MAP_ANONYMOUS,-1,0));
    if(canTrampoline==MAP_FAILED){compatibility=-4;return false;}
    constexpr size_t CanHookBytes=17;
    std::memcpy(canTrampoline,canTarget,CanHookBytes);WriteJump(canTrampoline+CanHookBytes,reinterpret_cast<uintptr_t>(canTarget+CanHookBytes));
    originalCanTeleport=reinterpret_cast<CanTeleportFn>(canTrampoline);
    uintptr_t canPage=reinterpret_cast<uintptr_t>(canTarget)&~(uintptr_t(pageSize)-1);
    if(mprotect(reinterpret_cast<void*>(canPage),size_t(pageSize),PROT_READ|PROT_WRITE|PROT_EXEC)!=0){compatibility=-5;return false;}
    unsigned char canPatch[CanHookBytes];std::memset(canPatch,0x90,sizeof(canPatch));WriteJump(canPatch,reinterpret_cast<uintptr_t>(&HookCanTeleport));
    std::memcpy(canTarget,canPatch,sizeof(canPatch));__builtin___clear_cache(reinterpret_cast<char*>(canTarget),reinterpret_cast<char*>(canTarget+sizeof(canPatch)));
    mprotect(reinterpret_cast<void*>(canPage),size_t(pageSize),PROT_READ|PROT_EXEC);
    nativeTryTeleport=reinterpret_cast<TryTeleportFn>(moduleBase+tryOffset);
    if(!LoadCapturedTeleportTemplate())
        LogMessage("No validated native teleport template is available; storage teleport remains disabled");
    nativeCaptureCompatibility=InitializeNativeTeleportCapture(moduleBase,moduleEnd,tryOffset)?1:-1;
    if(nativeCaptureCompatibility.load()!=1)
        LogMessage("Native delayed teleport parameter capture unavailable; storage teleport remains disabled");
    LogTeleportReflection();
    compatibility=1;return true;
}

bool QueueAdminTeleport(const std::string& characterGuid,const std::string& publicId,
                        const std::string& destinationId,const std::string& sessionPrefix){
    if(compatibility.load()!=1||characterGuid.empty()||destinationId.empty())return false;
    QueueLock lock;
    if(requests.size()>=QueueCapacity||std::any_of(requests.begin(),requests.end(),[&](const Request&r){return r.characterGuid==characterGuid;})||
       std::any_of(pendingTeleports.begin(),pendingTeleports.end(),[&](const PendingTeleport&p){return p.characterGuid==characterGuid;}))return false;
    Request request;request.characterGuid=characterGuid;request.publicId=publicId;request.destinationId=destinationId;request.sessionPrefix=sessionPrefix;
    requests.push_back(std::move(request));++queued;return true;
}

bool QueueAdminPlayerTeleport(const std::string& movingCharacterGuid,const std::string& movingPublicId,
                              const std::string& targetCharacterGuid,const std::string& targetPublicId,
                              const std::string& action,const std::string& sessionPrefix){
    if(compatibility.load()!=1||movingCharacterGuid.empty()||targetCharacterGuid.empty()||movingCharacterGuid==targetCharacterGuid||
       (action!="to-player"&&action!="player-to-me"))return false;
    QueueLock lock;
    if(requests.size()>=QueueCapacity||std::any_of(requests.begin(),requests.end(),[&](const Request&r){return r.characterGuid==movingCharacterGuid;})||
       std::any_of(pendingTeleports.begin(),pendingTeleports.end(),[&](const PendingTeleport&p){return p.characterGuid==movingCharacterGuid;}))return false;
    Request request;request.characterGuid=movingCharacterGuid;request.publicId=movingPublicId;request.sessionPrefix=sessionPrefix;request.targetCharacterGuid=targetCharacterGuid;request.targetPublicId=targetPublicId;request.action=action;
    requests.push_back(std::move(request));++queued;return true;
}

bool QueueAdminStorageTeleport(const std::string& characterGuid,const std::string& publicId,
                               const std::string& targetCharacterGuid,const std::string& targetPublicId,
                               const std::string& chestId,const std::string& sessionPrefix){
    if(!bpTriggerDelayedTeleport||!storageTeleportTemplateValid.load()||compatibility.load()!=1||groundTraceCompatibility.load()!=1||storagePipelineCompatibility.load()!=1||nativeCaptureCompatibility.load()!=1||storageTeleportCircuitBroken.load()||characterGuid.empty()||targetCharacterGuid.empty()||chestId.empty())return false;
    uintptr_t actor=0;FVector3d location{};if(!ResolveAdminStorageActor(targetCharacterGuid,chestId,actor,location))return false;
    const auto identity=actor&&g_Engine?g_Engine->GetObjectIdentity(actor):0;if(!actor||!identity)return false;
    QueueLock lock;
    if(requests.size()>=QueueCapacity||std::any_of(requests.begin(),requests.end(),[&](const Request&r){return r.characterGuid==characterGuid;})||
       std::any_of(pendingTeleports.begin(),pendingTeleports.end(),[&](const PendingTeleport&p){return p.characterGuid==characterGuid;}))return false;
    Request request;request.characterGuid=characterGuid;request.publicId=publicId;request.destinationId=chestId;request.sessionPrefix=sessionPrefix;request.targetCharacterGuid=targetCharacterGuid;request.targetPublicId=targetPublicId;request.action="storage";request.preparedActor=actor;request.preparedActorIdentity=identity;request.preparedLocation=location;
    requests.push_back(std::move(request));++queued;return true;
}

bool QueueAdminGravestoneTeleport(const std::string& characterGuid,const std::string& publicId,
                                  const std::string& ownerCharacterGuid,const std::string& ownerPublicId,
                                  const std::string& graveId,const std::string& sessionPrefix){
    if(!bpTriggerDelayedTeleport||!storageTeleportTemplateValid.load()||compatibility.load()!=1||groundTraceCompatibility.load()!=1||storagePipelineCompatibility.load()!=1||nativeCaptureCompatibility.load()!=1||storageTeleportCircuitBroken.load()||characterGuid.empty()||ownerCharacterGuid.empty()||graveId.empty())return false;
    uintptr_t actor=0;FVector3d location{};if(!ResolveAdminGravestoneLocation(ownerCharacterGuid,graveId,actor,location))return false;
    const auto identity=actor&&g_Engine?g_Engine->GetObjectIdentity(actor):0;
    QueueLock lock;
    if(requests.size()>=QueueCapacity||std::any_of(requests.begin(),requests.end(),[&](const Request&r){return r.characterGuid==characterGuid;})||
       std::any_of(pendingTeleports.begin(),pendingTeleports.end(),[&](const PendingTeleport&p){return p.characterGuid==characterGuid;}))return false;
    Request request;request.characterGuid=characterGuid;request.publicId=publicId;request.destinationId=graveId;request.sessionPrefix=sessionPrefix;request.targetCharacterGuid=ownerCharacterGuid;request.targetPublicId=ownerPublicId;request.action="gravestone";request.preparedActor=actor;request.preparedActorIdentity=identity;request.preparedLocation=location;
    requests.push_back(std::move(request));++queued;return true;
}

AdminTeleportDiagnostics GetAdminTeleportDiagnostics(){
    AdminTeleportDiagnostics d;d.compatibility=compatibility.load();d.lodestoneCompatibility=d.compatibility;d.playerCompatibility=d.compatibility;
    d.groundTraceCompatibility=groundTraceCompatibility.load();d.storagePipelineCompatibility=storagePipelineCompatibility.load();
    d.nativeCaptureCompatibility=nativeCaptureCompatibility.load();d.nativeTemplateAvailable=storageTeleportTemplateValid.load()?1:0;
    d.storageTransportCompatibility=storageTeleportCircuitBroken.load()?-1:(d.nativeCaptureCompatibility==1&&d.nativeTemplateAvailable==1?1:0);d.queuedRequests=queued.load();
    d.acceptedTeleports=accepted.load();d.rejectedRequests=rejected.load();d.maximumExecutionMicroseconds=maximumMicros.load();
    d.acceptedStorageTeleports=acceptedStorage.load();d.staleStorageDestinations=staleStorage.load();d.unsafeStorageDestinations=unsafeStorage.load();
    d.tickCalls=tickCalls.load();d.maximumTickMicroseconds=maximumTickMicros.load();d.slowTickCalls=slowTickCalls.load();
    d.maximumStorageTransportMicroseconds=maximumStorageTeleportMicros.load();d.slowStorageTransportCalls=slowStorageTeleportCalls.load();return d;
}
}
