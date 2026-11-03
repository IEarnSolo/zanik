#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <pthread.h>
#include <vector>

namespace PerformanceTrace {

enum class TimingScope : char { Zanik = 'z', NativeGame = 'n' };

struct Event {
    std::uint64_t microseconds = 0;
    char feature[24]{};
    char operation[40]{};
    char identity[40]{};
    char identityKind = 0;
    char origin[12]{};
    char timingScope = static_cast<char>(TimingScope::Zanik);
};

inline constexpr std::size_t Capacity = 512;
inline pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
inline std::array<Event, Capacity> events{};
inline std::size_t readIndex = 0, writeIndex = 0;
inline std::atomic<std::uint64_t> drops{0};

inline void Copy(char* destination, std::size_t size, const char* source) {
    if (!size) return;
    if (!source) { destination[0] = 0; return; }
    const auto length = strnlen(source, size - 1);
    std::memcpy(destination, source, length);
    destination[length] = 0;
}

inline void Record(const char* feature, const char* operation, std::uint64_t microseconds,
                   const char* identity = nullptr, char identityKind = 0,
                   const char* origin = "game",
                   TimingScope timingScope = TimingScope::Zanik) {
    if (microseconds < 100) return;
    if (pthread_mutex_trylock(&mutex) != 0) { ++drops; return; }
    if (writeIndex - readIndex >= Capacity) { ++drops; pthread_mutex_unlock(&mutex); return; }
    auto& event = events[writeIndex++ % Capacity];
    event = {};
    event.microseconds = microseconds;
    event.identityKind = identityKind;
    Copy(event.feature, sizeof(event.feature), feature);
    Copy(event.operation, sizeof(event.operation), operation);
    Copy(event.identity, sizeof(event.identity), identity);
    Copy(event.origin, sizeof(event.origin), origin);
    event.timingScope = static_cast<char>(timingScope);
    pthread_mutex_unlock(&mutex);
}

inline std::vector<Event> Drain() {
    std::vector<Event> result;
    if (pthread_mutex_trylock(&mutex) != 0) return result;
    result.reserve(writeIndex - readIndex);
    while (readIndex < writeIndex) result.push_back(events[readIndex++ % Capacity]);
    pthread_mutex_unlock(&mutex);
    return result;
}

inline std::uint64_t Dropped() { return drops.load(); }

} // namespace PerformanceTrace
