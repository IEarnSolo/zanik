#include "server_performance_store.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <fcntl.h>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

namespace {
constexpr std::size_t MaximumResponsePoints = 1200;
struct Lock{explicit Lock(pthread_mutex_t*mutex):mutex_(mutex){pthread_mutex_lock(mutex_);}~Lock(){pthread_mutex_unlock(mutex_);}pthread_mutex_t*mutex_;};

std::uint64_t ReadCpuTicks() {
    std::ifstream input("/proc/self/stat");std::string line;if(!std::getline(input,line))return 0;
    const auto close=line.rfind(')');if(close==std::string::npos)return 0;
    std::istringstream fields(line.substr(close+2));std::string value;std::vector<std::string> values;
    while(fields>>value)values.push_back(value);
    if(values.size()<13)return 0;
    try{return std::stoull(values[11])+std::stoull(values[12]);}catch(...){return 0;}
}

double ReadRssMiB() {
    std::ifstream input("/proc/self/status");std::string key;
    while(input>>key){if(key=="VmRSS:"){double kib=0;input>>kib;return kib/1024.0;}std::string rest;std::getline(input,rest);}return 0;
}

double ReadLoadOne() { std::ifstream input("/proc/loadavg");double value=0;input>>value;return value; }

void AtomicMaximum(std::atomic<std::uint64_t>& target,std::uint64_t value){auto current=target.load();while(value>current&&!target.compare_exchange_weak(current,value)){} }

std::string Number(double value) {
    if(!std::isfinite(value)||value<0)return "null";
    std::ostringstream out;out<<std::fixed<<std::setprecision(2)<<value;return out.str();
}
}

ServerPerformanceStore::ServerPerformanceStore(std::string path):path_(std::move(path)){}

std::uint64_t ServerPerformanceStore::Now(){return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();}

bool ServerPerformanceStore::Start(std::uint64_t timestamp){
    Lock lock(&mutex_);sessionStartedAt_=timestamp?timestamp:Now();points_.clear();
    const int fd=open(path_.c_str(),O_WRONLY|O_CREAT|O_TRUNC|O_CLOEXEC,0600);
    if(fd<0){persistenceHealthy_=false;++persistenceFailures_;return false;}
    const std::string header="# zanik-server-performance-v1\nS\t"+std::to_string(sessionStartedAt_)+"\n";
    const bool okay=write(fd,header.data(),header.size())==static_cast<ssize_t>(header.size())&&fsync(fd)==0;
    close(fd);chmod(path_.c_str(),0600);persistenceHealthy_=okay;if(!okay)++persistenceFailures_;return okay;
}

void ServerPerformanceStore::RecordHttpDuration(std::uint64_t microseconds){httpMicros_.fetch_add(microseconds);httpRequests_.fetch_add(1);AtomicMaximum(httpMaximumMicros_,microseconds);}

bool ServerPerformanceStore::Append(const Point& p){
    std::ostringstream row;row<<"P\t"<<p.timestamp<<'\t'<<p.gameFrameMs<<'\t'<<p.zanikGameThreadMs<<'\t'<<p.hitches<<'\t'<<p.averagePingMs<<'\t'<<p.maximumPingMs<<'\t'<<p.onlinePlayers<<'\t'<<p.processCpuPercent<<'\t'<<p.rssMiB<<'\t'<<p.loadOne<<'\t'<<p.httpAverageMs<<'\t'<<p.httpMaximumMs<<'\t'<<p.httpRequests<<'\n';
    const std::string data=row.str();const int fd=open(path_.c_str(),O_WRONLY|O_APPEND|O_CLOEXEC);
    if(fd<0)return false;const bool okay=write(fd,data.data(),data.size())==static_cast<ssize_t>(data.size());close(fd);return okay;
}

bool ServerPerformanceStore::Sample(const ServerPerformanceInput& input,std::uint64_t timestamp){
    Point p;p.timestamp=timestamp?timestamp:Now();p.gameFrameMs=input.gameFrameMs;p.zanikGameThreadMs=input.zanikGameThreadMs;p.hitches=input.gameThreadHitches;p.averagePingMs=input.averagePingMs;p.maximumPingMs=input.maximumPingMs;p.onlinePlayers=input.onlinePlayers;
    const auto nowMs=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();const auto ticks=ReadCpuTicks();const long ticksPerSecond=sysconf(_SC_CLK_TCK);
    if(previousCpuTimestampMs_&&ticks>=previousCpuTicks_&&ticksPerSecond>0){const auto elapsed=nowMs-previousCpuTimestampMs_;if(elapsed>0)p.processCpuPercent=double(ticks-previousCpuTicks_)*100000.0/(double(ticksPerSecond)*double(elapsed));}
    previousCpuTicks_=ticks;previousCpuTimestampMs_=nowMs;p.rssMiB=ReadRssMiB();p.loadOne=ReadLoadOne();
    p.httpRequests=httpRequests_.exchange(0);const auto total=httpMicros_.exchange(0);p.httpMaximumMs=double(httpMaximumMicros_.exchange(0))/1000.0;p.httpAverageMs=p.httpRequests?double(total)/double(p.httpRequests)/1000.0:0;
    Lock lock(&mutex_);points_.push_back(p);const bool okay=Append(p);persistenceHealthy_=okay;if(!okay)++persistenceFailures_;return okay;
}

ServerPerformanceStore::Point ServerPerformanceStore::Aggregate(const std::vector<Point>& values,std::size_t begin,std::size_t end) const {
    Point out=values[end-1];double pingTotal=0,httpTotal=0;std::uint64_t pingCount=0,httpCount=0;out.gameFrameMs=-1;out.zanikGameThreadMs=-1;out.maximumPingMs=-1;out.processCpuPercent=0;out.loadOne=0;out.httpMaximumMs=0;out.hitches=0;out.onlinePlayers=0;
    for(std::size_t i=begin;i<end;++i){const auto&p=values[i];out.gameFrameMs=std::max(out.gameFrameMs,p.gameFrameMs);out.zanikGameThreadMs=std::max(out.zanikGameThreadMs,p.zanikGameThreadMs);out.maximumPingMs=std::max(out.maximumPingMs,p.maximumPingMs);out.processCpuPercent=std::max(out.processCpuPercent,p.processCpuPercent);out.loadOne=std::max(out.loadOne,p.loadOne);out.httpMaximumMs=std::max(out.httpMaximumMs,p.httpMaximumMs);out.hitches+=p.hitches;out.onlinePlayers=std::max(out.onlinePlayers,p.onlinePlayers);if(p.averagePingMs>=0){pingTotal+=p.averagePingMs;++pingCount;}httpTotal+=p.httpAverageMs*p.httpRequests;httpCount+=p.httpRequests;}
    out.averagePingMs=pingCount?pingTotal/pingCount:-1;out.httpRequests=httpCount;out.httpAverageMs=httpCount?httpTotal/httpCount:0;return out;
}

std::string ServerPerformanceStore::Json(std::uint64_t windowSeconds) const {
    Lock lock(&mutex_);const auto now=Now();const auto cutoff=windowSeconds&&now>windowSeconds?now-windowSeconds:0;std::vector<Point> filtered;for(const auto&p:points_)if(p.timestamp>=cutoff)filtered.push_back(p);
    std::vector<Point> displayed;if(filtered.size()<=MaximumResponsePoints)displayed=filtered;else{const double width=double(filtered.size())/MaximumResponsePoints;for(std::size_t i=0;i<MaximumResponsePoints;++i){auto begin=std::size_t(std::floor(i*width));auto end=std::size_t(std::floor((i+1)*width));if(end<=begin)end=begin+1;if(end>filtered.size())end=filtered.size();displayed.push_back(Aggregate(filtered,begin,end));}}
    double maxFrame=-1,maxHook=-1,maxPing=-1,maxHttp=0,maxCpu=0,maxRss=0,maxLoad=0;std::uint64_t hitches=0;for(const auto&p:filtered){maxFrame=std::max(maxFrame,p.gameFrameMs);maxHook=std::max(maxHook,p.zanikGameThreadMs);maxPing=std::max(maxPing,p.maximumPingMs);maxHttp=std::max(maxHttp,p.httpMaximumMs);maxCpu=std::max(maxCpu,p.processCpuPercent);maxRss=std::max(maxRss,p.rssMiB);maxLoad=std::max(maxLoad,p.loadOne);hitches+=p.hitches;}
    struct Insight{std::uint64_t at;int rank;std::string title,detail,severity;};std::vector<Insight> insights;
    for(const auto&p:filtered){if(p.gameFrameMs>=50){const bool zanik=p.zanikGameThreadMs>=2&&p.zanikGameThreadMs>=p.gameFrameMs*.2;insights.push_back({p.timestamp,int(p.gameFrameMs),zanik?"Zanik game-thread work coincided with a hitch":"Engine or game workload caused a frame hitch",zanik?"Zanik's bounded game-thread work consumed a material share of this slow frame. Check recipe, quest, storage, and gravestone activity at this time.":"The game frame was slow while Zanik's measured hook stayed comparatively small, pointing to engine simulation, world streaming, saves, or another mod.",p.gameFrameMs>=100?"critical":"warning"});}if(p.maximumPingMs>=200)insights.push_back({p.timestamp,int(p.maximumPingMs),"High player network latency","At least one connected player's authoritative game ping was high. If game-frame time stayed healthy, this points to that player's route or the host network rather than server simulation.",p.maximumPingMs>=350?"critical":"warning"});if(p.httpMaximumMs>=500)insights.push_back({p.timestamp,int(p.httpMaximumMs),"Slow diagnostics/web request",p.processCpuPercent>=90||p.loadOne>=double(std::max(1L,sysconf(_SC_NPROCESSORS_ONLN)))*.9?"The request coincided with high host pressure, so CPU contention is the likely source.":"The web worker was slow without high sampled host pressure; a large response, slow client, or filesystem operation is more likely.",p.httpMaximumMs>=1500?"critical":"warning"});}
    std::sort(insights.begin(),insights.end(),[](const auto&a,const auto&b){return a.rank>b.rank;});if(insights.size()>12)insights.resize(12);
    std::ostringstream json;json<<"{\"sessionStartedAt\":"<<sessionStartedAt_<<",\"generatedAt\":"<<now<<",\"windowSeconds\":"<<windowSeconds<<",\"persistenceHealthy\":"<<(persistenceHealthy_?"true":"false")<<",\"persistenceFailures\":"<<persistenceFailures_<<",\"sampleIntervalSeconds\":10,\"hostCpuCount\":"<<std::max(1L,sysconf(_SC_NPROCESSORS_ONLN))<<",\"summary\":{\"samples\":"<<filtered.size()<<",\"maximumGameFrameMs\":"<<Number(maxFrame)<<",\"maximumZanikGameThreadMs\":"<<Number(maxHook)<<",\"maximumPingMs\":"<<Number(maxPing)<<",\"maximumHttpMs\":"<<Number(maxHttp)<<",\"maximumCpuPercent\":"<<Number(maxCpu)<<",\"maximumRssMiB\":"<<Number(maxRss)<<",\"maximumLoadOne\":"<<Number(maxLoad)<<",\"hitches\":"<<hitches<<"},\"points\":[";
    for(std::size_t i=0;i<displayed.size();++i){const auto&p=displayed[i];if(i)json<<',';json<<"{\"timestamp\":"<<p.timestamp<<",\"gameFrameMs\":"<<Number(p.gameFrameMs)<<",\"zanikGameThreadMs\":"<<Number(p.zanikGameThreadMs)<<",\"hitches\":"<<p.hitches<<",\"averagePingMs\":"<<Number(p.averagePingMs)<<",\"maximumPingMs\":"<<Number(p.maximumPingMs)<<",\"onlinePlayers\":"<<p.onlinePlayers<<",\"processCpuPercent\":"<<Number(p.processCpuPercent)<<",\"rssMiB\":"<<Number(p.rssMiB)<<",\"loadOne\":"<<Number(p.loadOne)<<",\"httpAverageMs\":"<<Number(p.httpAverageMs)<<",\"httpMaximumMs\":"<<Number(p.httpMaximumMs)<<",\"httpRequests\":"<<p.httpRequests<<'}';}json<<"],\"insights\":[";
    for(std::size_t i=0;i<insights.size();++i){const auto&v=insights[i];if(i)json<<',';json<<"{\"timestamp\":"<<v.at<<",\"severity\":\""<<v.severity<<"\",\"title\":\""<<v.title<<"\",\"detail\":\""<<v.detail<<"\"}";}json<<"]}";return json.str();
}

ServerPerformanceDiagnostics ServerPerformanceStore::Diagnostics() const {Lock lock(&mutex_);return {sessionStartedAt_,points_.size(),persistenceFailures_,persistenceHealthy_};}
