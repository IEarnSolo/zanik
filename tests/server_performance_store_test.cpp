#include "server_performance_store.h"

#include <cassert>
#include <cstdio>
#include <string>
#include <sys/stat.h>

int main(){
    const std::string path="dist/server-performance-test.db";std::remove(path.c_str());
    ServerPerformanceStore store(path);assert(store.Start(900));store.RecordHttpDuration(250000);
    ServerPerformanceInput first;first.engineTickMs=16.7;first.zanikGameThreadMs=.2;first.engineTicks=52;first.averagePingMs=42;first.maximumPingMs=55;first.onlinePlayers=2;assert(store.Sample(first,1010));
    ServerPerformanceInput spike;spike.engineTickMs=125;spike.zanikGameThreadMs=.4;spike.engineTicks=50;spike.gameThreadHitches=1;spike.averagePingMs=180;spike.maximumPingMs=380;spike.onlinePlayers=2;assert(store.Sample(spike,1020));assert(store.Sample(spike,1022));
    const auto json=store.Json(0);assert(json.find("\"sessionStartedAt\":900")!=std::string::npos);assert(json.find("\"engineTicks\":152")!=std::string::npos);assert(json.find("\"maximumEngineTickMs\":125.00")!=std::string::npos);assert(json.find("Engine dispatcher work caused a game-thread hitch")!=std::string::npos);assert(json.find("\"endTimestamp\":1022")!=std::string::npos);assert(json.find("\"occurrences\":2")!=std::string::npos);assert(json.find("High player network latency")!=std::string::npos);assert(json.find("\"httpMaximumMs\":250.00")!=std::string::npos);assert(json.find("\"sampleIntervalSeconds\":2")!=std::string::npos);
    struct stat mode{};assert(stat(path.c_str(),&mode)==0&&(mode.st_mode&0777)==0600);const auto diagnostics=store.Diagnostics();assert(diagnostics.samples==3&&diagnostics.persistenceHealthy);std::remove(path.c_str());
}
