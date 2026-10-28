#pragma once

#include <atomic>
#include <cstdint>
#include <pthread.h>
#include <string>
#include <vector>

struct ServerPerformanceInput {
    double gameFrameMs = -1;
    double zanikGameThreadMs = -1;
    std::uint64_t gameThreadHitches = 0;
    double averagePingMs = -1;
    double maximumPingMs = -1;
    std::uint32_t onlinePlayers = 0;
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
    bool Start(std::uint64_t timestamp = 0);
    bool Sample(const ServerPerformanceInput& input, std::uint64_t timestamp = 0);
    void RecordHttpDuration(std::uint64_t microseconds);
    std::string Json(std::uint64_t windowSeconds) const;
    ServerPerformanceDiagnostics Diagnostics() const;

private:
    struct Point {
        std::uint64_t timestamp = 0;
        double gameFrameMs = -1, zanikGameThreadMs = -1;
        std::uint64_t hitches = 0;
        double averagePingMs = -1, maximumPingMs = -1;
        std::uint32_t onlinePlayers = 0;
        double processCpuPercent = 0, rssMiB = 0, loadOne = 0;
        double httpAverageMs = 0, httpMaximumMs = 0;
        std::uint64_t httpRequests = 0;
    };

    static std::uint64_t Now();
    bool Append(const Point& point);
    Point Aggregate(const std::vector<Point>& points, std::size_t begin, std::size_t end) const;
    std::string path_;
    mutable pthread_mutex_t mutex_ = PTHREAD_MUTEX_INITIALIZER;
    std::vector<Point> points_;
    std::uint64_t sessionStartedAt_ = 0, persistenceFailures_ = 0;
    bool persistenceHealthy_ = true;
    std::uint64_t previousCpuTicks_ = 0;
    std::uint64_t previousCpuTimestampMs_ = 0;
    std::atomic<std::uint64_t> httpMicros_{0}, httpMaximumMicros_{0}, httpRequests_{0};
};
