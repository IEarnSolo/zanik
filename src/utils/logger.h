#pragma once

#include <array>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <string>
#include "runtime_paths.h"

namespace ZanikLog {

constexpr std::size_t QueueCapacity = 256;
constexpr std::size_t MessageCapacity = 768;

struct Slot {
    std::atomic<std::size_t> sequence{0};
    std::array<char,MessageCapacity> message{};
};

struct Queue {
    std::array<Slot,QueueCapacity> slots{};
    std::atomic<std::size_t> enqueue{0};
    std::size_t dequeue=0;
    std::atomic<unsigned long long> dropped{0};
    Queue(){for(std::size_t i=0;i<slots.size();++i)slots[i].sequence.store(i,std::memory_order_relaxed);}
};

inline Queue& DeferredQueue(){static Queue queue;return queue;}

inline bool Enqueue(const char* message){
    auto& queue=DeferredQueue();std::size_t position=queue.enqueue.load(std::memory_order_relaxed);
    while(true){
        Slot& slot=queue.slots[position%QueueCapacity];const std::size_t sequence=slot.sequence.load(std::memory_order_acquire);
        const auto difference=static_cast<std::intptr_t>(sequence)-static_cast<std::intptr_t>(position);
        if(difference==0){if(queue.enqueue.compare_exchange_weak(position,position+1,std::memory_order_relaxed)){
            std::snprintf(slot.message.data(),slot.message.size(),"%s",message?message:"");
            slot.sequence.store(position+1,std::memory_order_release);return true;
        }}else if(difference<0){++queue.dropped;return false;}else position=queue.enqueue.load(std::memory_order_relaxed);
    }
}

inline unsigned long long Dropped(){return DeferredQueue().dropped.load(std::memory_order_relaxed);}

}

inline void LogMessage(const std::string& message) {
    ZanikLog::Enqueue(message.c_str());
}

inline void FlushLogMessages() {
    auto& queue=ZanikLog::DeferredQueue();FILE* file=nullptr;bool attempted=false;
    while(true){
        auto& slot=queue.slots[queue.dequeue%ZanikLog::QueueCapacity];
        const std::size_t sequence=slot.sequence.load(std::memory_order_acquire);
        if(static_cast<std::intptr_t>(sequence)-static_cast<std::intptr_t>(queue.dequeue+1)!=0)break;
        std::fprintf(stderr,"[DWStatus] %s\n",slot.message.data());
        if(!attempted){const std::string path=RuntimePaths::DataFile("runtime.log");file=std::fopen(path.c_str(),"a");attempted=true;}
        if(file)std::fprintf(file,"[DWStatus] %s\n",slot.message.data());
        slot.sequence.store(queue.dequeue+ZanikLog::QueueCapacity,std::memory_order_release);++queue.dequeue;
    }
    if(file)std::fclose(file);
}

inline unsigned long long DroppedLogMessages(){return ZanikLog::Dropped();}

inline void LogVerbose(const std::string&) {}

inline std::string HexString(unsigned long long value) {
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "0x%llX", value);
    return buffer;
}
