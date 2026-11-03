#pragma once

#include <atomic>
#include <cstdint>
#include <pthread.h>
#include <string>
#include <vector>

struct PlayerNetworkInput {
    std::string id;
    std::string name;
    double pingMs = -1;
    std::uint64_t connectedSeconds = 0;
};

struct PerformanceAttributionInput {
    std::string feature, operation, playerId, playerName, origin;
    std::uint64_t microseconds = 0;
    std::string timingScope = "zanik";
};

struct ServerPerformanceInput {
    double engineTickMs = -1;
    double zanikGameThreadMs = -1;
    std::uint64_t engineTicks = 0, gameThreadHitches = 0;
    double averagePingMs = -1;
    double maximumPingMs = -1;
    std::uint32_t onlinePlayers = 0;
    std::vector<PlayerNetworkInput> players;
    std::vector<PerformanceAttributionInput> attributions;
    std::uint64_t attributionDrops = 0;
};

struct ServerPerformanceDiagnostics {
    std::uint64_t sessionStartedAt = 0;
    std::uint64_t samples = 0;
    std::uint64_t persistenceFailures = 0;
    bool persistenceHealthy = true;
};

class ServerPerformanceStore {
public:
    explicit ServerPerformanceStore(std::string path);
    ~ServerPerformanceStore();
    bool Start(std::uint64_t timestamp = 0);
    bool Sample(const ServerPerformanceInput& input, std::uint64_t timestamp = 0);
    void RecordHttpDuration(std::uint64_t microseconds, const std::string& route = {});
    std::string Json(std::uint64_t windowSeconds, bool newestInsightsFirst = false, std::uint64_t requestedInsightPage = 0) const;
    ServerPerformanceDiagnostics Diagnostics() const;

private:
    struct Point {
        struct PlayerPing {
            std::string id, name;
            double pingMs = -1;
            std::uint64_t connectedSeconds = 0;
        };
        std::uint64_t timestamp = 0;
        double engineTickMs = -1, zanikGameThreadMs = -1;
        std::uint64_t engineTicks = 0, hitches = 0;
        double averagePingMs = -1, maximumPingMs = -1;
        std::uint32_t onlinePlayers = 0;
        double processCpuPercent = 0, rssMiB = 0, loadOne = 0;
        double httpAverageMs = 0, httpMaximumMs = 0;
        std::string httpMaximumRoute;
        std::uint64_t httpRequests = 0, samples = 1;
        double networkRxKiBps = -1, networkTxKiBps = -1;
        std::uint64_t networkRxDrops = 0, networkTxDrops = 0;
        std::uint64_t networkRxErrors = 0, networkTxErrors = 0;
        bool hostNetworkAvailable = false;
        std::vector<PlayerPing> players;
        std::vector<PerformanceAttributionInput> attributions;
        std::uint64_t attributionDrops = 0;
    };

    static std::uint64_t Now();
    bool Append(const Point& point);
    void Compact();
    Point Aggregate(const std::vector<Point>& points, std::size_t begin, std::size_t end) const;
    std::string path_;
    mutable pthread_mutex_t mutex_ = PTHREAD_MUTEX_INITIALIZER;
    std::vector<Point> points_;
    std::uint64_t sessionStartedAt_ = 0, persistenceFailures_ = 0;
    bool persistenceHealthy_ = true;
    std::uint64_t previousCpuTicks_ = 0;
    std::uint64_t previousCpuTimestampMs_ = 0;
    std::uint64_t previousNetworkTimestampMs_ = 0;
    std::uint64_t previousNetworkRxBytes_ = 0, previousNetworkTxBytes_ = 0;
    std::uint64_t previousNetworkRxDrops_ = 0, previousNetworkTxDrops_ = 0;
    std::uint64_t previousNetworkRxErrors_ = 0, previousNetworkTxErrors_ = 0;
    bool previousNetworkAvailable_ = false;
    int fileDescriptor_ = -1;
    std::atomic<std::uint64_t> httpMicros_{0}, httpMaximumMicros_{0}, httpRequests_{0};
    pthread_mutex_t httpRouteMutex_ = PTHREAD_MUTEX_INITIALIZER;
    std::string httpMaximumRoute_;
};
