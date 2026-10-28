#include "server_performance_store.h"

#include <cassert>
#include <cstdio>
#include <string>
#include <sys/stat.h>

int main(){
    const std::string path="dist/server-performance-test.db";std::remove(path.c_str());
    ServerPerformanceStore store(path);assert(store.Start(1000));store.RecordHttpDuration(250000);
    ServerPerformanceInput first;first.gameFrameMs=16.7;first.zanikGameThreadMs=.2;first.averagePingMs=42;first.maximumPingMs=55;first.onlinePlayers=2;assert(store.Sample(first,1010));
    ServerPerformanceInput spike;spike.gameFrameMs=125;spike.zanikGameThreadMs=.4;spike.gameThreadHitches=1;spike.averagePingMs=180;spike.maximumPingMs=380;spike.onlinePlayers=2;assert(store.Sample(spike,1020));
    const auto json=store.Json(0);assert(json.find("\"sessionStartedAt\":1000")!=std::string::npos);assert(json.find("\"maximumGameFrameMs\":125.00")!=std::string::npos);assert(json.find("Engine or game workload caused a frame hitch")!=std::string::npos);assert(json.find("High player network latency")!=std::string::npos);assert(json.find("\"httpMaximumMs\":250.00")!=std::string::npos);
    struct stat mode{};assert(stat(path.c_str(),&mode)==0&&(mode.st_mode&0777)==0600);const auto diagnostics=store.Diagnostics();assert(diagnostics.samples==2&&diagnostics.persistenceHealthy);std::remove(path.c_str());
}
