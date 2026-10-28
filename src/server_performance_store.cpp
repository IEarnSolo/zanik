#include "server_performance_store.h"

#include <algorithm>
#include <cerrno>
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
constexpr std::size_t MaximumRetainedPoints = 100000;
constexpr std::uint64_t SampleIntervalSeconds = 2;
constexpr std::uint64_t IncidentMergeGapSeconds = 6;
struct Lock { explicit Lock(pthread_mutex_t* mutex):mutex_(mutex){pthread_mutex_lock(mutex_);} ~Lock(){pthread_mutex_unlock(mutex_);} pthread_mutex_t* mutex_; };

bool WriteAll(int fd,const char* data,std::size_t size){
    while(size){const auto written=write(fd,data,size);if(written<0){if(errno==EINTR)continue;return false;}if(!written)return false;data+=written;size-=static_cast<std::size_t>(written);}return true;
}

std::uint64_t ReadCpuTicks(){
    std::ifstream input("/proc/self/stat");std::string line;if(!std::getline(input,line))return 0;const auto close=line.rfind(')');if(close==std::string::npos)return 0;
    std::istringstream fields(line.substr(close+2));std::string value;std::vector<std::string> values;while(fields>>value)values.push_back(value);if(values.size()<13)return 0;
    try{return std::stoull(values[11])+std::stoull(values[12]);}catch(...){return 0;}
}

double ReadRssMiB(){std::ifstream input("/proc/self/status");std::string key;while(input>>key){if(key=="VmRSS:"){double kib=0;input>>kib;return kib/1024.0;}std::string rest;std::getline(input,rest);}return 0;}
double ReadLoadOne(){std::ifstream input("/proc/loadavg");double value=0;input>>value;return value;}
void AtomicMaximum(std::atomic<std::uint64_t>& target,std::uint64_t value){auto current=target.load();while(value>current&&!target.compare_exchange_weak(current,value)){}}
std::string Number(double value){if(!std::isfinite(value)||value<0)return "null";std::ostringstream out;out<<std::fixed<<std::setprecision(2)<<value;return out.str();}
}

ServerPerformanceStore::ServerPerformanceStore(std::string path):path_(std::move(path)){}
ServerPerformanceStore::~ServerPerformanceStore(){if(fileDescriptor_>=0)close(fileDescriptor_);}
std::uint64_t ServerPerformanceStore::Now(){return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();}

bool ServerPerformanceStore::Start(std::uint64_t timestamp){
    Lock lock(&mutex_);sessionStartedAt_=timestamp?timestamp:Now();points_.clear();previousCpuTicks_=0;previousCpuTimestampMs_=0;
    if(fileDescriptor_>=0)close(fileDescriptor_);
    fileDescriptor_=open(path_.c_str(),O_WRONLY|O_CREAT|O_TRUNC|O_CLOEXEC,0600);
    if(fileDescriptor_<0){persistenceHealthy_=false;++persistenceFailures_;return false;}
    const std::string header="# zanik-server-performance-v2\nS\t"+std::to_string(sessionStartedAt_)+"\n";const bool okay=WriteAll(fileDescriptor_,header.data(),header.size())&&fsync(fileDescriptor_)==0;
    chmod(path_.c_str(),0600);persistenceHealthy_=okay;if(!okay){++persistenceFailures_;close(fileDescriptor_);fileDescriptor_=-1;}return okay;
}

void ServerPerformanceStore::RecordHttpDuration(std::uint64_t microseconds){httpMicros_.fetch_add(microseconds);httpRequests_.fetch_add(1);AtomicMaximum(httpMaximumMicros_,microseconds);}

bool ServerPerformanceStore::Append(const Point& p){
    std::ostringstream row;row<<"P\t"<<p.timestamp<<'\t'<<p.engineTickMs<<'\t'<<p.zanikGameThreadMs<<'\t'<<p.engineTicks<<'\t'<<p.hitches<<'\t'<<p.averagePingMs<<'\t'<<p.maximumPingMs<<'\t'<<p.onlinePlayers<<'\t'<<p.processCpuPercent<<'\t'<<p.rssMiB<<'\t'<<p.loadOne<<'\t'<<p.httpAverageMs<<'\t'<<p.httpMaximumMs<<'\t'<<p.httpRequests<<'\n';
    const std::string data=row.str();return fileDescriptor_>=0&&WriteAll(fileDescriptor_,data.data(),data.size());
}

bool ServerPerformanceStore::Sample(const ServerPerformanceInput& input,std::uint64_t timestamp){
    Point p;p.timestamp=timestamp?timestamp:Now();p.engineTickMs=input.engineTickMs;p.zanikGameThreadMs=input.zanikGameThreadMs;p.engineTicks=input.engineTicks;p.hitches=input.gameThreadHitches;p.averagePingMs=input.averagePingMs;p.maximumPingMs=input.maximumPingMs;p.onlinePlayers=input.onlinePlayers;
    const auto nowMs=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();const auto ticks=ReadCpuTicks();const long ticksPerSecond=sysconf(_SC_CLK_TCK);
    if(previousCpuTimestampMs_&&ticks>=previousCpuTicks_&&ticksPerSecond>0){const auto elapsed=nowMs-previousCpuTimestampMs_;if(elapsed>0)p.processCpuPercent=double(ticks-previousCpuTicks_)*100000.0/(double(ticksPerSecond)*double(elapsed));}
    previousCpuTicks_=ticks;previousCpuTimestampMs_=nowMs;p.rssMiB=ReadRssMiB();p.loadOne=ReadLoadOne();p.httpRequests=httpRequests_.exchange(0);const auto total=httpMicros_.exchange(0);p.httpMaximumMs=double(httpMaximumMicros_.exchange(0))/1000.0;p.httpAverageMs=p.httpRequests?double(total)/double(p.httpRequests)/1000.0:0;
    Lock lock(&mutex_);points_.push_back(p);Compact();const bool okay=Append(p);persistenceHealthy_=okay;if(!okay)++persistenceFailures_;return okay;
}

void ServerPerformanceStore::Compact(){
    if(points_.size()<=MaximumRetainedPoints)return;
    constexpr std::size_t count=MaximumRetainedPoints/2;std::vector<Point> compacted;compacted.reserve(points_.size()-count/2);
    for(std::size_t i=0;i<count;i+=2)compacted.push_back(Aggregate(points_,i,i+2));
    compacted.insert(compacted.end(),points_.begin()+count,points_.end());points_.swap(compacted);
}

ServerPerformanceStore::Point ServerPerformanceStore::Aggregate(const std::vector<Point>& values,std::size_t begin,std::size_t end) const {
    Point out=values[end-1];double pingTotal=0,httpTotal=0;std::uint64_t pingCount=0,httpCount=0;out.engineTickMs=-1;out.zanikGameThreadMs=-1;out.maximumPingMs=-1;out.processCpuPercent=0;out.loadOne=0;out.httpMaximumMs=0;out.engineTicks=0;out.hitches=0;out.onlinePlayers=0;out.samples=0;
    for(std::size_t i=begin;i<end;++i){const auto& p=values[i];out.engineTickMs=std::max(out.engineTickMs,p.engineTickMs);out.zanikGameThreadMs=std::max(out.zanikGameThreadMs,p.zanikGameThreadMs);out.maximumPingMs=std::max(out.maximumPingMs,p.maximumPingMs);out.processCpuPercent=std::max(out.processCpuPercent,p.processCpuPercent);out.loadOne=std::max(out.loadOne,p.loadOne);out.httpMaximumMs=std::max(out.httpMaximumMs,p.httpMaximumMs);out.engineTicks+=p.engineTicks;out.hitches+=p.hitches;out.onlinePlayers=std::max(out.onlinePlayers,p.onlinePlayers);out.samples+=p.samples;if(p.averagePingMs>=0){pingTotal+=p.averagePingMs*p.samples;pingCount+=p.samples;}httpTotal+=p.httpAverageMs*p.httpRequests;httpCount+=p.httpRequests;}
    out.averagePingMs=pingCount?pingTotal/pingCount:-1;out.httpRequests=httpCount;out.httpAverageMs=httpCount?httpTotal/httpCount:0;return out;
}

std::string ServerPerformanceStore::Json(std::uint64_t windowSeconds) const {
    Lock lock(&mutex_);const auto now=Now();const auto cutoff=windowSeconds&&now>windowSeconds?now-windowSeconds:0;std::vector<Point> filtered;for(const auto& p:points_)if(p.timestamp>=cutoff)filtered.push_back(p);
    std::vector<Point> displayed;if(filtered.size()<=MaximumResponsePoints)displayed=filtered;else{const double width=double(filtered.size())/MaximumResponsePoints;for(std::size_t i=0;i<MaximumResponsePoints;++i){auto begin=std::size_t(std::floor(i*width));auto end=std::size_t(std::floor((i+1)*width));if(end<=begin)end=begin+1;if(end>filtered.size())end=filtered.size();displayed.push_back(Aggregate(filtered,begin,end));}}
    double maxEngine=-1,maxHook=-1,maxPing=-1,maxHttp=0,maxCpu=0,maxRss=0,maxLoad=0;std::uint64_t engineTicks=0,hitches=0,samples=0;for(const auto& p:filtered){maxEngine=std::max(maxEngine,p.engineTickMs);maxHook=std::max(maxHook,p.zanikGameThreadMs);maxPing=std::max(maxPing,p.maximumPingMs);maxHttp=std::max(maxHttp,p.httpMaximumMs);maxCpu=std::max(maxCpu,p.processCpuPercent);maxRss=std::max(maxRss,p.rssMiB);maxLoad=std::max(maxLoad,p.loadOne);engineTicks+=p.engineTicks;hitches+=p.hitches;samples+=p.samples;}
    struct Candidate{std::uint64_t at,occurrences;int rank;std::string key,title,detail,severity;};std::vector<Candidate> candidates;
    for(const auto& p:filtered){
        if(p.engineTickMs>=50&&(p.engineTickMs>=100||p.hitches>=2)){const bool startup=p.timestamp<sessionStartedAt_+60,zanik=p.zanikGameThreadMs>=2&&p.zanikGameThreadMs>=(p.engineTickMs+p.zanikGameThreadMs)*.2;candidates.push_back({p.timestamp,std::max<std::uint64_t>(1,p.hitches),int(p.engineTickMs),startup?"engine-startup":zanik?"zanik":"engine",startup?"Server startup engine activity":zanik?"Zanik game-thread work coincided with a slow engine tick":"Engine dispatcher work caused a game-thread hitch",startup?"The engine dispatcher was slow during the first minute of startup, when world initialization and asset loading commonly cause temporary pressure.":zanik?"Both the intercepted engine tick and Zanik's bounded follow-up work were slow. Check recipe, quest, storage, and gravestone activity around this incident.":"The intercepted engine dispatcher itself ran slowly while Zanik's measured follow-up stayed comparatively small. Engine simulation, world streaming, saves, host scheduling, or another mod may be responsible.",!startup&&p.engineTickMs>=150?"critical":"warning"});}
        if(p.onlinePlayers&&p.maximumPingMs>=200)candidates.push_back({p.timestamp,1,int(p.maximumPingMs),"ping","High player network latency","At least one connected player's authoritative game ping was high. If engine timing stayed healthy, this points to that player's route or the host network rather than server simulation.",p.maximumPingMs>=350?"critical":"warning"});
        if(p.httpMaximumMs>=500){const bool startup=p.timestamp<sessionStartedAt_+60;candidates.push_back({p.timestamp,1,int(p.httpMaximumMs),startup?"http-startup":"http",startup?"Server startup web activity":"Slow diagnostics/web request",startup?"A web request was slow during the first minute while the server and Zanik were initializing.":p.processCpuPercent>=90||p.loadOne>=double(std::max(1L,sysconf(_SC_NPROCESSORS_ONLN)))*.9?"The request coincided with elevated server-process or host pressure, so CPU contention may be involved.":"The web worker was slow without high sampled host pressure; a large response, slow client, or filesystem operation is more likely.",!startup&&p.httpMaximumMs>=1500?"critical":"warning"});}
    }
    std::sort(candidates.begin(),candidates.end(),[](const auto& a,const auto& b){return a.key<b.key||(a.key==b.key&&a.at<b.at);});
    struct Insight{std::uint64_t at,end,occurrences;int rank;std::string key,title,detail,severity;};std::vector<Insight> insights;
    for(const auto& c:candidates){if(!insights.empty()&&insights.back().key==c.key&&c.at<=insights.back().end+IncidentMergeGapSeconds){auto& v=insights.back();v.end=c.at;v.occurrences+=c.occurrences;if(c.rank>v.rank)v.rank=c.rank;if(c.severity=="critical")v.severity="critical";}else insights.push_back({c.at,c.at,c.occurrences,c.rank,c.key,c.title,c.detail,c.severity});}
    std::sort(insights.begin(),insights.end(),[](const auto& a,const auto& b){return a.rank>b.rank;});if(insights.size()>12)insights.resize(12);
    std::ostringstream json;json<<"{\"sessionStartedAt\":"<<sessionStartedAt_<<",\"generatedAt\":"<<now<<",\"windowSeconds\":"<<windowSeconds<<",\"persistenceHealthy\":"<<(persistenceHealthy_?"true":"false")<<",\"persistenceFailures\":"<<persistenceFailures_<<",\"sampleIntervalSeconds\":"<<SampleIntervalSeconds<<",\"hostCpuCount\":"<<std::max(1L,sysconf(_SC_NPROCESSORS_ONLN))<<",\"summary\":{\"samples\":"<<samples<<",\"engineTicks\":"<<engineTicks<<",\"maximumEngineTickMs\":"<<Number(maxEngine)<<",\"maximumZanikGameThreadMs\":"<<Number(maxHook)<<",\"maximumPingMs\":"<<Number(maxPing)<<",\"maximumHttpMs\":"<<Number(maxHttp)<<",\"maximumCpuPercent\":"<<Number(maxCpu)<<",\"maximumRssMiB\":"<<Number(maxRss)<<",\"maximumLoadOne\":"<<Number(maxLoad)<<",\"hitches\":"<<hitches<<"},\"points\":[";
    for(std::size_t i=0;i<displayed.size();++i){const auto& p=displayed[i];if(i)json<<',';json<<"{\"timestamp\":"<<p.timestamp<<",\"engineTickMs\":"<<Number(p.engineTickMs)<<",\"zanikGameThreadMs\":"<<Number(p.zanikGameThreadMs)<<",\"engineTicks\":"<<p.engineTicks<<",\"hitches\":"<<p.hitches<<",\"averagePingMs\":"<<Number(p.averagePingMs)<<",\"maximumPingMs\":"<<Number(p.maximumPingMs)<<",\"onlinePlayers\":"<<p.onlinePlayers<<",\"processCpuPercent\":"<<Number(p.processCpuPercent)<<",\"rssMiB\":"<<Number(p.rssMiB)<<",\"loadOne\":"<<Number(p.loadOne)<<",\"httpAverageMs\":"<<Number(p.httpAverageMs)<<",\"httpMaximumMs\":"<<Number(p.httpMaximumMs)<<",\"httpRequests\":"<<p.httpRequests<<",\"samples\":"<<p.samples<<'}';}json<<"],\"insights\":[";
    for(std::size_t i=0;i<insights.size();++i){const auto& v=insights[i];if(i)json<<',';json<<"{\"timestamp\":"<<v.at<<",\"endTimestamp\":"<<v.end<<",\"occurrences\":"<<v.occurrences<<",\"maximum\":"<<v.rank<<",\"severity\":\""<<v.severity<<"\",\"title\":\""<<v.title<<"\",\"detail\":\""<<v.detail<<"\"}";}json<<"]}";return json.str();
}

ServerPerformanceDiagnostics ServerPerformanceStore::Diagnostics() const {Lock lock(&mutex_);std::uint64_t samples=0;for(const auto& p:points_)samples+=p.samples;return {sessionStartedAt_,samples,persistenceFailures_,persistenceHealthy_};}
